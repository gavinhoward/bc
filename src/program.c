/*
 * *****************************************************************************
 *
 * Copyright 2018 Gavin D. Howard
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * *****************************************************************************
 *
 * Code to execute bc programs.
 *
 */

#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include <read.h>
#include <parse.h>
#include <program.h>
#include <vm.h>

BcVec* bc_program_search(BcProgram *p, char *id, bool var) {

	BcStatus s;
	BcId e, *ptr;
	BcVec *v, *map;
	size_t i;
	BcResultData data;
	bool new;

	v = var ? &p->vars : &p->arrs;
	map = var ? &p->var_map : &p->arr_map;

	e.name = id;
	e.idx = v->len;
	s = bc_map_insert(map, &e, &i);
	new = s != BC_STATUS_VEC_ITEM_EXISTS;

	if (new) {
		bc_array_init(&data.v, var);
		bc_vec_push(v, &data.v);
	}

	ptr = bc_vec_item(map, i);
	if (new) ptr->name = bc_vm_strdup(e.name);

	return bc_vec_item(v, ptr->idx);
}

BcStatus bc_program_num(BcProgram *p, BcResult *r, BcNum **num, bool hex) {

	BcStatus s = BC_STATUS_SUCCESS;

	switch (r->t) {

		case BC_RESULT_STR:
		case BC_RESULT_TEMP:
		case BC_RESULT_IBASE:
		case BC_RESULT_SCALE:
		case BC_RESULT_OBASE:
		{
			*num = &r->d.n;
			break;
		}

		case BC_RESULT_CONSTANT:
		{
			char **str = bc_vec_item(&p->consts, r->d.id.idx);
			size_t base_t, len = strlen(*str);
			BcNum *base;

			bc_num_init(&r->d.n, len);

			hex = hex && len == 1;
			base = hex ? &p->hexb : &p->ib;
			base_t = hex ? BC_NUM_MAX_IBASE : p->ib_t;
			s = bc_num_parse(&r->d.n, *str, base, base_t);

			if (s) {
				bc_num_free(&r->d.n);
				return s;
			}

			*num = &r->d.n;
			r->t = BC_RESULT_TEMP;

			break;
		}

		case BC_RESULT_VAR:
		case BC_RESULT_ARRAY:
		case BC_RESULT_ARRAY_ELEM:
		{
			BcVec *v;

			v = bc_program_search(p, r->d.id.name, r->t == BC_RESULT_VAR);

			if (r->t == BC_RESULT_ARRAY_ELEM) {
				v = bc_vec_top(v);
				if (v->len <= r->d.id.idx) bc_array_expand(v, r->d.id.idx + 1);
				*num = bc_vec_item(v, r->d.id.idx);
			}
			else *num = bc_vec_top(v);

			break;
		}

		case BC_RESULT_LAST:
		{
			*num = &p->last;
			break;
		}

		case BC_RESULT_ONE:
		{
			*num = &p->one;
			break;
		}
#ifndef NDEBUG
		default:
		{
			assert(false);
			break;
		}
#endif // NDEBUG
	}

	return s;
}

BcStatus bc_program_binOpPrep(BcProgram *p, BcResult **l, BcNum **ln,
                              BcResult **r, BcNum **rn, bool assign)
{
	BcStatus s;
	bool hex;
	BcResultType lt, rt;

	assert(p && l && ln && r && rn);

	if (!BC_PROG_STACK(&p->results, 2)) return BC_STATUS_EXEC_STACK;

	*r = bc_vec_item_rev(&p->results, 0);
	*l = bc_vec_item_rev(&p->results, 1);

	lt = (*l)->t;
	rt = (*r)->t;
	hex = assign && (lt == BC_RESULT_IBASE || lt == BC_RESULT_OBASE);

	s = bc_program_num(p, *l, ln, false);
	if (s) return s;
	s = bc_program_num(p, *r, rn, hex);
	if (s) return s;

	// We run this again under these conditions in case any vector has been
	// reallocated out from under the BcNums or arrays we had.
	if (lt == rt && (lt == BC_RESULT_VAR || lt == BC_RESULT_ARRAY_ELEM)) {
		s = bc_program_num(p, *l, ln, false);
		if (s) return s;
	}

	if (!BC_PROG_NUM((*l), (*ln)) && (!assign || (*l)->t != BC_RESULT_VAR))
		return BC_STATUS_EXEC_BAD_TYPE;
	if (!assign && !BC_PROG_NUM((*r), (*ln))) return BC_STATUS_EXEC_BAD_TYPE;

#if DC_ENABLED
	assert(lt != BC_RESULT_VAR || !BC_PROG_STR(*ln) || assign);
#else // DC_ENABLED
	assert(rt != BC_RESULT_STR);
#endif // DC_ENABLED

	return s;
}

void bc_program_binOpRetire(BcProgram *p, BcResult *r) {
	r->t = BC_RESULT_TEMP;
	bc_vec_pop(&p->results);
	bc_vec_pop(&p->results);
	bc_vec_push(&p->results, r);
}

BcStatus bc_program_prep(BcProgram *p, BcResult **r, BcNum **n) {

	BcStatus s;

	assert(p && r && n);

	if (!BC_PROG_STACK(&p->results, 1)) return BC_STATUS_EXEC_STACK;
	*r = bc_vec_top(&p->results);

	s = bc_program_num(p, *r, n, false);
	if (s) return s;

#if DC_ENABLED
	assert((*r)->t != BC_RESULT_VAR || !BC_PROG_STR(*n));
#endif // DC_ENABLED

	if (!BC_PROG_NUM((*r), (*n))) return BC_STATUS_EXEC_BAD_TYPE;

	return s;
}

void bc_program_retire(BcProgram *p, BcResult *r, BcResultType t) {
	r->t = t;
	bc_vec_pop(&p->results);
	bc_vec_push(&p->results, r);
}

BcStatus bc_program_op(BcProgram *p, char inst) {

	BcStatus s;
	BcResult *opd1, *opd2, res;
	BcNum *n1, *n2 = NULL;

	s = bc_program_binOpPrep(p, &opd1, &n1, &opd2, &n2, false);
	if (s) return s;
	bc_num_init(&res.d.n, BC_NUM_DEF_SIZE);

	s = bc_program_ops[inst - BC_INST_POWER](n1, n2, &res.d.n, p->scale);
	if (s) goto err;
	bc_program_binOpRetire(p, &res);

	return s;

err:
	bc_num_free(&res.d.n);
	return s;
}

