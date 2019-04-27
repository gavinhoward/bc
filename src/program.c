/*
 * *****************************************************************************
 *
 * Copyright (c) 2018-2019 Gavin D. Howard and contributors.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * *****************************************************************************
 *
 * Code to execute bc programs.
 *
 */

#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include <signal.h>

#include <read.h>
#include <parse.h>
#include <program.h>
#include <vm.h>

#ifndef BC_PROG_NO_STACK_CHECK
static BcStatus bc_program_checkStack(BcVec *v, size_t n) {
#if DC_ENABLED
	if (!BC_IS_BC && BC_ERR(!BC_PROG_STACK(v, n)))
		return bc_vm_err(BC_ERROR_EXEC_STACK);
#endif // DC_ENABLED
	assert(BC_PROG_STACK(v, n));
	return BC_STATUS_SUCCESS;
}
#endif // BC_PROG_NO_STACK_CHECK

static BcStatus bc_program_type_num(BcResult *r, BcNum *n) {
#if BC_ENABLED
	assert(r->t != BC_RESULT_VOID);
#endif // BC_ENABLED
	if (BC_ERR(!BC_PROG_NUM(r, n))) return bc_vm_err(BC_ERROR_EXEC_TYPE);
	return BC_STATUS_SUCCESS;
}

static BcStatus bc_program_type_match(BcResult *r, BcType t) {
#if DC_ENABLED
	assert(!BC_IS_BC || BC_NO_ERR(r->t != BC_RESULT_STR));
#endif // DC_ENABLED
	if (BC_ERR((r->t != BC_RESULT_ARRAY) != (!t)))
		return bc_vm_err(BC_ERROR_EXEC_TYPE);
	return BC_STATUS_SUCCESS;
}

static char* bc_program_str(BcProgram *p, size_t idx, bool str) {

	BcFunc *f;
	BcVec *v;
	size_t i;

	if (BC_IS_BC) {
		BcInstPtr *ip = bc_vec_item_rev(&p->stack, 0);
		i = ip->func;
	}
	else i = BC_PROG_MAIN;

	f = bc_vec_item(&p->fns, i);
	v = str ? &f->strs : &f->consts;

	return *((char**) bc_vec_item(v, idx));
}

static size_t bc_program_index(const char *restrict code, size_t *restrict bgn)
{
	uchar amt = (uchar) code[(*bgn)++], i = 0;
	size_t res = 0;

	for (; i < amt; ++i, ++(*bgn)) {
		size_t temp = ((size_t) ((int) (uchar) code[*bgn]) & UCHAR_MAX);
		res |= (temp << (i * CHAR_BIT));
	}

	return res;
}

static char* bc_program_name(const char *restrict code, size_t *restrict bgn) {

	size_t i;
	uchar c;
	char *s;
	const char *str = code + *bgn, *ptr = strchr(str, BC_PARSE_STREND);

	assert(ptr);

	s = bc_vm_malloc(((size_t) (ptr - str)) + 1);

	for (i = 0; (c = (uchar) code[(*bgn)++]) && c != BC_PARSE_STREND; ++i)
		s[i] = (char) c;

	s[i] = '\0';

	return s;
}

static void bc_program_prepGlobals(BcProgram *p) {
	bc_vec_push(&p->scale_v, &p->scale);
	bc_vec_push(&p->ib_v, &p->ib_t);
	bc_vec_push(&p->ob_v, &p->ob_t);
}

#if BC_ENABLED
static BcVec* bc_program_dereference(BcProgram *p, BcVec *vec) {

	BcVec *v;
	size_t vidx, nidx, i = 0;

	assert(vec->size == sizeof(uchar));

	vidx = bc_program_index(vec->v, &i);
	nidx = bc_program_index(vec->v, &i);

	v = bc_vec_item(&p->arrs, vidx);
	v = bc_vec_item(v, nidx);

	assert(v->size != sizeof(uchar));

	return v;
}
#endif // BC_ENABLED

static BcVec* bc_program_search(BcProgram *p, char *id, BcType type) {

	BcId e, *ptr;
	BcVec *v, *map;
	size_t i;
	BcResultData data;
	bool new, var = (type == BC_TYPE_VAR);

	v = var ? &p->vars : &p->arrs;
	map = var ? &p->var_map : &p->arr_map;

	e.name = id;
	e.idx = v->len;
	new = bc_map_insert(map, &e, &i);

	if (new) {
		bc_array_init(&data.v, var);
		bc_vec_push(v, &data.v);
	}

	ptr = bc_vec_item(map, i);
	if (new) ptr->name = bc_vm_strdup(e.name);

	return bc_vec_item(v, ptr->idx);
}

static BcStatus bc_program_num(BcProgram *p, BcResult *r, BcNum **num) {

	BcStatus s = BC_STATUS_SUCCESS;
	BcNum *n = &r->d.n;

	switch (r->t) {

		case BC_RESULT_CONSTANT:
		{
			char *str = bc_program_str(p, r->d.id.idx, false);
			size_t len = strlen(str);

			bc_num_init(n, len);

			s = bc_num_parse(n, str, &p->ib, p->ib_t, len == 1);
			assert(!s || s == BC_STATUS_SIGNAL);

#if BC_ENABLE_SIGNALS
			// bc_num_parse() should only do operations that can
			// only fail when signals happen. Thus, if signals
			// are not enabled, we don't need this check.
			if (BC_ERROR_SIGNAL_ONLY(s)) {
				bc_num_free(n);
				return s;
			}
#endif // BC_ENABLE_SIGNALS

			r->t = BC_RESULT_TEMP;
			break;
		}

		case BC_RESULT_STR:
		case BC_RESULT_TEMP:
		case BC_RESULT_IBASE:
		case BC_RESULT_SCALE:
		case BC_RESULT_OBASE:
		{
			break;
		}

		case BC_RESULT_VAR:
		case BC_RESULT_ARRAY:
		case BC_RESULT_ARRAY_ELEM:
		{
			BcVec *v;
			BcType type = (r->t == BC_RESULT_VAR) ? BC_TYPE_VAR : BC_TYPE_ARRAY;

			v = bc_program_search(p, r->d.id.name, type);

			if (r->t == BC_RESULT_ARRAY_ELEM) {

				size_t idx = r->d.id.idx;

				v = bc_vec_top(v);

#if BC_ENABLED
				if (v->size == sizeof(uchar)) v = bc_program_dereference(p, v);
#endif // BC_ENABLED

				assert(v->size == sizeof(BcNum));

				if (v->len <= idx) bc_array_expand(v, idx + 1);
				n = bc_vec_item(v, idx);
			}
			else n = bc_vec_top(v);

			break;
		}

#if BC_ENABLED
		case BC_RESULT_LAST:
		{
			n = &p->last;
			break;
		}

		case BC_RESULT_ONE:
		{
			n = &p->one;
			break;
		}

		case BC_RESULT_VOID:
		{
#ifndef NDEBUG
			assert(false);
			break;
#endif // NDEBUG
		}
#endif // BC_ENABLED
	}

	*num = n;

	return s;
}

