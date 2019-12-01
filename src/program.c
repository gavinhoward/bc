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
static BcStatus bc_program_checkStack(const BcVec *v, size_t n) {
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

#if BC_ENABLED
static BcStatus bc_program_type_match(BcResult *r, BcType t) {
#if DC_ENABLED
	assert(!BC_IS_BC || BC_NO_ERR(r->t != BC_RESULT_STR));
#endif // DC_ENABLED
	if (BC_ERR((r->t != BC_RESULT_ARRAY) != (!t)))
		return bc_vm_err(BC_ERROR_EXEC_TYPE);
	return BC_STATUS_SUCCESS;
}
#endif // BC_ENABLED

static BcFunc* bc_program_func(const BcProgram *p) {

	size_t i;

	if (BC_IS_BC) {
		BcInstPtr *ip = bc_vec_item_rev(&p->stack, 0);
		i = ip->func;
	}
	else i = BC_PROG_MAIN;

	return bc_vec_item(&p->fns, i);
}

static BcConst* bc_program_const(const BcProgram *p, size_t idx) {
	BcFunc *f = bc_program_func(p);
	return bc_vec_item(&f->consts, idx);
}

static char* bc_program_str(const BcProgram *p, size_t idx) {
	BcFunc *f = bc_program_func(p);
	return *((char**) bc_vec_item(&f->strs, idx));
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

static void bc_program_prepGlobals(BcProgram *p) {
	size_t i;
	for (i = 0; i < BC_PROG_GLOBALS_LEN; ++i)
		bc_vec_push(p->globals_v + i, p->globals + i);
}

#if BC_ENABLED
static BcVec* bc_program_dereference(const BcProgram *p, BcVec *vec) {

	BcVec *v;
	size_t vidx, nidx, i = 0;

	assert(vec->size == sizeof(uchar));

	vidx = bc_program_index(vec->v, &i);
	nidx = bc_program_index(vec->v, &i);

	v = bc_vec_item(bc_vec_item(&p->arrs, vidx), nidx);

	assert(v->size != sizeof(uchar));

	return v;
}
#endif // BC_ENABLED

size_t bc_program_search(BcProgram *p, char *id, bool var) {

	BcId e, *ptr;
	BcVec *v, *map;
	size_t i;
	BcResultData data;
	bool new;

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

	return ptr->idx;
}

static BcVec* bc_program_vec(const BcProgram *p, size_t idx, BcType type) {
	const BcVec *v = (type == BC_TYPE_VAR) ? &p->vars : &p->arrs;
	return bc_vec_item(v, idx);
}

static BcStatus bc_program_num(BcProgram *p, BcResult *r, BcNum **num) {

	BcStatus s = BC_STATUS_SUCCESS;
	BcNum *n;

	switch (r->t) {

		case BC_RESULT_CONSTANT:
		{
			BcConst *c = bc_program_const(p, r->d.loc.loc);
			BcBigDig base = BC_PROG_IBASE(p);

			if (c->base != base) {

				if (c->num.num == NULL)
					bc_num_init(&c->num, BC_NUM_RDX(strlen(c->val)));

				s = bc_num_parse(&c->num, c->val, base, !c->val[1]);
				assert(!s || s == BC_STATUS_SIGNAL);

#if BC_ENABLE_SIGNALS
				// bc_num_parse() should only do operations that can
				// only fail when signals happen. Thus, if signals
				// are not enabled, we don't need this check.
				if (BC_ERROR_SIGNAL_ONLY(s)) return s;
#endif // BC_ENABLE_SIGNALS

				c->base = base;
			}

			n = &r->d.n;
			bc_num_createCopy(n, &c->num);

			r->t = BC_RESULT_TEMP;
			break;
		}

		case BC_RESULT_STR:
		case BC_RESULT_TEMP:
		case BC_RESULT_IBASE:
		case BC_RESULT_SCALE:
		case BC_RESULT_OBASE:
		{
			n = &r->d.n;
			break;
		}

		case BC_RESULT_VAR:
#if BC_ENABLED
		case BC_RESULT_ARRAY:
#endif // BC_ENABLED
		case BC_RESULT_ARRAY_ELEM:
		{
			BcVec *v;
			BcType type = (r->t == BC_RESULT_VAR) ? BC_TYPE_VAR : BC_TYPE_ARRAY;

			v = bc_program_vec(p, r->d.loc.loc, type);

			if (r->t == BC_RESULT_ARRAY_ELEM) {

				size_t idx = r->d.loc.idx;

				v = bc_vec_top(v);

#if BC_ENABLED
				if (v->size == sizeof(uchar)) v = bc_program_dereference(p, v);
#endif // BC_ENABLED

				assert(v->size == sizeof(BcNum));

				if (v->len <= idx) bc_array_expand(v, bc_vm_growSize(idx, 1));

				n = bc_vec_item(v, idx);
			}
			else n = bc_vec_top(v);

			break;
		}

		case BC_RESULT_ONE:
		{
			n = &p->one;
			break;
		}

#if BC_ENABLED
		case BC_RESULT_LAST:
		{
			n = &p->last;
			break;
		}

		case BC_RESULT_VOID:
		{
#ifndef NDEBUG
			assert(false);
#endif // NDEBUG
			n = &r->d.n;
			break;
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

	assert(p != NULL && l != NULL && ln != NULL && r != NULL && rn != NULL);

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
	           lt == BC_RESULT_ONE))
	{
		return bc_vm_err(BC_ERROR_EXEC_TYPE);
	}

#if BC_ENABLED
	if (BC_ERR(lt == BC_RESULT_ARRAY))
		return bc_vm_err(BC_ERROR_EXEC_TYPE);
#endif // BC_ENABLED

#if DC_ENABLED
	if(!BC_IS_BC) good = (((*r)->t == BC_RESULT_STR || BC_PROG_STR(*rn)) &&
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

	assert(p != NULL && r != NULL && n != NULL);

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
	BcNum *n1, *n2;
	size_t idx = inst - BC_INST_POWER;

	s = bc_program_binOpPrep(p, &opd1, &n1, &opd2, &n2);
	if (BC_ERR(s)) return s;
	bc_num_init(&res.d.n, bc_program_opReqs[idx](n1, n2, BC_PROG_SCALE(p)));

	s = bc_program_ops[idx](n1, n2, &res.d.n, BC_PROG_SCALE(p));
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

	s = bc_read_line(&buf, BC_IS_BC ? "read> " : "?> ");
	if (BC_ERR(s)) {
		if (s == BC_STATUS_EOF) s = bc_vm_err(BC_ERROR_EXEC_READ_EXPR);
		goto io_err;
	}

	bc_parse_init(&parse, p, BC_PROG_READ);

	s = bc_parse_text(&parse, buf.v);
	if (BC_ERR(s)) goto exec_err;
	s = vm->expr(&parse, BC_PARSE_NOREAD | BC_PARSE_NEEDVAL);
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
#if DC_ENABLED
	if (!BC_IS_BC) {
		size_t temp = 0;
		bc_vec_push(&p->tail_calls, &temp);
	}
#endif // DC_ENABLED

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

	for (i = 0; i < len; ++i) {

		int c = str[i];

		if (c == '\\' && i != len - 1) {

			const char *ptr;

			c = str[++i];
			ptr = strchr(bc_program_esc_chars, c);

			if (ptr != NULL) {
				if (c == 'n') vm->nchars = SIZE_MAX;
				c = bc_program_esc_seqs[(size_t) (ptr - bc_program_esc_chars)];
			}
			else {
				// Just print the backslash. The following
				// character will be printed later.
				bc_vm_putchar('\\');
			}
		}

		bc_vm_putchar(c);
	}
}

static BcStatus bc_program_print(BcProgram *p, uchar inst, size_t idx) {

	BcStatus s = BC_STATUS_SUCCESS;
	BcResult *r;
	char *str;
	BcNum *n;
	bool pop = (inst != BC_INST_PRINT);

	assert(p != NULL);

#ifndef BC_PROG_NO_STACK_CHECK
	s = bc_program_checkStack(&p->results, idx + 1);
	if (BC_ERR(s)) return s;
#endif // BC_PROG_NO_STACK_CHECK

	r = bc_vec_item_rev(&p->results, idx);

#if BC_ENABLED
	if (r->t == BC_RESULT_VOID) {
		if (BC_ERR(pop)) return bc_vm_err(BC_ERROR_EXEC_VOID_VAL);
		bc_vec_pop(&p->results);
		return s;
	}
#endif // BC_ENABLED

	s = bc_program_num(p, r, &n);
	if (BC_ERR(s)) return s;

	if (BC_PROG_NUM(r, n)) {
		assert(inst != BC_INST_PRINT_STR);
		s = bc_num_print(n, BC_PROG_OBASE(p), !pop);
#if BC_ENABLED
		if (BC_NO_ERR(!s) && BC_IS_BC) bc_num_copy(&p->last, n);
#endif // BC_ENABLED
	}
	else {

		size_t i = (r->t == BC_RESULT_STR) ? r->d.loc.loc : n->scale;

		str = bc_program_str(p, i);

		if (inst == BC_INST_PRINT_STR) bc_program_printChars(str);
		else {
			bc_program_printString(str);
			if (inst == BC_INST_PRINT) bc_vm_putchar('\n');
		}
	}

	if (BC_NO_ERR(!s) && (BC_IS_BC || pop)) bc_vec_pop(&p->results);

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
	BcNum *num;

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
	BcNum *n1, *n2;
	bool cond = 0;
	ssize_t cmp;

	s = bc_program_binOpPrep(p, &opd1, &n1, &opd2, &n2);
	if (BC_ERR(s)) return s;

	if (inst == BC_INST_BOOL_AND)
		cond = (bc_num_cmpZero(n1) && bc_num_cmpZero(n2));
	else if (inst == BC_INST_BOOL_OR)
		cond = (bc_num_cmpZero(n1) || bc_num_cmpZero(n2));
	else {

		cmp = bc_num_cmp(n1, n2);

#if BC_ENABLE_SIGNALS
		if (BC_NUM_CMP_SIGNAL(cmp)) return BC_STATUS_SIGNAL;
#endif // BC_ENABLE_SIGNALS

		switch (inst) {

			case BC_INST_REL_EQ:
			{
				cond = (cmp == 0);
				break;
			}

			case BC_INST_REL_LE:
			{
				cond = (cmp <= 0);
				break;
			}

			case BC_INST_REL_GE:
			{
				cond = (cmp >= 0);
				break;
			}

			case BC_INST_REL_NE:
			{
				cond = (cmp != 0);
				break;
			}

			case BC_INST_REL_LT:
			{
				cond = (cmp < 0);
				break;
			}

			case BC_INST_REL_GT:
			{
				cond = (cmp > 0);
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

	bc_num_init(&res.d.n, BC_NUM_DEF_SIZE);
	if (cond) bc_num_one(&res.d.n);

	bc_program_binOpRetire(p, &res);

	return s;
}

#if DC_ENABLED
static BcStatus bc_program_assignStr(BcProgram *p, BcResult *r,
                                     BcVec *v, bool push)
{
	BcNum n2;

	memset(&n2, 0, sizeof(BcNum));
	n2.num = NULL;
	n2.scale = r->d.loc.loc;

	if (!push) {
#ifndef BC_PROG_NO_STACK_CHECK
		BcStatus s = bc_program_checkStack(&p->results, 2);
		if (BC_ERR(s)) return s;
#endif // BC_PROG_NO_STACK_CHECK
		bc_vec_pop(v);
		bc_vec_pop(&p->results);
	}

	bc_vec_pop(&p->results);
	bc_vec_push(v, &n2);

	return BC_STATUS_SUCCESS;
}
#endif // DC_ENABLED

static BcStatus bc_program_copyToVar(BcProgram *p, size_t idx,
                                     BcType t, bool last)
{
	BcStatus s = BC_STATUS_SUCCESS;
	BcResult *ptr, r;
	BcVec *vec;
	BcNum *n = NULL;
	bool var = (t == BC_TYPE_VAR);

#if BC_ENABLED
#if DC_ENABLED
	if (!BC_IS_BC) s = bc_program_operand(p, &ptr, &n, 0);
	else
#endif // DC_ENABLED
	{
		ptr = bc_vec_top(&p->results);

		s = bc_program_type_match(ptr, t);
		if (BC_ERR(s)) return s;

		if (last) s = bc_program_num(p, ptr, &n);
		else if (var)
			n = bc_vec_item_rev(bc_program_vec(p, ptr->d.loc.loc, t), 1);
	}
#else // BC_ENABLED
	s = bc_program_operand(p, &ptr, &n, 0);
#endif // BC_ENABLED

	if (BC_ERR(s)) return s;

	vec = bc_program_vec(p, idx, t);

#if DC_ENABLED
	if (ptr->t == BC_RESULT_STR) {
		if (BC_ERR(!var)) return bc_vm_err(BC_ERROR_EXEC_TYPE);
		return bc_program_assignStr(p, ptr, vec, true);
	}
#endif // DC_ENABLED

	if (var) bc_num_createCopy(&r.d.n, n);
	else {

		BcVec *v = (BcVec*) n, *rv = &r.d.v;
#if BC_ENABLED
		BcVec *parent;
		bool ref, ref_size;

		parent = bc_program_vec(p, ptr->d.loc.loc, t);
		assert(parent != NULL);

		if (!last) v = bc_vec_item_rev(parent, !last);
		assert(v != NULL);

		ref = (v->size == sizeof(BcNum) && t == BC_TYPE_REF);
		ref_size = (v->size == sizeof(uchar));

		if (ref || (ref_size && t == BC_TYPE_REF)) {

			bc_vec_init(rv, sizeof(uchar), NULL);

			if (ref) {

				assert(parent->len >= (size_t) (!last + 1));

				// Make sure the pointer was not invalidated.
				vec = bc_program_vec(p, idx, t);

				bc_vec_pushIndex(rv, ptr->d.loc.loc);
				bc_vec_pushIndex(rv, parent->len - !last - 1);
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
	BcNum *l, *r;
	bool ob, sc, use_val = BC_INST_USE_VAL(inst);

	s = bc_program_assignPrep(p, &left, &l, &right, &r);
	if (BC_ERR(s)) return s;

#if DC_ENABLED
	assert(left->t != BC_RESULT_STR);

	if (right->t == BC_RESULT_STR) {

		size_t idx = right->d.loc.loc;

		if (left->t == BC_RESULT_ARRAY_ELEM) {
			bc_num_free(l);
			memset(l, 0, sizeof(BcNum));
			l->num = NULL;
			l->scale = idx;
		}
		else {
			BcVec *v = bc_program_vec(p, left->d.loc.loc, BC_TYPE_VAR);
			s = bc_program_assignStr(p, right, v, false);
		}

		return s;
	}
#endif // DC_ENABLED

	if (BC_INST_IS_ASSIGN(inst)) bc_num_copy(l, r);
#if BC_ENABLED
	else {

		BcBigDig scale = BC_PROG_SCALE(p);

		if (!use_val)
			inst -= (BC_INST_ASSIGN_POWER_NO_VAL - BC_INST_ASSIGN_POWER);

		s = bc_program_ops[inst - BC_INST_ASSIGN_POWER](l, r, l, scale);
		if (BC_ERR(s)) return s;
	}
#endif // BC_ENABLED

	ob = (left->t == BC_RESULT_OBASE);
	sc = (left->t == BC_RESULT_SCALE);

	if (ob || sc || left->t == BC_RESULT_IBASE) {

		BcVec *v;
		BcBigDig *ptr, *ptr_t, val, max, min;
		BcError e;

		s = bc_num_bigdig(l, &val);
		if (BC_ERR(s)) return s;
		e = left->t - BC_RESULT_IBASE + BC_ERROR_EXEC_IBASE;

		if (sc) {
			min = 0;
			max = vm->maxes[BC_PROG_GLOBALS_SCALE];
			v = p->globals_v + BC_PROG_GLOBALS_SCALE;
			ptr_t = p->globals + BC_PROG_GLOBALS_SCALE;
		}
		else {
			min = BC_NUM_MIN_BASE;
			if (BC_ENABLE_EXTRA_MATH && ob && (!BC_IS_BC || !BC_IS_POSIX))
				min = 0;
			max = vm->maxes[ob + BC_PROG_GLOBALS_IBASE];
			v = p->globals_v + BC_PROG_GLOBALS_IBASE + ob;
			ptr_t = p->globals + BC_PROG_GLOBALS_IBASE + ob;
		}

		if (BC_ERR(val > max || val < min)) return bc_vm_verr(e, min, max);

		ptr = bc_vec_top(v);
		*ptr = val;
		*ptr_t = val;
	}

	if (use_val) {
		bc_num_createCopy(&res.d.n, l);
		bc_program_binOpRetire(p, &res);
	}
	else {
		bc_vec_pop(&p->results);
		bc_vec_pop(&p->results);
	}

	return s;
}

static BcStatus bc_program_pushVar(BcProgram *p, const char *restrict code,
                                   size_t *restrict bgn, bool pop, bool copy)
{
	BcStatus s = BC_STATUS_SUCCESS;
	BcResult r;
	size_t idx = bc_program_index(code, bgn);

	r.t = BC_RESULT_VAR;
	r.d.loc.loc = idx;

#if DC_ENABLED
	if (pop || copy) {

		BcVec *v = bc_program_vec(p, idx, BC_TYPE_VAR);
		BcNum *num = bc_vec_top(v);

		s = bc_program_checkStack(v, 2 - copy);
		if (BC_ERR(s)) return s;

		if (!BC_PROG_STR(num)) {
			r.t = BC_RESULT_TEMP;
			bc_num_createCopy(&r.d.n, num);
		}
		else {
			r.t = BC_RESULT_STR;
			r.d.loc.loc = num->scale;
		}

		if (!copy) bc_vec_pop(v);
	}
#endif // DC_ENABLED

	bc_vec_push(&p->results, &r);

	return s;
}

static BcStatus bc_program_pushArray(BcProgram *p, const char *restrict code,
                                     size_t *restrict bgn, uchar inst)
{
	BcStatus s = BC_STATUS_SUCCESS;
	BcResult r, *operand;
	BcNum *num;
	BcBigDig temp;

	r.d.loc.loc = bc_program_index(code, bgn);

#if BC_ENABLED
	if (inst == BC_INST_ARRAY) {
		r.t = BC_RESULT_ARRAY;
		bc_vec_push(&p->results, &r);
		return s;
	}
#endif // BC_ENABLED

	s = bc_program_prep(p, &operand, &num);
	if (BC_ERR(s)) return s;
	s = bc_num_bigdig(num, &temp);
	if (BC_ERR(s)) return s;

	r.d.loc.idx = (size_t) temp;
	bc_program_retire(p, &r, BC_RESULT_ARRAY_ELEM);

	return s;
}

#if BC_ENABLED
static BcStatus bc_program_incdec(BcProgram *p, uchar inst) {

	BcStatus s;
	BcResult *ptr, res, copy;
	BcNum *num;
	uchar inst2;
	bool post, use_val;

	s = bc_program_prep(p, &ptr, &num);
	if (BC_ERR(s)) return s;

	use_val = (inst != BC_INST_INC_NO_VAL && inst != BC_INST_DEC_NO_VAL);
	post = use_val && (inst == BC_INST_INC_POST || inst == BC_INST_DEC_POST);

	if (post) {
		copy.t = BC_RESULT_TEMP;
		bc_num_createCopy(&copy.d.n, num);
	}

	res.t = BC_RESULT_ONE;
	inst2 = BC_INST_ASSIGN_PLUS;

	if (!use_val) {
		inst2 += (inst == BC_INST_DEC_NO_VAL);
		inst2 += (BC_INST_ASSIGN_PLUS_NO_VAL - BC_INST_ASSIGN_PLUS);
	}
	else inst2 += (inst & 0x01);

	bc_vec_push(&p->results, &res);
	s = bc_program_assign(p, inst2);

	if (post) {

		if (BC_ERR(s)) {
			bc_num_free(&copy.d.n);
			return s;
		}

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
	BcLoc *a;
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

		arg = bc_vec_top(&p->results);
		if (BC_ERR(arg->t == BC_RESULT_VOID))
			return bc_vm_err(BC_ERROR_EXEC_VOID_VAL);

		a = bc_vec_item(&f->autos, nparams - 1 - i);

		// If I have already pushed to a var, I need to make sure I
		// get the previous version, not the already pushed one.
		if (arg->t == BC_RESULT_VAR || arg->t == BC_RESULT_ARRAY) {
			for (j = 0; j < i && last; ++j) {
				BcLoc *loc = bc_vec_item(&f->autos, nparams - 1 - j);
				last = (arg->d.loc.loc != loc->loc ||
				        (!loc->idx) != (arg->t == BC_RESULT_VAR));
			}
		}

		s = bc_program_copyToVar(p, a->loc, (BcType) a->idx, last);
		if (BC_ERR(s)) return s;
	}

	for (; i < f->autos.len; ++i) {

		a = bc_vec_item(&f->autos, i);
		v = bc_program_vec(p, a->loc, (BcType) a->idx);

		if (a->idx == BC_TYPE_VAR) {
			bc_num_init(&param.n, BC_NUM_DEF_SIZE);
			bc_vec_push(v, &param.n);
		}
		else {
			assert(a->idx == BC_TYPE_ARRAY);
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

		BcNum *num;
		BcResult *operand;

		s = bc_program_operand(p, &operand, &num, 0);
		if (BC_ERR(s)) return s;

		bc_num_createCopy(&res.d.n, num);
	}
	else if (inst == BC_INST_RET_VOID) res.t = BC_RESULT_VOID;
	else bc_num_init(&res.d.n, BC_NUM_DEF_SIZE);

	// We need to pop arguments as well, so this takes that into account.
	for (i = 0; i < f->autos.len; ++i) {

		BcLoc *a = bc_vec_item(&f->autos, i);
		BcVec *v = bc_program_vec(p, a->loc, (BcType) a->idx);

		bc_vec_pop(v);
	}

	bc_vec_npop(&p->results, p->results.len - ip->len);

	if (BC_G) {

		for (i = 0; i < BC_PROG_GLOBALS_LEN; ++i) {
			BcVec *v = p->globals_v + i;
			bc_vec_pop(v);
			p->globals[i] = BC_PROG_GLOBAL(v);
		}
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
	BcNum *num, *resn = &res.d.n;
	bool len = (inst == BC_INST_LENGTH);

	assert(inst >= BC_INST_LENGTH && inst <= BC_INST_ABS);

	s = bc_program_operand(p, &opd, &num, 0);
	if (BC_ERR(s)) return s;

	assert(num != NULL);

#if DC_ENABLED
	if (!len && inst != BC_INST_SCALE_FUNC) {
		s = bc_program_type_num(opd, num);
		if (BC_ERR(s)) return s;
	}
#endif // DC_ENABLED

	if (inst == BC_INST_SQRT) {
		s = bc_num_sqrt(num, resn, BC_PROG_SCALE(p));
		if (BC_ERR(s)) return s;
	}
	else if (inst == BC_INST_ABS) {
		bc_num_createCopy(resn, num);
		resn->neg = false;
	}
	else {

		BcBigDig val = 0;

		if (len) {
#if BC_ENABLED
			if (BC_IS_BC && opd->t == BC_RESULT_ARRAY)
				val = (BcBigDig) ((BcVec*) num)->len;
			else
#endif // BC_ENABLED
			{
#if DC_ENABLED
				if (!BC_PROG_NUM(opd, num)) {
					size_t idx;
					idx = opd->t == BC_RESULT_STR ? opd->d.loc.loc : num->scale;
					val = (BcBigDig) strlen(bc_program_str(p, idx));
				}
				else
#endif // DC_ENABLED
				{
					val = (BcBigDig) bc_num_len(num);
				}
			}
		}
		else if (BC_IS_BC || BC_PROG_NUM(opd, num))
			val = (BcBigDig) bc_num_scale(num);

		bc_num_createFromBigdig(resn, val);
	}

	bc_program_retire(p, &res, BC_RESULT_TEMP);

	return s;
}

#if DC_ENABLED
static BcStatus bc_program_divmod(BcProgram *p) {

	BcStatus s;
	BcResult *opd1, *opd2, res, res2;
	BcNum *n1, *n2, *resn = &res.d.n, *resn2 = &res2.d.n;
	size_t req;

	s = bc_program_binOpPrep(p, &opd1, &n1, &opd2, &n2);
	if (BC_ERR(s)) return s;

	req = bc_num_mulReq(n1, n2, BC_PROG_SCALE(p));
	bc_num_init(resn, req);
	bc_num_init(resn2, req);

	s = bc_num_divmod(n1, n2, resn2, resn, BC_PROG_SCALE(p));
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
	BcNum *n1, *n2, *n3, *resn = &res.d.n;

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
	bc_num_createFromBigdig(&res.d.n, (BcBigDig) p->results.len);
	bc_vec_push(&p->results, &res);
}

static BcStatus bc_program_asciify(BcProgram *p) {

	BcStatus s;
	BcResult *r, res;
	BcNum *n, num;
	char str[2], *str2, c;
	size_t len;
	BcBigDig val;
	BcFunc f, *func;

	s = bc_program_operand(p, &r, &n, 0);
	if (BC_ERR(s)) return s;

	assert(n != NULL);

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
		// [0, UCHAR_MAX], which is definitely in range for a BcBigDig. And
		// it is not negative.
		s = bc_num_bigdig(&num, &val);
		assert(!s || s == BC_STATUS_SIGNAL);
#if BC_ENABLE_SIGNALS
		if (BC_ERROR_SIGNAL_ONLY(s)) goto num_err;
#endif // BC_ENABLE_SIGNALS

		c = (char) val;

		bc_num_free(&num);
	}
	else {
		size_t idx = r->t == BC_RESULT_STR ? r->d.loc.loc : n->scale;
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
	res.d.loc.loc = len;
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
	BcNum *n;

	s = bc_program_operand(p, &r, &n, 0);
	if (BC_ERR(s)) return s;

	assert(n != NULL);

	if (BC_PROG_NUM(r, n)) s = bc_num_stream(n, p->strm);
	else {
		size_t idx = (r->t == BC_RESULT_STR) ? r->d.loc.loc : n->scale;
		bc_program_printChars(bc_program_str(p, idx));
	}

	return s;
}

static BcStatus bc_program_nquit(BcProgram *p, uchar inst) {

	BcStatus s;
	BcResult *opnd;
	BcNum *num;
	BcBigDig val;
	size_t i;

	assert(p->stack.len == p->tail_calls.len);

	if (inst == BC_INST_QUIT) {
		val = 2;
		s = BC_STATUS_SUCCESS;
	}
	else {

		s = bc_program_prep(p, &opnd, &num);
		if (BC_ERR(s)) return s;
		s = bc_num_bigdig(num, &val);
		if (BC_ERR(s)) return s;

		bc_vec_pop(&p->results);
	}

	for (i = 0; val && i < p->tail_calls.len; ++i) {
		size_t calls = *((size_t*) bc_vec_item_rev(&p->tail_calls, i)) + 1;
		if (calls >= val) val = 0;
		else val -= calls;
	}

	if (i == p->stack.len) s = BC_STATUS_QUIT;
	else {
		bc_vec_npop(&p->stack, i);
		bc_vec_npop(&p->tail_calls, i);
	}

	return s;
}

static BcStatus bc_program_execStr(BcProgram *p, const char *restrict code,
                                   size_t *restrict bgn, bool cond, size_t len)
{
	BcStatus s = BC_STATUS_SUCCESS;
	BcResult *r;
	char *str;
	BcFunc *f;
	BcParse prs;
	BcInstPtr ip;
	size_t fidx, sidx;
	BcNum *n;
	bool exec;

	assert(p->stack.len == p->tail_calls.len);

	s = bc_program_operand(p, &r, &n, 0);
	if (BC_ERR(s)) return s;

	if (cond) {

		size_t idx = SIZE_MAX, then_idx, else_idx;

		then_idx = bc_program_index(code, bgn);
		else_idx = bc_program_index(code, bgn);

		exec = (r->d.n.len != 0);

		if (exec) idx = then_idx;
		else {
			exec = (else_idx != SIZE_MAX);
			idx = else_idx;
		}

		if (exec) n = bc_vec_top(bc_program_vec(p, idx, BC_TYPE_VAR));
		else goto exit;

		if (BC_ERR(!BC_PROG_STR(n))) {
			s = bc_vm_err(BC_ERROR_EXEC_TYPE);
			goto exit;
		}

		sidx = n->scale;
	}
	else {

		// In non-conditional situations, only the top of stack can be executed,
		// and in those cases, variables are not allowed to be "on the stack";
		// they are only put on the stack to be assigned to.
		assert(r->t != BC_RESULT_VAR);

		if (r->t == BC_RESULT_STR) sidx = r->d.loc.loc;
		else goto no_exec;
	}

	fidx = sidx + BC_PROG_REQ_FUNCS;
	str = bc_program_str(p, sidx);
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

	// Tail call.
	if (p->stack.len > 1 && *bgn == len - 1 && code[*bgn] == BC_INST_POP_EXEC) {
		size_t *call_ptr = bc_vec_top(&p->tail_calls);
		*call_ptr += 1;
		bc_vec_pop(&p->stack);
	}
	else bc_vec_push(&p->tail_calls, &ip.idx);

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

static BcStatus bc_program_printStack(BcProgram *p) {

	BcStatus s = BC_STATUS_SUCCESS;
	size_t idx;

	for (idx = 0; BC_NO_ERR(!s) && idx < p->results.len; ++idx)
		s = bc_program_print(p, BC_INST_PRINT, idx);

	return s;
}
#endif // DC_ENABLED

static void bc_program_pushBigDig(BcProgram *p, BcBigDig dig, BcResultType type)
{
	BcResult res;

	bc_num_createFromBigdig(&res.d.n, dig);

	res.t = type;
	bc_vec_push(&p->results, &res);
}

static void bc_program_pushGlobal(BcProgram *p, uchar inst) {

	BcResultType t;

	assert(inst >= BC_INST_IBASE && inst <= BC_INST_SCALE);

	t = inst - BC_INST_IBASE + BC_RESULT_IBASE;
	bc_program_pushBigDig(p, p->globals[inst - BC_INST_IBASE], t);
}

#ifndef NDEBUG
void bc_program_free(BcProgram *p) {

	size_t i;

	assert(p != NULL);

	for (i = 0; i < BC_PROG_GLOBALS_LEN; ++i) bc_vec_free(p->globals_v + i);

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
	if (BC_IS_BC) {
		bc_num_free(&p->last);
	}
#endif // BC_ENABLED

#if DC_ENABLED
	if (!BC_IS_BC) bc_vec_free(&p->tail_calls);
#endif // DC_ENABLED
}
#endif // NDEBUG

void bc_program_init(BcProgram *p) {

	BcInstPtr ip;
	size_t i;
	BcBigDig val = BC_BASE;

	assert(p != NULL);

	memset(p, 0, sizeof(BcProgram));
	memset(&ip, 0, sizeof(BcInstPtr));

	for (i = 0; i < BC_PROG_GLOBALS_LEN; ++i) {
		bc_vec_init(p->globals_v + i, sizeof(BcBigDig), NULL);
		val = i == BC_PROG_GLOBALS_SCALE ? 0 : val;
		bc_vec_push(p->globals_v + i, &val);
		p->globals[i] = val;
	}

#if DC_ENABLED
	if (!BC_IS_BC) {

		bc_vec_init(&p->tail_calls, sizeof(size_t), NULL);
		i = 0;
		bc_vec_push(&p->tail_calls, &i);

		p->strm = UCHAR_MAX + 1;
		bc_num_setup(&p->strmb, p->strmb_num, BC_NUM_BIGDIG_LOG10);
		bc_num_bigdig2num(&p->strmb, p->strm);
	}
#endif // DC_ENABLED

	bc_num_setup(&p->one, p->one_num, BC_PROG_ONE_CAP);
	bc_num_one(&p->one);

#if BC_ENABLED
	if (BC_IS_BC) bc_num_init(&p->last, BC_NUM_DEF_SIZE);
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

	assert(p != NULL && name != NULL);

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

	vm->sig_chk = vm->sig;

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
	BcNum *num;
#endif // BC_ENABLED

	while (BC_NO_SIG && BC_NO_ERR(!s) && ip->idx < func->code.len) {

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
				idx = bc_program_index(code, &ip->idx);

				if (inst == BC_INST_JUMP || cond) {

					size_t *addr = bc_vec_item(&func->labels, idx);

					assert(*addr != SIZE_MAX);

					ip->idx = *addr;
				}

				break;
			}

			case BC_INST_CALL:
			{
				s = bc_program_call(p, code, &ip->idx);
				ip = bc_vec_top(&p->stack);
				func = bc_vec_item(&p->fns, ip->func);
				code = func->code.v;
				break;
			}

			case BC_INST_INC_PRE:
			case BC_INST_DEC_PRE:
			case BC_INST_INC_POST:
			case BC_INST_DEC_POST:
			case BC_INST_INC_NO_VAL:
			case BC_INST_DEC_NO_VAL:
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
				ip = bc_vec_top(&p->stack);
				func = bc_vec_item(&p->fns, ip->func);
				code = func->code.v;
				break;
			}
#endif // BC_ENABLED

			case BC_INST_BOOL_OR:
			case BC_INST_BOOL_AND:
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
				ip = bc_vec_top(&p->stack);
				func = bc_vec_item(&p->fns, ip->func);
				code = func->code.v;
				break;
			}

			case BC_INST_MAXIBASE:
			case BC_INST_MAXOBASE:
			case BC_INST_MAXSCALE:
			{
				BcBigDig dig = vm->maxes[inst - BC_INST_MAXIBASE];
				bc_program_pushBigDig(p, dig, BC_RESULT_TEMP);
				break;
			}

			case BC_INST_VAR:
			{
				s = bc_program_pushVar(p, code, &ip->idx, false, false);
				break;
			}

			case BC_INST_ARRAY_ELEM:
#if BC_ENABLED
			case BC_INST_ARRAY:
#endif // BC_ENABLED
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
				r.d.loc.loc = bc_program_index(code, &ip->idx);
				bc_vec_push(&p->results, &r);
				break;
			}

			case BC_INST_ONE:
#if BC_ENABLED
			case BC_INST_LAST:
#endif // BC_ENABLED
			{
				r.t = BC_RESULT_ONE + (inst - BC_INST_ONE);
				bc_vec_push(&p->results, &r);
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
				r.d.loc.loc = bc_program_index(code, &ip->idx);
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
			case BC_INST_ASSIGN:
			case BC_INST_ASSIGN_POWER_NO_VAL:
			case BC_INST_ASSIGN_MULTIPLY_NO_VAL:
			case BC_INST_ASSIGN_DIVIDE_NO_VAL:
			case BC_INST_ASSIGN_MODULUS_NO_VAL:
			case BC_INST_ASSIGN_PLUS_NO_VAL:
			case BC_INST_ASSIGN_MINUS_NO_VAL:
#if BC_ENABLE_EXTRA_MATH
			case BC_INST_ASSIGN_PLACES_NO_VAL:
			case BC_INST_ASSIGN_LSHIFT_NO_VAL:
			case BC_INST_ASSIGN_RSHIFT_NO_VAL:
#endif // BC_ENABLE_EXTRA_MATH
#endif // BC_ENABLED
			case BC_INST_ASSIGN_NO_VAL:
			{
				s = bc_program_assign(p, inst);
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

#if DC_ENABLED
			case BC_INST_POP_EXEC:
			{
				assert(BC_PROG_STACK(&p->stack, 2));
				bc_vec_pop(&p->stack);
				bc_vec_pop(&p->tail_calls);
				ip = bc_vec_top(&p->stack);
				func = bc_vec_item(&p->fns, ip->func);
				code = func->code.v;
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
				s = bc_program_execStr(p, code, &ip->idx, cond, func->code.len);
				ip = bc_vec_top(&p->stack);
				func = bc_vec_item(&p->fns, ip->func);
				code = func->code.v;
				break;
			}

			case BC_INST_PRINT_STACK:
			{
				s = bc_program_printStack(p);
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
				ip = bc_vec_top(&p->stack);
				func = bc_vec_item(&p->fns, ip->func);
				code = func->code.v;
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
				idx = bc_program_index(code, &ip->idx);
				s = bc_program_copyToVar(p, idx, BC_TYPE_VAR, true);
				break;
			}

			case BC_INST_QUIT:
			case BC_INST_NQUIT:
			{
				s = bc_program_nquit(p, inst);
				ip = bc_vec_top(&p->stack);
				func = bc_vec_item(&p->fns, ip->func);
				code = func->code.v;
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
	}

	if (BC_ERR(s && s != BC_STATUS_QUIT) || BC_SIG) s = bc_program_reset(p, s);

	return s;
}

#if BC_DEBUG_CODE
#if BC_ENABLED && DC_ENABLED
BcStatus bc_program_printStackDebug(BcProgram *p) {

	BcStatus s;

	printf("-------------- Stack ----------\n");
	s = bc_program_printStack(p);
	printf("-------------- Stack End ------\n");

	return s;
}

static void bc_program_printIndex(const char *restrict code,
                                  size_t *restrict bgn)
{
	uchar byte, i, bytes = (uchar) code[(*bgn)++];
	ulong val = 0;

	for (byte = 1, i = 0; byte && i < bytes; ++i) {
		byte = (uchar) code[(*bgn)++];
		if (byte) val |= ((ulong) byte) << (CHAR_BIT * i);
	}

	bc_vm_printf(" (%lu) ", val);
}

static void bc_program_printStr(const BcProgram *p, const char *restrict code,
                         size_t *restrict bgn)
{
	size_t idx = bc_program_index(code, bgn);
	char *s;

	s = bc_program_str(p, idx);

	bc_vm_printf(" (\"%s\") ", s);
}

void bc_program_printInst(const BcProgram *p, const char *restrict code,
                          size_t *restrict bgn)
{
	uchar inst = (uchar) code[(*bgn)++];

	bc_vm_printf("Inst: %s [%u]; ", bc_inst_names[inst], inst);

	if (inst == BC_INST_VAR || inst == BC_INST_ARRAY_ELEM ||
	    inst == BC_INST_ARRAY)
	{
		bc_program_printIndex(code, bgn);
	}
	else if (inst == BC_INST_STR) bc_program_printStr(p, code, bgn);
	else if (inst == BC_INST_NUM) {
		size_t idx = bc_program_index(code, bgn);
		BcConst *c = bc_program_const(p, idx);
		bc_vm_printf("(%s)", c->val);
	}
	else if (inst == BC_INST_CALL ||
	         (inst > BC_INST_STR && inst <= BC_INST_JUMP_ZERO))
	{
		bc_program_printIndex(code, bgn);
		if (inst == BC_INST_CALL) bc_program_printIndex(code, bgn);
	}

	bc_vm_putchar('\n');
}

void bc_program_code(const BcProgram* p) {

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