BcStatus bc_program_read(BcProgram *p) {

	BcStatus s;
	BcParse parse;
	BcVec buf;
	BcInstPtr ip;
	size_t i;
	BcFunc *f = bc_vec_item(&p->fns, BC_PROG_READ);

	for (i = 0; i < p->stack.len; ++i) {
		BcInstPtr *ip_ptr = bc_vec_item(&p->stack, i);
		if (ip_ptr->func == BC_PROG_READ) return BC_STATUS_EXEC_REC_READ;
	}

	bc_vec_npop(&f->code, f->code.len);
	bc_vec_init(&buf, sizeof(char), NULL);

	s = bc_read_line(&buf, "read> ");
	if (s) goto io_err;

	vm->parse_init(&parse, p, BC_PROG_READ);
	bc_lex_file(&parse.l, bc_program_stdin_name);

	s = bc_parse_text(&parse, buf.v);
	if (s) goto exec_err;
	s = vm->parse_expr(&parse, BC_PARSE_NOREAD);
	if (s) goto exec_err;

	if (parse.l.t.t != BC_LEX_NLINE && parse.l.t.t != BC_LEX_EOF) {
		s = BC_STATUS_EXEC_BAD_READ_EXPR;
		goto exec_err;
	}

	ip.func = BC_PROG_READ;
	ip.idx = 0;
	ip.len = p->results.len;

	// Update this pointer, just in case.
	f = bc_vec_item(&p->fns, BC_PROG_READ);

	bc_vec_pushByte(&f->code, BC_INST_POP_EXEC);
	bc_vec_push(&p->stack, &ip);

exec_err:
	bc_parse_free(&parse);
io_err:
	bc_vec_free(&buf);
	return s;
}

size_t bc_program_index(char *code, size_t *bgn) {

	char amt = code[(*bgn)++], i = 0;
	size_t res = 0;

	for (; i < amt; ++i, ++(*bgn))
		res |= (((size_t) ((int) code[*bgn]) & UCHAR_MAX) << (i * CHAR_BIT));

	return res;
}

char* bc_program_name(char *code, size_t *bgn) {

	size_t i;
	char c, *s, *str = code + *bgn, *ptr = strchr(str, BC_PARSE_STREND);

	assert(ptr);

	s = bc_vm_malloc(ptr - str + 1);
	c = code[(*bgn)++];

	for (i = 0; c != 0 && c != BC_PARSE_STREND; c = code[(*bgn)++], ++i)
		s[i] = c;

	s[i] = '\0';

	return s;
}

void bc_program_printString(const char *str, size_t *nchars) {

	size_t i, len = strlen(str);

#if DC_ENABLED
	if (len == 0) {
		bc_vm_putchar('\0');
		return;
	}
#endif // DC_ENABLED

	for (i = 0; i < len; ++i, ++(*nchars)) {

		int c = str[i];

		if (c != '\\' || i == len - 1) bc_vm_putchar(c);
		else {

			c = str[++i];

			switch (c) {

				case 'a':
				{
					bc_vm_putchar('\a');
					break;
				}

				case 'b':
				{
					bc_vm_putchar('\b');
					break;
				}

				case '\\':
				case 'e':
				{
					bc_vm_putchar('\\');
					break;
				}

				case 'f':
				{
					bc_vm_putchar('\f');
					break;
				}

				case 'n':
				{
					bc_vm_putchar('\n');
					*nchars = SIZE_MAX;
					break;
				}

				case 'r':
				{
					bc_vm_putchar('\r');
					break;
				}

				case 'q':
				{
					bc_vm_putchar('"');
					break;
				}

				case 't':
				{
					bc_vm_putchar('\t');
					break;
				}

				default:
				{
					// Just print the backslash and following character.
					bc_vm_putchar('\\');
					++(*nchars);
					bc_vm_putchar(c);
					break;
				}
			}
		}
	}
}

BcStatus bc_program_print(BcProgram *p, char inst, size_t idx) {

	BcStatus s = BC_STATUS_SUCCESS;
	BcResult *r;
	size_t len, i;
	char *str;
	BcNum *num = NULL;
	bool pop = inst != BC_INST_PRINT;

	assert(p);

	if (!BC_PROG_STACK(&p->results, idx + 1)) return BC_STATUS_EXEC_STACK;

	r = bc_vec_item_rev(&p->results, idx);
	s = bc_program_num(p, r, &num, false);
	if (s) return s;

	if (BC_PROG_NUM(r, num)) {
		assert(inst != BC_INST_PRINT_STR);
		s = bc_num_print(num, &p->ob, p->ob_t, !pop, &p->nchars);
		if (!s) bc_num_copy(&p->last, num);
	}
	else {

		idx = (r->t == BC_RESULT_STR) ? r->d.id.idx : num->rdx;
		assert(idx < p->strs.len);
		str = *((char**) bc_vec_item(&p->strs, idx));

		if (inst == BC_INST_PRINT_STR) {
			for (i = 0, len = strlen(str); i < len; ++i) {
				char c = str[i];
				bc_vm_putchar(c);
				if (c == '\n') p->nchars = SIZE_MAX;
				++p->nchars;
			}
		}
		else {
			bc_program_printString(str, &p->nchars);
			if (inst == BC_INST_PRINT) bc_vm_putchar('\n');
		}
	}

	if (!s && pop) bc_vec_pop(&p->results);

	return s;
}

BcStatus bc_program_negate(BcProgram *p) {

	BcStatus s;
	BcResult res, *ptr;
	BcNum *num = NULL;

	s = bc_program_prep(p, &ptr, &num);
	if (s) return s;

	bc_num_init(&res.d.n, num->len);
	bc_num_copy(&res.d.n, num);
	if (res.d.n.len) res.d.n.neg = !res.d.n.neg;

	bc_program_retire(p, &res, BC_RESULT_TEMP);

	return s;
}

BcStatus bc_program_logical(BcProgram *p, char inst) {

	BcStatus s;
	BcResult *opd1, *opd2, res;
	BcNum *n1, *n2;
	bool cond = 0;
	ssize_t cmp;

	s = bc_program_binOpPrep(p, &opd1, &n1, &opd2, &n2, false);
	if (s) return s;
	bc_num_init(&res.d.n, BC_NUM_DEF_SIZE);

	if (inst == BC_INST_BOOL_AND)
		cond = bc_num_cmp(n1, &p->zero) && bc_num_cmp(n2, &p->zero);
	else if (inst == BC_INST_BOOL_OR)
		cond = bc_num_cmp(n1, &p->zero) || bc_num_cmp(n2, &p->zero);
	else {

		cmp = bc_num_cmp(n1, n2);

		switch (inst) {

			case BC_INST_REL_EQ:
			{
				cond = cmp == 0;
				break;
			}

			case BC_INST_REL_LE:
			{
				cond = cmp <= 0;
				break;
			}

			case BC_INST_REL_GE:
			{
				cond = cmp >= 0;
				break;
			}

			case BC_INST_REL_NE:
			{
				cond = cmp != 0;
				break;
			}

			case BC_INST_REL_LT:
			{
				cond = cmp < 0;
				break;
			}

			case BC_INST_REL_GT:
			{
				cond = cmp > 0;
				break;
			}
#ifndef NDEBUG
			default:
			{
				assert(false);
				break;
			}
#endif // NDEBUG
		}
	}

	(cond ? bc_num_one : bc_num_zero)(&res.d.n);

	bc_program_binOpRetire(p, &res);

	return s;
}

