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

#include <unistd.h>

#include <io.h>
#include <parse.h>
#include <program.h>
#include <vm.h>

BcStatus bc_program_search(BcProgram *p, char *name, BcVec **ret, bool var) {

	BcStatus s;
	BcEntry e, *ptr;
	BcVec *v;
	BcVecO *vo;
	size_t i;
	BcResultData data;
	bool new;

	v = var ? &p->vars : &p->arrs;
	vo = var ? &p->var_map : &p->arr_map;

	e.name = name;
	e.idx = v->len;

	if ((new = (s = bc_veco_insert(vo, &e, &i)) != BC_STATUS_VEC_ITEM_EXISTS)) {
		if (s) return s;
		if ((s = bc_array_init(&data.v, var))) return s;
		if ((s = bc_vec_push(v, &data.v))) goto err;
	}

	ptr = bc_veco_item(vo, i);
	if (new && !(ptr->name = strdup(e.name))) return BC_STATUS_ALLOC_ERR;
	*ret = bc_vec_item(v, ptr->idx);

	return BC_STATUS_SUCCESS;

err:
	bc_vec_free(&data.v);
	return s;
}

BcStatus bc_program_num(BcProgram *p, BcResult *r, BcNum **num, bool hex) {

	BcStatus s = BC_STATUS_SUCCESS;

	switch (r->t) {

		case BC_RESULT_STR:
		case BC_RESULT_TEMP:
		case BC_RESULT_SCALE:
		{
			*num = &r->d.n;
			break;
		}

		case BC_RESULT_CONSTANT:
		{
			char **str = bc_vec_item(&p->consts, r->d.id.idx);
			size_t base_t, len = strlen(*str);
			BcNum *base;

			if ((s = bc_num_init(&r->d.n, len))) return s;

			hex = hex && len == 1;
			base = hex ? &p->hexb : &p->ib;
			base_t = hex ? BC_NUM_MAX_IBASE : p->ib_t;

			if ((s = bc_num_parse(&r->d.n, *str, base, base_t))) {
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

			s = bc_program_search(p, r->d.id.name, &v, r->t == BC_RESULT_VAR);
			if (s) return s;

			if (r->t == BC_RESULT_ARRAY_ELEM) {

				if ((v = bc_vec_top(v))->len <= r->d.id.idx) {
					if ((s = bc_array_expand(v, r->d.id.idx + 1))) return s;
				}

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

		case BC_RESULT_IBASE:
		{
			*num = &p->ib;
			break;
		}

		case BC_RESULT_OBASE:
		{
			*num = &p->ob;
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

BcStatus bc_program_binOpPrep(BcProgram *p, BcResult **left, BcNum **lval,
                              BcResult **right, BcNum **rval, bool assign)
{
	BcStatus s;
	bool hex;
	BcResultType lt, rt;

	assert(p && left && lval && right && rval);

	if (!BC_PROG_CHECK_STACK(&p->results, 2)) return BC_STATUS_EXEC_SMALL_STACK;

	*right = bc_vec_item_rev(&p->results, 0);
	*left = bc_vec_item_rev(&p->results, 1);

	lt = (*left)->t;
	rt = (*right)->t;
	hex = assign && (lt == BC_RESULT_IBASE || lt == BC_RESULT_OBASE);

	if (lt == BC_RESULT_ARRAY || lt == BC_RESULT_STR || rt == BC_RESULT_ARRAY)
		return BC_STATUS_EXEC_BAD_TYPE;

	if ((s = bc_program_num(p, *left, lval, false))) return s;
	if ((s = bc_program_num(p, *right, rval, hex))) return s;

#ifdef DC_ENABLED
	assert(lt != BC_RESULT_VAR || !BC_PROG_STR_VAR(*lval) || assign);
#else // DC_ENABLED
	assert(rt != BC_RESULT_STR);
#endif // DC_ENABLED

	// We run this again under these conditions in case any vector has been
	// reallocated out from under the BcNums or arrays we had.
	if (lt == rt && (lt == BC_RESULT_VAR || lt == BC_RESULT_ARRAY_ELEM))
		s = bc_program_num(p, *left, lval, false);

	return s;
}

BcStatus bc_program_binOpRetire(BcProgram *p, BcResult *r) {
	r->t = BC_RESULT_TEMP;
	bc_vec_pop(&p->results);
	bc_vec_pop(&p->results);
	return bc_vec_push(&p->results, r);
}

BcStatus bc_program_prep(BcProgram *p, BcResult **r, BcNum **n, bool arr)
{
	BcStatus s;

	assert(p && r && n);

	if (!BC_PROG_CHECK_STACK(&p->results, 1)) return BC_STATUS_EXEC_SMALL_STACK;

	*r = bc_vec_top(&p->results);

	if (((*r)->t == BC_RESULT_ARRAY && !arr) || (*r)->t == BC_RESULT_STR)
		return BC_STATUS_EXEC_BAD_TYPE;

	if ((s = bc_program_num(p, *r, n, false))) return s;

#ifdef DC_ENABLED
	assert((*r)->t != BC_RESULT_VAR || !BC_PROG_STR_VAR(*n));
#endif // DC_ENABLED

	return s;
}

BcStatus bc_program_retire(BcProgram *p, BcResult *r, BcResultType t) {
	r->t = t;
	bc_vec_pop(&p->results);
	return bc_vec_push(&p->results, r);
}

BcStatus bc_program_op(BcProgram *p, uint8_t inst) {

	BcStatus s;
	BcResult *opd1, *opd2, res;
	BcNum *n1, *n2;
	BcNumBinaryOp op;

	if ((s = bc_program_binOpPrep(p, &opd1, &n1, &opd2, &n2, false))) return s;
	if ((s = bc_num_init(&res.d.n, BC_NUM_DEF_SIZE))) return s;

	op = bc_program_ops[inst - BC_INST_POWER];
	if ((s = op(n1, n2, &res.d.n, p->scale))) goto err;
	if ((s = bc_program_binOpRetire(p, &res))) goto err;

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
	BcFunc *f = bc_vec_item(&p->fns, BC_PROG_READ);

	f->code.len = 0;

	if ((s = bc_vec_init(&buf, sizeof(char), NULL))) return BC_STATUS_ALLOC_ERR;
	if ((s = bc_io_getline(&buf, "read> "))) goto io_err;

	if ((s = p->parse_init(&parse, p, BC_PROG_READ))) goto io_err;
	bc_lex_file(&parse.l, bc_program_stdin_name);
	if ((s = bc_lex_text(&parse.l, buf.v))) goto exec_err;

	if ((s = p->parse_expr(&parse, BC_PARSE_NOREAD))) return s;

	if (parse.l.t.t != BC_LEX_NLINE && parse.l.t.t != BC_LEX_EOF) {
		s = BC_STATUS_EXEC_BAD_READ_EXPR;
		goto exec_err;
	}

	ip.func = BC_PROG_READ;
	ip.idx = 0;
	ip.len = p->results.len;

	if ((s = bc_vec_push(&p->stack, &ip))) goto exec_err;
	if ((s = bc_program_exec(p))) goto exec_err;

	bc_vec_pop(&p->stack);

exec_err:
	bc_parse_free(&parse);
io_err:
	bc_vec_free(&buf);
	return s;
}

size_t bc_program_index(char *code, size_t *bgn) {

	uint8_t amt = code[(*bgn)++], i = 0;
	size_t res = 0;

	for (; i < amt; ++i) res |= (((size_t) code[(*bgn)++]) << (i * CHAR_BIT));

	return res;
}

char* bc_program_name(char *code, size_t *bgn) {

	size_t len, i;
	char byte, *s, *string = (char*) (code + *bgn), *ptr;

	ptr = strchr(string, BC_PARSE_STREND);
	if (ptr) len = ((unsigned long) ptr) - ((unsigned long) string);
	else len = strlen(string);

	if (!(s = malloc(len + 1))) return NULL;

	for (i = 0; (byte = (char) code[(*bgn)++]) && byte != BC_PARSE_STREND; ++i)
		s[i] = byte;

	s[i] = '\0';

	return s;
}

BcStatus bc_program_printString(const char *str, size_t *nchars) {

	size_t i, len = strlen(str);

	for (i = 0; i < len; ++i, ++(*nchars)) {

		int err, c;

		if ((c = str[i]) != '\\') err = putchar(c);
		else {

			assert(i + 1 < len);
			c = str[++i];

			switch (c) {

				case 'a':
				{
					err = putchar('\a');
					break;
				}

				case 'b':
				{
					err = putchar('\b');
					break;
				}

				case 'e':
				{
					err = putchar('\\');
					break;
				}

				case 'f':
				{
					err = putchar('\f');
					break;
				}

				case 'n':
				{
					err = putchar('\n');
					*nchars = SIZE_MAX;
					break;
				}

				case 'r':
				{
					err = putchar('\r');
					break;
				}

				case 'q':
				{
					err = putchar('"');
					break;
				}

				case 't':
				{
					err = putchar('\t');
					break;
				}

				default:
				{
					// Just print the character.
					err = putchar(c);
					break;
				}
			}
		}

		if (err == EOF) return BC_STATUS_IO_ERR;
	}

	return BC_STATUS_SUCCESS;
}

BcStatus bc_program_print(BcProgram *p, uint8_t inst, size_t idx) {

	BcStatus s = BC_STATUS_SUCCESS;
	BcResult *r;
	size_t len, i;
	char *str;
	bool pop = inst != BC_INST_PRINT;

	assert(p);

	if (!BC_PROG_CHECK_STACK(&p->results, idx + 1))
		return BC_STATUS_EXEC_SMALL_STACK;

	r = bc_vec_item_rev(&p->results, idx);

	if (r->t == BC_RESULT_STR) {

		assert(r->d.id.idx < p->strs.len);

		str = *((char**) bc_vec_item(&p->strs, r->d.id.idx));

		if (inst == BC_INST_PRINT_STR) {
			for (i = 0, len = strlen(str); i < len; ++i) {
				char c = str[i];
				if (putchar(c) == EOF) return BC_STATUS_IO_ERR;
				if (c == '\n') p->nchars = SIZE_MAX;
				++p->nchars;
			}
		}
		else {
			if ((s = bc_program_printString(str, &p->nchars))) return s;
			if (inst == BC_INST_PRINT && putchar('\n') == EOF)
				s = BC_STATUS_IO_ERR;
		}
	}
	else {

		BcNum *num = NULL;

		assert(inst != BC_INST_PRINT_STR);

		if ((s = bc_program_num(p, r, &num, false))) return s;

		s = bc_num_print(num, &p->ob, p->ob_t, !pop, &p->nchars, p->len);
		if (!s && !BC_PROG_STR_VAR(num)) s = bc_num_copy(&p->last, num);
	}

	if (!s && pop) bc_vec_pop(&p->results);

	return s;
}

BcStatus bc_program_negate(BcProgram *p) {

	BcStatus s;
	BcResult res, *ptr;
	BcNum *num = NULL;

	if ((s = bc_program_prep(p, &ptr, &num, false))) return s;
	if ((s = bc_num_init(&res.d.n, num->len))) return s;
	if ((s = bc_num_copy(&res.d.n, num))) goto err;

	res.d.n.neg = !res.d.n.neg;

	if ((s = bc_program_retire(p, &res, BC_RESULT_TEMP))) goto err;

	return s;

err:
	bc_num_free(&res.d.n);
	return s;
}

BcStatus bc_program_logical(BcProgram *p, uint8_t inst) {

	BcStatus s;
	BcResult *opd1, *opd2, res;
	BcNum *n1, *n2;
	bool cond = 0;
	ssize_t cmp;

	if ((s = bc_program_binOpPrep(p, &opd1, &n1, &opd2, &n2, false))) return s;
	if ((s = bc_num_init(&res.d.n, BC_NUM_DEF_SIZE))) return s;

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

	if ((s = bc_program_binOpRetire(p, &res))) goto err;

	return s;

err:
	bc_num_free(&res.d.n);
	return s;
}

#ifdef DC_ENABLED
BcStatus bc_program_assignStr(BcProgram *p, BcResult *r, BcVec *v, bool push)
{
	BcStatus s;
	BcNum n2;
	BcResult res;

	memset(&n2, 0, sizeof(BcNum));
	n2.rdx = res.d.id.idx = r->d.id.idx;
	res.t = BC_RESULT_STR;

	if (!push) {
		if (!BC_PROG_CHECK_STACK(&p->results, 2))
			return BC_STATUS_EXEC_SMALL_STACK;
		bc_vec_pop(v);
		bc_vec_pop(&p->results);
	}

	bc_vec_pop(&p->results);

	if ((s = bc_vec_push(&p->results, &res))) return s;

	return bc_vec_push(v, &n2);
}
#endif // DC_ENABLED

BcStatus bc_program_copyToVar(BcProgram *p, char *name, bool var) {

	BcStatus s;
	BcResult *ptr, r;
	BcVec *v;
	BcNum *n;

	if (!BC_PROG_CHECK_STACK(&p->results, 1)) return BC_STATUS_EXEC_SMALL_STACK;

	ptr = bc_vec_top(&p->results);
	if ((ptr->t == BC_RESULT_ARRAY) != !var) return BC_STATUS_EXEC_BAD_TYPE;

	if ((s = bc_program_search(p, name, &v, var))) return s;

#ifdef DC_ENABLED
	if (ptr->t == BC_RESULT_STR && !var) return BC_STATUS_EXEC_BAD_TYPE;
	if (ptr->t == BC_RESULT_STR) return bc_program_assignStr(p, ptr, v, true);
#endif // DC_ENABLED

	if ((s = bc_program_num(p, ptr, &n, false))) return s;

	// Do this once more to make sure that pointers were not invalidated.
	if ((s = bc_program_search(p, name, &v, var))) return s;

	if (var) {
		if ((s = bc_num_init(&r.d.n, BC_NUM_DEF_SIZE))) return s;
		s = bc_num_copy(&r.d.n, n);
	}
	else {
		if ((s = bc_array_init(&r.d.v, true))) return s;
		s = bc_array_copy(&r.d.v, (BcVec*) n);
	}

	if (s || (s = bc_vec_push(v, &r.d))) goto err;

	bc_vec_pop(&p->results);

	return s;

err:
	if (var) bc_num_free(&r.d.n);
	else bc_vec_free(&r.d.v);
	return s;
}

BcStatus bc_program_assign(BcProgram *p, uint8_t inst) {

	BcStatus s;
	BcResult *left, *right, res;
	BcNum *l, *r;
	unsigned long val, max;
	bool assign = inst == BC_INST_ASSIGN;

	if ((s = bc_program_binOpPrep(p, &left, &l, &right, &r, assign))) return s;

#ifdef DC_ENABLED
	assert(left->t != BC_RESULT_STR);

	if (right->t == BC_RESULT_STR) {

		BcVec *v;

		assert(assign);

		if (left->t != BC_RESULT_VAR) return BC_STATUS_EXEC_BAD_TYPE;
		if ((s = bc_program_search(p, left->d.id.name, &v, true))) return s;

		return bc_program_assignStr(p, right, v, false);
	}
#endif // DC_ENABLED

	if (left->t == BC_RESULT_CONSTANT || left->t == BC_RESULT_TEMP)
		return BC_STATUS_PARSE_BAD_ASSIGN;

#ifdef BC_ENABLED
	if (inst == BC_INST_ASSIGN_DIVIDE && !bc_num_cmp(r, &p->zero))
		return BC_STATUS_MATH_DIVIDE_BY_ZERO;

	if (assign) s = bc_num_copy(l, r);
	else s = bc_program_ops[inst - BC_INST_ASSIGN_POWER](l, r, l, p->scale);

	if (s) return s;
#else // BC_ENABLED
	assert(assign);
	if ((s = bc_num_copy(l, r))) return s;
#endif // BC_ENABLED

	if (left->t == BC_RESULT_IBASE || left->t == BC_RESULT_OBASE) {

		size_t *ptr = left->t == BC_RESULT_IBASE ? &p->ib_t : &p->ob_t;

		if ((s = bc_num_ulong(r, &val))) return s;

		max = left->t == BC_RESULT_IBASE ? BC_NUM_MAX_IBASE : BC_MAX_OBASE;

		if (val < BC_NUM_MIN_BASE || val > max)
			return left->t - BC_RESULT_IBASE + BC_STATUS_EXEC_BAD_IBASE;

		*ptr = (size_t) val;
	}
	else if (left->t == BC_RESULT_SCALE) {

		if ((s = bc_num_ulong(l, &val))) return s;
		if (val > (unsigned long) BC_MAX_SCALE) return BC_STATUS_EXEC_BAD_SCALE;

		p->scale = (size_t) val;
	}

	if ((s = bc_num_init(&res.d.n, l->len))) return s;
	if ((s = bc_num_copy(&res.d.n, l))) goto err;
	if ((s = bc_program_binOpRetire(p, &res))) goto err;

	return s;

err:
	bc_num_free(&res.d.n);
	return s;
}

BcStatus bc_program_pushVar(BcProgram *p, char *code, size_t *bgn,
                            bool pop, bool copy)
{
	BcStatus s;
	BcResult r;
	char *name;
#ifdef DC_ENABLED // Exclude
	BcNum *num;
	BcVec *v;
#else // DC_ENABLED
	(void) pop;
#endif // DC_ENABLED Exclude

	if (!(name = bc_program_name(code, bgn))) return BC_STATUS_ALLOC_ERR;
	r.t = BC_RESULT_VAR;
	r.d.id.name = name;

#ifdef DC_ENABLED
	if ((s = bc_program_search(p, name, &v, true))) goto err;
	num = bc_vec_top(v);

	if (pop || copy) {

		free(name);
		name = NULL;

		if ((pop = !BC_PROG_STR_VAR(num))) {

			r.t = BC_RESULT_TEMP;

			if (!BC_PROG_CHECK_STACK(v, 2 - copy)) {
				s = BC_STATUS_EXEC_SMALL_STACK;
				goto err;
			}

			if ((s = bc_num_init(&r.d.n, BC_NUM_DEF_SIZE))) goto err;
			if ((s = bc_num_copy(&r.d.n, num))) goto copy_err;
		}
		else {
			r.t = BC_RESULT_STR;
			r.d.id.idx = num->rdx;
		}

		if (!copy) bc_vec_pop(v);
	}
#endif // DC_ENABLED

	s = bc_vec_push(&p->results, &r);

#ifdef DC_ENABLED
copy_err:
	if (s && pop) bc_num_free(&r.d.n);
err:
#endif // DC_ENABLED
	if (s) free(name);
	return s;
}

BcStatus bc_program_pushArray(BcProgram *p, char *code,
                              size_t *bgn, uint8_t inst)
{
	BcStatus s;
	BcResult r;
	BcNum *num;

	if (!(r.d.id.name = bc_program_name(code, bgn))) return BC_STATUS_ALLOC_ERR;

	if (inst == BC_INST_ARRAY) {
		r.t = BC_RESULT_ARRAY;
		s = bc_vec_push(&p->results, &r);
	}
	else {

		BcResult *operand;
		unsigned long temp;

		if ((s = bc_program_prep(p, &operand, &num, false))) goto err;
		if ((s = bc_num_ulong(num, &temp))) goto err;

		if (temp > (unsigned long) BC_MAX_DIM) {
			s = BC_STATUS_EXEC_ARRAY_LEN;
			goto err;
		}

		r.d.id.idx = (size_t) temp;
		s = bc_program_retire(p, &r, BC_RESULT_ARRAY_ELEM);
	}

err:
	if (s) free(r.d.id.name);
	return s;
}

#ifdef BC_ENABLED
BcStatus bc_program_incdec(BcProgram *p, uint8_t inst) {

	BcStatus s;
	BcResult *ptr, res, copy;
	BcNum *num = NULL;
	uint8_t inst2 = inst;

	if ((s = bc_program_prep(p, &ptr, &num, false))) return s;

	if (inst == BC_INST_INC_POST || inst == BC_INST_DEC_POST) {
		copy.t = BC_RESULT_TEMP;
		if ((s = bc_num_init(&copy.d.n, num->len))) return s;
		if ((s = bc_num_copy(&copy.d.n, num))) goto err;
	}

	res.t = BC_RESULT_ONE;
	inst = inst == BC_INST_INC_PRE || inst == BC_INST_INC_POST ?
	           BC_INST_ASSIGN_PLUS : BC_INST_ASSIGN_MINUS;

	if ((s = bc_vec_push(&p->results, &res))) goto err;
	if ((s = bc_program_assign(p, inst))) goto err;

	if (inst2 == BC_INST_INC_POST || inst2 == BC_INST_DEC_POST) {
		bc_vec_pop(&p->results);
		if ((s = bc_vec_push(&p->results, &copy))) goto err;
	}

	return s;

err:
	if (inst2 == BC_INST_INC_POST || inst2 == BC_INST_DEC_POST)
		bc_num_free(&copy.d.n);
	return s;
}

BcStatus bc_program_call(BcProgram *p, char *code, size_t *idx) {

	BcStatus s = BC_STATUS_SUCCESS;
	BcInstPtr ip;
	size_t i, nparams = bc_program_index(code, idx);
	BcFunc *func;
	BcVec *v;
	BcAuto *a;
	BcResultData param;
	BcResult *arg;

	ip.idx = 0;
	ip.func = bc_program_index(code, idx);
	func = bc_vec_item(&p->fns, ip.func);

	if (!func->code.len) return BC_STATUS_EXEC_UNDEFINED_FUNC;
	if (nparams != func->nparams) return BC_STATUS_EXEC_MISMATCHED_PARAMS;
	ip.len = p->results.len - nparams;

	assert(BC_PROG_CHECK_STACK(&p->results, nparams));

	for (i = 0; i < nparams; ++i) {

		a = bc_vec_item(&func->autos, nparams - 1 - i);
		arg = bc_vec_top(&p->results);

		if (!a->var != (arg->t == BC_RESULT_ARRAY) || arg->t == BC_RESULT_STR)
			return BC_STATUS_EXEC_BAD_TYPE;

		if ((s = bc_program_copyToVar(p, a->name, a->var))) return s;
	}

	for (; i < func->autos.len; ++i) {

		a = bc_vec_item(&func->autos, i);
		if ((s = bc_program_search(p, a->name, &v, a->var))) return s;

		if (a->var) {
			if ((s = bc_num_init(&param.n, BC_NUM_DEF_SIZE))) return s;
			if ((s = bc_vec_push(v, &param.n))) goto err;
		}
		else {
			if ((s = bc_array_init(&param.v, true))) return s;
			if ((s = bc_vec_push(v, &param.v))) goto err;
		}
	}

	return bc_vec_push(&p->stack, &ip);

err:
	if (a->var) bc_num_free(&param.n);
	else bc_vec_free(&param.v);
	return s;
}

BcStatus bc_program_return(BcProgram *p, uint8_t inst) {

	BcStatus s;
	BcResult res;
	BcFunc *f;
	size_t i;
	BcInstPtr *ip = bc_vec_top(&p->stack);

	assert(BC_PROG_CHECK_STACK(&p->stack, 2));

	if (!BC_PROG_CHECK_STACK(&p->results, ip->len + inst == BC_INST_RET))
		return BC_STATUS_EXEC_SMALL_STACK;

	f = bc_vec_item(&p->fns, ip->func);
	res.t = BC_RESULT_TEMP;

	if (inst == BC_INST_RET) {

		BcNum *num;
		BcResult *operand = bc_vec_top(&p->results);

		if ((s = bc_program_num(p, operand, &num, false))) return s;
		if ((s = bc_num_init(&res.d.n, num->len))) return s;
		if ((s = bc_num_copy(&res.d.n, num))) goto err;
	}
	else {
		if ((s = bc_num_init(&res.d.n, BC_NUM_DEF_SIZE))) return s;
		bc_num_zero(&res.d.n);
	}

	// We need to pop arguments as well, so this takes that into account.
	for (i = 0; i < f->autos.len; ++i) {

		BcVec *v;
		BcAuto *a = bc_vec_item(&f->autos, i);

		if ((s = bc_program_search(p, a->name, &v, a->var))) goto err;

		bc_vec_pop(v);
	}

	bc_vec_npop(&p->results, p->results.len - ip->len);
	if ((s = bc_vec_push(&p->results, &res))) goto err;
	bc_vec_pop(&p->stack);

	return s;

err:
	bc_num_free(&res.d.n);
	return s;
}
#endif // BC_ENABLED

unsigned long bc_program_scale(BcNum *n) {
	return (unsigned long) n->rdx;
}

unsigned long bc_program_len(BcNum *n) {

	unsigned long len = n->len;

	if (n->rdx == n->len) {
		size_t i;
		for (i = n->len - 1; i < n->len && !n->num[i]; --len, --i);
	}

	return len;
}

BcStatus bc_program_builtin(BcProgram *p, uint8_t inst) {

	BcStatus s;
	BcResult *opnd;
	BcNum *num = NULL;
	BcResult res;

	if ((s = bc_program_prep(p, &opnd, &num, inst == BC_INST_LENGTH))) return s;
	if ((s = bc_num_init(&res.d.n, BC_NUM_DEF_SIZE))) return s;

	if (inst == BC_INST_SQRT) s = bc_num_sqrt(num, &res.d.n, p->scale);
#ifdef BC_ENABLED
	else if (inst == BC_INST_LENGTH && opnd->t == BC_RESULT_ARRAY) {
		BcVec *vec = (BcVec*) num;
		s = bc_num_ulong2num(&res.d.n, (unsigned long) vec->len);
	}
#endif // BC_ENABLED
	else {
		assert(opnd->t != BC_RESULT_ARRAY);
		BcProgramBuiltIn f = inst == BC_INST_LENGTH ? bc_program_len :
		                                              bc_program_scale;
		s = bc_num_ulong2num(&res.d.n, f(num));
	}

	if (s || (s = bc_program_retire(p, &res, BC_RESULT_TEMP))) goto err;

	return s;

err:
	bc_num_free(&res.d.n);
	return s;
}

#ifdef DC_ENABLED
BcStatus bc_program_divmod(BcProgram *p) {

	BcStatus s;
	BcResult *opd1, *opd2, res, res2;
	BcNum *n1, *n2;

	if ((s = bc_program_binOpPrep(p, &opd1, &n1, &opd2, &n2, false))) return s;
	if ((s = bc_num_init(&res.d.n, BC_NUM_DEF_SIZE))) return s;
	if ((s = bc_num_init(&res2.d.n, n2->len))) goto res2_err;

	if ((s = bc_num_div(n1, n2, &res2.d.n, p->scale))) goto err;
	if ((s = bc_num_rem(n1, n2, &res.d.n, p->scale))) goto err;

	if ((s = bc_program_binOpRetire(p, &res2))) goto err;
	res.t = BC_RESULT_TEMP;
	if ((s = bc_vec_push(&p->results, &res))) goto res2_err;

	return s;

err:
	bc_num_free(&res2.d.n);
res2_err:
	bc_num_free(&res.d.n);
	return s;
}

BcStatus bc_program_modexp(BcProgram *p) {

	BcStatus s;
	BcResult *opd1, *opd2, *opd3, res;
	BcNum *n1, *n2, *n3;

	if (!BC_PROG_CHECK_STACK(&p->results, 3)) return BC_STATUS_EXEC_SMALL_STACK;
	if ((s = bc_program_binOpPrep(p, &opd2, &n2, &opd3, &n3, false))) return s;

	opd1 = bc_vec_item_rev(&p->results, 2);
	if ((s = bc_program_num(p, opd1, &n1, false))) return s;

	// Make sure that the values have their pointers updated, if necessary.
	if (opd1->t == BC_RESULT_VAR || opd1->t == BC_RESULT_ARRAY_ELEM) {
		if (opd1->t == opd2->t) {
			if ((s = bc_program_num(p, opd2, &n2, false))) return s;
		}
		if (opd1->t == opd3->t) {
			if ((s = bc_program_num(p, opd3, &n3, false))) return s;
		}
	}

	if ((s = bc_num_init(&res.d.n, n3->len))) return s;

	if ((s = bc_num_modexp(n1, n2, n3, &res.d.n, p->scale))) goto err;
	bc_vec_pop(&p->results);

	if ((s = bc_program_binOpRetire(p, &res))) goto err;

	return s;

err:
	bc_num_free(&res.d.n);
	return s;
}

BcStatus bc_program_stackLen(BcProgram *p) {

	BcStatus s;
	BcResult res;
	size_t len = p->results.len;

	res.t = BC_RESULT_TEMP;

	if ((s = bc_num_init(&res.d.n, BC_NUM_DEF_SIZE))) return s;
	if ((s = bc_num_ulong2num(&res.d.n, len))) goto err;
	if ((s = bc_vec_push(&p->results, &res))) goto err;

	return s;

err:
	bc_num_free(&res.d.n);
	return s;
}

BcStatus bc_program_nquit(BcProgram *p) {

	BcStatus s;
	BcResult *opnd;
	BcNum *num = NULL;
	unsigned long val;

	if ((s = bc_program_prep(p, &opnd, &num, false))) return s;
	if ((s = bc_num_ulong(num, &val))) return s;

	bc_vec_pop(&p->results);

	if (p->stack.len < val) return BC_STATUS_EXEC_SMALL_STACK;
	else if (p->stack.len == val) return BC_STATUS_QUIT;

	bc_vec_npop(&p->stack, val);

	return s;
}

BcStatus bc_program_executeStr(BcProgram *p, char *code, size_t *bgn, bool cond)
{
	BcStatus s = BC_STATUS_SUCCESS;
	BcResult *r;
	char **str;
	BcFunc *f;
	BcParse prs;
	BcInstPtr ip;
	size_t fidx, sidx;
	BcNum *n;
	bool exec;

	if (!BC_PROG_CHECK_STACK(&p->results, cond))
		return BC_STATUS_EXEC_SMALL_STACK;

	r = bc_vec_top(&p->results);

	if (cond) {

		BcVec *v;
		char *name, *then_name = bc_program_name(code, bgn), *else_name = NULL;

		if (code[*bgn] == BC_PARSE_STREND) (*bgn) += 1;
		else else_name = bc_program_name(code, bgn);

		if ((exec = r->d.n.len)) name = then_name;
		else if ((exec = (else_name != NULL))) name = else_name;

		if (exec) {
			s = bc_program_search(p, name, &v, true);
			n = bc_vec_top(v);
		}

		free(then_name);
		free(else_name);

		if (s || !exec) goto exit;
		if (!BC_PROG_STR_VAR(n)) {
			s = BC_STATUS_EXEC_BAD_TYPE;
			goto exit;
		}

		sidx = n->rdx;
	}
	else {

		if (r->t == BC_RESULT_STR) sidx = r->d.id.idx;
		else if (r->t == BC_RESULT_VAR) {
			if ((s = bc_program_num(p, r, &n, false))) goto exit;
			if (!BC_PROG_STR_VAR(n)) goto exit;
			sidx = n->rdx;
		}
		else goto exit;
	}

	fidx = sidx + BC_PROG_REQ_FUNCS;
	assert(p->strs.len > sidx && p->fns.len > fidx);

	str = bc_vec_item(&p->strs, sidx);
	f = bc_vec_item(&p->fns, fidx);

	if (!f->code.len) {

		if ((s = p->parse_init(&prs, p, fidx))) goto exit;
		if ((s = bc_lex_text(&prs.l, *str))) goto err;

		if ((s = p->parse_expr(&prs, BC_PARSE_NOCALL))) goto err;

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

	return bc_vec_push(&p->stack, &ip);

err:
	bc_parse_free(&prs);
	bc_vec_npop(&f->code, f->code.len);
exit:
	bc_vec_pop(&p->results);
	return s;
}
#endif // DC_ENABLED

BcStatus bc_program_pushScale(BcProgram *p) {

	BcStatus s;
	BcResult res;

	res.t = BC_RESULT_SCALE;

	if ((s = bc_num_init(&res.d.n, BC_NUM_DEF_SIZE))) return s;
	if ((s = bc_num_ulong2num(&res.d.n, (unsigned long) p->scale))) goto err;
	if ((s = bc_vec_push(&p->results, &res))) goto err;

	return s;

err:
	bc_num_free(&res.d.n);
	return s;
}

BcStatus bc_program_init(BcProgram *p, size_t line_len,
                         BcParseInit init, BcParseExpr expr)
{
	BcStatus s;
	size_t idx;
	char *main_name = NULL, *read_name = NULL;
	BcInstPtr ip;

	assert(p);

	assert((unsigned long) sysconf(_SC_BC_BASE_MAX) <= BC_MAX_OBASE);
	assert((unsigned long) sysconf(_SC_BC_DIM_MAX) <= BC_MAX_DIM);
	assert((unsigned long) sysconf(_SC_BC_SCALE_MAX) <= BC_MAX_SCALE);
	assert((unsigned long) sysconf(_SC_BC_STRING_MAX) <= BC_MAX_STRING);

	p->nchars = p->scale = 0;
	p->len = line_len;
	p->parse_init = init;
	p->parse_expr = expr;

	if ((s = bc_num_init(&p->ib, BC_NUM_DEF_SIZE))) return s;
	bc_num_ten(&p->ib);
	p->ib_t = 10;

	if ((s = bc_num_init(&p->ob, BC_NUM_DEF_SIZE))) goto obase_err;
	bc_num_ten(&p->ob);
	p->ob_t = 10;

	if ((s = bc_num_init(&p->hexb, BC_NUM_DEF_SIZE))) goto hexb_err;
	bc_num_ten(&p->hexb);
	p->hexb.num[0] = 6;

	if ((s = bc_num_init(&p->last, BC_NUM_DEF_SIZE))) goto last_err;
	bc_num_zero(&p->last);

	if ((s = bc_num_init(&p->zero, BC_NUM_DEF_SIZE))) goto zero_err;
	bc_num_zero(&p->zero);

	if ((s = bc_num_init(&p->one, BC_NUM_DEF_SIZE))) goto one_err;
	bc_num_one(&p->one);

	if ((s = bc_vec_init(&p->fns, sizeof(BcFunc), bc_func_free))) goto fn_err;

	s = bc_veco_init(&p->fn_map, sizeof(BcEntry), bc_entry_free, bc_entry_cmp);
	if (s) goto func_map_err;

	if (!(main_name = strdup(bc_func_main))) {
		s = BC_STATUS_ALLOC_ERR;
		goto name_err;
	}

	s = bc_program_addFunc(p, main_name, &idx);
	if (s || idx != BC_PROG_MAIN) goto name_err;
	main_name = NULL;

	if (!(read_name = strdup(bc_func_read))) {
		s = BC_STATUS_ALLOC_ERR;
		goto name_err;
	}

	s = bc_program_addFunc(p, read_name, &idx);
	if (s || idx != BC_PROG_READ) goto name_err;
	read_name = NULL;

	if ((s = bc_vec_init(&p->vars, sizeof(BcVec), bc_vec_free))) goto name_err;
	s = bc_veco_init(&p->var_map, sizeof(BcEntry), bc_entry_free, bc_entry_cmp);
	if (s) goto var_map_err;

	if ((s = bc_vec_init(&p->arrs, sizeof(BcVec), bc_vec_free))) goto arr_err;
	s = bc_veco_init(&p->arr_map, sizeof(BcEntry), bc_entry_free, bc_entry_cmp);
	if (s) goto array_map_err;

	s = bc_vec_init(&p->strs, sizeof(char*), bc_string_free);
	if (s) goto string_err;

	s = bc_vec_init(&p->consts, sizeof(char*), bc_string_free);
	if (s) goto const_err;

	s = bc_vec_init(&p->results, sizeof(BcResult), bc_result_free);
	if (s) goto expr_err;

	if ((s = bc_vec_init(&p->stack, sizeof(BcInstPtr), NULL))) goto stack_err;
	memset(&ip, 0, sizeof(BcInstPtr));
	if ((s = bc_vec_push(&p->stack, &ip))) goto push_err;

	return s;

push_err:
	bc_vec_free(&p->stack);
stack_err:
	bc_vec_free(&p->results);
expr_err:
	bc_vec_free(&p->consts);
const_err:
	bc_vec_free(&p->strs);
string_err:
	bc_veco_free(&p->arr_map);
array_map_err:
	bc_vec_free(&p->arrs);
arr_err:
	bc_veco_free(&p->var_map);
var_map_err:
	bc_vec_free(&p->vars);
name_err:
	bc_veco_free(&p->fn_map);
func_map_err:
	bc_vec_free(&p->fns);
fn_err:
	bc_num_free(&p->one);
one_err:
	bc_num_free(&p->zero);
zero_err:
	bc_num_free(&p->last);
last_err:
	bc_num_free(&p->hexb);
hexb_err:
	bc_num_free(&p->ob);
obase_err:
	bc_num_free(&p->ib);
	return s;
}

BcStatus bc_program_addFunc(BcProgram *p, char *name, size_t *idx) {

	BcStatus s;
	BcEntry entry, *entry_ptr;
	BcFunc f;

	assert(p && name && idx);

	entry.name = name;
	entry.idx = p->fns.len;

	if ((s = bc_veco_insert(&p->fn_map, &entry, idx))) {
		free(name);
		if (s != BC_STATUS_VEC_ITEM_EXISTS) return s;
	}

	entry_ptr = bc_veco_item(&p->fn_map, *idx);
	*idx = entry_ptr->idx;

	if (s == BC_STATUS_VEC_ITEM_EXISTS) {

		BcFunc *func = bc_vec_item(&p->fns, entry_ptr->idx);
		s = BC_STATUS_SUCCESS;

		// We need to reset these, so the function can be repopulated.
		func->nparams = 0;
		bc_vec_npop(&func->autos, func->autos.len);
		bc_vec_npop(&func->code, func->code.len);
		bc_vec_npop(&func->labels, func->labels.len);
	}
	else {
		if ((s = bc_func_init(&f))) return s;
		if ((s = bc_vec_push(&p->fns, &f))) bc_func_free(&f);
	}

	return s;
}

BcStatus bc_program_reset(BcProgram *p, BcStatus s) {

	BcFunc *f;
	BcInstPtr *ip;

	bc_vec_npop(&p->stack, p->stack.len - 1);
	bc_vec_npop(&p->results, p->results.len);

	f = bc_vec_item(&p->fns, 0);
	ip = bc_vec_top(&p->stack);
	ip->idx = f->code.len;

	if (!s && bcg.signe && !bcg.tty) return BC_STATUS_QUIT;

	bcg.sigc += bcg.signe;
	bcg.signe = bcg.sig != bcg.sigc;

	if (!s || s == BC_STATUS_EXEC_SIGNAL) {
		if (bcg.ttyin) {
			if (fputs(bc_program_ready_msg, stderr) < 0 || fflush(stderr) < 0)
				s = BC_STATUS_IO_ERR;
			else s = BC_STATUS_SUCCESS;
		}
		else s = BC_STATUS_QUIT;
	}

	return s;
}

BcStatus bc_program_exec(BcProgram *p) {

	BcStatus s = BC_STATUS_SUCCESS;
	size_t idx;
	BcResult res;
	BcResult *ptr;
	BcNum *num;
	BcInstPtr *ip = bc_vec_top(&p->stack);
	BcFunc *func = bc_vec_item(&p->fns, ip->func);
	char *code = func->code.v;
	bool cond = false;

	while (!s && !bcg.sig_other && ip->idx < func->code.len) {

		uint8_t inst = code[(ip->idx)++];

		switch (inst) {

#ifdef BC_ENABLED
			case BC_INST_JUMP_ZERO:
			{
				if ((s = bc_program_prep(p, &ptr, &num, false))) return s;
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

			case BC_INST_IBASE:
			case BC_INST_LAST:
			case BC_INST_OBASE:
			{
				res.t = inst - BC_INST_IBASE + BC_RESULT_IBASE;
				s = bc_vec_push(&p->results, &res);
				break;
			}

			case BC_INST_SCALE:
			{
				s = bc_program_pushScale(p);
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
				res.t = BC_RESULT_CONSTANT;
				res.d.id.idx = bc_program_index(code, &ip->idx);
				s = bc_vec_push(&p->results, &res);
				break;
			}

			case BC_INST_POP:
			{
				bc_vec_pop(&p->results);
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
				res.t = BC_RESULT_STR;
				res.d.id.idx = bc_program_index(code, &ip->idx);
				s = bc_vec_push(&p->results, &res);
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
				if ((s = bc_program_prep(p, &ptr, &num, false))) return s;
				if ((s = bc_num_init(&res.d.n, BC_NUM_DEF_SIZE))) return s;

				if (!bc_num_cmp(num, &p->zero)) bc_num_one(&res.d.n);
				else bc_num_zero(&res.d.n);

				s = bc_program_retire(p, &res, BC_RESULT_TEMP);
				if (s) bc_num_free(&res.d.n);

				break;
			}

			case BC_INST_NEG:
			{
				s = bc_program_negate(p);
				break;
			}

#ifdef BC_ENABLED
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

#ifdef DC_ENABLED
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
				s = bc_program_executeStr(p, code, &ip->idx, cond);
				break;
			}

			case BC_INST_POP_EXEC:
			{
				assert(BC_PROG_CHECK_STACK(&p->stack, 2));
				bc_vec_pop(&p->stack);
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
				s = bc_program_stackLen(p);
				break;
			}

			case BC_INST_DUPLICATE:
			{
				if (!BC_PROG_CHECK_STACK(&p->results, 1))
					return BC_STATUS_EXEC_SMALL_STACK;
				ptr = bc_vec_top(&p->results);
				if ((s = bc_result_copy(&res, ptr))) return s;
				s = bc_vec_push(&p->results, &res);
				break;
			}

			case BC_INST_SWAP:
			{
				BcResult *ptr2;

				if (!BC_PROG_CHECK_STACK(&p->results, 2))
					return BC_STATUS_EXEC_SMALL_STACK;

				ptr = bc_vec_item_rev(&p->results, 0);
				ptr2 = bc_vec_item_rev(&p->results, 1);
				memcpy(&res, ptr, sizeof(BcResult));
				memcpy(ptr, ptr2, sizeof(BcResult));
				memcpy(ptr2, &res, sizeof(BcResult));

				break;
			}

			case BC_INST_PUSH_TO_VAR:
			{
				char *name;
				if (!(name = bc_program_name(code, &ip->idx))) return s;
				s = bc_program_copyToVar(p, name, true);
				free(name);
				break;
			}

			case BC_INST_LOAD:
			case BC_INST_PUSH_VAR:
			{
				bool copy = inst == BC_INST_LOAD;
				s = bc_program_pushVar(p, code, &ip->idx, true, copy);
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

		if ((s && s != BC_STATUS_QUIT) || bcg.signe) s = bc_program_reset(p, s);

		// If the stack has changed, pointers may be invalid.
		ip = bc_vec_top(&p->stack);
		func = bc_vec_item(&p->fns, ip->func);
		code = func->code.v;
	}

	return s;
}

void bc_program_free(BcProgram *p) {

	assert(p);

	bc_num_free(&p->ib);
	bc_num_free(&p->ob);
	bc_num_free(&p->hexb);

	bc_vec_free(&p->fns);
	bc_veco_free(&p->fn_map);

	bc_vec_free(&p->vars);
	bc_veco_free(&p->var_map);

	bc_vec_free(&p->arrs);
	bc_veco_free(&p->arr_map);

	bc_vec_free(&p->strs);
	bc_vec_free(&p->consts);

	bc_vec_free(&p->results);
	bc_vec_free(&p->stack);

	bc_num_free(&p->last);
	bc_num_free(&p->zero);
	bc_num_free(&p->one);
}

#ifndef NDEBUG
BcStatus bc_program_printIndex(char *code, size_t *bgn) {

	char byte, i, bytes = code[(*bgn)++];
	unsigned long val = 0;

	for (byte = 1, i = 0; byte && i < bytes; ++i) {
		byte = code[(*bgn)++];
		if (byte) val |= ((unsigned long) byte) << (CHAR_BIT * i);
	}

	return printf(" (%lu) ", val) < 0 ? BC_STATUS_IO_ERR : BC_STATUS_SUCCESS;
}

BcStatus bc_program_printName(char *code, size_t *bgn) {

	char byte = (char) code[(*bgn)++];

	if (printf(" (") < 0) return BC_STATUS_IO_ERR;

	for (; byte && byte != BC_PARSE_STREND; byte = (char) code[(*bgn)++]) {
		if (putchar(byte) == EOF) return BC_STATUS_IO_ERR;
	}

	assert(byte);

	if (printf(") ") < 0) return BC_STATUS_IO_ERR;

	return BC_STATUS_SUCCESS;
}

BcStatus bc_program_printStr(BcProgram *p, char *code, size_t *bgn) {

	size_t idx = bc_program_index(code, bgn);
	char *s;

	assert(idx < p->strs.len);

	s = *((char**) bc_vec_item(&p->strs, idx));

	if (printf(" (\"%s\") ", s) < 0) return BC_STATUS_IO_ERR;

	return BC_STATUS_SUCCESS;
}

BcStatus bc_program_printInst(BcProgram *p, char *code, size_t *bgn) {

	BcStatus s = BC_STATUS_SUCCESS;
	uint8_t inst = code[(*bgn)++];

	if (putchar(bc_inst_chars[inst]) == EOF) return BC_STATUS_IO_ERR;

	if (inst == BC_INST_VAR || inst == BC_INST_ARRAY_ELEM ||
	    inst == BC_INST_ARRAY)
	{
		s = bc_program_printName(code, bgn);
	}
	else if (inst == BC_INST_STR) {
		s = bc_program_printStr(p, code, bgn);
	}
	else if (inst == BC_INST_NUM) {
		size_t idx = bc_program_index(code, bgn);
		char **str = bc_vec_item(&p->consts, idx);
		if (printf("(%s)", *str) < 0) s = BC_STATUS_IO_ERR;
	}
	else if (inst == BC_INST_CALL ||
	         (inst > BC_INST_STR && inst <= BC_INST_JUMP_ZERO))
	{
		if ((s = bc_program_printIndex(code, bgn))) return s;
		if (inst == BC_INST_CALL) s = bc_program_printIndex(code, bgn);
	}

	if (!s && fflush(stdout) < 0) s = BC_STATUS_IO_ERR;

	return s;
}

BcStatus bc_program_code(BcProgram *p) {

	BcStatus s = BC_STATUS_SUCCESS;
	BcFunc *f;
	char *code;
	BcInstPtr ip;
	size_t i;

	for (i = 0; !s && !bcg.sig_other && i < p->fns.len; ++i) {

		bool sig;

		ip.idx = ip.len = 0;
		ip.func = i;

		f = bc_vec_item(&p->fns, ip.func);
		code = f->code.v;

		if (printf("func[%zu]:\n", ip.func) < 0) return BC_STATUS_IO_ERR;

		while (ip.idx < f->code.len) s = bc_program_printInst(p, code, &ip.idx);

		if (printf("\n\n") < 0) s = BC_STATUS_IO_ERR;

		sig = bcg.sig != bcg.sigc;
		if (s || sig) s = bc_program_reset(p, s);
	}

	return s;
}
#endif // NDEBUG