static BcStatus bc_program_operand(BcProgram *p, BcResult **r,
                                   BcNum **n, size_t idx)
{
#ifndef BC_PROG_NO_STACK_CHECK
	BcStatus s = bc_program_checkStack(&p->results, idx + 1);

	if (BC_ERR(s)) return s;
#endif // BC_PROG_NO_STACK_CHECK

	*r = bc_vec_item_rev(&p->results, idx);
#if BC_ENABLED
	if (BC_ERR((*r)->t == BC_RESULT_VOID))
		return bc_vm_err(BC_ERROR_EXEC_VOID_VAL);
#endif // BC_ENABLED

	return bc_program_num(p, *r, n);
}

static BcStatus bc_program_binPrep(BcProgram *p, BcResult **l, BcNum **ln,
                                   BcResult **r, BcNum **rn)
{
	BcStatus s;
	BcResultType lt;

	assert(p && l && ln && r && rn);

	s = bc_program_operand(p, l, ln, 1);
	if (BC_ERR(s)) return s;
	s = bc_program_operand(p, r, rn, 0);
	if (BC_ERR(s)) return s;

	lt = (*l)->t;

#if BC_ENABLED
	assert(lt != BC_RESULT_VOID && (*r)->t != BC_RESULT_VOID);
#endif // BC_ENABLED

	// We run this again under these conditions in case any vector has been
	// reallocated out from under the BcNums or arrays we had.
	if (lt == (*r)->t && (lt == BC_RESULT_VAR || lt == BC_RESULT_ARRAY_ELEM))
		s = bc_program_num(p, *l, ln);

	if (BC_NO_ERR(!s) && BC_ERR(lt == BC_RESULT_STR))
		return bc_vm_err(BC_ERROR_EXEC_TYPE);

	return s;
}

static BcStatus bc_program_binOpPrep(BcProgram *p, BcResult **l, BcNum **ln,
                                     BcResult **r, BcNum **rn)
{
	BcStatus s;

	s = bc_program_binPrep(p, l, ln, r, rn);
	if (BC_ERR(s)) return s;

	s = bc_program_type_num(*l, *ln);
	if (BC_ERR(s)) return s;

	return bc_program_type_num(*r, *rn);
}

static BcStatus bc_program_assignPrep(BcProgram *p, BcResult **l, BcNum **ln,
                                      BcResult **r, BcNum **rn)
{
	BcStatus s;
	bool good = false;
	BcResultType lt;

	s = bc_program_binPrep(p, l, ln, r, rn);
	if (BC_ERR(s)) return s;

	lt = (*l)->t;

	if (BC_ERR(lt == BC_RESULT_CONSTANT || lt == BC_RESULT_TEMP ||
	           lt == BC_RESULT_ARRAY))
	{
		return bc_vm_err(BC_ERROR_EXEC_TYPE);
	}

#if BC_ENABLED
	assert(!BC_IS_BC || BC_NO_ERR(lt != BC_RESULT_ONE));
#endif // BC_ENABLED

#if DC_ENABLED
	good = (((*r)->t == BC_RESULT_STR || BC_PROG_STR(*rn)) &&
	        (lt == BC_RESULT_VAR || lt == BC_RESULT_ARRAY_ELEM));
#else
	assert((*r)->t != BC_RESULT_STR);
#endif // DC_ENABLED

	if (!good) s = bc_program_type_num(*r, *rn);

	return s;
}

static void bc_program_binOpRetire(BcProgram *p, BcResult *r) {
	r->t = BC_RESULT_TEMP;
	bc_vec_pop(&p->results);
	bc_vec_pop(&p->results);
	bc_vec_push(&p->results, r);
}

static BcStatus bc_program_prep(BcProgram *p, BcResult **r, BcNum **n) {

	BcStatus s;

	assert(p && r && n);

	s = bc_program_operand(p, r, n, 0);
	if (BC_ERR(s)) return s;

#if DC_ENABLED
	assert((*r)->t != BC_RESULT_VAR || !BC_PROG_STR(*n));
#endif // DC_ENABLED

	return bc_program_type_num(*r, *n);
}

static void bc_program_retire(BcProgram *p, BcResult *r, BcResultType t) {
	r->t = t;
	bc_vec_pop(&p->results);
	bc_vec_push(&p->results, r);
}

static BcStatus bc_program_op(BcProgram *p, uchar inst) {

	BcStatus s;
	BcResult *opd1, *opd2, res;
	BcNum *n1 = NULL, *n2 = NULL;
	size_t idx = inst - BC_INST_POWER;

	s = bc_program_binOpPrep(p, &opd1, &n1, &opd2, &n2);
	if (BC_ERR(s)) return s;
	bc_num_init(&res.d.n, bc_program_opReqs[idx](n1, n2, p->scale));

	s = bc_program_ops[idx](n1, n2, &res.d.n, p->scale);
	if (BC_ERR(s)) goto err;
	bc_program_binOpRetire(p, &res);

	return s;

err:
	bc_num_free(&res.d.n);
	return s;
}

static BcStatus bc_program_read(BcProgram *p) {

	BcStatus s;
	BcParse parse;
	BcVec buf;
	BcInstPtr ip;
	size_t i;
	const char* file;
	BcFunc *f = bc_vec_item(&p->fns, BC_PROG_READ);

	for (i = 0; i < p->stack.len; ++i) {
		BcInstPtr *ip_ptr = bc_vec_item(&p->stack, i);
		if (ip_ptr->func == BC_PROG_READ)
			return bc_vm_err(BC_ERROR_EXEC_REC_READ);
	}

	file = vm->file;
	bc_lex_file(&parse.l, bc_program_stdin_name);
	bc_vec_npop(&f->code, f->code.len);
	bc_vec_init(&buf, sizeof(char), NULL);

	s = bc_read_line(&buf, "read> ");
	if (BC_ERR(s)) {
		if (s == BC_STATUS_EOF) s = bc_vm_err(BC_ERROR_EXEC_READ_EXPR);
		goto io_err;
	}

	bc_parse_init(&parse, p, BC_PROG_READ);

	s = bc_parse_text(&parse, buf.v);
	if (BC_ERR(s)) goto exec_err;
	s = vm->expr(&parse, BC_PARSE_NOREAD);
	if (BC_ERR(s)) goto exec_err;

	if (BC_ERR(parse.l.t != BC_LEX_NLINE && parse.l.t != BC_LEX_EOF)) {
		s = bc_vm_err(BC_ERROR_EXEC_READ_EXPR);
		goto exec_err;
	}

	if (BC_G) bc_program_prepGlobals(p);

	ip.func = BC_PROG_READ;
	ip.idx = 0;
	ip.len = p->results.len;

	// Update this pointer, just in case.
	f = bc_vec_item(&p->fns, BC_PROG_READ);

	bc_vec_pushByte(&f->code, vm->read_ret);
	bc_vec_push(&p->stack, &ip);

exec_err:
	bc_parse_free(&parse);
io_err:
	bc_vec_free(&buf);
	vm->file = file;
	return s;
}