#if DC_ENABLED
BcStatus bc_program_assignStr(BcProgram *p, BcResult *r, BcVec *v, bool push) {

	BcNum n2;
	BcResult res;

	memset(&n2, 0, sizeof(BcNum));
	n2.rdx = res.d.id.idx = r->d.id.idx;
	res.t = BC_RESULT_STR;

	if (!push) {
		if (!BC_PROG_STACK(&p->results, 2)) return BC_STATUS_EXEC_STACK;
		bc_vec_pop(v);
		bc_vec_pop(&p->results);
	}

	bc_vec_pop(&p->results);

	bc_vec_push(&p->results, &res);
	bc_vec_push(v, &n2);

	return BC_STATUS_SUCCESS;
}
#endif // DC_ENABLED

BcStatus bc_program_copyToVar(BcProgram *p, char *name, bool var) {

	BcStatus s;
	BcResult *ptr, r;
	BcVec *v;
	BcNum *n;

	if (!BC_PROG_STACK(&p->results, 1)) return BC_STATUS_EXEC_STACK;

	ptr = bc_vec_top(&p->results);
	if ((ptr->t == BC_RESULT_ARRAY) != !var) return BC_STATUS_EXEC_BAD_TYPE;
	v = bc_program_search(p, name, var);

#if DC_ENABLED
	if (ptr->t == BC_RESULT_STR && !var) return BC_STATUS_EXEC_BAD_TYPE;
	if (ptr->t == BC_RESULT_STR) return bc_program_assignStr(p, ptr, v, true);
#endif // DC_ENABLED

	s = bc_program_num(p, ptr, &n, false);
	if (s) return s;

	// Do this once more to make sure that pointers were not invalidated.
	v = bc_program_search(p, name, var);

	if (var) {
		bc_num_init(&r.d.n, BC_NUM_DEF_SIZE);
		bc_num_copy(&r.d.n, n);
	}
	else {
		bc_array_init(&r.d.v, true);
		bc_array_copy(&r.d.v, (BcVec*) n);
	}

	bc_vec_push(v, &r.d);
	bc_vec_pop(&p->results);

	return s;
}

BcStatus bc_program_assign(BcProgram *p, char inst) {

	BcStatus s;
	BcResult *left, *right, res;
	BcNum *l = NULL, *r = NULL;
	unsigned long val, max;
	bool assign = inst == BC_INST_ASSIGN, ib, sc;

	s = bc_program_binOpPrep(p, &left, &l, &right, &r, assign);
	if (s) return s;

	ib = left->t == BC_RESULT_IBASE;
	sc = left->t == BC_RESULT_SCALE;

#if DC_ENABLED
	assert(left->t != BC_RESULT_STR);

	if (right->t == BC_RESULT_STR) {

		BcVec *v;

		assert(assign);

		if (left->t != BC_RESULT_VAR) return BC_STATUS_EXEC_BAD_TYPE;
		v = bc_program_search(p, left->d.id.name, true);

		return bc_program_assignStr(p, right, v, false);
	}
#endif // DC_ENABLED

	if (left->t == BC_RESULT_CONSTANT || left->t == BC_RESULT_TEMP)
		return BC_STATUS_PARSE_BAD_ASSIGN;

#if BC_ENABLED
	if (inst == BC_INST_ASSIGN_DIVIDE && !bc_num_cmp(r, &p->zero))
		return BC_STATUS_MATH_DIVIDE_BY_ZERO;

	if (assign) bc_num_copy(l, r);
	else s = bc_program_ops[inst - BC_INST_ASSIGN_POWER](l, r, l, p->scale);

	if (s) return s;
#else // BC_ENABLED
	assert(assign);
	bc_num_copy(l, r);
#endif // BC_ENABLED

	if (ib || sc || left->t == BC_RESULT_OBASE) {

		size_t *ptr;

		s = bc_num_ulong(l, &val);
		if (s) return s;
		s = left->t - BC_RESULT_IBASE + BC_STATUS_EXEC_BAD_IBASE;

		if (sc) {
			max = BC_MAX_SCALE;
			ptr = &p->scale;
		}
		else {
			if (val < BC_NUM_MIN_BASE) return s;
			max = ib ? BC_NUM_MAX_IBASE : BC_MAX_OBASE;
			ptr = ib ? &p->ib_t : &p->ob_t;
		}

		if (val > max) return s;
		if (!sc) bc_num_copy(ib ? &p->ib : &p->ob, l);

		*ptr = (size_t) val;
		s = BC_STATUS_SUCCESS;
	}

	bc_num_init(&res.d.n, l->len);
	bc_num_copy(&res.d.n, l);
	bc_program_binOpRetire(p, &res);

	return s;
}

BcStatus bc_program_pushVar(BcProgram *p, char *code, size_t *bgn,
                            bool pop, bool copy)
{
	BcStatus s = BC_STATUS_SUCCESS;
	BcResult r;
	char *name = bc_program_name(code, bgn);

	r.t = BC_RESULT_VAR;
	r.d.id.name = name;

#if DC_ENABLED
	{
		BcVec *v = bc_program_search(p, name, true);
		BcNum *num = bc_vec_top(v);

		if (pop || copy) {

			if (!BC_PROG_STACK(v, 2 - copy)) {
				free(name);
				return BC_STATUS_EXEC_STACK;
			}

			free(name);
			name = NULL;

			if (!BC_PROG_STR(num)) {

				r.t = BC_RESULT_TEMP;

				bc_num_init(&r.d.n, BC_NUM_DEF_SIZE);
				bc_num_copy(&r.d.n, num);
			}
			else {
				r.t = BC_RESULT_STR;
				r.d.id.idx = num->rdx;
			}

			if (!copy) bc_vec_pop(v);
		}
	}
#endif // DC_ENABLED

	bc_vec_push(&p->results, &r);

	return s;
}

BcStatus bc_program_pushArray(BcProgram *p, char *code, size_t *bgn, char inst)
{
	BcStatus s = BC_STATUS_SUCCESS;
	BcResult r;
	BcNum *num;

	r.d.id.name = bc_program_name(code, bgn);

	if (inst == BC_INST_ARRAY) {
		r.t = BC_RESULT_ARRAY;
		bc_vec_push(&p->results, &r);
	}
	else {

		BcResult *operand;
		unsigned long temp;

		s = bc_program_prep(p, &operand, &num);
		if (s) goto err;
		s = bc_num_ulong(num, &temp);
		if (s) goto err;

		if (temp > BC_MAX_DIM) {
			s = BC_STATUS_EXEC_ARRAY_LEN;
			goto err;
		}

		r.d.id.idx = (size_t) temp;
		bc_program_retire(p, &r, BC_RESULT_ARRAY_ELEM);
	}

err:
	if (s) free(r.d.id.name);
	return s;
}

#if BC_ENABLED
BcStatus bc_program_incdec(BcProgram *p, char inst) {

	BcStatus s;
	BcResult *ptr, res, copy;
	BcNum *num = NULL;
	char inst2 = inst;

	s = bc_program_prep(p, &ptr, &num);
	if (s) return s;

	if (inst == BC_INST_INC_POST || inst == BC_INST_DEC_POST) {
		copy.t = BC_RESULT_TEMP;
		bc_num_init(&copy.d.n, num->len);
		bc_num_copy(&copy.d.n, num);
	}

	res.t = BC_RESULT_ONE;
	inst = inst == BC_INST_INC_PRE || inst == BC_INST_INC_POST ?
	           BC_INST_ASSIGN_PLUS : BC_INST_ASSIGN_MINUS;

	bc_vec_push(&p->results, &res);
	bc_program_assign(p, inst);

	if (inst2 == BC_INST_INC_POST || inst2 == BC_INST_DEC_POST) {
		bc_vec_pop(&p->results);
		bc_vec_push(&p->results, &copy);
	}

	return s;
}

BcStatus bc_program_call(BcProgram *p, char *code, size_t *idx) {

	BcStatus s = BC_STATUS_SUCCESS;
	BcInstPtr ip;
	size_t i, nparams = bc_program_index(code, idx);
	BcFunc *func;
	BcVec *v;
	BcId *a;
	BcResultData param;
	BcResult *arg;

	ip.idx = 0;
	ip.func = bc_program_index(code, idx);
	func = bc_vec_item(&p->fns, ip.func);

	if (func->code.len == 0) return BC_STATUS_EXEC_UNDEFINED_FUNC;
	if (nparams != func->nparams) return BC_STATUS_EXEC_MISMATCHED_PARAMS;
	ip.len = p->results.len - nparams;

	assert(BC_PROG_STACK(&p->results, nparams));

	for (i = 0; i < nparams; ++i) {

		a = bc_vec_item(&func->autos, nparams - 1 - i);
		arg = bc_vec_top(&p->results);

		if ((!a->idx) != (arg->t == BC_RESULT_ARRAY) || arg->t == BC_RESULT_STR)
			return BC_STATUS_EXEC_BAD_TYPE;

		s = bc_program_copyToVar(p, a->name, a->idx);
		if (s) return s;
	}

	for (; i < func->autos.len; ++i) {

		a = bc_vec_item(&func->autos, i);
		v = bc_program_search(p, a->name, a->idx);

		if (a->idx) {
			bc_num_init(&param.n, BC_NUM_DEF_SIZE);
			bc_vec_push(v, &param.n);
		}
		else {
			bc_array_init(&param.v, true);
			bc_vec_push(v, &param.v);
		}
	}

	bc_vec_push(&p->stack, &ip);

	return BC_STATUS_SUCCESS;
}

BcStatus bc_program_return(BcProgram *p, char inst) {

	BcStatus s;
	BcResult res;
	BcFunc *f;
	size_t i;
	BcInstPtr *ip = bc_vec_top(&p->stack);

	assert(BC_PROG_STACK(&p->stack, 2));

	if (!BC_PROG_STACK(&p->results, ip->len + inst == BC_INST_RET))
		return BC_STATUS_EXEC_STACK;

	f = bc_vec_item(&p->fns, ip->func);
	res.t = BC_RESULT_TEMP;

	if (inst == BC_INST_RET) {

		BcNum *num;
		BcResult *operand = bc_vec_top(&p->results);

		s = bc_program_num(p, operand, &num, false);
		if (s) return s;
		bc_num_init(&res.d.n, num->len);
		bc_num_copy(&res.d.n, num);
	}
	else {
		bc_num_init(&res.d.n, BC_NUM_DEF_SIZE);
		bc_num_zero(&res.d.n);
	}

	// We need to pop arguments as well, so this takes that into account.
	for (i = 0; i < f->autos.len; ++i) {

		BcVec *v;
		BcId *a = bc_vec_item(&f->autos, i);

		v = bc_program_search(p, a->name, a->idx);
		bc_vec_pop(v);
	}

	bc_vec_npop(&p->results, p->results.len - ip->len);
	bc_vec_push(&p->results, &res);
	bc_vec_pop(&p->stack);

	return BC_STATUS_SUCCESS;
}
#endif // BC_ENABLED

unsigned long bc_program_scale(BcNum *n) {
	return (unsigned long) n->rdx;
}

unsigned long bc_program_len(BcNum *n) {

	unsigned long len = n->len;
	size_t i;

	if (n->rdx != n->len) return len;
	for (i = n->len - 1; i < n->len && n->num[i] == 0; --len, --i);

	return len;
}