static void bc_program_printChars(const char *str) {
	const char *nl;
	vm->nchars += bc_vm_printf("%s", str);
	nl = strrchr(str, '\n');
	if (nl) vm->nchars = strlen(nl + 1);
}

static void bc_program_printString(const char *restrict str) {

	size_t i, len = strlen(str);

#if DC_ENABLED
	if (!len && !BC_IS_BC) {
		bc_vm_putchar('\0');
		return;
	}
#endif // DC_ENABLED

	for (i = 0; i < len; ++i, ++vm->nchars) {

		int c = str[i];

		if (c == '\\' && i != len - 1) {

			const char *ptr;

			c = str[++i];
			ptr = strchr(bc_program_esc_chars, c);

			if (ptr) {
				if (c == 'n') vm->nchars = SIZE_MAX;
				c = bc_program_esc_seqs[(size_t) (ptr - bc_program_esc_chars)];
			}
			else {
				// Just print the backslash. The following
				// character will be printed later.
				bc_vm_putchar('\\');
				vm->nchars += 1;
			}
		}

		bc_vm_putchar(c);
	}
}

static BcStatus bc_program_print(BcProgram *p, uchar inst, size_t idx) {

	BcStatus s = BC_STATUS_SUCCESS;
	BcResult *r;
	char *str;
	BcNum *n = NULL;
	bool pop = (inst != BC_INST_PRINT);

	assert(p);

#ifndef BC_PROG_NO_STACK_CHECK
	s = bc_program_checkStack(&p->results, idx + 1);
	if (BC_ERR(s)) return s;
#endif // BC_PROG_NO_STACK_CHECK

	r = bc_vec_item_rev(&p->results, idx);

#if BC_ENABLED
	if (r->t == BC_RESULT_VOID) {
		if (BC_ERR(pop)) return bc_vm_err(BC_ERROR_EXEC_VOID_VAL);
		return s;
	}
#endif // BC_ENABLED

	s = bc_program_num(p, r, &n);
	if (BC_ERR(s)) return s;

	if (BC_PROG_NUM(r, n)) {
		assert(inst != BC_INST_PRINT_STR);
		s = bc_num_print(n, &p->ob, BC_PROG_GLOBAL(&p->ob_v), !pop);
#if BC_ENABLED
		if (BC_NO_ERR(!s)) bc_num_copy(&p->last, n);
#endif // BC_ENABLED
	}
	else {

		size_t i = (r->t == BC_RESULT_STR) ? r->d.id.idx : n->rdx;

		str = bc_program_str(p, i, true);

		if (inst == BC_INST_PRINT_STR) bc_program_printChars(str);
		else {
			bc_program_printString(str);
			if (inst == BC_INST_PRINT) {
				bc_vm_putchar('\n');
				vm->nchars = 0;
			}
		}
	}

	if (BC_NO_ERR(!s) && pop) bc_vec_pop(&p->results);

	return s;
}

void bc_program_negate(BcResult *r, BcNum *n) {
	BcNum *rn = &r->d.n;
	bc_num_copy(rn, n);
	if (BC_NUM_NONZERO(rn)) rn->neg = !rn->neg;
}

void bc_program_not(BcResult *r, BcNum *n) {
	if (!bc_num_cmpZero(n)) bc_num_one(&r->d.n);
}

#if BC_ENABLE_EXTRA_MATH
void bc_program_trunc(BcResult *r, BcNum *n) {
	BcNum *rn = &r->d.n;
	bc_num_copy(rn, n);
	bc_num_truncate(rn, n->scale);
}
#endif // BC_ENABLE_EXTRA_MATH

static BcStatus bc_program_unary(BcProgram *p, uchar inst) {

	BcStatus s;
	BcResult res, *ptr;
	BcNum *num = NULL;

	s = bc_program_prep(p, &ptr, &num);
	if (BC_ERR(s)) return s;

	bc_num_init(&res.d.n, num->len);
	bc_program_unarys[inst - BC_INST_NEG](&res, num);
	bc_program_retire(p, &res, BC_RESULT_TEMP);

	return s;
}