BcStatus bc_program_builtin(BcProgram *p, char inst) {

	BcStatus s;
	BcResult *opnd;
	BcNum *num = NULL;
	BcResult res;
	bool len = inst == BC_INST_LENGTH;

	if (!BC_PROG_STACK(&p->results, 1)) return BC_STATUS_EXEC_STACK;
	opnd = bc_vec_top(&p->results);

	s = bc_program_num(p, opnd, &num, false);
	if (s) return s;

#if DC_ENABLED
	if (!BC_PROG_NUM(opnd, num) && !len) return BC_STATUS_EXEC_BAD_TYPE;
#endif // DC_ENABLED

	bc_num_init(&res.d.n, BC_NUM_DEF_SIZE);

	if (inst == BC_INST_SQRT) s = bc_num_sqrt(num, &res.d.n, p->scale);
#if BC_ENABLED
	else if (len != 0 && opnd->t == BC_RESULT_ARRAY) {
		bc_num_ulong2num(&res.d.n, (unsigned long) ((BcVec*) num)->len);
	}
#endif // BC_ENABLED
#if DC_ENABLED
	else if (len != 0 && !BC_PROG_NUM(opnd, num)) {

		char **str;
		size_t idx = opnd->t == BC_RESULT_STR ? opnd->d.id.idx : num->rdx;

		assert(idx < p->strs.len);

		str = bc_vec_item(&p->strs, idx);
		bc_num_ulong2num(&res.d.n, strlen(*str));
	}
#endif // DC_ENABLED
	else {
		BcProgramBuiltIn f = len ? bc_program_len : bc_program_scale;
		assert(opnd->t != BC_RESULT_ARRAY);
		bc_num_ulong2num(&res.d.n, f(num));
	}

	bc_program_retire(p, &res, BC_RESULT_TEMP);

	return s;
}

#if DC_ENABLED
BcStatus bc_program_divmod(BcProgram *p) {

	BcStatus s;
	BcResult *opd1, *opd2, res, res2;
	BcNum *n1, *n2 = NULL;

	s = bc_program_binOpPrep(p, &opd1, &n1, &opd2, &n2, false);
	if (s) return s;

	bc_num_init(&res.d.n, BC_NUM_DEF_SIZE);
	bc_num_init(&res2.d.n, n2->len);

	s = bc_num_divmod(n1, n2, &res2.d.n, &res.d.n, p->scale);
	if (s) goto err;

	bc_program_binOpRetire(p, &res2);
	res.t = BC_RESULT_TEMP;
	bc_vec_push(&p->results, &res);

	return s;

err:
	bc_num_free(&res2.d.n);
	bc_num_free(&res.d.n);
	return s;
}

BcStatus bc_program_modexp(BcProgram *p) {

	BcStatus s;
	BcResult *r1, *r2, *r3, res;
	BcNum *n1, *n2, *n3;

	if (!BC_PROG_STACK(&p->results, 3)) return BC_STATUS_EXEC_STACK;
	s = bc_program_binOpPrep(p, &r2, &n2, &r3, &n3, false);
	if (s) return s;

	r1 = bc_vec_item_rev(&p->results, 2);
	s = bc_program_num(p, r1, &n1, false);
	if (s) return s;
	if (!BC_PROG_NUM(r1, n1)) return BC_STATUS_EXEC_BAD_TYPE;

	// Make sure that the values have their pointers updated, if necessary.
	if (r1->t == BC_RESULT_VAR || r1->t == BC_RESULT_ARRAY_ELEM) {

		if (r1->t == r2->t) {
			s = bc_program_num(p, r2, &n2, false);
			if (s) return s;
		}

		if (r1->t == r3->t) {
			s = bc_program_num(p, r3, &n3, false);
			if (s) return s;
		}
	}

	bc_num_init(&res.d.n, n3->len);
	s = bc_num_modexp(n1, n2, n3, &res.d.n);
	if (s) goto err;

	bc_vec_pop(&p->results);
	bc_program_binOpRetire(p, &res);

	return s;

err:
	bc_num_free(&res.d.n);
	return s;
}

void bc_program_stackLen(BcProgram *p) {

	BcResult res;
	size_t len = p->results.len;

	res.t = BC_RESULT_TEMP;

	bc_num_init(&res.d.n, BC_NUM_DEF_SIZE);
	bc_num_ulong2num(&res.d.n, len);
	bc_vec_push(&p->results, &res);
}

BcStatus bc_program_asciify(BcProgram *p) {

	BcStatus s;
	BcResult *r, res;
	BcNum *num = NULL, n;
	char *str, *str2, c;
	size_t len = p->strs.len, idx;
	unsigned long val;

	if (!BC_PROG_STACK(&p->results, 1)) return BC_STATUS_EXEC_STACK;
	r = bc_vec_top(&p->results);

	s = bc_program_num(p, r, &num, false);
	if (s) return s;

	if (BC_PROG_NUM(r, num)) {

		bc_num_init(&n, BC_NUM_DEF_SIZE);
		bc_num_copy(&n, num);
		bc_num_truncate(&n, n.rdx);

		s = bc_num_mod(&n, &p->strmb, &n, 0);
		if (s) goto num_err;
		s = bc_num_ulong(&n, &val);
		if (s) goto num_err;

		c = (char) val;

		bc_num_free(&n);
	}
	else {
		idx = (r->t == BC_RESULT_STR) ? r->d.id.idx : num->rdx;
		assert(idx < p->strs.len);
		str2 = *((char**) bc_vec_item(&p->strs, idx));
		c = str2[0];
	}

	str = bc_vm_malloc(2);
	str[0] = c;
	str[1] = '\0';

	str2 = bc_vm_strdup(str);
	bc_program_addFunc(p, str2, &idx);

	if (idx != len + BC_PROG_REQ_FUNCS) {

		for (idx = 0; idx < p->strs.len; ++idx) {
			if (!strcmp(*((char**) bc_vec_item(&p->strs, idx)), str)) {
				len = idx;
				break;
			}
		}

		free(str);
	}
	else bc_vec_push(&p->strs, &str);

	res.t = BC_RESULT_STR;
	res.d.id.idx = len;
	bc_vec_pop(&p->results);
	bc_vec_push(&p->results, &res);

	return BC_STATUS_SUCCESS;

num_err:
	bc_num_free(&n);
	return s;
}

BcStatus bc_program_printStream(BcProgram *p) {

	BcStatus s;
	BcResult *r;
	BcNum *n = NULL;
	size_t idx;
	char *str;

	if (!BC_PROG_STACK(&p->results, 1)) return BC_STATUS_EXEC_STACK;
	r = bc_vec_top(&p->results);

	s = bc_program_num(p, r, &n, false);
	if (s) return s;

	if (BC_PROG_NUM(r, n)) s = bc_num_stream(n, &p->strmb, &p->nchars);
	else {
		idx = (r->t == BC_RESULT_STR) ? r->d.id.idx : n->rdx;
		assert(idx < p->strs.len);
		str = *((char**) bc_vec_item(&p->strs, idx));
		bc_vm_printf("%s", str);
	}

	return s;
}

BcStatus bc_program_nquit(BcProgram *p) {

	BcStatus s;
	BcResult *opnd;
	BcNum *num = NULL;
	unsigned long val;

	s = bc_program_prep(p, &opnd, &num);
	if (s) return s;
	s = bc_num_ulong(num, &val);
	if (s) return s;

	bc_vec_pop(&p->results);

	if (p->stack.len < val) return BC_STATUS_EXEC_STACK;
	else if (p->stack.len == val) return BC_STATUS_QUIT;

	bc_vec_npop(&p->stack, val);

	return s;
}

BcStatus bc_program_execStr(BcProgram *p, char *code, size_t *bgn, bool cond) {

	BcStatus s = BC_STATUS_SUCCESS;
	BcResult *r;
	char **str;
	BcFunc *f;
	BcParse prs;
	BcInstPtr ip;
	size_t fidx, sidx;
	BcNum *n;
	bool exec;

	if (!BC_PROG_STACK(&p->results, 1)) return BC_STATUS_EXEC_STACK;

	r = bc_vec_top(&p->results);

	if (cond) {

		BcVec *v;
		char *name, *then_name = bc_program_name(code, bgn), *else_name = NULL;

		if (code[*bgn] == BC_PARSE_STREND) (*bgn) += 1;
		else else_name = bc_program_name(code, bgn);

		exec = r->d.n.len != 0;

		if (exec) name = then_name;
		else if (else_name != NULL) {
			exec = true;
			name = else_name;
		}

		if (exec) {
			v = bc_program_search(p, name, true);
			n = bc_vec_top(v);
		}

		free(then_name);
		free(else_name);

		if (!exec) goto exit;
		if (!BC_PROG_STR(n)) {
			s = BC_STATUS_EXEC_BAD_TYPE;
			goto exit;
		}

		sidx = n->rdx;
	}
	else {

		if (r->t == BC_RESULT_STR) sidx = r->d.id.idx;
		else if (r->t == BC_RESULT_VAR) {
			s = bc_program_num(p, r, &n, false);
			if (s || !BC_PROG_STR(n)) goto exit;
			sidx = n->rdx;
		}
		else goto exit;
	}

	fidx = sidx + BC_PROG_REQ_FUNCS;
	assert(p->strs.len > sidx && p->fns.len > fidx);

	str = bc_vec_item(&p->strs, sidx);
	f = bc_vec_item(&p->fns, fidx);

	if (f->code.len == 0) {

		vm->parse_init(&prs, p, fidx);
		s = bc_parse_text(&prs, *str);
		if (s) goto err;
		s = vm->parse_expr(&prs, BC_PARSE_NOCALL);
		if (s) goto err;

		if (prs.l.t.t != BC_LEX_EOF) {
			s = BC_STATUS_PARSE_BAD_EXP;
			goto err;
		}

		bc_parse_free(&prs);
	}

	ip.idx = 0;
	ip.len = p->results.len;
	ip.func = fidx;

	bc_vec_pop(&p->results);
	bc_vec_push(&p->stack, &ip);

	return BC_STATUS_SUCCESS;

err:
	bc_parse_free(&prs);
	f = bc_vec_item(&p->fns, fidx);
	bc_vec_npop(&f->code, f->code.len);
exit:
	bc_vec_pop(&p->results);
	return s;
}
#endif // DC_ENABLED

void bc_program_pushGlobal(BcProgram *p, char inst) {

	BcResult res;
	unsigned long val;

	assert(inst == BC_INST_IBASE || inst == BC_INST_SCALE ||
	       inst == BC_INST_OBASE);

	res.t = inst - BC_INST_IBASE + BC_RESULT_IBASE;
	if (inst == BC_INST_IBASE) val = (unsigned long) p->ib_t;
	else if (inst == BC_INST_SCALE) val = (unsigned long) p->scale;
	else val = (unsigned long) p->ob_t;

	bc_num_init(&res.d.n, BC_NUM_DEF_SIZE);
	bc_num_ulong2num(&res.d.n, val);
	bc_vec_push(&p->results, &res);
}

void bc_program_free(BcProgram *p) {
	assert(p);
	bc_num_free(&p->ib);
	bc_num_free(&p->ob);
	bc_num_free(&p->hexb);
#if DC_ENABLED
	bc_num_free(&p->strmb);
#endif // DC_ENABLED
	bc_vec_free(&p->fns);
	bc_vec_free(&p->fn_map);
	bc_vec_free(&p->vars);
	bc_vec_free(&p->var_map);
	bc_vec_free(&p->arrs);
	bc_vec_free(&p->arr_map);
	bc_vec_free(&p->strs);
	bc_vec_free(&p->consts);
	bc_vec_free(&p->results);
	bc_vec_free(&p->stack);
	bc_num_free(&p->last);
	bc_num_free(&p->zero);
	bc_num_free(&p->one);
}

void bc_program_init(BcProgram *p) {

	size_t idx;
	BcInstPtr ip;

	assert(p);

	memset(p, 0, sizeof(BcProgram));
	memset(&ip, 0, sizeof(BcInstPtr));

	p->nchars = p->scale = 0;

	bc_num_init(&p->ib, BC_NUM_DEF_SIZE);
	bc_num_ten(&p->ib);
	p->ib_t = 10;

	bc_num_init(&p->ob, BC_NUM_DEF_SIZE);
	bc_num_ten(&p->ob);
	p->ob_t = 10;

	bc_num_init(&p->hexb, BC_NUM_DEF_SIZE);
	bc_num_ten(&p->hexb);
	p->hexb.num[0] = 6;

#if DC_ENABLED
	bc_num_init(&p->strmb, BC_NUM_DEF_SIZE);
	bc_num_ulong2num(&p->strmb, UCHAR_MAX + 1);
#endif // DC_ENABLED

	bc_num_init(&p->last, BC_NUM_DEF_SIZE);
	bc_num_zero(&p->last);

	bc_num_init(&p->zero, BC_NUM_DEF_SIZE);
	bc_num_zero(&p->zero);

	bc_num_init(&p->one, BC_NUM_DEF_SIZE);
	bc_num_one(&p->one);

	bc_vec_init(&p->fns, sizeof(BcFunc), bc_func_free);
	bc_map_init(&p->fn_map);

	bc_program_addFunc(p, bc_vm_strdup(bc_func_main), &idx);
	assert(idx == BC_PROG_MAIN);
	bc_program_addFunc(p, bc_vm_strdup(bc_func_read), &idx);
	assert(idx == BC_PROG_READ);

	bc_vec_init(&p->vars, sizeof(BcVec), bc_vec_free);
	bc_map_init(&p->var_map);

	bc_vec_init(&p->arrs, sizeof(BcVec), bc_vec_free);
	bc_map_init(&p->arr_map);

	bc_vec_init(&p->strs, sizeof(char*), bc_string_free);
	bc_vec_init(&p->consts, sizeof(char*), bc_string_free);
	bc_vec_init(&p->results, sizeof(BcResult), bc_result_free);
	bc_vec_init(&p->stack, sizeof(BcInstPtr), NULL);
	bc_vec_push(&p->stack, &ip);
}