static BcStatus bc_program_logical(BcProgram *p, uchar inst) {

	BcStatus s;
	BcResult *opd1, *opd2, res;
	BcNum *n1 = NULL, *n2 = NULL;
	bool cond = 0;
	ssize_t cmp;

	s = bc_program_binOpPrep(p, &opd1, &n1, &opd2, &n2);
	if (BC_ERR(s)) return s;
	bc_num_init(&res.d.n, BC_NUM_DEF_SIZE);

	if (inst == BC_INST_BOOL_AND)
		cond = bc_num_cmpZero(n1) && bc_num_cmpZero(n2);
	else if (inst == BC_INST_BOOL_OR)
		cond = bc_num_cmpZero(n1) || bc_num_cmpZero(n2);
	else {

		cmp = bc_num_cmp(n1, n2);
		if (cmp == BC_NUM_SSIZE_MIN) {
			bc_num_free(&res.d.n);
			return BC_STATUS_SIGNAL;
		}

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

	if (cond) bc_num_one(&res.d.n);

	bc_program_binOpRetire(p, &res);

	return s;
}

#if DC_ENABLED
static BcStatus bc_program_assignStr(BcProgram *p, BcResult *r,
                                     BcVec *v, bool push)
{
	BcNum n2;
	BcResult res;

	memset(&n2, 0, sizeof(BcNum));
	n2.rdx = res.d.id.idx = r->d.id.idx;
	res.t = BC_RESULT_STR;

	if (!push) {
#ifndef BC_PROG_NO_STACK_CHECK
		BcStatus s = bc_program_checkStack(&p->results, 2);
		if (BC_ERR(s)) return s;
#endif // BC_PROG_NO_STACK_CHECK
		bc_vec_pop(v);
		bc_vec_pop(&p->results);
	}

	bc_vec_pop(&p->results);

	bc_vec_push(&p->results, &res);
	bc_vec_push(v, &n2);

	return BC_STATUS_SUCCESS;
}
#endif // DC_ENABLED

static BcStatus bc_program_copyToVar(BcProgram *p, char *name,
                                     BcType t, bool last)
{
	BcStatus s = BC_STATUS_SUCCESS;
	BcResult *ptr, r;
	BcVec *vec;
	BcNum *n = NULL;
	bool var = (t == BC_TYPE_VAR);

#if BC_ENABLED
	if (last) s = bc_program_operand(p, &ptr, &n, 0);
	else {

		ptr = bc_vec_top(&p->results);

		// The only place that last could not be true is when doing parameters
		// for a function that are either a variable or an array. Thus, we can
		// assert that here instead of check.
		assert(ptr->t == BC_RESULT_VAR || ptr->t == BC_RESULT_ARRAY);

		n = bc_vec_item_rev(bc_program_search(p, ptr->d.id.name, t), 1);
	}
#else // BC_ENABLED
	s = bc_program_operand(p, &ptr, &n, 0);
#endif // BC_ENABLED

	if (BC_ERR(s)) return s;

	s = bc_program_type_match(ptr, t);
	if (BC_ERR(s)) return s;

	vec = bc_program_search(p, name, t);

#if DC_ENABLED
	if (ptr->t == BC_RESULT_STR) {
		if (BC_ERR(!var)) return bc_vm_err(BC_ERROR_EXEC_TYPE);
		return bc_program_assignStr(p, ptr, vec, true);
	}
#endif // DC_ENABLED

	// Do this once more to make sure that pointers were not invalidated.
	vec = bc_program_search(p, name, t);

	if (var) bc_num_createCopy(&r.d.n, n);
	else {

		BcVec *v = (BcVec*) n, *rv = &r.d.v;
#if BC_ENABLED
		bool ref, ref_size;

		ref = (v->size == sizeof(BcVec) && t != BC_TYPE_ARRAY);
		ref_size = (v->size == sizeof(uchar));

		if (ref || (ref_size && t == BC_TYPE_REF))
		{
			bc_vec_init(rv, sizeof(uchar), NULL);

			if (ref) {

				size_t vidx, idx;
				BcId id;

				id.name = ptr->d.id.name;
				v = bc_program_search(p, ptr->d.id.name, BC_TYPE_REF);

				// Make sure the pointer was not invalidated.
				vec = bc_program_search(p, name, t);

				vidx = bc_map_index(&p->arr_map, &id);
				assert(vidx != BC_VEC_INVALID_IDX);
				vidx = ((BcId*) bc_vec_item(&p->arr_map, vidx))->idx;
				idx = v->len - 1;

				bc_vec_pushIndex(rv, vidx);
				bc_vec_pushIndex(rv, idx);
			}
			// If we get here, we are copying a ref to a ref.
			else bc_vec_npush(rv, v->len * sizeof(uchar), v->v);

			// We need to return early.
			bc_vec_push(vec, &r.d);
			bc_vec_pop(&p->results);

			return s;
		}
		else if (ref_size && t != BC_TYPE_REF) v = bc_program_dereference(p, v);
#endif // BC_ENABLED

		bc_array_init(rv, true);
		bc_array_copy(rv, v);
	}

	bc_vec_push(vec, &r.d);
	bc_vec_pop(&p->results);

	return s;
}

static BcStatus bc_program_assign(BcProgram *p, uchar inst) {

	BcStatus s;
	BcResult *left, *right, res;
	BcNum *l = NULL, *r = NULL;
	bool ib, ob, sc;

	s = bc_program_assignPrep(p, &left, &l, &right, &r);
	if (BC_ERR(s)) return s;

	ib = (left->t == BC_RESULT_IBASE);
	ob = (left->t == BC_RESULT_OBASE);
	sc = (left->t == BC_RESULT_SCALE);

#if DC_ENABLED
	assert(left->t != BC_RESULT_STR);

	if (right->t == BC_RESULT_STR) {

		size_t idx =  right->d.id.idx;

		if (left->t == BC_RESULT_ARRAY_ELEM) {
			bc_num_free(l);
			memset(l, 0, sizeof(BcNum));
			l->rdx = idx;
		}
		else {
			BcVec *v = bc_program_search(p, left->d.id.name, BC_TYPE_VAR);
			s = bc_program_assignStr(p, right, v, false);
		}

		return s;
	}
#endif // DC_ENABLED

	if (!BC_IS_BC || inst == BC_INST_ASSIGN) bc_num_copy(l, r);
#if BC_ENABLED
	else {
		s = bc_program_ops[inst - BC_INST_ASSIGN_POWER](l, r, l, p->scale);
		if (BC_ERR(s)) return s;
	}
#endif // BC_ENABLED

	if (ib || ob || sc) {

		BcVec *v;
		size_t *ptr, *ptr_t;
		unsigned long val, max, min;
		BcError e;

		s = bc_num_ulong(l, &val);
		if (BC_ERR(s)) return s;
		e = left->t - BC_RESULT_IBASE + BC_ERROR_EXEC_IBASE;

		if (sc) {
			min = 0;
			max = BC_MAX_SCALE;
			v = &p->scale_v;
			ptr_t = &p->scale;
		}
		else {
			min = BC_NUM_MIN_BASE;
			if (BC_ENABLE_EXTRA_MATH && ob && (!BC_IS_BC || !BC_IS_POSIX))
				min = 0;
			max = ib ? vm->max_ibase : BC_MAX_OBASE;
			v = ib ? &p->ib_v : &p->ob_v;
			ptr_t = ib ? &p->ib_t : &p->ob_t;
		}

		ptr = bc_vec_top(v);

		if (BC_ERR(val > max || val < min)) return bc_vm_verr(e, min, max);
		if (!sc) bc_num_ulong2num(ib ? &p->ib : &p->ob, val);

		*ptr = (size_t) val;
		*ptr_t = (size_t) val;
	}

	bc_num_createCopy(&res.d.n, l);
	bc_program_binOpRetire(p, &res);

	return s;
}

static BcStatus bc_program_pushVar(BcProgram *p, const char *restrict code,
                                   size_t *restrict bgn, bool pop, bool copy)
{
	BcStatus s = BC_STATUS_SUCCESS;
	BcResult r;
	char *name = bc_program_name(code, bgn);

	r.t = BC_RESULT_VAR;
	r.d.id.name = name;

#if DC_ENABLED
	{
		BcVec *v = bc_program_search(p, name, BC_TYPE_VAR);
		BcNum *num = bc_vec_top(v);

		if (pop || copy) {

			free(name);
			name = NULL;

			s = bc_program_checkStack(v, 2 - copy);
			if (BC_ERR(s)) return s;

			if (!BC_PROG_STR(num)) {
				r.t = BC_RESULT_TEMP;
				bc_num_createCopy(&r.d.n, num);
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

static BcStatus bc_program_pushArray(BcProgram *p, const char *restrict code,
                                     size_t *restrict bgn, uchar inst)
{
	BcStatus s = BC_STATUS_SUCCESS;
	BcResult r;
	BcNum *num = NULL;

	r.d.id.name = bc_program_name(code, bgn);

	if (inst == BC_INST_ARRAY) {
		r.t = BC_RESULT_ARRAY;
		bc_vec_push(&p->results, &r);
	}
	else {

		BcResult *operand;
		unsigned long temp;

		s = bc_program_prep(p, &operand, &num);
		if (BC_ERR(s)) goto err;
		s = bc_num_ulong(num, &temp);
		if (BC_ERR(s)) goto err;

		r.d.id.idx = (size_t) temp;
		bc_program_retire(p, &r, BC_RESULT_ARRAY_ELEM);
	}

err:
	if (BC_ERR(s)) free(r.d.id.name);
	return s;
}

#if BC_ENABLED
static BcStatus bc_program_incdec(BcProgram *p, uchar inst) {

	BcStatus s;
	BcResult *ptr, res, copy;
	BcNum *num = NULL;
	uchar inst2;

	s = bc_program_prep(p, &ptr, &num);
	if (BC_ERR(s)) return s;

	if (inst == BC_INST_INC_POST || inst == BC_INST_DEC_POST) {
		copy.t = BC_RESULT_TEMP;
		bc_num_createCopy(&copy.d.n, num);
	}

	res.t = BC_RESULT_ONE;
	inst2 = BC_INST_ASSIGN_PLUS + (inst & 0x01);

	bc_vec_push(&p->results, &res);
	bc_program_assign(p, inst2);

	if (inst == BC_INST_INC_POST || inst == BC_INST_DEC_POST) {
		bc_vec_pop(&p->results);
		bc_vec_push(&p->results, &copy);
	}

	return s;
}

static BcStatus bc_program_call(BcProgram *p, const char *restrict code,
                                size_t *restrict idx)
{
	BcStatus s = BC_STATUS_SUCCESS;
	BcInstPtr ip;
	size_t i, nparams = bc_program_index(code, idx);
	BcFunc *f;
	BcVec *v;
	BcId *a;
	BcResultData param;
	BcResult *arg;

	ip.idx = 0;
	ip.func = bc_program_index(code, idx);
	f = bc_vec_item(&p->fns, ip.func);

	if (BC_ERR(!f->code.len))
		return bc_vm_verr(BC_ERROR_EXEC_UNDEF_FUNC, f->name);
	if (BC_ERR(nparams != f->nparams))
		return bc_vm_verr(BC_ERROR_EXEC_PARAMS, f->nparams, nparams);
	ip.len = p->results.len - nparams;

	assert(BC_PROG_STACK(&p->results, nparams));

	if (BC_G) bc_program_prepGlobals(p);

	for (i = 0; i < nparams; ++i) {

		size_t j;
		bool last = true;

		a = bc_vec_item(&f->autos, nparams - 1 - i);
		arg = bc_vec_top(&p->results);

		// If I have already pushed to a var, I need to make sure I
		// get the previous version, not the already pushed one.
		if (arg->t == BC_RESULT_VAR || arg->t == BC_RESULT_ARRAY) {
			for (j = 0; j < i && last; ++j) {
				BcId *id = bc_vec_item(&f->autos, nparams - 1 - j);
				last = (strcmp(arg->d.id.name, id->name) != 0 ||
				       (!id->idx) != (arg->t == BC_RESULT_VAR));
			}
		}

		s = bc_program_copyToVar(p, a->name, (BcType) a->idx, last);
		if (BC_ERR(s)) return s;
	}

	for (; i < f->autos.len; ++i) {

		a = bc_vec_item(&f->autos, i);
		v = bc_program_search(p, a->name, (BcType) a->idx);

		if (a->idx == BC_TYPE_VAR) {
			bc_num_init(&param.n, BC_NUM_DEF_SIZE);
			bc_vec_push(v, &param.n);
		}
		else {
#if BC_ENABLED
			assert(a->idx == BC_TYPE_ARRAY);
#endif // BC_ENABLED
			bc_array_init(&param.v, true);
			bc_vec_push(v, &param.v);
		}
	}

	bc_vec_push(&p->stack, &ip);

	return BC_STATUS_SUCCESS;
}

static BcStatus bc_program_return(BcProgram *p, uchar inst) {

	BcStatus s;
	BcResult res;
	BcFunc *f;
	size_t i;
	BcInstPtr *ip = bc_vec_top(&p->stack);

	assert(BC_PROG_STACK(&p->stack, 2));

#ifndef BC_PROG_NO_STACK_CHECK
	s = bc_program_checkStack(&p->results, ip->len + (inst == BC_INST_RET));
	if (BC_ERR(s)) return s;
#endif // BC_PROG_NO_STACK_CHECK

	f = bc_vec_item(&p->fns, ip->func);
	res.t = BC_RESULT_TEMP;

	if (inst == BC_INST_RET) {

		BcNum *num = NULL;
		BcResult *operand;

		s = bc_program_operand(p, &operand, &num, 0);
		if (BC_ERR(s)) return s;

		bc_num_createCopy(&res.d.n, num);
	}
	else if (inst == BC_INST_RET_VOID) res.t = BC_RESULT_VOID;
	else bc_num_init(&res.d.n, BC_NUM_DEF_SIZE);

	// We need to pop arguments as well, so this takes that into account.
	for (i = 0; i < f->autos.len; ++i) {

		BcVec *v;
		BcId *a = bc_vec_item(&f->autos, i);

		v = bc_program_search(p, a->name, (BcType) a->idx);
		bc_vec_pop(v);
	}

	bc_vec_npop(&p->results, p->results.len - ip->len);

	if (BC_G) {

		bc_vec_pop(&p->scale_v);
		p->scale = BC_PROG_GLOBAL(&p->scale_v);

		bc_vec_pop(&p->ib_v);
		p->ib_t = BC_PROG_GLOBAL(&p->ib_v);
		bc_num_ulong2num(&p->ib, (unsigned long) p->ib_t);

		bc_vec_pop(&p->ob_v);
		p->ob_t = BC_PROG_GLOBAL(&p->ob_v);
		bc_num_ulong2num(&p->ob, (unsigned long) p->ob_t);
	}

	bc_vec_push(&p->results, &res);
	bc_vec_pop(&p->stack);

	return BC_STATUS_SUCCESS;
}
#endif // BC_ENABLED

static BcStatus bc_program_builtin(BcProgram *p, uchar inst) {

	BcStatus s;
	BcResult *opd;
	BcResult res;
	BcNum *num = NULL, *resn = &res.d.n;
	bool len = (inst == BC_INST_LENGTH);

	assert(inst >= BC_INST_LENGTH && inst <= BC_INST_ABS);

	s = bc_program_operand(p, &opd, &num, 0);
	if (BC_ERR(s)) return s;

#if DC_ENABLED
	if (!len && inst != BC_INST_SCALE_FUNC) {
		s = bc_program_type_num(opd, num);
		if (BC_ERR(s)) return s;
	}
#endif // DC_ENABLED

	if (inst == BC_INST_SQRT) {
		s = bc_num_sqrt(num, resn, p->scale);
		if (BC_ERR(s)) return s;
	}
	else if (inst == BC_INST_ABS) {
		bc_num_createCopy(resn, num);
		resn->neg = false;
	}
	else {

		unsigned long val = 0;

		if (len) {
			if (BC_IS_BC && opd->t == BC_RESULT_ARRAY)
				val = (unsigned long) ((BcVec*) num)->len;
#if DC_ENABLED
			else if (!BC_PROG_NUM(opd, num)) {
				size_t idx = opd->t == BC_RESULT_STR ? opd->d.id.idx : num->rdx;
				val = strlen(bc_program_str(p, idx, true));
			}
#endif // DC_ENABLED
			else val = (unsigned long) bc_num_len(num);
		}
		else if (BC_IS_BC || BC_PROG_NUM(opd, num))
			val = (unsigned long) bc_num_scale(num);

		bc_num_createFromUlong(resn, val);
	}

	bc_program_retire(p, &res, BC_RESULT_TEMP);

	return s;
}

#if DC_ENABLED
static BcStatus bc_program_divmod(BcProgram *p) {

	BcStatus s;
	BcResult *opd1, *opd2, res, res2;
	BcNum *n1, *n2 = NULL, *resn = &res.d.n, *resn2 = &res2.d.n;
	size_t req;

	s = bc_program_binOpPrep(p, &opd1, &n1, &opd2, &n2);
	if (BC_ERR(s)) return s;

	req = bc_num_mulReq(n1, n2, p->scale);
	bc_num_init(resn, req);
	bc_num_init(resn2, req);

	s = bc_num_divmod(n1, n2, resn2, resn, p->scale);
	if (BC_ERR(s)) goto err;

	bc_program_binOpRetire(p, &res2);
	res.t = BC_RESULT_TEMP;
	bc_vec_push(&p->results, &res);

	return s;

err:
	bc_num_free(resn2);
	bc_num_free(resn);
	return s;
}

static BcStatus bc_program_modexp(BcProgram *p) {

	BcStatus s;
	BcResult *r1, *r2, *r3, res;
	BcNum *n1 = NULL, *n2 = NULL, *n3, *resn = &res.d.n;

	s = bc_program_operand(p, &r1, &n1, 2);
	if (BC_ERR(s)) return s;
	s = bc_program_type_num(r1, n1);
	if (BC_ERR(s)) return s;

	s = bc_program_binOpPrep(p, &r2, &n2, &r3, &n3);
	if (BC_ERR(s)) return s;

	// Make sure that the values have their pointers updated, if necessary.
	// Only array elements are possible.
	if (r1->t == BC_RESULT_ARRAY_ELEM && (r1->t == r2->t || r1->t == r3->t)) {
		s = bc_program_num(p, r1, &n1);
		if (BC_ERR(s)) return s;
	}

	bc_num_init(resn, n3->len);
	s = bc_num_modexp(n1, n2, n3, resn);
	if (BC_ERR(s)) goto err;

	bc_vec_pop(&p->results);
	bc_program_binOpRetire(p, &res);

	return s;

err:
	bc_num_free(resn);
	return s;
}

static void bc_program_stackLen(BcProgram *p) {
	BcResult res;
	res.t = BC_RESULT_TEMP;
	bc_num_createFromUlong(&res.d.n, p->results.len);
	bc_vec_push(&p->results, &res);
}

static BcStatus bc_program_asciify(BcProgram *p) {

	BcStatus s;
	BcResult *r, res;
	BcNum *n = NULL, num;
	char str[2], *str2, c;
	size_t len;
	unsigned long val;
	BcFunc f, *func;

	s = bc_program_operand(p, &r, &n, 0);
	if (BC_ERR(s)) return s;

	func = bc_vec_item(&p->fns, BC_PROG_MAIN);
	len = func->strs.len;

	assert(len + BC_PROG_REQ_FUNCS == p->fns.len);

	if (BC_PROG_NUM(r, n)) {

		bc_num_createCopy(&num, n);
		bc_num_truncate(&num, num.scale);
		num.neg = false;

		// This is guaranteed to not have a divide by 0
		// because strmb is equal to UCHAR_MAX + 1.
		s = bc_num_mod(&num, &p->strmb, &num, 0);
		assert(!s || s == BC_STATUS_SIGNAL);
#if BC_ENABLE_SIGNALS
		if (BC_ERROR_SIGNAL_ONLY(s)) goto num_err;
#endif // BC_ENABLE_SIGNALS

		// This is also guaranteed to not error because num is in the range
		// [0, UCHAR_MAX], which is definitely in range for an unsigned long.
		// And it is not negative.
		s = bc_num_ulong(&num, &val);
		assert(!s || s == BC_STATUS_SIGNAL);
#if BC_ENABLE_SIGNALS
		if (BC_ERROR_SIGNAL_ONLY(s)) goto num_err;
#endif // BC_ENABLE_SIGNALS

		c = (char) val;

		bc_num_free(&num);
	}
	else {
		size_t idx = r->t == BC_RESULT_STR ? r->d.id.idx : n->rdx;
		str2 = *((char**) bc_vec_item(&func->strs, idx));
		c = str2[0];
	}

	str[0] = c;
	str[1] = '\0';

	bc_program_addFunc(p, &f, bc_func_main);
	str2 = bc_vm_strdup(str);

	// Make sure the pointer is updated.
	func = bc_vec_item(&p->fns, BC_PROG_MAIN);
	bc_vec_push(&func->strs, &str2);

	res.t = BC_RESULT_STR;
	res.d.id.idx = len;
	bc_vec_pop(&p->results);
	bc_vec_push(&p->results, &res);

	return BC_STATUS_SUCCESS;

#if BC_ENABLE_SIGNALS
num_err:
	bc_num_free(&num);
	return s;
#endif // BC_ENABLE_SIGNALS
}

static BcStatus bc_program_printStream(BcProgram *p) {

	BcStatus s;
	BcResult *r;
	BcNum *n = NULL;

	s = bc_program_operand(p, &r, &n, 0);
	if (BC_ERR(s)) return s;

	if (BC_PROG_NUM(r, n)) s = bc_num_stream(n, &p->strmb);
	else {
		size_t idx = (r->t == BC_RESULT_STR) ? r->d.id.idx : n->rdx;
		bc_program_printChars(bc_program_str(p, idx, true));
	}

	return s;
}

static BcStatus bc_program_nquit(BcProgram *p) {

	BcStatus s;
	BcResult *opnd;
	BcNum *num = NULL;
	unsigned long val;

	s = bc_program_prep(p, &opnd, &num);
	if (BC_ERR(s)) return s;
	s = bc_num_ulong(num, &val);
	if (BC_ERR(s)) return s;

	bc_vec_pop(&p->results);

	s = bc_program_checkStack(&p->stack, val);
	if (BC_ERR(s)) return s;
	else if (p->stack.len == val) return BC_STATUS_QUIT;

	bc_vec_npop(&p->stack, val);

	return s;
}

static BcStatus bc_program_execStr(BcProgram *p, const char *restrict code,
                                   size_t *restrict bgn, bool cond)
{
	BcStatus s = BC_STATUS_SUCCESS;
	BcResult *r;
	char *str;
	BcFunc *f;
	BcParse prs;
	BcInstPtr ip;
	size_t fidx, sidx;
	BcNum *n = NULL;
	bool exec;

	s = bc_program_operand(p, &r, &n, 0);
	if (BC_ERR(s)) return s;

	if (cond) {

		char *name = NULL, *then_name, *else_name = NULL;

		then_name = bc_program_name(code, bgn);

		if (((uchar) code[*bgn]) == BC_PARSE_STREND) (*bgn) += 1;
		else else_name = bc_program_name(code, bgn);

		exec = (r->d.n.len != 0);

		if (exec) name = then_name;
		else if (else_name != NULL) {
			exec = true;
			name = else_name;
		}

		if (exec) n = bc_vec_top(bc_program_search(p, name, BC_TYPE_VAR));

		free(then_name);
		free(else_name);

		if (!exec) goto exit;
		if (BC_ERR(!BC_PROG_STR(n))) {
			s = bc_vm_err(BC_ERROR_EXEC_TYPE);
			goto exit;
		}

		sidx = n->rdx;
	}
	else {

		// In non-conditional situations, only the top of stack can be executed,
		// and in those cases, variables are not allowed to be "on the stack";
		// they are only put on the stack to be assigned to.
		assert(r->t != BC_RESULT_VAR);

		if (r->t == BC_RESULT_STR) sidx = r->d.id.idx;
		else goto no_exec;
	}

	fidx = sidx + BC_PROG_REQ_FUNCS;
	str = bc_program_str(p, sidx, true);
	f = bc_vec_item(&p->fns, fidx);

	if (!f->code.len) {

		bc_parse_init(&prs, p, fidx);
		bc_lex_file(&prs.l, vm->file);
		s = bc_parse_text(&prs, str);
		if (BC_ERR(s)) goto err;
		s = vm->expr(&prs, BC_PARSE_NOCALL);
		if (BC_ERR(s)) goto err;

		// We can just assert this here because
		// dc should parse everything until EOF.
		assert(prs.l.t == BC_LEX_EOF);

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
no_exec:
	return s;
}
#endif // DC_ENABLED

static void bc_program_pushGlobal(BcProgram *p, uchar inst) {

	BcResult res;

	assert(inst >= BC_INST_IBASE && inst <= BC_INST_SCALE);

	if (inst == BC_INST_SCALE)
		bc_num_createFromUlong(&res.d.n, (unsigned long) p->scale);
	else bc_num_createCopy(&res.d.n, inst == BC_INST_IBASE ? &p->ib : &p->ob);

	res.t = inst - BC_INST_IBASE + BC_RESULT_IBASE;
	bc_vec_push(&p->results, &res);
}

#ifndef NDEBUG
void bc_program_free(BcProgram *p) {
	assert(p);
	bc_vec_free(&p->scale_v);
	bc_vec_free(&p->ib_v);
	bc_vec_free(&p->ob_v);
	bc_vec_free(&p->fns);
#if BC_ENABLED
	bc_vec_free(&p->fn_map);
#endif // BC_ENABLED
	bc_vec_free(&p->vars);
	bc_vec_free(&p->var_map);
	bc_vec_free(&p->arrs);
	bc_vec_free(&p->arr_map);
	bc_vec_free(&p->results);
	bc_vec_free(&p->stack);
#if BC_ENABLED
	bc_num_free(&p->last);
#endif // BC_ENABLED
}
#endif // NDEBUG

void bc_program_init(BcProgram *p) {

	BcInstPtr ip;

	assert(p);

	memset(p, 0, sizeof(BcProgram));
	memset(&ip, 0, sizeof(BcInstPtr));

	bc_vec_init(&p->scale_v, sizeof(size_t), NULL);
	bc_vec_push(&p->scale_v, &p->scale);

	bc_num_setup(&p->ib, p->ib_num, BC_NUM_LONG_LOG10);
	bc_num_ten(&p->ib);
	p->ib_t = BC_BASE;
	bc_vec_init(&p->ib_v, sizeof(size_t), NULL);
	bc_vec_push(&p->ib_v, &p->ib_t);

	bc_num_setup(&p->ob, p->ob_num, BC_NUM_LONG_LOG10);
	bc_num_ten(&p->ob);
	p->ob_t = BC_BASE;
	bc_vec_init(&p->ob_v, sizeof(size_t), NULL);
	bc_vec_push(&p->ob_v, &p->ob_t);

#if DC_ENABLED
	bc_num_setup(&p->strmb, p->strmb_num, BC_NUM_LONG_LOG10);
	bc_num_ulong2num(&p->strmb, UCHAR_MAX + 1);
#endif // DC_ENABLED

#if BC_ENABLED
	bc_num_setup(&p->one, p->one_num, BC_PROG_ONE_CAP);
	bc_num_one(&p->one);
	bc_num_init(&p->last, BC_NUM_DEF_SIZE);
#endif // BC_ENABLED

	bc_vec_init(&p->fns, sizeof(BcFunc), bc_func_free);
#if BC_ENABLED
	bc_map_init(&p->fn_map);
	bc_program_insertFunc(p, bc_vm_strdup(bc_func_main));
	bc_program_insertFunc(p, bc_vm_strdup(bc_func_read));
#else // BC_ENABLED
	{
		BcFunc f;
		bc_program_addFunc(p, &f, bc_func_main);
		bc_program_addFunc(p, &f, bc_func_read);
	}
#endif // BC_ENABLED

	bc_vec_init(&p->vars, sizeof(BcVec), bc_vec_free);
	bc_map_init(&p->var_map);

	bc_vec_init(&p->arrs, sizeof(BcVec), bc_vec_free);
	bc_map_init(&p->arr_map);

	bc_vec_init(&p->results, sizeof(BcResult), bc_result_free);
	bc_vec_init(&p->stack, sizeof(BcInstPtr), NULL);
	bc_vec_push(&p->stack, &ip);
}

void bc_program_addFunc(BcProgram *p, BcFunc *f, const char *name) {
	bc_func_init(f, name);
	bc_vec_push(&p->fns, f);
}

#if BC_ENABLED
size_t bc_program_insertFunc(BcProgram *p, char *name) {

	BcId id;
	BcFunc f;
	bool new;
	size_t idx;

	assert(p && name);

	id.name = name;
	id.idx = p->fns.len;

	new = bc_map_insert(&p->fn_map, &id, &idx);
	idx = ((BcId*) bc_vec_item(&p->fn_map, idx))->idx;

	if (!new) {
		BcFunc *func = bc_vec_item(&p->fns, idx);
		bc_func_reset(func);
		free(name);
	}
	else bc_program_addFunc(p, &f, name);

	return idx;
}
#endif // BC_ENABLED

BcStatus bc_program_reset(BcProgram *p, BcStatus s) {

	BcFunc *f;
	BcInstPtr *ip;

	bc_vec_npop(&p->stack, p->stack.len - 1);
	bc_vec_npop(&p->results, p->results.len);

	f = bc_vec_item(&p->fns, 0);
	ip = bc_vec_top(&p->stack);
	ip->idx = f->code.len;

#if BC_ENABLE_SIGNALS
	if (BC_SIGTERM || (!s && BC_SIGINT && BC_I)) return BC_STATUS_QUIT;
	vm->sig = 0;

	if (!s || s == BC_STATUS_SIGNAL) {
		if (BC_TTYIN || BC_I) {
			bc_vm_puts(bc_program_ready_msg, stderr);
			bc_vm_fflush(stderr);
			s = BC_STATUS_SUCCESS;
		}
		else s = BC_STATUS_QUIT;
	}
#endif // BC_ENABLE_SIGNALS

	return s;
}

BcStatus bc_program_exec(BcProgram *p) {

	BcStatus s = BC_STATUS_SUCCESS;
	size_t idx;
	BcResult r, *ptr;
	BcInstPtr *ip = bc_vec_top(&p->stack);
	BcFunc *func = bc_vec_item(&p->fns, ip->func);
	char *code = func->code.v;
	bool cond = false;
#if BC_ENABLED
	BcNum *num = NULL;
#endif // BC_ENABLED

	while (BC_NO_SIG && BC_NO_ERR(!s) && s != BC_STATUS_QUIT &&
	       ip->idx < func->code.len)
	{
		uchar inst = (uchar) code[(ip->idx)++];

		switch (inst) {

#if BC_ENABLED
			case BC_INST_JUMP_ZERO:
			{
				s = bc_program_prep(p, &ptr, &num);
				if (BC_ERR(s)) return s;
				cond = !bc_num_cmpZero(num);
				bc_vec_pop(&p->results);
			}
			// Fallthrough.
			case BC_INST_JUMP:
			{
				size_t *addr;
				idx = bc_program_index(code, &ip->idx);
				addr = bc_vec_item(&func->labels, idx);
				assert(*addr != SIZE_MAX);
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
			case BC_INST_RET_VOID:
			{
				s = bc_program_return(p, inst);
				break;
			}

			case BC_INST_LAST:
			{
				r.t = BC_RESULT_LAST;
				bc_vec_push(&p->results, &r);
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
			case BC_INST_SCALE:
			case BC_INST_OBASE:
			{
				bc_program_pushGlobal(p, inst);
				break;
			}

			case BC_INST_LENGTH:
			case BC_INST_SCALE_FUNC:
			case BC_INST_SQRT:
			case BC_INST_ABS:
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
#ifndef BC_PROG_NO_STACK_CHECK
				s = bc_program_checkStack(&p->results, 1);
				if (BC_ERR(s)) return s;
#endif // BC_PROG_NO_STACK_CHECK
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
#if BC_ENABLE_EXTRA_MATH
			case BC_INST_PLACES:
			case BC_INST_LSHIFT:
			case BC_INST_RSHIFT:
#endif // BC_ENABLE_EXTRA_MATH
			{
				s = bc_program_op(p, inst);
				break;
			}

			case BC_INST_NEG:
			case BC_INST_BOOL_NOT:
#if BC_ENABLE_EXTRA_MATH
			case BC_INST_TRUNC:
#endif // BC_ENABLE_EXTRA_MATH
			{
				s = bc_program_unary(p, inst);
				break;
			}

#if BC_ENABLED
			case BC_INST_ASSIGN_POWER:
			case BC_INST_ASSIGN_MULTIPLY:
			case BC_INST_ASSIGN_DIVIDE:
			case BC_INST_ASSIGN_MODULUS:
			case BC_INST_ASSIGN_PLUS:
			case BC_INST_ASSIGN_MINUS:
#if BC_ENABLE_EXTRA_MATH
			case BC_INST_ASSIGN_PLACES:
			case BC_INST_ASSIGN_LSHIFT:
			case BC_INST_ASSIGN_RSHIFT:
#endif // BC_ENABLE_EXTRA_MATH
#endif // BC_ENABLED
			case BC_INST_ASSIGN:
			{
				s = bc_program_assign(p, inst);
				break;
			}
#if DC_ENABLED
			case BC_INST_POP_EXEC:
			{
				assert(BC_PROG_STACK(&p->stack, 2));
				bc_vec_pop(&p->stack);
				break;
			}

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
				cond = (inst == BC_INST_EXEC_COND);
				s = bc_program_execStr(p, code, &ip->idx, cond);
				break;
			}

			case BC_INST_PRINT_STACK:
			{
				for (idx = 0; BC_NO_ERR(!s) && idx < p->results.len; ++idx)
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
				s = bc_program_checkStack(&p->results, 1);
				if (BC_ERR(s)) break;
				ptr = bc_vec_top(&p->results);
				bc_result_copy(&r, ptr);
				bc_vec_push(&p->results, &r);
				break;
			}

			case BC_INST_SWAP:
			{
				BcResult *ptr2;

				s = bc_program_checkStack(&p->results, 2);
				if (BC_ERR(s)) break;

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
				bool copy = (inst == BC_INST_LOAD);
				s = bc_program_pushVar(p, code, &ip->idx, true, copy);
				break;
			}

			case BC_INST_PUSH_TO_VAR:
			{
				char *name = bc_program_name(code, &ip->idx);
				s = bc_program_copyToVar(p, name, BC_TYPE_VAR, true);
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

		if (BC_UNLIKELY((s && s != BC_STATUS_QUIT)) || BC_SIG)
			s = bc_program_reset(p, s);

		// If the stack has changed, pointers may be invalid.
		ip = bc_vec_top(&p->stack);
		func = bc_vec_item(&p->fns, ip->func);
		code = func->code.v;
	}

	return s;
}

#if BC_DEBUG_CODE
#if BC_ENABLED && DC_ENABLED
static void bc_program_printIndex(const char *restrict code,
                                  size_t *restrict bgn)
{
	uchar byte, i, bytes = (uchar) code[(*bgn)++];
	unsigned long val = 0;

	for (byte = 1, i = 0; byte && i < bytes; ++i) {
		byte = (uchar) code[(*bgn)++];
		if (byte) val |= ((unsigned long) byte) << (CHAR_BIT * i);
	}

	bc_vm_printf(" (%lu) ", val);
}

static void bc_program_printName(const char *restrict code,
                                 size_t *restrict bgn)
{
	uchar byte;

	bc_vm_printf(" (");

	while ((byte = (uchar) code[(*bgn)++]) && byte != BC_PARSE_STREND)
		bc_vm_putchar(byte);

	assert(byte);

	bc_vm_printf(") ");
}

static void bc_program_printStr(BcProgram *p, const char *restrict code,
                         size_t *restrict bgn)
{
	size_t idx = bc_program_index(code, bgn);
	char *s;

	s = bc_program_str(p, idx, true);

	bc_vm_printf(" (\"%s\") ", s);
}

void bc_program_printInst(BcProgram *p, const char *restrict code,
                          size_t *restrict bgn)
{
	uchar inst = (uchar) code[(*bgn)++];

	bc_vm_printf("Inst: %u, %c; ", inst, (char) inst);

	if (inst == BC_INST_VAR || inst == BC_INST_ARRAY_ELEM ||
	    inst == BC_INST_ARRAY)
	{
		bc_program_printName(code, bgn);
	}
	else if (inst == BC_INST_STR) bc_program_printStr(p, code, bgn);
	else if (inst == BC_INST_NUM) {
		size_t idx = bc_program_index(code, bgn);
		char *str = bc_program_str(p, idx, false);
		bc_vm_printf("(%s)", str);
	}
	else if (inst == BC_INST_CALL ||
	         (inst > BC_INST_STR && inst <= BC_INST_JUMP_ZERO))
	{
		bc_program_printIndex(code, bgn);
		if (inst == BC_INST_CALL) bc_program_printIndex(code, bgn);
	}

	bc_vm_putchar('\n');
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
#endif // BC_DEBUG_CODE