void bc_program_addFunc(BcProgram *p, char *name, size_t *idx) {

	BcStatus s;
	BcId entry, *entry_ptr;
	BcFunc f;

	assert(p && name && idx);

	entry.name = name;
	entry.idx = p->fns.len;

	s = bc_map_insert(&p->fn_map, &entry, idx);
	if (s) free(name);

	entry_ptr = bc_vec_item(&p->fn_map, *idx);
	*idx = entry_ptr->idx;

	if (s == BC_STATUS_VEC_ITEM_EXISTS) {

		BcFunc *func = bc_vec_item(&p->fns, entry_ptr->idx);

		// We need to reset these, so the function can be repopulated.
		func->nparams = 0;
		bc_vec_npop(&func->autos, func->autos.len);
		bc_vec_npop(&func->code, func->code.len);
		bc_vec_npop(&func->labels, func->labels.len);
	}
	else {
		bc_func_init(&f);
		bc_vec_push(&p->fns, &f);
	}
}

BcStatus bc_program_reset(BcProgram *p, BcStatus s) {

	BcFunc *f;
	BcInstPtr *ip;

	bc_vec_npop(&p->stack, p->stack.len - 1);
	bc_vec_npop(&p->results, p->results.len);

	f = bc_vec_item(&p->fns, 0);
	ip = bc_vec_top(&p->stack);
	ip->idx = f->code.len;

	if (!s && BC_SIGINT && BC_I) return BC_STATUS_QUIT;

	vm->sig = 0;

	if (!s || s == BC_STATUS_EXEC_SIGNAL) {
		if (vm->ttyin) {
			bc_vm_puts(bc_program_ready_msg, stderr);
			bc_vm_fflush(stderr);
			s = BC_STATUS_SUCCESS;
		}
		else s = BC_STATUS_QUIT;
	}

	return s;
}

BcStatus bc_program_exec(BcProgram *p) {

	BcStatus s = BC_STATUS_SUCCESS;
	size_t idx;
	BcResult r, *ptr;
	BcNum *num;
	BcInstPtr *ip = bc_vec_top(&p->stack);
	BcFunc *func = bc_vec_item(&p->fns, ip->func);
	char *code = func->code.v;
	bool cond = false;

	while (!s && ip->idx < func->code.len) {

		char inst = code[(ip->idx)++];

		switch (inst) {

#if BC_ENABLED
			case BC_INST_JUMP_ZERO:
			{
				s = bc_program_prep(p, &ptr, &num);
				if (s) return s;
				cond = !bc_num_cmp(num, &p->zero);
				bc_vec_pop(&p->results);
			}
			// Fallthrough.
			case BC_INST_JUMP:
			{
				size_t *addr;
				idx = bc_program_index(code, &ip->idx);
				addr = bc_vec_item(&func->labels, idx);
				if (inst == BC_INST_JUMP || cond) ip->idx = *addr;
				break;
			}

			case BC_INST_CALL:
			{
				s = bc_program_call(p, code, &ip->idx);
				break;
			}

			case BC_INST_INC_PRE:
			case BC_INST_DEC_PRE:
			case BC_INST_INC_POST:
			case BC_INST_DEC_POST:
			{
				s = bc_program_incdec(p, inst);
				break;
			}

			case BC_INST_HALT:
			{
				s = BC_STATUS_QUIT;
				break;
			}

			case BC_INST_RET:
			case BC_INST_RET0:
			{
				s = bc_program_return(p, inst);
				break;
			}

			case BC_INST_BOOL_OR:
			case BC_INST_BOOL_AND:
#endif // BC_ENABLED
			case BC_INST_REL_EQ:
			case BC_INST_REL_LE:
			case BC_INST_REL_GE:
			case BC_INST_REL_NE:
			case BC_INST_REL_LT:
			case BC_INST_REL_GT:
			{
				s = bc_program_logical(p, inst);
				break;
			}

			case BC_INST_READ:
			{
				s = bc_program_read(p);
				break;
			}

			case BC_INST_VAR:
			{
				s = bc_program_pushVar(p, code, &ip->idx, false, false);
				break;
			}

			case BC_INST_ARRAY_ELEM:
			case BC_INST_ARRAY:
			{
				s = bc_program_pushArray(p, code, &ip->idx, inst);
				break;
			}

			case BC_INST_LAST:
			{
				r.t = BC_RESULT_LAST;
				bc_vec_push(&p->results, &r);
				break;
			}

			case BC_INST_IBASE:
			case BC_INST_SCALE:
			case BC_INST_OBASE:
			{
				bc_program_pushGlobal(p, inst);
				break;
			}

			case BC_INST_SCALE_FUNC:
			case BC_INST_LENGTH:
			case BC_INST_SQRT:
			{
				s = bc_program_builtin(p, inst);
				break;
			}

			case BC_INST_NUM:
			{
				r.t = BC_RESULT_CONSTANT;
				r.d.id.idx = bc_program_index(code, &ip->idx);
				bc_vec_push(&p->results, &r);
				break;
			}

			case BC_INST_POP:
			{
				if (!BC_PROG_STACK(&p->results, 1)) s = BC_STATUS_EXEC_STACK;
				else bc_vec_pop(&p->results);
				break;
			}

			case BC_INST_POP_EXEC:
			{
				assert(BC_PROG_STACK(&p->stack, 2));
				bc_vec_pop(&p->stack);
				break;
			}

			case BC_INST_PRINT:
			case BC_INST_PRINT_POP:
			case BC_INST_PRINT_STR:
			{
				s = bc_program_print(p, inst, 0);
				break;
			}

			case BC_INST_STR:
			{
				r.t = BC_RESULT_STR;
				r.d.id.idx = bc_program_index(code, &ip->idx);
				bc_vec_push(&p->results, &r);
				break;
			}

			case BC_INST_POWER:
			case BC_INST_MULTIPLY:
			case BC_INST_DIVIDE:
			case BC_INST_MODULUS:
			case BC_INST_PLUS:
			case BC_INST_MINUS:
			{
				s = bc_program_op(p, inst);
				break;
			}

			case BC_INST_BOOL_NOT:
			{
				s = bc_program_prep(p, &ptr, &num);
				if (s) return s;

				bc_num_init(&r.d.n, BC_NUM_DEF_SIZE);
				(!bc_num_cmp(num, &p->zero) ? bc_num_one : bc_num_zero)(&r.d.n);
				bc_program_retire(p, &r, BC_RESULT_TEMP);

				break;
			}

			case BC_INST_NEG:
			{
				s = bc_program_negate(p);
				break;
			}

#if BC_ENABLED
			case BC_INST_ASSIGN_POWER:
			case BC_INST_ASSIGN_MULTIPLY:
			case BC_INST_ASSIGN_DIVIDE:
			case BC_INST_ASSIGN_MODULUS:
			case BC_INST_ASSIGN_PLUS:
			case BC_INST_ASSIGN_MINUS:
#endif // BC_ENABLED
			case BC_INST_ASSIGN:
			{
				s = bc_program_assign(p, inst);
				break;
			}
#if DC_ENABLED
			case BC_INST_MODEXP:
			{
				s = bc_program_modexp(p);
				break;
			}

			case BC_INST_DIVMOD:
			{
				s = bc_program_divmod(p);
				break;
			}

			case BC_INST_EXECUTE:
			case BC_INST_EXEC_COND:
			{
				cond = inst == BC_INST_EXEC_COND;
				s = bc_program_execStr(p, code, &ip->idx, cond);
				break;
			}

			case BC_INST_PRINT_STACK:
			{
				for (idx = 0; !s && idx < p->results.len; ++idx)
					s = bc_program_print(p, BC_INST_PRINT, idx);
				break;
			}

			case BC_INST_CLEAR_STACK:
			{
				bc_vec_npop(&p->results, p->results.len);
				break;
			}

			case BC_INST_STACK_LEN:
			{
				bc_program_stackLen(p);
				break;
			}

			case BC_INST_DUPLICATE:
			{
				if (!BC_PROG_STACK(&p->results, 1)) return BC_STATUS_EXEC_STACK;
				ptr = bc_vec_top(&p->results);
				bc_result_copy(&r, ptr);
				bc_vec_push(&p->results, &r);
				break;
			}

			case BC_INST_SWAP:
			{
				BcResult *ptr2;

				if (!BC_PROG_STACK(&p->results, 2)) return BC_STATUS_EXEC_STACK;

				ptr = bc_vec_item_rev(&p->results, 0);
				ptr2 = bc_vec_item_rev(&p->results, 1);
				memcpy(&r, ptr, sizeof(BcResult));
				memcpy(ptr, ptr2, sizeof(BcResult));
				memcpy(ptr2, &r, sizeof(BcResult));

				break;
			}

			case BC_INST_ASCIIFY:
			{
				s = bc_program_asciify(p);
				break;
			}

			case BC_INST_PRINT_STREAM:
			{
				s = bc_program_printStream(p);
				break;
			}

			case BC_INST_LOAD:
			case BC_INST_PUSH_VAR:
			{
				bool copy = inst == BC_INST_LOAD;
				s = bc_program_pushVar(p, code, &ip->idx, true, copy);
				break;
			}

			case BC_INST_PUSH_TO_VAR:
			{
				char *name = bc_program_name(code, &ip->idx);
				s = bc_program_copyToVar(p, name, true);
				free(name);
				break;
			}

			case BC_INST_QUIT:
			{
				if (p->stack.len <= 2) s = BC_STATUS_QUIT;
				else bc_vec_npop(&p->stack, 2);
				break;
			}

			case BC_INST_NQUIT:
			{
				s = bc_program_nquit(p);
				break;
			}
#endif // DC_ENABLED
#ifndef NDEBUG
			default:
			{
				assert(false);
				break;
			}
#endif // NDEBUG
		}

		if ((s && s != BC_STATUS_QUIT) || BC_SIGINT) s = bc_program_reset(p, s);

		// If the stack has changed, pointers may be invalid.
		ip = bc_vec_top(&p->stack);
		func = bc_vec_item(&p->fns, ip->func);
		code = func->code.v;
	}

	return s;
}

#ifndef NDEBUG
#if BC_ENABLED && DC_ENABLED
void bc_program_printIndex(char *code, size_t *bgn) {

	char byte, i, bytes = code[(*bgn)++];
	unsigned long val = 0;

	for (byte = 1, i = 0; byte != 0 && i < bytes; ++i) {
		byte = code[(*bgn)++];
		if (byte != 0) val |= ((unsigned long) byte) << (CHAR_BIT * i);
	}

	bc_vm_printf(" (%lu) ", val);
}

void bc_program_printName(char *code, size_t *bgn) {

	char byte = code[(*bgn)++];

	bc_vm_printf(" (");

	for (; byte != 0 && byte != BC_PARSE_STREND; byte = code[(*bgn)++])
		bc_vm_putchar(byte);

	assert(byte);

	bc_vm_printf(") ");
}

void bc_program_printStr(BcProgram *p, char *code, size_t *bgn) {

	size_t idx = bc_program_index(code, bgn);
	char *s;

	assert(idx < p->strs.len);

	s = *((char**) bc_vec_item(&p->strs, idx));

	bc_vm_printf(" (\"%s\") ", s);
}

void bc_program_printInst(BcProgram *p, char *code, size_t *bgn) {

	char inst = code[(*bgn)++];

	bc_vm_putchar(bc_inst_chars[(uint32_t) inst]);

	if (inst == BC_INST_VAR || inst == BC_INST_ARRAY_ELEM ||
	    inst == BC_INST_ARRAY)
	{
		bc_program_printName(code, bgn);
	}
	else if (inst == BC_INST_STR) bc_program_printStr(p, code, bgn);
	else if (inst == BC_INST_NUM) {
		size_t idx = bc_program_index(code, bgn);
		char **str = bc_vec_item(&p->consts, idx);
		bc_vm_printf("(%s)", *str);
	}
	else if (inst == BC_INST_CALL ||
	         (inst > BC_INST_STR && inst <= BC_INST_JUMP_ZERO))
	{
		bc_program_printIndex(code, bgn);
		if (inst == BC_INST_CALL) bc_program_printIndex(code, bgn);
	}
}

void bc_program_code(BcProgram *p) {

	BcFunc *f;
	char *code;
	BcInstPtr ip;
	size_t i;

	for (i = 0; i < p->fns.len; ++i) {

		ip.idx = ip.len = 0;
		ip.func = i;

		f = bc_vec_item(&p->fns, ip.func);
		code = f->code.v;

		bc_vm_printf("func[%zu]:\n", ip.func);
		while (ip.idx < f->code.len) bc_program_printInst(p, code, &ip.idx);
		bc_vm_printf("\n\n");
	}
}
#endif // BC_ENABLED && DC_ENABLED
#endif // NDEBUG
