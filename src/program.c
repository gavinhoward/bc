/*
 * *****************************************************************************
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2018-2021 Gavin D. Howard and contributors.
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

#include <setjmp.h>

#include <signal.h>

#include <time.h>

#include <read.h>
#include <parse.h>
#include <program.h>
#include <vm.h>

/**
 * Quickly sets the const and strs vector pointers in the program. This is a
 * convenience function.
 * @param p  The program.
 * @param f  The new function.
 */
static inline void bc_program_setVecs(BcProgram *p, BcFunc *f) {
	p->consts = &f->consts;
	p->strs = &f->strs;
}

/**
 * Does a type check for something that expects a number.
 * @param r  The result that will be checked.
 * @param n  The result's number.
 */
static inline void bc_program_type_num(BcResult *r, BcNum *n) {

#if BC_ENABLED

	// This should have already been taken care of.
	assert(r->t != BC_RESULT_VOID);

#endif // BC_ENABLED

	if (BC_ERR(!BC_PROG_NUM(r, n))) bc_err(BC_ERR_EXEC_TYPE);
}

#if BC_ENABLED

/**
 * Does a type check.
 * @param r  The result to check.
 * @param t  The type that the result should be.
 */
static void bc_program_type_match(BcResult *r, BcType t) {
	if (BC_ERR((r->t != BC_RESULT_ARRAY) != (!t))) bc_err(BC_ERR_EXEC_TYPE);
}
#endif // BC_ENABLED

/**
 * Pulls an index out of a bytecode vector and updates the index into the vector
 * to point to the spot after the index. For more details on bytecode indices,
 * see the development manual (manuals/development.md#bytecode-indices).
 * @param code  The bytecode vector.
 * @param bgn   An in/out parameter; the index into the vector that will be
 *              updated.
 * @return      The index at @a bgn in the bytecode vector.
 */
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

/**
 * Returns a string from a result and its number.
 * @param p  The program.
 * @param n  The number tied to the result.
 * @return   The string corresponding to the result and number.
 */
static char* bc_program_string(BcProgram *p, const BcNum *n) {
	BcFunc *f = bc_vec_item(&p->fns, n->rdx);
	return *((char**) bc_vec_item(&f->strs, n->scale));
}

#if BC_ENABLED

/**
 * Prepares the globals for a function call. This is only called when global
 * stacks are on because it pushes a copy of the current globals onto each of
 * their respective stacks.
 * @param p  The program.
 */
static void bc_program_prepGlobals(BcProgram *p) {

	size_t i;

	for (i = 0; i < BC_PROG_GLOBALS_LEN; ++i)
		bc_vec_push(p->globals_v + i, p->globals + i);

#if BC_ENABLE_EXTRA_MATH
	bc_rand_push(&p->rng);
#endif // BC_ENABLE_EXTRA_MATH
}

/**
 * Pops globals stacks on returning from a function, or in the case of reset,
 * pops all but one item on each global stack.
 * @param p      The program.
 * @param reset  True if all but one item on each stack should be popped, false
 *               otherwise.
 */
static void bc_program_popGlobals(BcProgram *p, bool reset) {

	size_t i;

	for (i = 0; i < BC_PROG_GLOBALS_LEN; ++i) {
		BcVec *v = p->globals_v + i;
		bc_vec_npop(v, reset ? v->len - 1 : 1);
		p->globals[i] = BC_PROG_GLOBAL(v);
	}

#if BC_ENABLE_EXTRA_MATH
	bc_rand_pop(&p->rng, reset);
#endif // BC_ENABLE_EXTRA_MATH
}

/**
 * Derefeneces an array reference and returns a pointer to the real array.
 * @param p    The program.
 * @param vec  The reference vector.
 * @return     A pointer to the desired array.
 */
static BcVec* bc_program_dereference(const BcProgram *p, BcVec *vec) {

	BcVec *v;
	size_t vidx, nidx, i = 0;

	// We want to be sure we have a reference vector.
	assert(vec->size == sizeof(uchar));

	// Get the index of the vector in arrs, then the index of the original
	// referenced vector.
	vidx = bc_program_index(vec->v, &i);
	nidx = bc_program_index(vec->v, &i);

	v = bc_vec_item(bc_vec_item(&p->arrs, vidx), nidx);

	// We want to be sure we do *not* have a reference vector.
	assert(v->size != sizeof(uchar));

	return v;
}
#endif // BC_ENABLED

/**
 * Creates a BcNum from a BcBigDig and pushes onto the results stack. This is a
 * convenience function.
 * @param p     The program.
 * @param dig   The BcBigDig to push onto the results stack.
 * @param type  The type that the pushed result should be.
 */
static void bc_program_pushBigdig(BcProgram *p, BcBigDig dig, BcResultType type)
{
	BcResult res;

	res.t = type;

	BC_SIG_LOCK;

	bc_num_createFromBigdig(&res.d.n, dig);
	bc_vec_push(&p->results, &res);

	BC_SIG_UNLOCK;
}

size_t bc_program_addString(BcProgram *p, const char *str, size_t fidx) {

	BcFunc *f;
	char **str_ptr;
	BcVec *slabs;

	BC_SIG_ASSERT_LOCKED;

	// Push an empty string on the proper vector.
	f = bc_vec_item(&p->fns, fidx);
	str_ptr = bc_vec_pushEmpty(&f->strs);

	// Figure out which slab vector to use.
	slabs = fidx == BC_PROG_MAIN || fidx == BC_PROG_READ ?
	        &vm.main_slabs : &vm.other_slabs;

	*str_ptr = bc_slabvec_strdup(slabs, str);

	return f->strs.len - 1;
}

size_t bc_program_search(BcProgram *p, const char *id, bool var) {

	BcVec *v, *map;
	size_t i;

	// Grab the right vector and map.
	v = var ? &p->vars : &p->arrs;
	map = var ? &p->var_map : &p->arr_map;

	BC_SIG_LOCK;

	// We do an insert because the variable might not exist yet. This is because
	// the parser calls this function. If the insert succeeds, we create a stack
	// for the variable/array. But regardless, bc_map_insert() gives us the
	// index of the item in i.
	if (bc_map_insert(map, id, v->len, &i)) {
		BcVec *temp = bc_vec_pushEmpty(v);
		bc_array_init(temp, var);
	}

	BC_SIG_UNLOCK;

	return ((BcId*) bc_vec_item(map, i))->idx;
}

/**
 * Returns the correct variable or array stack for the type.
 * @param p     The program.
 * @param idx   The index of the variable or array in the variable or array
 *              vector.
 * @param type  The type of vector to return.
 * @return      A pointer to the variable or array stack.
 */
static inline BcVec* bc_program_vec(const BcProgram *p, size_t idx, BcType type)
{
	const BcVec *v = (type == BC_TYPE_VAR) ? &p->vars : &p->arrs;
	return bc_vec_item(v, idx);
}

/**
 * Returns a pointer to the BcNum corresponding to the result. There is one
 * case, however, where this returns a pointer to a BcVec: if the type of the
 * result is array. In that case, the pointer is casted to a pointer to BcNum,
 * but is never used. The function that calls this expecting an array casts the
 * pointer back. This function is called a lot and needs to be as fast as
 * possible.
 * @param p  The program.
 * @param r  The result whose number will be returned.
 * @return   The BcNum corresponding to the result.
 */
static BcNum* bc_program_num(BcProgram *p, BcResult *r) {

	BcNum *n = NULL;

	switch (r->t) {

		case BC_RESULT_STR:
		case BC_RESULT_TEMP:
		case BC_RESULT_IBASE:
		case BC_RESULT_SCALE:
		case BC_RESULT_OBASE:
#if BC_ENABLE_EXTRA_MATH
		case BC_RESULT_SEED:
#endif // BC_ENABLE_EXTRA_MATH
		{
			n = &r->d.n;
			break;
		}

		case BC_RESULT_VAR:
		case BC_RESULT_ARRAY:
		case BC_RESULT_ARRAY_ELEM:
		{
			BcVec *v;
			BcType type = (r->t == BC_RESULT_VAR) ? BC_TYPE_VAR : BC_TYPE_ARRAY;

			// Get the correct variable or array vector.
			v = bc_program_vec(p, r->d.loc.loc, type);

			// Surprisingly enough, the hard case is *not* returning an array;
			// it's returning an array element. This is because we have to dig
			// deeper to get *to* the element. That's what the code inside this
			// if statement does.
			if (r->t == BC_RESULT_ARRAY_ELEM) {

				size_t idx = r->d.loc.idx;

				v = bc_vec_top(v);

#if BC_ENABLED
				// If this is true, we have a reference vector, so dereference
				// it. The reason we don't need to worry about it for returning
				// a straight array is because we only care about references
				// when we access elements of an array that is a reference. That
				// is this code, so in essence, this line takes care of arrays
				// as well.
				if (v->size == sizeof(uchar)) v = bc_program_dereference(p, v);
#endif // BC_ENABLED

				// We want to be sure we got a valid array of numbers.
				assert(v->size == sizeof(BcNum));

				// The bc spec says that if an element is accessed that does not
				// exist, it should be preinitialized to 0. Well, if we access
				// an element *way* out there, we have to preinitialize all
				// elements between the current last element and the actual
				// accessed element.
				if (v->len <= idx) {
					BC_SIG_LOCK;
					bc_array_expand(v, bc_vm_growSize(idx, 1));
					BC_SIG_UNLOCK;
				}

				n = bc_vec_item(v, idx);
			}
			// This is either a number (for a var) or an array (for an array).
			// Because bc_vec_top() returns a void*, we don't need to cast.
			else n = bc_vec_top(v);

			break;
		}

		case BC_RESULT_ZERO:
		{
			n = &vm.zero;
			break;
		}

		case BC_RESULT_ONE:
		{
			n = &vm.one;
			break;
		}

#if BC_ENABLED
		// We should never get here; this is taken care of earlier because a
		// result is expected.
		case BC_RESULT_VOID:
#ifndef NDEBUG
		{
			abort();
		}
#endif // NDEBUG
		// Fallthrough
		case BC_RESULT_LAST:
		{
			n = &p->last;
			break;
		}
#endif // BC_ENABLED
	}

	return n;
}

/**
 * Prepares an operand for use.
 * @param p    The program.
 * @param r    An out parameter; this is set to the pointer to the result that
 *             we care about.
 * @param n    An out parameter; this is set to the pointer to the number that
 *             we care about.
 * @param idx  The index of the result from the top of the results stack.
 */
static void bc_program_operand(BcProgram *p, BcResult **r,
                               BcNum **n, size_t idx)
{
	*r = bc_vec_item_rev(&p->results, idx);

#if BC_ENABLED
	if (BC_ERR((*r)->t == BC_RESULT_VOID)) bc_err(BC_ERR_EXEC_VOID_VAL);
#endif // BC_ENABLED

	*n = bc_program_num(p, *r);
}

/**
 * Prepares the operands of a binary operator.
 * @param p    The program.
 * @param l    An out parameter; this is set to the pointer to the result for
 *             the left operand.
 * @param ln   An out parameter; this is set to the pointer to the number for
 *             the left operand.
 * @param r    An out parameter; this is set to the pointer to the result for
 *             the right operand.
 * @param rn   An out parameter; this is set to the pointer to the number for
 *             the right operand.
 * @param idx  The starting index where the operands are in the results stack,
 *             starting from the top.
 */
static void bc_program_binPrep(BcProgram *p, BcResult **l, BcNum **ln,
                               BcResult **r, BcNum **rn, size_t idx)
{
	BcResultType lt;

	assert(p != NULL && l != NULL && ln != NULL && r != NULL && rn != NULL);

#ifndef BC_PROG_NO_STACK_CHECK
	// Check the stack for dc.
	if (BC_IS_DC) {
		if (BC_ERR(!BC_PROG_STACK(&p->results, idx + 2)))
			bc_err(BC_ERR_EXEC_STACK);
	}
#endif // BC_PROG_NO_STACK_CHECK

	assert(BC_PROG_STACK(&p->results, idx + 2));

	// Get the operands.
	bc_program_operand(p, l, ln, idx + 1);
	bc_program_operand(p, r, rn, idx);

	lt = (*l)->t;

#if BC_ENABLED
	// bc_program_operand() checked these for us.
	assert(lt != BC_RESULT_VOID && (*r)->t != BC_RESULT_VOID);
#endif // BC_ENABLED

	// We run this again under these conditions in case any vector has been
	// reallocated out from under the BcNums or arrays we had. In other words,
	// this is to fix pointer invalidation.
	if (lt == (*r)->t && (lt == BC_RESULT_VAR || lt == BC_RESULT_ARRAY_ELEM))
		*ln = bc_program_num(p, *l);

	if (BC_ERR(lt == BC_RESULT_STR)) bc_err(BC_ERR_EXEC_TYPE);
}

/**
 * Prepares the operands of a binary operator and type checks them. This is
 * separate from bc_program_binPrep() because some places want this, others want
 * bc_program_binPrep().
 * @param p    The program.
 * @param l    An out parameter; this is set to the pointer to the result for
 *             the left operand.
 * @param ln   An out parameter; this is set to the pointer to the number for
 *             the left operand.
 * @param r    An out parameter; this is set to the pointer to the result for
 *             the right operand.
 * @param rn   An out parameter; this is set to the pointer to the number for
 *             the right operand.
 * @param idx  The starting index where the operands are in the results stack,
 *             starting from the top.
 */
static void bc_program_binOpPrep(BcProgram *p, BcResult **l, BcNum **ln,
                                 BcResult **r, BcNum **rn, size_t idx)
{
	bc_program_binPrep(p, l, ln, r, rn, idx);
	bc_program_type_num(*l, *ln);
	bc_program_type_num(*r, *rn);
}

/**
 * Prepares the operands of an assignment operator.
 * @param p   The program.
 * @param l   An out parameter; this is set to the pointer to the result for the
 *            left operand.
 * @param ln  An out parameter; this is set to the pointer to the number for the
 *            left operand.
 * @param r   An out parameter; this is set to the pointer to the result for the
 *            right operand.
 * @param rn  An out parameter; this is set to the pointer to the number for the
 *            right operand.
 */
static void bc_program_assignPrep(BcProgram *p, BcResult **l, BcNum **ln,
                                  BcResult **r, BcNum **rn)
{
	BcResultType lt, min;

	// This is the min non-allowable result type. dc allows strings.
	min = BC_RESULT_TEMP - ((unsigned int) (BC_IS_BC));

	// Prepare the operands.
	bc_program_binPrep(p, l, ln, r, rn, 0);

	lt = (*l)->t;

	// Typecheck the left.
	if (BC_ERR(lt >= min && lt <= BC_RESULT_ONE)) bc_err(BC_ERR_EXEC_TYPE);

	// Strings can be assigned to variables. We are already good if we are
	// assigning a string.
	bool good = ((*r)->t == BC_RESULT_STR && lt <= BC_RESULT_ARRAY_ELEM);

	assert(BC_PROG_STR(*rn) || (*r)->t != BC_RESULT_STR);

	// If not, type check for a number.
	if (!good) bc_program_type_num(*r, *rn);
}

/**
 * Prepares a single operand and type checks it. This is separate from
 * bc_program_operand() because different places want one or the other.
 * @param p    The program.
 * @param r    An out parameter; this is set to the pointer to the result that
 *             we care about.
 * @param n    An out parameter; this is set to the pointer to the number that
 *             we care about.
 * @param idx  The index of the result from the top of the results stack.
 */
static void bc_program_prep(BcProgram *p, BcResult **r, BcNum **n, size_t idx) {

	assert(p != NULL && r != NULL && n != NULL);

#ifndef BC_PROG_NO_STACK_CHECK
	// Check the stack for dc.
	if (BC_IS_DC) {
		if (BC_ERR(!BC_PROG_STACK(&p->results, idx + 1)))
			bc_err(BC_ERR_EXEC_STACK);
	}
#endif // BC_PROG_NO_STACK_CHECK

	assert(BC_PROG_STACK(&p->results, idx + 1));

	bc_program_operand(p, r, n, idx);

	// dc does not allow strings in this case.
	bc_program_type_num(*r, *n);
}

/**
 * Prepares and returns a clean result for the result of an operation.
 * @param p  The program.
 * @return   A clean result.
 */
static BcResult* bc_program_prepResult(BcProgram *p) {

	BcResult *res = bc_vec_pushEmpty(&p->results);

	bc_result_clear(res);

	return res;
}

/**
 * Prepares a constant for use. This parses the constant into a number and then
 * pushes that number onto the results stack.
 * @param p     The program.
 * @param code  The bytecode vector that we will pull the index of the constant
 *              from.
 * @param bgn   An in/out parameter; marks the start of the index in the
 *              bytecode vector and will be updated to point to after the index.
 */
static void bc_program_const(BcProgram *p, const char *code, size_t *bgn) {

	// I lied. I actually push the result first. I can do this because the
	// result will be popped on error. I also get the constant itself.
	BcResult *r = bc_program_prepResult(p);
	BcConst *c = bc_vec_item(p->consts, bc_program_index(code, bgn));
	BcBigDig base = BC_PROG_IBASE(p);

	// Only reparse if the base changed.
	if (c->base != base) {

		// Allocate if we haven't yet.
		if (c->num.num == NULL) {
			BC_SIG_LOCK;
			bc_num_init(&c->num, BC_NUM_RDX(strlen(c->val)));
			BC_SIG_UNLOCK;
		}

		// bc_num_parse() should only do operations that cannot fail.
		bc_num_parse(&c->num, c->val, base);

		c->base = base;
	}

	BC_SIG_LOCK;

	bc_num_createCopy(&r->d.n, &c->num);

	BC_SIG_UNLOCK;
}

/**
 * Executes a binary operator operation.
 * @param p     The program.
 * @param inst  The instruction corresponding to the binary operator to execute.
 */
static void bc_program_op(BcProgram *p, uchar inst) {

	BcResult *opd1, *opd2, *res;
	BcNum *n1, *n2;
	size_t idx = inst - BC_INST_POWER;

	res = bc_program_prepResult(p);

	bc_program_binOpPrep(p, &opd1, &n1, &opd2, &n2, 1);

	BC_SIG_LOCK;

	// Initialize the number with enough space, using the correct
	// BcNumBinaryOpReq function. This looks weird because it is executing an
	// item of an array. Rest assured that item is a function.
	bc_num_init(&res->d.n, bc_program_opReqs[idx](n1, n2, BC_PROG_SCALE(p)));

	BC_SIG_UNLOCK;

	assert(BC_NUM_RDX_VALID(n1));
	assert(BC_NUM_RDX_VALID(n2));

	// Run the operation. This also executes an item of an array.
	bc_program_ops[idx](n1, n2, &res->d.n, BC_PROG_SCALE(p));

	bc_program_retire(p, 1, 2);
}

/**
 * Executes a read() or ? command.
 * @param p  The program.
 */
static void bc_program_read(BcProgram *p) {

	BcStatus s;
	BcInstPtr ip;
	size_t i;
	const char* file;
	bool is_stdin;
	BcFunc *f = bc_vec_item(&p->fns, BC_PROG_READ);

	// If we are already executing a read, that is an error. So look for a read
	// and barf.
	for (i = 0; i < p->stack.len; ++i) {
		BcInstPtr *ip_ptr = bc_vec_item(&p->stack, i);
		if (ip_ptr->func == BC_PROG_READ) bc_err(BC_ERR_EXEC_REC_READ);
	}

	BC_SIG_LOCK;

	// Save the filename because we are going to overwrite it.
	file = vm.file;
	is_stdin = vm.is_stdin;

	// It is a parse error if there needs to be more than one line, so we unset
	// this to tell the lexer to not request more. We set it back later.
	vm.is_stdin = false;

	if (!BC_PARSE_IS_INITED(&vm.read_prs, p)) {

		// We need to parse, but we don't want to use the existing parser
		// because it has state it needs to keep. (It could have a partial parse
		// state.) So we create a new parser. This parser is in the BcVm struct
		// so that it is not local, which means that a longjmp() could change
		// it.
		bc_parse_init(&vm.read_prs, p, BC_PROG_READ);

		// We need a separate input buffer; that's why it is also in the BcVm
		// struct.
		bc_vec_init(&vm.read_buf, sizeof(char), BC_DTOR_NONE);
	}
	// This needs to be updated because the parser could have been used
	// somewhere else
	else bc_parse_updateFunc(&vm.read_prs, BC_PROG_READ);

	BC_SETJMP_LOCKED(exec_err);

	BC_SIG_UNLOCK;

	// Set up the lexer and the read function.
	bc_lex_file(&vm.read_prs.l, bc_program_stdin_name);
	bc_vec_popAll(&f->code);

	// Read a line.
	if (!BC_R) s = bc_read_line(&vm.read_buf, "");
	else s = bc_read_line(&vm.read_buf, BC_IS_BC ? "read> " : "?> ");

	// We should *not* have run into EOF.
	if (s == BC_STATUS_EOF) bc_err(BC_ERR_EXEC_READ_EXPR);

	// Parse *one* expression.
	bc_parse_text(&vm.read_prs, vm.read_buf.v, false);
	vm.expr(&vm.read_prs, BC_PARSE_NOREAD | BC_PARSE_NEEDVAL);

	// We *must* have a valid expression. A semicolon cannot end an expression,
	// although EOF can.
	if (BC_ERR(vm.read_prs.l.t != BC_LEX_NLINE &&
	           vm.read_prs.l.t != BC_LEX_EOF))
	{
		bc_err(BC_ERR_EXEC_READ_EXPR);
	}

#if BC_ENABLED
	// Push on the globals stack if necessary.
	if (BC_G) bc_program_prepGlobals(p);
#endif // BC_ENABLED

	// Set up a new BcInstPtr.
	ip.func = BC_PROG_READ;
	ip.idx = 0;
	ip.len = p->results.len;

	// Update this pointer, just in case.
	f = bc_vec_item(&p->fns, BC_PROG_READ);

	// We want a return instruction to simplify things.
	bc_vec_pushByte(&f->code, vm.read_ret);
	bc_vec_push(&p->stack, &ip);

#if DC_ENABLED
	// We need a new tail call entry for dc.
	if (BC_IS_DC) {
		size_t temp = 0;
		bc_vec_push(&p->tail_calls, &temp);
	}
#endif // DC_ENABLED

exec_err:
	BC_SIG_MAYLOCK;
	vm.is_stdin = is_stdin;
	vm.file = file;
	BC_LONGJMP_CONT;
}

#if BC_ENABLE_EXTRA_MATH

/**
 * Execute a rand().
 * @param p  The program.
 */
static void bc_program_rand(BcProgram *p) {

	BcRand rand = bc_rand_int(&p->rng);

	bc_program_pushBigdig(p, (BcBigDig) rand, BC_RESULT_TEMP);

#ifndef NDEBUG
	// This is just to ensure that the generated number is correct. I also use
	// braces because I declare every local at the top of the scope.
	{
		BcResult *r = bc_vec_top(&p->results);
		assert(BC_NUM_RDX_VALID_NP(r->d.n));
	}
#endif // NDEBUG
}
#endif // BC_ENABLE_EXTRA_MATH

/**
 * Prints a series of characters, without escapes.
 * @param str  The string (series of characters).
 */
static void bc_program_printChars(const char *str) {

	const char *nl;
	size_t len = vm.nchars + strlen(str);

	bc_file_puts(&vm.fout, bc_flush_save, str);

	// We need to update the number of characters, so we find the last newline
	// and set the characters accordingly.
	nl = strrchr(str, '\n');

	if (nl != NULL) len = strlen(nl + 1);

	vm.nchars = len > UINT16_MAX ? UINT16_MAX : (uint16_t) len;
}

/**
 * Prints a string with escapes.
 * @param str  The string.
 */
static void bc_program_printString(const char *restrict str) {

	size_t i, len = strlen(str);

#if DC_ENABLED
	// This is to ensure a nul byte is printed for dc's stream operation.
	if (!len && BC_IS_DC) {
		bc_vm_putchar('\0', bc_flush_save);
		return;
	}
#endif // DC_ENABLED

	// Loop over the characters, processing escapes and printing the rest.
	for (i = 0; i < len; ++i) {

		int c = str[i];

		// If we have an escape...
		if (c == '\\' && i != len - 1) {

			const char *ptr;

			// Get the escape character and its companion.
			c = str[++i];
			ptr = strchr(bc_program_esc_chars, c);

			// If we have a companion character...
			if (ptr != NULL) {

				// We need to specially handle a newline.
				if (c == 'n') vm.nchars = UINT16_MAX;

				// Grab the actual character.
				c = bc_program_esc_seqs[(size_t) (ptr - bc_program_esc_chars)];
			}
			else {
				// Just print the backslash if there is no companion character.
				// The following character will be printed later after the outer
				// if statement.
				bc_vm_putchar('\\', bc_flush_save);
			}
		}

		bc_vm_putchar(c, bc_flush_save);
	}
}

/**
 * Executes a print. This function handles all printing except streaming.
 * @param p     The program.
 * @param inst  The instruction for the type of print we are doing.
 * @param idx   The index of the result that we are printing.
 */
static void bc_program_print(BcProgram *p, uchar inst, size_t idx) {

	BcResult *r;
	char *str;
	BcNum *n;
	bool pop = (inst != BC_INST_PRINT);

	assert(p != NULL);

#ifndef BC_PROG_NO_STACK_CHECK
	if (BC_IS_DC) {
		if (BC_ERR(!BC_PROG_STACK(&p->results, idx + 1)))
			bc_err(BC_ERR_EXEC_STACK);
	}
#endif // BC_PROG_NO_STACK_CHECK

	assert(BC_PROG_STACK(&p->results, idx + 1));

	r = bc_vec_item_rev(&p->results, idx);

#if BC_ENABLED
	// If we have a void value, that's not necessarily an error. It is if pop is
	// true because that means that we are executing a print statement, but
	// attempting to do a print on a lone void value is allowed because that's
	// exactly how we want void values used.
	if (r->t == BC_RESULT_VOID) {
		if (BC_ERR(pop)) bc_err(BC_ERR_EXEC_VOID_VAL);
		bc_vec_pop(&p->results);
		return;
	}
#endif // BC_ENABLED

	n = bc_program_num(p, r);

	// If we have a number...
	if (BC_PROG_NUM(r, n)) {

#if BC_ENABLED
		assert(inst != BC_INST_PRINT_STR);
#endif // BC_ENABLED

		// Print the number.
		bc_num_print(n, BC_PROG_OBASE(p), !pop);

#if BC_ENABLED
		// Need to store the number in last.
		if (BC_IS_BC) bc_num_copy(&p->last, n);
#endif // BC_ENABLED
	}
	else {

		// We want to flush any stuff in the stdout buffer first.
		bc_file_flush(&vm.fout, bc_flush_save);
		str = bc_program_string(p, n);

#if BC_ENABLED
		if (inst == BC_INST_PRINT_STR) bc_program_printChars(str);
		else
#endif // BC_ENABLED
		{
			bc_program_printString(str);

			// Need to print a newline only in this case.
			if (inst == BC_INST_PRINT)
				bc_vm_putchar('\n', bc_flush_err);
		}
	}

	// bc always pops.
	if (BC_IS_BC || pop) bc_vec_pop(&p->results);
}

void bc_program_negate(BcResult *r, BcNum *n) {
	bc_num_copy(&r->d.n, n);
	if (BC_NUM_NONZERO(&r->d.n)) BC_NUM_NEG_TGL_NP(r->d.n);
}

void bc_program_not(BcResult *r, BcNum *n) {
	if (!bc_num_cmpZero(n)) bc_num_one(&r->d.n);
}

#if BC_ENABLE_EXTRA_MATH
void bc_program_trunc(BcResult *r, BcNum *n) {
	bc_num_copy(&r->d.n, n);
	bc_num_truncate(&r->d.n, n->scale);
}
#endif // BC_ENABLE_EXTRA_MATH

/**
 * Runs a unary operation.
 * @param p     The program.
 * @param inst  The unary operation.
 */
static void bc_program_unary(BcProgram *p, uchar inst) {

	BcResult *res, *ptr;
	BcNum *num;

	res = bc_program_prepResult(p);

	bc_program_prep(p, &ptr, &num, 1);

	BC_SIG_LOCK;

	bc_num_init(&res->d.n, num->len);

	BC_SIG_UNLOCK;

	// This calls a function that is in an array.
	bc_program_unarys[inst - BC_INST_NEG](res, num);
	bc_program_retire(p, 1, 1);
}

/**
 * Executes a logical operator.
 * @param p     The program.
 * @param inst  The operator.
 */
static void bc_program_logical(BcProgram *p, uchar inst) {

	BcResult *opd1, *opd2, *res;
	BcNum *n1, *n2;
	bool cond = 0;
	ssize_t cmp;

	res = bc_program_prepResult(p);

	// All logical operators (except boolean not, which is taken care of by
	// bc_program_unary()), are binary operators.
	bc_program_binOpPrep(p, &opd1, &n1, &opd2, &n2, 1);

	// Boolean and and or are not short circuiting. This is why; they can be
	// implemented much easier this way.
	if (inst == BC_INST_BOOL_AND)
		cond = (bc_num_cmpZero(n1) && bc_num_cmpZero(n2));
	else if (inst == BC_INST_BOOL_OR)
		cond = (bc_num_cmpZero(n1) || bc_num_cmpZero(n2));
	else {

		// We have a relational operator, so do a comparison.
		cmp = bc_num_cmp(n1, n2);

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
				// There is a bug if we get here.
				abort();
			}
#endif // NDEBUG
		}
	}

	BC_SIG_LOCK;

	bc_num_init(&res->d.n, BC_NUM_DEF_SIZE);

	BC_SIG_UNLOCK;

	if (cond) bc_num_one(&res->d.n);

	bc_program_retire(p, 1, 2);
}

/**
 * Assigns a string to a variable.
 * @param p     The program.
 * @param num   The location of the string as a BcNum.
 * @param v     The stack for the variable.
 * @param push  Whether to push the string or not. To push means to move the
 *              string from the results stack and push it onto the variable
 *              stack.
 */
static void bc_program_assignStr(BcProgram *p, BcNum *num, BcVec *v, bool push)
{
	BcNum *n;

	assert(BC_PROG_STACK(&p->results, 1 + !push));
	assert(num != NULL && num->num == NULL && num->cap == 0);

	// If we are not pushing onto the variable stack, we need to replace the
	// top of the variable stack.
	if (!push) bc_vec_pop(v);

	bc_vec_npop(&p->results, 1 + !push);

	n = bc_vec_pushEmpty(v);

	// We can just copy because the num should not have allocated anything.
	memcpy(n, num, sizeof(BcNum));
}

/**
 * Copies a value to a variable. This is used for storing in dc as well as to
 * set function parameters to arguments in bc.
 * @param p     The program.
 * @param idx   The index of the variable or array to copy to.
 * @param t     The type to copy to. This could be a variable or an array.
 * @param last  Whether to grab the last item on the variable stack or not (for
 *              bc function parameters). This is important because if a new
 *              value has been pushed to the variable already, we need to grab
 *              the value pushed before. This happens when you have a parameter
 *              named something like "x", and a variable "x" is passed to
 *              another parameter.
 */
static void bc_program_copyToVar(BcProgram *p, size_t idx, BcType t, bool last)
{
	BcResult *ptr = NULL, r;
	BcVec *vec;
	BcNum *n = NULL;
	bool var = (t == BC_TYPE_VAR);

#if DC_ENABLED
	// Check the stack for dc.
	if (BC_IS_DC) {
		if (BC_ERR(!BC_PROG_STACK(&p->results, 1))) bc_err(BC_ERR_EXEC_STACK);
	}
#endif

	assert(BC_PROG_STACK(&p->results, 1));

	bc_program_operand(p, &ptr, &n, 0);

#if BC_ENABLED
	// Get the variable for a bc function call.
	if (BC_IS_BC)
	{
		// Type match the result.
		bc_program_type_match(ptr, t);

		// Get the variable or array, taking care to get the real item. We take
		// care of last with arrays later.
		if (!last && var)
			n = bc_vec_item_rev(bc_program_vec(p, ptr->d.loc.loc, t), 1);
	}
#endif // BC_ENABLED

	vec = bc_program_vec(p, idx, t);

	// We can shortcut in dc if it's assigning a string by using
	// bc_program_assignStr().
	if (ptr->t == BC_RESULT_STR) {

		assert(BC_PROG_STR(n));

		if (BC_ERR(!var)) bc_err(BC_ERR_EXEC_TYPE);

		bc_program_assignStr(p, n, vec, true);

		return;
	}

	BC_SIG_LOCK;

	// Just create and copy for a normal variable.
	if (var) {
		if (BC_PROG_STR(n)) memcpy(&r.d.n, n, sizeof(BcNum));
		else bc_num_createCopy(&r.d.n, n);
	}
	else {

		// If we get here, we are handling an array. This is one place we need
		// to cast the number from bc_program_num() to a vector.
		BcVec *v = (BcVec*) n, *rv = &r.d.v;

#if BC_ENABLED

		if (BC_IS_BC) {

			BcVec *parent;
			bool ref, ref_size;

			// We need to figure out if the parameter is a reference or not and
			// construct the reference vector, if necessary. So this gets the
			// parent stack for the array.
			parent = bc_program_vec(p, ptr->d.loc.loc, t);
			assert(parent != NULL);

			// This takes care of last for arrays. Mostly.
			if (!last) v = bc_vec_item_rev(parent, !last);
			assert(v != NULL);

			// True if we are using a reference.
			ref = (v->size == sizeof(BcNum) && t == BC_TYPE_REF);

			// True if we already have a reference vector. This is slightly
			// (okay, a lot; it just doesn't look that way) different from
			// above. The above means that we need to construct a reference
			// vector, whereas this means that we have one and we might have to
			// *dereference* it.
			ref_size = (v->size == sizeof(uchar));

			// If we *should* have a reference.
			if (ref || (ref_size && t == BC_TYPE_REF)) {

				// Create a new reference vector.
				bc_vec_init(rv, sizeof(uchar), BC_DTOR_NONE);

				// If this is true, then we need to construct a reference.
				if (ref) {

					assert(parent->len >= (size_t) (!last + 1));

					// Make sure the pointer was not invalidated.
					vec = bc_program_vec(p, idx, t);

					// Push the indices onto the reference vector. This takes
					// care of last; it ensures the reference goes to the right
					// place.
					bc_vec_pushIndex(rv, ptr->d.loc.loc);
					bc_vec_pushIndex(rv, parent->len - !last - 1);
				}
				// If we get here, we are copying a ref to a ref. Just push a
				// copy of all of the bytes.
				else bc_vec_npush(rv, v->len * sizeof(uchar), v->v);

				// Push the reference vector onto the array stack and pop the
				// source.
				bc_vec_push(vec, &r.d);
				bc_vec_pop(&p->results);

				// We need to return early to avoid executing code that we must
				// not touch.
				BC_SIG_UNLOCK;
				return;
			}
			// If we get here, we have a reference, but we need an array, so
			// dereference the array.
			else if (ref_size && t != BC_TYPE_REF)
				v = bc_program_dereference(p, v);
		}
#endif // BC_ENABLED

		// If we get here, we need to copy the array because in bc, all
		// arguments are passed by value. Yes, this is expensive.
		bc_array_init(rv, true);
		bc_array_copy(rv, v);
	}

	// Push the vector onto the array stack and pop the source.
	bc_vec_push(vec, &r.d);
	bc_vec_pop(&p->results);

	BC_SIG_UNLOCK;
}

/**
 * Executes an assignment operator.
 * @param p     The program.
 * @param inst  The assignment operator to execute.
 */
static void bc_program_assign(BcProgram *p, uchar inst) {

	// The local use_val is true when the assigned value needs to be copied.
	BcResult *left, *right, res;
	BcNum *l, *r;
	bool ob, sc, use_val = BC_INST_USE_VAL(inst);

	bc_program_assignPrep(p, &left, &l, &right, &r);

	// Assigning to a string should be impossible simply because of the parse.
	assert(left->t != BC_RESULT_STR);

	// If we are assigning a string...
	if (right->t == BC_RESULT_STR) {

		assert(BC_PROG_STR(r));

#if BC_ENABLED
		if (inst != BC_INST_ASSIGN && inst != BC_INST_ASSIGN_NO_VAL)
			bc_err(BC_ERR_EXEC_TYPE);
#endif // BC_ENABLED

		// If we are assigning to an array element...
		if (left->t == BC_RESULT_ARRAY_ELEM) {

			BC_SIG_LOCK;

			// We need to free the number and clear it.
			bc_num_free(l);

			memcpy(l, r, sizeof(BcNum));

			// Now we can pop the results.
			bc_vec_npop(&p->results, 2);

			BC_SIG_UNLOCK;
		}
		else {

			// If we get here, we are assigning to a variable, which we can use
			// bc_program_assignStr() for.
			BcVec *v = bc_program_vec(p, left->d.loc.loc, BC_TYPE_VAR);
			bc_program_assignStr(p, r, v, false);
		}

#if BC_ENABLED

		// If this is true, the value is going to be used again, so we want to
		// push a temporary with the string.
		if (inst == BC_INST_ASSIGN) {
			res.t = BC_RESULT_STR;
			memcpy(&res.d.n, r, sizeof(BcNum));
			bc_vec_push(&p->results, &res);
		}

#endif // BC_ENABLED

		// By using bc_program_assignStr(), we short-circuited this, so return.
		return;
	}

	// If we have a normal assignment operator, not a math one...
	if (BC_INST_IS_ASSIGN(inst)) {

		// Assigning to a variable that has a string here is fine because there
		// is no math done on it.

		// BC_RESULT_TEMP, BC_RESULT_IBASE, BC_RESULT_OBASE, BC_RESULT_SCALE,
		// and BC_RESULT_SEED all have temporary copies. Because that's the
		// case, we can free the left and just move the value over. We set the
		// type of right to BC_RESULT_ZERO in order to prevent it from being
		// freed. We also don't have to worry about BC_RESULT_STR because it's
		// take care of above.
		if (right->t == BC_RESULT_TEMP || right->t >= BC_RESULT_IBASE) {

			BC_SIG_LOCK;

			bc_num_free(l);
			memcpy(l, r, sizeof(BcNum));
			right->t = BC_RESULT_ZERO;

			BC_SIG_UNLOCK;
		}
		// Copy over.
		else bc_num_copy(l, r);
	}
#if BC_ENABLED
	else {

		// If we get here, we are doing a math assignment (+=, -=, etc.). So
		// we need to prepare for a binary operator.
		BcBigDig scale = BC_PROG_SCALE(p);

		// At this point, the left side could still be a string because it could
		// be a variable that has the string. If that's the case, we have a type
		// error.
		if (BC_PROG_STR(l)) bc_err(BC_ERR_EXEC_TYPE);

		// Get the right type of assignment operator, whether val is used or
		// NO_VAL for performance.
		if (!use_val)
			inst -= (BC_INST_ASSIGN_POWER_NO_VAL - BC_INST_ASSIGN_POWER);

		assert(BC_NUM_RDX_VALID(l));
		assert(BC_NUM_RDX_VALID(r));

		// Run the actual operation. We do not need worry about reallocating l
		// because bc_num_binary() does that behind the scenes for us.
		bc_program_ops[inst - BC_INST_ASSIGN_POWER](l, r, l, scale);
	}
#endif // BC_ENABLED

	ob = (left->t == BC_RESULT_OBASE);
	sc = (left->t == BC_RESULT_SCALE);

	// The globals need special handling, especially the non-seed ones. The
	// first part of the if statement handles them.
	if (ob || sc || left->t == BC_RESULT_IBASE) {

		BcVec *v;
		BcBigDig *ptr, *ptr_t, val, max, min;

		// Get the actual value.
		val = bc_num_bigdig(l);

		// Scale needs handling separate from ibase and obase.
		if (sc) {

			// Set the min and max.
			min = 0;
			max = vm.maxes[BC_PROG_GLOBALS_SCALE];

			// Get a pointer to the stack and to the current value.
			v = p->globals_v + BC_PROG_GLOBALS_SCALE;
			ptr_t = p->globals + BC_PROG_GLOBALS_SCALE;
		}
		else {

			// Set the min and max.
			min = BC_NUM_MIN_BASE;
			if (BC_ENABLE_EXTRA_MATH && ob && (BC_IS_DC || !BC_IS_POSIX))
				min = 0;
			max = vm.maxes[ob + BC_PROG_GLOBALS_IBASE];

			// Get a pointer to the stack and to the current value.
			v = p->globals_v + BC_PROG_GLOBALS_IBASE + ob;
			ptr_t = p->globals + BC_PROG_GLOBALS_IBASE + ob;
		}

		// Check for error.
		if (BC_ERR(val > max || val < min)) {

			// This grabs the right error.
			BcErr e = left->t - BC_RESULT_IBASE + BC_ERR_EXEC_IBASE;

			bc_verr(e, min, max);
		}

		// Set the top of the stack and the actual global value.
		ptr = bc_vec_top(v);
		*ptr = val;
		*ptr_t = val;
	}
#if BC_ENABLE_EXTRA_MATH
	// To assign to steed, let bc_num_rng() do its magic.
	else if (left->t == BC_RESULT_SEED) bc_num_rng(l, &p->rng);
#endif // BC_ENABLE_EXTRA_MATH

	BC_SIG_LOCK;

	// If we needed to use the value, then we need to copy it. Otherwise, we can
	// pop indiscriminately. Oh, and the copy should be a BC_RESULT_TEMP.
	if (use_val) {
		bc_num_createCopy(&res.d.n, l);
		res.t = BC_RESULT_TEMP;
		bc_vec_npop(&p->results, 2);
		bc_vec_push(&p->results, &res);
	}
	else bc_vec_npop(&p->results, 2);

	BC_SIG_UNLOCK;
}

/**
 * Pushes a variable's value onto the results stack.
 * @param p     The program.
 * @param code  The bytecode vector to pull the variable's index out of.
 * @param bgn   An in/out parameter; the start of the index in the bytecode
 *              vector, and will be updated to point after the index on return.
 * @param pop   True if the variable's value should be popped off its stack.
 *              This is only used in dc.
 * @param copy  True if the variable's value should be copied to the results
 *              stack. This is only used in dc.
 */
static void bc_program_pushVar(BcProgram *p, const char *restrict code,
                               size_t *restrict bgn, bool pop, bool copy)
{
	BcResult r;
	size_t idx = bc_program_index(code, bgn);

	// Set the result appropriately.
	r.t = BC_RESULT_VAR;
	r.d.loc.loc = idx;

#if DC_ENABLED
	// If this condition is true, then we have the hard case, where we have to
	// adjust dc registers.
	if (BC_IS_DC && (pop || copy)) {

		// Get the stack for the variable and the number at the top.
		BcVec *v = bc_program_vec(p, idx, BC_TYPE_VAR);
		BcNum *num = bc_vec_top(v);

		// Ensure there are enough elements on the stack.
		if (BC_ERR(!BC_PROG_STACK(v, 2 - copy))) {
			const char *name = bc_map_name(&p->var_map, idx);
			bc_verr(BC_ERR_EXEC_STACK_REGISTER, name);
		}

		assert(BC_PROG_STACK(v, 2 - copy));

		// If the top of the stack is actually a number...
		if (!BC_PROG_STR(num)) {

			BC_SIG_LOCK;

			// Create a copy to go onto the results stack as appropriate.
			r.t = BC_RESULT_TEMP;
			bc_num_createCopy(&r.d.n, num);

			// If we are not actually copying, we need to do a replace, so pop.
			if (!copy) bc_vec_pop(v);

			bc_vec_push(&p->results, &r);

			BC_SIG_UNLOCK;

			return;
		}
		else {
			// Set the string result. We can just memcpy because all of the
			// fields in the num should be cleared.
			memcpy(&r.d.n, num, sizeof(BcNum));
			r.t = BC_RESULT_STR;
		}

		// If we are not actually copying, we need to do a replace, so pop.
		if (!copy) bc_vec_pop(v);
	}
#endif // DC_ENABLED

	bc_vec_push(&p->results, &r);
}

/**
 * Pushes an array or an array element onto the results stack.
 * @param p     The program.
 * @param code  The bytecode vector to pull the variable's index out of.
 * @param bgn   An in/out parameter; the start of the index in the bytecode
 *              vector, and will be updated to point after the index on return.
 * @param inst  The instruction; whether to push an array or an array element.
 */
static void bc_program_pushArray(BcProgram *p, const char *restrict code,
                                 size_t *restrict bgn, uchar inst)
{
	BcResult r, *operand;
	BcNum *num;
	BcBigDig temp;

	// Get the index of the array.
	r.d.loc.loc = bc_program_index(code, bgn);

	// Doing an array is easy; just set the result type and finish.
	if (inst == BC_INST_ARRAY) {
		r.t = BC_RESULT_ARRAY;
		bc_vec_push(&p->results, &r);
		return;
	}

	// Grab the top element of the results stack for the array index.
	bc_program_prep(p, &operand, &num, 0);
	temp = bc_num_bigdig(num);

	// Set the result.
	r.t = BC_RESULT_ARRAY_ELEM;
	r.d.loc.idx = (size_t) temp;

	BC_SIG_LOCK;

	// Pop the index and push the element.
	bc_vec_pop(&p->results);
	bc_vec_push(&p->results, &r);

	BC_SIG_UNLOCK;
}

#if BC_ENABLED

/**
 * Executes an increment or decrement operator. This only handles postfix
 * inc/dec because the parser translates prefix inc/dec into an assignment where
 * the value is used.
 * @param p     The program.
 * @param inst  The instruction; whether to do an increment or decrement.
 */
static void bc_program_incdec(BcProgram *p, uchar inst) {

	BcResult *ptr, res, copy;
	BcNum *num;
	uchar inst2;

	bc_program_prep(p, &ptr, &num, 0);

	BC_SIG_LOCK;

	// We need a copy from *before* the operation.
	copy.t = BC_RESULT_TEMP;
	bc_num_createCopy(&copy.d.n, num);

	BC_SETJMP_LOCKED(exit);

	BC_SIG_UNLOCK;

	// Create the proper assignment.
	res.t = BC_RESULT_ONE;
	inst2 = BC_INST_ASSIGN_PLUS_NO_VAL + (inst & 0x01);

	bc_vec_push(&p->results, &res);
	bc_program_assign(p, inst2);

	BC_SIG_LOCK;

	bc_vec_push(&p->results, &copy);

	BC_UNSETJMP;

	BC_SIG_UNLOCK;

	// No need to free the copy here because we pushed it onto the stack.
	return;

exit:
	BC_SIG_MAYLOCK;
	bc_num_free(&copy.d.n);
	BC_LONGJMP_CONT;
}

/**
 * Executes a function call for bc.
 * @param p     The program.
 * @param code  The bytecode vector to pull the number of arguments and the
 *              function index out of.
 * @param bgn   An in/out parameter; the start of the indices in the bytecode
 *              vector, and will be updated to point after the indices on
 *              return.
 */
static void bc_program_call(BcProgram *p, const char *restrict code,
                            size_t *restrict bgn)
{
	BcInstPtr ip;
	size_t i, nargs;
	BcFunc *f;
	BcVec *v;
	BcAuto *a;
	BcResult *arg;

	// Pull the number of arguments out of the bytecode vector.
	nargs = bc_program_index(code, bgn);

	// Set up instruction pointer.
	ip.idx = 0;
	ip.func = bc_program_index(code, bgn);
	f = bc_vec_item(&p->fns, ip.func);

	// Error checking.
	if (BC_ERR(!f->code.len)) bc_verr(BC_ERR_EXEC_UNDEF_FUNC, f->name);
	if (BC_ERR(nargs != f->nparams))
		bc_verr(BC_ERR_EXEC_PARAMS, f->nparams, nargs);

	// Set the length of the results stack. We discount the argument, of course.
	ip.len = p->results.len - nargs;

	assert(BC_PROG_STACK(&p->results, nargs));

	// Prepare the globals' stacks.
	if (BC_G) bc_program_prepGlobals(p);

	// Push the arguments onto the stacks of their respective parameters.
	for (i = 0; i < nargs; ++i) {

		size_t j;
		bool last = true;

		arg = bc_vec_top(&p->results);
		if (BC_ERR(arg->t == BC_RESULT_VOID)) bc_err(BC_ERR_EXEC_VOID_VAL);

		// Get the corresponding parameter.
		a = bc_vec_item(&f->autos, nargs - 1 - i);

		// If I have already pushed to a var, I need to make sure I
		// get the previous version, not the already pushed one. This condition
		// must be true for that to even be possible.
		if (arg->t == BC_RESULT_VAR || arg->t == BC_RESULT_ARRAY) {

			// Loop through all of the previous parameters.
			for (j = 0; j < i && last; ++j) {

				BcAuto *aptr = bc_vec_item(&f->autos, nargs - 1 - j);

				// This condition is true if there is a previous parameter with
				// the same name *and* type because variables and arrays do not
				// interfere with each other.
				last = (arg->d.loc.loc != aptr->idx ||
				        (!aptr->type) != (arg->t == BC_RESULT_VAR));
			}
		}

		// Actually push the value onto the parameter's stack.
		bc_program_copyToVar(p, a->idx, a->type, last);
	}

	BC_SIG_LOCK;

	// Push zeroes onto the stacks of the auto variables.
	for (; i < f->autos.len; ++i) {

		// Get the auto and its stack.
		a = bc_vec_item(&f->autos, i);
		v = bc_program_vec(p, a->idx, a->type);

		// If a variable, just push a 0; otherwise, push an array.
		if (a->type == BC_TYPE_VAR) {
			BcNum *n = bc_vec_pushEmpty(v);
			bc_num_init(n, BC_NUM_DEF_SIZE);
		}
		else {

			BcVec *v2;

			assert(a->type == BC_TYPE_ARRAY);

			v2 = bc_vec_pushEmpty(v);
			bc_array_init(v2, true);
		}
	}

	// Push the instruction pointer onto the execution stack.
	bc_vec_push(&p->stack, &ip);

	BC_SIG_UNLOCK;
}

/**
 * Executes a return instruction.
 * @param p     The program.
 * @param inst  The return instruction. bc can return void, and we need to know
 *              if it is.
 */
static void bc_program_return(BcProgram *p, uchar inst) {

	BcResult *res;
	BcFunc *f;
	BcInstPtr *ip;
	size_t i, nresults;

	// Get the instruction pointer.
	ip = bc_vec_top(&p->stack);

	// Get the difference between the actual number of results and the number of
	// results the caller expects.
	nresults = p->results.len - ip->len;

	// If this isn't true, there was a missing call somewhere.
	assert(BC_PROG_STACK(&p->stack, 2));

	// If this isn't true, the parser screwed by giving us no value when we
	// expected one, or giving us a value when we expected none.
	assert(BC_PROG_STACK(&p->results, ip->len + (inst == BC_INST_RET)));

	// Get the function we are returning from.
	f = bc_vec_item(&p->fns, ip->func);

	res = bc_program_prepResult(p);

	// If we are returning normally...
	if (inst == BC_INST_RET) {

		BcNum *num;
		BcResult *operand;

		// Prepare and copy the return value.
		bc_program_operand(p, &operand, &num, 1);

		if (BC_PROG_STR(num)) {

			// We need to set this because otherwise, it will be a
			// BC_RESULT_TEMP, and BC_RESULT_TEMP needs an actual number to make
			// it easier to do type checking.
			res->t = BC_RESULT_STR;

			memcpy(&res->d.n, num, sizeof(BcNum));
		}
		else {

			BC_SIG_LOCK;

			bc_num_createCopy(&res->d.n, num);
		}
	}
	// Void is easy; set the result.
	else if (inst == BC_INST_RET_VOID) res->t = BC_RESULT_VOID;
	else {

		BC_SIG_LOCK;

		// If we get here, the instruction is for returning a zero, so do that.
		bc_num_init(&res->d.n, BC_NUM_DEF_SIZE);
	}

	BC_SIG_MAYUNLOCK;

	// We need to pop items off of the stacks of arguments and autos as well.
	for (i = 0; i < f->autos.len; ++i) {

		BcAuto *a = bc_vec_item(&f->autos, i);
		BcVec *v = bc_program_vec(p, a->idx, a->type);

		bc_vec_pop(v);
	}

	// When we retire, pop all of the unused results.
	bc_program_retire(p, 1, nresults);

	// Pop the globals, if necessary.
	if (BC_G) bc_program_popGlobals(p, false);

	// Pop the stack. This is what causes the function to actually "return."
	bc_vec_pop(&p->stack);
}
#endif // BC_ENABLED

/**
 * Executes a builtin function.
 * @param p     The program.
 * @param inst  The builtin to execute.
 */
static void bc_program_builtin(BcProgram *p, uchar inst) {

	BcResult *opd, *res;
	BcNum *num;
	bool len = (inst == BC_INST_LENGTH);

	// Ensure we have a valid builtin.
#if BC_ENABLE_EXTRA_MATH
	assert(inst >= BC_INST_LENGTH && inst <= BC_INST_IRAND);
#else // BC_ENABLE_EXTRA_MATH
	assert(inst >= BC_INST_LENGTH && inst <= BC_INST_ABS);
#endif // BC_ENABLE_EXTRA_MATH

#ifndef BC_PROG_NO_STACK_CHECK
	// Check stack for dc.
	if (BC_IS_DC && BC_ERR(!BC_PROG_STACK(&p->results, 1)))
		bc_err(BC_ERR_EXEC_STACK);
#endif // BC_PROG_NO_STACK_CHECK

	assert(BC_PROG_STACK(&p->results, 1));

	res = bc_program_prepResult(p);

	bc_program_operand(p, &opd, &num, 1);

	assert(num != NULL);

	// We need to ensure that strings and arrays aren't passed to most builtins.
	// The scale function can take strings in dc.
	if (!len && (inst != BC_INST_SCALE_FUNC || BC_IS_BC))
		bc_program_type_num(opd, num);

	// Square root is easy.
	if (inst == BC_INST_SQRT) bc_num_sqrt(num, &res->d.n, BC_PROG_SCALE(p));

	// Absolute value is easy.
	else if (inst == BC_INST_ABS) {

		BC_SIG_LOCK;

		bc_num_createCopy(&res->d.n, num);

		BC_SIG_UNLOCK;

		BC_NUM_NEG_CLR_NP(res->d.n);
	}
#if BC_ENABLE_EXTRA_MATH
	// irand() is easy.
	else if (inst == BC_INST_IRAND) {

		BC_SIG_LOCK;

		bc_num_init(&res->d.n, num->len - BC_NUM_RDX_VAL(num));

		BC_SIG_UNLOCK;

		bc_num_irand(num, &res->d.n, &p->rng);
	}
#endif // BC_ENABLE_EXTRA_MATH

	// Everything else is...not easy.
	else {

		BcBigDig val = 0;

		// Well, scale() is easy, but length() is not.
		if (len) {

			// If we are bc and we have an array...
			if (opd->t == BC_RESULT_ARRAY) {

				// Yes, this is one place where we need to cast the number from
				// bc_program_num() to a vector.
				BcVec *v = (BcVec*) num;

#if BC_ENABLED
				// Dereference the array, if necessary.
				if (BC_IS_BC && v->size == sizeof(uchar))
					v = bc_program_dereference(p, v);
#endif // BC_ENABLED

				assert(v->size == sizeof(BcNum));

				val = (BcBigDig) v->len;
			}
			else {

				// If the item is a string...
				if (!BC_PROG_NUM(opd, num)) {

					char *str;

					// Get the string, then get the length.
					str = bc_program_string(p, num);
					val = (BcBigDig) strlen(str);
				}
				else
				{
					// Calculate the length of the number.
					val = (BcBigDig) bc_num_len(num);
				}
			}
		}
		// Like I said; scale() is actually easy. It just also needs the integer
		// conversion that length() does.
		else if (BC_IS_BC || BC_PROG_NUM(opd, num))
			val = (BcBigDig) bc_num_scale(num);

		BC_SIG_LOCK;

		// Create the result.
		bc_num_createFromBigdig(&res->d.n, val);

		BC_SIG_UNLOCK;
	}

	bc_program_retire(p, 1, 1);
}

/**
 * Executes a divmod.
 * @param p  The program.
 */
static void bc_program_divmod(BcProgram *p) {

	BcResult *opd1, *opd2, *res, *res2;
	BcNum *n1, *n2;
	size_t req;

	// We grow first to avoid pointer invalidation.
	bc_vec_grow(&p->results, 2);

	// We don't need to update the pointer because
	// the capacity is enough due to the line above.
	res2 = bc_program_prepResult(p);
	res = bc_program_prepResult(p);

	// Prepare the operands.
	bc_program_binOpPrep(p, &opd1, &n1, &opd2, &n2, 2);

	req = bc_num_mulReq(n1, n2, BC_PROG_SCALE(p));

	BC_SIG_LOCK;

	// Initialize the results.
	bc_num_init(&res->d.n, req);
	bc_num_init(&res2->d.n, req);

	BC_SIG_UNLOCK;

	// Execute.
	bc_num_divmod(n1, n2, &res2->d.n, &res->d.n, BC_PROG_SCALE(p));

	bc_program_retire(p, 2, 2);
}

/**
 * Executes modular exponentiation.
 * @param p  The program.
 */
static void bc_program_modexp(BcProgram *p) {

	BcResult *r1, *r2, *r3, *res;
	BcNum *n1, *n2, *n3;

#if DC_ENABLED

	// Check the stack.
	if (BC_IS_DC && BC_ERR(!BC_PROG_STACK(&p->results, 3)))
		bc_err(BC_ERR_EXEC_STACK);

#endif // DC_ENABLED

	assert(BC_PROG_STACK(&p->results, 3));

	res = bc_program_prepResult(p);

	// Get the first operand and typecheck.
	bc_program_operand(p, &r1, &n1, 3);
	bc_program_type_num(r1, n1);

	// Get the last two operands.
	bc_program_binOpPrep(p, &r2, &n2, &r3, &n3, 1);

	// Make sure that the values have their pointers updated, if necessary.
	// Only array elements are possible because this is dc.
	if (r1->t == BC_RESULT_ARRAY_ELEM && (r1->t == r2->t || r1->t == r3->t))
		n1 = bc_program_num(p, r1);

	BC_SIG_LOCK;

	bc_num_init(&res->d.n, n3->len);

	BC_SIG_UNLOCK;

	bc_num_modexp(n1, n2, n3, &res->d.n);

	bc_program_retire(p, 1, 3);
}

/**
 * Asciifies a number for dc. This is a helper for bc_program_asciify().
 * @param p  The program.
 * @param n  The number to asciify.
 */
static uchar bc_program_asciifyNum(BcProgram *p, BcNum *n) {

	BcNum num;
	BcBigDig val;

#ifndef NDEBUG
	// This is entirely to satisfy a useless scan-build error.
	val = 0;
#endif // NDEBUG

	bc_num_clear(&num);

	BC_SETJMP(num_err);

	BC_SIG_LOCK;

	bc_num_createCopy(&num, n);

	BC_SIG_UNLOCK;

	// We want to clear the scale and sign for easy mod later.
	bc_num_truncate(&num, num.scale);
	BC_NUM_NEG_CLR_NP(num);

	// This is guaranteed to not have a divide by 0
	// because strmb is equal to 256.
	bc_num_mod(&num, &p->strmb, &num, 0);

	// This is also guaranteed to not error because num is in the range
	// [0, UCHAR_MAX], which is definitely in range for a BcBigDig. And
	// it is not negative.
	val = bc_num_bigdig2(&num);

num_err:
	BC_SIG_MAYLOCK;
	bc_num_free(&num);
	BC_LONGJMP_CONT;
	return (uchar) val;
}

/**
 * Executes the "asciify" command in dc.
 * @param p     The program.
 * @param fidx  The index of the current function.
 */
static void bc_program_asciify(BcProgram *p, size_t fidx) {

	BcResult *r, res;
	BcNum *n;
	char str[2], *str2;
	uchar c;
	size_t idx;

	// Check the stack.
	if (BC_ERR(!BC_PROG_STACK(&p->results, 1))) bc_err(BC_ERR_EXEC_STACK);

	assert(BC_PROG_STACK(&p->results, 1));

	// Get the top of the results stack.
	bc_program_operand(p, &r, &n, 0);

	assert(n != NULL);

	// Asciify.
	if (BC_PROG_NUM(r, n)) c = bc_program_asciifyNum(p, n);
	else {

		// Get the string itself, then the first character.
		str2 = bc_program_string(p, n);
		c = (uchar) str2[0];
	}

	// Fill the resulting string.
	str[0] = (char) c;
	str[1] = '\0';

	// Add the string to the data structures.
	BC_SIG_LOCK;
	idx = bc_program_addString(p, str, fidx);
	BC_SIG_UNLOCK;

	// Set the result
	res.t = BC_RESULT_STR;
	bc_num_clear(&res.d.n);
	res.d.n.rdx = fidx;
	res.d.n.scale = idx;

	// Pop and push.
	bc_vec_pop(&p->results);
	bc_vec_push(&p->results, &res);
}

/**
 * Streams a number or a string to stdout.
 * @param p  The program.
 */
static void bc_program_printStream(BcProgram *p) {

	BcResult *r;
	BcNum *n;

	// Check the stack.
	if (BC_ERR(!BC_PROG_STACK(&p->results, 1))) bc_err(BC_ERR_EXEC_STACK);

	assert(BC_PROG_STACK(&p->results, 1));

	// Get the top of the results stack.
	bc_program_operand(p, &r, &n, 0);

	assert(n != NULL);

	// Stream appropriately.
	if (BC_PROG_NUM(r, n)) bc_num_stream(n);
	else bc_program_printChars(bc_program_string(p, n));

	// Pop the operand.
	bc_vec_pop(&p->results);
}

#if DC_ENABLED

/**
 * Gets the length of a register in dc and pushes it onto the results stack.
 * @param p     The program.
 * @param code  The bytecode vector to pull the register's index out of.
 * @param bgn   An in/out parameter; the start of the index in the bytecode
 *              vector, and will be updated to point after the index on return.
 */
static void bc_program_regStackLen(BcProgram *p, const char *restrict code,
                                   size_t *restrict bgn)
{
	size_t idx = bc_program_index(code, bgn);
	BcVec *v = bc_program_vec(p, idx, BC_TYPE_VAR);

	bc_program_pushBigdig(p, (BcBigDig) v->len, BC_RESULT_TEMP);
}

/**
 * Pushes the length of the results stack onto the results stack.
 * @param p  The program.
 */
static void bc_program_stackLen(BcProgram *p) {
	bc_program_pushBigdig(p, (BcBigDig) p->results.len, BC_RESULT_TEMP);
}

/**
 * Pops a certain number of elements off the execution stack.
 * @param p     The program.
 * @param inst  The instruction to tell us how many. There is one to pop up to
 *              2, and one to pop the amount equal to the number at the top of
 *              the results stack.
 */
static void bc_program_nquit(BcProgram *p, uchar inst) {

	BcResult *opnd;
	BcNum *num;
	BcBigDig val;
	size_t i;

	// Ensure that the tail calls stack is correct.
	assert(p->stack.len == p->tail_calls.len);

	// Get the number of executions to pop.
	if (inst == BC_INST_QUIT) val = 2;
	else {

		bc_program_prep(p, &opnd, &num, 0);
		val = bc_num_bigdig(num);

		bc_vec_pop(&p->results);
	}

	// Loop over the tail call stack and adjust the quit value appropriately.
	for (i = 0; val && i < p->tail_calls.len; ++i) {

		// Get the number of tail calls for this one.
		size_t calls = *((size_t*) bc_vec_item_rev(&p->tail_calls, i)) + 1;

		// Adjust the value.
		if (calls >= val) val = 0;
		else val -= (BcBigDig) calls;
	}

	// If we don't have enough executions, just quit.
	if (i == p->stack.len) {
		vm.status = BC_STATUS_QUIT;
		BC_JMP;
	}
	else {
		// We can always pop the last item we reached on the tail call stack
		// because these are for tail calls. That means that any executions that
		// we would not have quit in that position on the stack would have quit
		// anyway.
		bc_vec_npop(&p->stack, i);
		bc_vec_npop(&p->tail_calls, i);
	}
}

/**
 * Pushes the depth of the execution stack onto the stack.
 * @param p  The program.
 */
static void bc_program_execStackLen(BcProgram *p) {

	size_t i, amt, len = p->tail_calls.len;

	amt = len;

	for (i = 0; i < len; ++i)
		amt += *((size_t*) bc_vec_item(&p->tail_calls, i));

	bc_program_pushBigdig(p, (BcBigDig) amt, BC_RESULT_TEMP);
}

/**
 *
 * @param p     The program.
 * @param code  The bytecode vector to pull the register's index out of.
 * @param bgn   An in/out parameter; the start of the index in the bytecode
 *              vector, and will be updated to point after the index on return.
 * @param cond  True if the execution is conditional, false otherwise.
 * @param len   The number of bytes in the bytecode vector.
 */
static void bc_program_execStr(BcProgram *p, const char *restrict code,
                               size_t *restrict bgn, bool cond, size_t len)
{
	BcResult *r;
	char *str;
	BcFunc *f;
	BcInstPtr ip;
	size_t fidx;
	BcNum *n;

	assert(p->stack.len == p->tail_calls.len);

	// Check the stack.
	if (BC_ERR(!BC_PROG_STACK(&p->results, 1))) bc_err(BC_ERR_EXEC_STACK);

	assert(BC_PROG_STACK(&p->results, 1));

	// Get the operand.
	bc_program_operand(p, &r, &n, 0);

	// If execution is conditional...
	if (cond) {

		bool exec;
		size_t idx, then_idx, else_idx;

		// Get the index of the "then" var and "else" var.
		then_idx = bc_program_index(code, bgn);
		else_idx = bc_program_index(code, bgn);

		// Figure out if we should execute.
		exec = (r->d.n.len != 0);

		idx = exec ? then_idx : else_idx;

		BC_SIG_LOCK;
		BC_SETJMP_LOCKED(exit);

		// If we are supposed to execute, execute. If else_idx == SIZE_MAX, that
		// means there was no else clause, so if execute is false and else does
		// not exist, we don't execute. The goto skips all of the setup for the
		// execution.
		if (exec || (else_idx != SIZE_MAX))
			n = bc_vec_top(bc_program_vec(p, idx, BC_TYPE_VAR));
		else goto exit;

		if (BC_ERR(!BC_PROG_STR(n))) bc_err(BC_ERR_EXEC_TYPE);

		BC_UNSETJMP;
		BC_SIG_UNLOCK;
	}
	else {

		// In non-conditional situations, only the top of stack can be executed,
		// and in those cases, variables are not allowed to be "on the stack";
		// they are only put on the stack to be assigned to.
		assert(r->t != BC_RESULT_VAR);

		if (r->t != BC_RESULT_STR) return;
	}

	assert(BC_PROG_STR(n));

	// Get the string.
	str = bc_program_string(p, n);

	// Get the function index and function.
	BC_SIG_LOCK;
	fidx = bc_program_insertFunc(p, str);
	BC_SIG_UNLOCK;
	f = bc_vec_item(&p->fns, fidx);

	// If the function has not been parsed yet...
	if (!f->code.len) {

		BC_SIG_LOCK;

		if (!BC_PARSE_IS_INITED(&vm.read_prs, p)) {

			bc_parse_init(&vm.read_prs, p, fidx);

			// Initialize this too because bc_vm_shutdown() expects them to be
			// initialized togther.
			bc_vec_init(&vm.read_buf, sizeof(char), BC_DTOR_NONE);
		}
		// This needs to be updated because the parser could have been used
		// somewhere else
		else bc_parse_updateFunc(&vm.read_prs, fidx);

		bc_lex_file(&vm.read_prs.l, vm.file);

		BC_SETJMP_LOCKED(err);

		BC_SIG_UNLOCK;

		// Parse.
		bc_parse_text(&vm.read_prs, str, false);
		vm.expr(&vm.read_prs, BC_PARSE_NOCALL);

		BC_SIG_LOCK;

		BC_UNSETJMP;

		// We can just assert this here because
		// dc should parse everything until EOF.
		assert(vm.read_prs.l.t == BC_LEX_EOF);

		BC_SIG_UNLOCK;
	}

	// Set the instruction pointer.
	ip.idx = 0;
	ip.len = p->results.len;
	ip.func = fidx;

	// Pop the operand.
	bc_vec_pop(&p->results);

	// Tail call processing. This condition means that there is more on the
	// execution stack, and we are at the end of the bytecode vector, and the
	// last instruction is just a BC_INST_POP_EXEC, which would return.
	if (p->stack.len > 1 && *bgn == len - 1 && code[*bgn] == BC_INST_POP_EXEC) {

		size_t *call_ptr = bc_vec_top(&p->tail_calls);

		// Add one to the tail call.
		*call_ptr += 1;

		// Pop the execution stack before pushing the new instruction pointer
		// on.
		bc_vec_pop(&p->stack);
	}
	// If not a tail call, just push a new one.
	else bc_vec_push(&p->tail_calls, &ip.idx);

	// Push the new function onto the execution stack and return.
	bc_vec_push(&p->stack, &ip);

	return;

err:
	BC_SIG_MAYLOCK;

	f = bc_vec_item(&p->fns, fidx);

	// Make sure to erase the bytecode vector so dc knows it is not parsed.
	bc_vec_popAll(&f->code);

exit:
	bc_vec_pop(&p->results);
	BC_LONGJMP_CONT;
}

/**
 * Prints every item on the results stack, one per line.
 * @param p  The program.
 */
static void bc_program_printStack(BcProgram *p) {

	size_t idx;

	for (idx = 0; idx < p->results.len; ++idx)
		bc_program_print(p, BC_INST_PRINT, idx);
}
#endif // DC_ENABLED

/**
 * Pushes the value of a global onto the results stack.
 * @param p     The program.
 * @param inst  Which global to push, as an instruction.
 */
static void bc_program_pushGlobal(BcProgram *p, uchar inst) {

	BcResultType t;

	// Make sure the instruction is valid.
	assert(inst >= BC_INST_IBASE && inst <= BC_INST_SCALE);

	// Push the global.
	t = inst - BC_INST_IBASE + BC_RESULT_IBASE;
	bc_program_pushBigdig(p, p->globals[inst - BC_INST_IBASE], t);
}

#if BC_ENABLE_EXTRA_MATH

/**
 * Pushes the value of seed on the stack.
 * @param p  The program.
 */
static void bc_program_pushSeed(BcProgram *p) {

	BcResult *res;

	res = bc_program_prepResult(p);
	res->t = BC_RESULT_SEED;

	BC_SIG_LOCK;

	// We need 2*BC_RAND_NUM_SIZE because of the size of the state.
	bc_num_init(&res->d.n, 2 * BC_RAND_NUM_SIZE);

	BC_SIG_UNLOCK;

	bc_num_createFromRNG(&res->d.n, &p->rng);
}

#endif // BC_ENABLE_EXTRA_MATH

/**
 * Adds a function to the fns array. The function's ID must have already been
 * inserted into the map.
 * @param p       The program.
 * @param id_ptr  The ID of the function as inserted into the map.
 */
static void bc_program_addFunc(BcProgram *p, BcId *id_ptr) {

	BcInstPtr *ip;
	BcFunc *f;

	BC_SIG_ASSERT_LOCKED;

	// Push and init.
	f = bc_vec_pushEmpty(&p->fns);
	bc_func_init(f, id_ptr->name);

	// This is to make sure pointers are updated if the array was moved.
	if (p->stack.len) {
		ip = bc_vec_top(&p->stack);
		bc_program_setVecs(p, (BcFunc*) bc_vec_item(&p->fns, ip->func));
	}
}

size_t bc_program_insertFunc(BcProgram *p, const char *name) {

	BcId *id_ptr;
	bool new;
	size_t idx;

	BC_SIG_ASSERT_LOCKED;

	assert(p != NULL && name != NULL);

	// Insert into the map and get the resulting ID.
	new = bc_map_insert(&p->fn_map, name, p->fns.len, &idx);
	id_ptr = (BcId*) bc_vec_item(&p->fn_map, idx);
	idx = id_ptr->idx;

	// If the function is new...
	if (new) {

		// Add the function to the fns array.
		bc_program_addFunc(p, id_ptr);
	}
#if BC_ENABLED
	// bc has to reset the function because it's about to be redefined.
	else if (BC_IS_BC) {
		BcFunc *func = bc_vec_item(&p->fns, idx);
		bc_func_reset(func);
	}
#endif // BC_ENABLED

	return idx;
}

#ifndef NDEBUG
void bc_program_free(BcProgram *p) {

	size_t i;

	BC_SIG_ASSERT_LOCKED;

	assert(p != NULL);

	// Free the globals stacks.
	for (i = 0; i < BC_PROG_GLOBALS_LEN; ++i) bc_vec_free(p->globals_v + i);

	bc_vec_free(&p->fns);
	bc_vec_free(&p->fn_map);
	bc_vec_free(&p->vars);
	bc_vec_free(&p->var_map);
	bc_vec_free(&p->arrs);
	bc_vec_free(&p->arr_map);
	bc_vec_free(&p->results);
	bc_vec_free(&p->stack);

#if BC_ENABLED
	if (BC_IS_BC) bc_num_free(&p->last);
#endif // BC_ENABLED

#if BC_ENABLE_EXTRA_MATH
	bc_rand_free(&p->rng);
#endif // BC_ENABLE_EXTRA_MATH

#if DC_ENABLED
	if (BC_IS_DC) bc_vec_free(&p->tail_calls);
#endif // DC_ENABLED
}
#endif // NDEBUG

void bc_program_init(BcProgram *p) {

	BcInstPtr ip;
	size_t i;

	BC_SIG_ASSERT_LOCKED;

	assert(p != NULL);

	// We want this clear.
	memset(&ip, 0, sizeof(BcInstPtr));

	// Setup the globals stacks and the current values.
	for (i = 0; i < BC_PROG_GLOBALS_LEN; ++i) {

		BcBigDig val = i == BC_PROG_GLOBALS_SCALE ? 0 : BC_BASE;

		bc_vec_init(p->globals_v + i, sizeof(BcBigDig), BC_DTOR_NONE);
		bc_vec_push(p->globals_v + i, &val);

		p->globals[i] = val;
	}

#if DC_ENABLED
	// dc-only setup.
	if (BC_IS_DC) {

		bc_vec_init(&p->tail_calls, sizeof(size_t), BC_DTOR_NONE);

		// We want an item for the main function on the tail call stack.
		i = 0;
		bc_vec_push(&p->tail_calls, &i);
	}
#endif // DC_ENABLED

	bc_num_setup(&p->strmb, p->strmb_num, BC_NUM_BIGDIG_LOG10);
	bc_num_bigdig2num(&p->strmb, BC_NUM_STREAM_BASE);

#if BC_ENABLE_EXTRA_MATH
	// We need to initialize srand() just in case /dev/urandom and /dev/random
	// are not available.
	srand((unsigned int) time(NULL));
	bc_rand_init(&p->rng);
#endif // BC_ENABLE_EXTRA_MATH

#if BC_ENABLED
	if (BC_IS_BC) bc_num_init(&p->last, BC_NUM_DEF_SIZE);
#endif // BC_ENABLED

#ifndef NDEBUG
	bc_vec_init(&p->fns, sizeof(BcFunc), BC_DTOR_FUNC);
#else // NDEBUG
	bc_vec_init(&p->fns, sizeof(BcFunc), BC_DTOR_NONE);
#endif // NDEBUG
	bc_map_init(&p->fn_map);
	bc_program_insertFunc(p, bc_func_main);
	bc_program_insertFunc(p, bc_func_read);

	bc_vec_init(&p->vars, sizeof(BcVec), BC_DTOR_VEC);
	bc_map_init(&p->var_map);

	bc_vec_init(&p->arrs, sizeof(BcVec), BC_DTOR_VEC);
	bc_map_init(&p->arr_map);

	bc_vec_init(&p->results, sizeof(BcResult), BC_DTOR_RESULT);

	// Push the first instruction pointer onto the execution stack.
	bc_vec_init(&p->stack, sizeof(BcInstPtr), BC_DTOR_NONE);
	bc_vec_push(&p->stack, &ip);

	// Make sure the pointers are properly set up.
	bc_program_setVecs(p, (BcFunc*) bc_vec_item(&p->fns, BC_PROG_MAIN));

	assert(p->consts != NULL && p->strs != NULL);
}

void bc_program_reset(BcProgram *p) {

	BcFunc *f;
	BcInstPtr *ip;

	BC_SIG_ASSERT_LOCKED;

	// Pop all but the last execution and all results.
	bc_vec_npop(&p->stack, p->stack.len - 1);
	bc_vec_popAll(&p->results);

#if BC_ENABLED
	// Clear the globals' stacks.
	if (BC_G) bc_program_popGlobals(p, true);
#endif // BC_ENABLED

	// Clear the bytecode vector of the main function.
	f = bc_vec_item(&p->fns, BC_PROG_MAIN);
	bc_vec_npop(&f->code, f->code.len);

	// Reset the instruction pointer.
	ip = bc_vec_top(&p->stack);
	bc_program_setVecs(p, f);
	memset(ip, 0, sizeof(BcInstPtr));

	// Write the ready message for a signal, and clear the signal.
	if (vm.sig) {
		bc_file_write(&vm.fout, bc_flush_none, bc_program_ready_msg,
		              bc_program_ready_msg_len);
		bc_file_flush(&vm.fout, bc_flush_err);
		vm.sig = 0;
	}
}

void bc_program_exec(BcProgram *p) {

	size_t idx;
	BcResult r, *ptr;
	BcInstPtr *ip;
	BcFunc *func;
	char *code;
	bool cond = false;
	uchar inst;
#if BC_ENABLED
	BcNum *num;
#endif // BC_ENABLED
#if !BC_HAS_COMPUTED_GOTO
#ifndef NDEBUG
	size_t jmp_bufs_len;
#endif // NDEBUG
#endif // !BC_HAS_COMPUTED_GOTO

#if BC_HAS_COMPUTED_GOTO
	BC_PROG_LBLS;
	BC_PROG_LBLS_ASSERT;

	// BC_INST_INVALID is a marker for the end so that we don't have to have an
	// execution loop.
	func = (BcFunc*) bc_vec_item(&p->fns, BC_PROG_MAIN);
	bc_vec_pushByte(&func->code, BC_INST_INVALID);
#endif // BC_HAS_COMPUTED_GOTO

	ip = bc_vec_top(&p->stack);
	func = (BcFunc*) bc_vec_item(&p->fns, ip->func);
	code = func->code.v;

	// Ensure the pointers are correct.
	bc_program_setVecs(p, func);

#if !BC_HAS_COMPUTED_GOTO

#ifndef NDEBUG
	jmp_bufs_len = vm.jmp_bufs.len;
#endif // NDEBUG

	// This loop is the heart of the execution engine. It *is* the engine. For
	// computed goto, it is ignored.
	while (ip->idx < func->code.len)
#endif // !BC_HAS_COMPUTED_GOTO
	{

		BC_SIG_ASSERT_NOT_LOCKED;

#if BC_HAS_COMPUTED_GOTO

		BC_PROG_JUMP(inst, code, ip);

#else // BC_HAS_COMPUTED_GOTO

		// Get the next instruction and increment the index.
		inst = (uchar) code[(ip->idx)++];

#endif // BC_HAS_COMPUTED_GOTO

#if BC_DEBUG_CODE
		bc_file_printf(&vm.ferr, "inst: %s\n", bc_inst_names[inst]);
		bc_file_flush(&vm.ferr, bc_flush_none);
#endif // BC_DEBUG_CODE

#if !BC_HAS_COMPUTED_GOTO
		switch (inst)
#endif // !BC_HAS_COMPUTED_GOTO
		{

#if BC_ENABLED
			// This just sets up the condition for the unconditional jump below,
			// which checks the condition, if necessary.
			BC_PROG_LBL(BC_INST_JUMP_ZERO):
			{
				bc_program_prep(p, &ptr, &num, 0);

				cond = !bc_num_cmpZero(num);
				bc_vec_pop(&p->results);

				BC_PROG_DIRECT_JUMP(BC_INST_JUMP)
			}
			// Fallthrough.
			BC_PROG_FALLTHROUGH

			BC_PROG_LBL(BC_INST_JUMP):
			{
				idx = bc_program_index(code, &ip->idx);

				// If a jump is required...
				if (inst == BC_INST_JUMP || cond) {

					// Get the address to jump to.
					size_t *addr = bc_vec_item(&func->labels, idx);

					// If this fails, then the parser failed to set up the
					// labels correctly.
					assert(*addr != SIZE_MAX);

					// Set the new address.
					ip->idx = *addr;
				}

				BC_PROG_JUMP(inst, code, ip);
			}

			BC_PROG_LBL(BC_INST_CALL):
			{
				assert(BC_IS_BC);

				bc_program_call(p, code, &ip->idx);

				// Because we changed the execution stack and where we are
				// executing, we have to update all of this.
				ip = bc_vec_top(&p->stack);
				func = bc_vec_item(&p->fns, ip->func);
				code = func->code.v;
				bc_program_setVecs(p, func);

				BC_PROG_JUMP(inst, code, ip);
			}

			BC_PROG_LBL(BC_INST_INC):
			BC_PROG_LBL(BC_INST_DEC):
			{
				bc_program_incdec(p, inst);
				BC_PROG_JUMP(inst, code, ip);
			}

			BC_PROG_LBL(BC_INST_HALT):
			{
				vm.status = BC_STATUS_QUIT;

				// Just jump out. The jump series will take care of everything.
				BC_JMP;

				BC_PROG_JUMP(inst, code, ip);
			}

			BC_PROG_LBL(BC_INST_RET):
			BC_PROG_LBL(BC_INST_RET0):
			BC_PROG_LBL(BC_INST_RET_VOID):
			{
				bc_program_return(p, inst);

				// Because we changed the execution stack and where we are
				// executing, we have to update all of this.
				ip = bc_vec_top(&p->stack);
				func = bc_vec_item(&p->fns, ip->func);
				code = func->code.v;
				bc_program_setVecs(p, func);

				BC_PROG_JUMP(inst, code, ip);
			}
#endif // BC_ENABLED

			BC_PROG_LBL(BC_INST_BOOL_OR):
			BC_PROG_LBL(BC_INST_BOOL_AND):
			BC_PROG_LBL(BC_INST_REL_EQ):
			BC_PROG_LBL(BC_INST_REL_LE):
			BC_PROG_LBL(BC_INST_REL_GE):
			BC_PROG_LBL(BC_INST_REL_NE):
			BC_PROG_LBL(BC_INST_REL_LT):
			BC_PROG_LBL(BC_INST_REL_GT):
			{
				bc_program_logical(p, inst);
				BC_PROG_JUMP(inst, code, ip);
			}

			BC_PROG_LBL(BC_INST_READ):
			{
				// We want to flush output before
				// this in case there is a prompt.
				bc_file_flush(&vm.fout, bc_flush_save);

				bc_program_read(p);

				// Because we changed the execution stack and where we are
				// executing, we have to update all of this.
				ip = bc_vec_top(&p->stack);
				func = bc_vec_item(&p->fns, ip->func);
				code = func->code.v;
				bc_program_setVecs(p, func);

				BC_PROG_JUMP(inst, code, ip);
			}

#if BC_ENABLE_EXTRA_MATH
			BC_PROG_LBL(BC_INST_RAND):
			{
				bc_program_rand(p);
				BC_PROG_JUMP(inst, code, ip);
			}
#endif // BC_ENABLE_EXTRA_MATH

			BC_PROG_LBL(BC_INST_MAXIBASE):
			BC_PROG_LBL(BC_INST_MAXOBASE):
			BC_PROG_LBL(BC_INST_MAXSCALE):
#if BC_ENABLE_EXTRA_MATH
			BC_PROG_LBL(BC_INST_MAXRAND):
#endif // BC_ENABLE_EXTRA_MATH
			{
				BcBigDig dig = vm.maxes[inst - BC_INST_MAXIBASE];
				bc_program_pushBigdig(p, dig, BC_RESULT_TEMP);
				BC_PROG_JUMP(inst, code, ip);
			}

			BC_PROG_LBL(BC_INST_VAR):
			{
				bc_program_pushVar(p, code, &ip->idx, false, false);
				BC_PROG_JUMP(inst, code, ip);
			}

			BC_PROG_LBL(BC_INST_ARRAY_ELEM):
			BC_PROG_LBL(BC_INST_ARRAY):
			{
				bc_program_pushArray(p, code, &ip->idx, inst);
				BC_PROG_JUMP(inst, code, ip);
			}

			BC_PROG_LBL(BC_INST_IBASE):
			BC_PROG_LBL(BC_INST_SCALE):
			BC_PROG_LBL(BC_INST_OBASE):
			{
				bc_program_pushGlobal(p, inst);
				BC_PROG_JUMP(inst, code, ip);
			}

#if BC_ENABLE_EXTRA_MATH
			BC_PROG_LBL(BC_INST_SEED):
			{
				bc_program_pushSeed(p);
				BC_PROG_JUMP(inst, code, ip);
			}
#endif // BC_ENABLE_EXTRA_MATH

			BC_PROG_LBL(BC_INST_LENGTH):
			BC_PROG_LBL(BC_INST_SCALE_FUNC):
			BC_PROG_LBL(BC_INST_SQRT):
			BC_PROG_LBL(BC_INST_ABS):
#if BC_ENABLE_EXTRA_MATH
			BC_PROG_LBL(BC_INST_IRAND):
#endif // BC_ENABLE_EXTRA_MATH
			{
				bc_program_builtin(p, inst);
				BC_PROG_JUMP(inst, code, ip);
			}

			BC_PROG_LBL(BC_INST_ASCIIFY):
			{
				bc_program_asciify(p, ip->func);

				// Because we changed the execution stack and where we are
				// executing, we have to update all of this.
				ip = bc_vec_top(&p->stack);
				func = bc_vec_item(&p->fns, ip->func);
				code = func->code.v;
				bc_program_setVecs(p, func);

				BC_PROG_JUMP(inst, code, ip);
			}

			BC_PROG_LBL(BC_INST_NUM):
			{
				bc_program_const(p, code, &ip->idx);
				BC_PROG_JUMP(inst, code, ip);
			}

			BC_PROG_LBL(BC_INST_ZERO):
			BC_PROG_LBL(BC_INST_ONE):
#if BC_ENABLED
			BC_PROG_LBL(BC_INST_LAST):
#endif // BC_ENABLED
			{
				r.t = BC_RESULT_ZERO + (inst - BC_INST_ZERO);
				bc_vec_push(&p->results, &r);
				BC_PROG_JUMP(inst, code, ip);
			}

			BC_PROG_LBL(BC_INST_PRINT):
			BC_PROG_LBL(BC_INST_PRINT_POP):
#if BC_ENABLED
			BC_PROG_LBL(BC_INST_PRINT_STR):
#endif // BC_ENABLED
			{
				bc_program_print(p, inst, 0);

				// We want to flush right away to save the output for history,
				// if history must preserve it when taking input.
				bc_file_flush(&vm.fout, bc_flush_save);

				BC_PROG_JUMP(inst, code, ip);
			}

			BC_PROG_LBL(BC_INST_STR):
			{
				// Set up the result and push.
				r.t = BC_RESULT_STR;
				bc_num_clear(&r.d.n);
				r.d.n.rdx = bc_program_index(code, &ip->idx);
				r.d.n.scale = bc_program_index(code, &ip->idx);
				bc_vec_push(&p->results, &r);
				BC_PROG_JUMP(inst, code, ip);
			}

			BC_PROG_LBL(BC_INST_POWER):
			BC_PROG_LBL(BC_INST_MULTIPLY):
			BC_PROG_LBL(BC_INST_DIVIDE):
			BC_PROG_LBL(BC_INST_MODULUS):
			BC_PROG_LBL(BC_INST_PLUS):
			BC_PROG_LBL(BC_INST_MINUS):
#if BC_ENABLE_EXTRA_MATH
			BC_PROG_LBL(BC_INST_PLACES):
			BC_PROG_LBL(BC_INST_LSHIFT):
			BC_PROG_LBL(BC_INST_RSHIFT):
#endif // BC_ENABLE_EXTRA_MATH
			{
				bc_program_op(p, inst);
				BC_PROG_JUMP(inst, code, ip);
			}

			BC_PROG_LBL(BC_INST_NEG):
			BC_PROG_LBL(BC_INST_BOOL_NOT):
#if BC_ENABLE_EXTRA_MATH
			BC_PROG_LBL(BC_INST_TRUNC):
#endif // BC_ENABLE_EXTRA_MATH
			{
				bc_program_unary(p, inst);
				BC_PROG_JUMP(inst, code, ip);
			}

#if BC_ENABLED
			BC_PROG_LBL(BC_INST_ASSIGN_POWER):
			BC_PROG_LBL(BC_INST_ASSIGN_MULTIPLY):
			BC_PROG_LBL(BC_INST_ASSIGN_DIVIDE):
			BC_PROG_LBL(BC_INST_ASSIGN_MODULUS):
			BC_PROG_LBL(BC_INST_ASSIGN_PLUS):
			BC_PROG_LBL(BC_INST_ASSIGN_MINUS):
#if BC_ENABLE_EXTRA_MATH
			BC_PROG_LBL(BC_INST_ASSIGN_PLACES):
			BC_PROG_LBL(BC_INST_ASSIGN_LSHIFT):
			BC_PROG_LBL(BC_INST_ASSIGN_RSHIFT):
#endif // BC_ENABLE_EXTRA_MATH
			BC_PROG_LBL(BC_INST_ASSIGN):
			BC_PROG_LBL(BC_INST_ASSIGN_POWER_NO_VAL):
			BC_PROG_LBL(BC_INST_ASSIGN_MULTIPLY_NO_VAL):
			BC_PROG_LBL(BC_INST_ASSIGN_DIVIDE_NO_VAL):
			BC_PROG_LBL(BC_INST_ASSIGN_MODULUS_NO_VAL):
			BC_PROG_LBL(BC_INST_ASSIGN_PLUS_NO_VAL):
			BC_PROG_LBL(BC_INST_ASSIGN_MINUS_NO_VAL):
#if BC_ENABLE_EXTRA_MATH
			BC_PROG_LBL(BC_INST_ASSIGN_PLACES_NO_VAL):
			BC_PROG_LBL(BC_INST_ASSIGN_LSHIFT_NO_VAL):
			BC_PROG_LBL(BC_INST_ASSIGN_RSHIFT_NO_VAL):
#endif // BC_ENABLE_EXTRA_MATH
#endif // BC_ENABLED
			BC_PROG_LBL(BC_INST_ASSIGN_NO_VAL):
			{
				bc_program_assign(p, inst);
				BC_PROG_JUMP(inst, code, ip);
			}

			BC_PROG_LBL(BC_INST_POP):
			{
#ifndef BC_PROG_NO_STACK_CHECK
				// dc must do a stack check, but bc does not.
				if (BC_IS_DC) {
					if (BC_ERR(!BC_PROG_STACK(&p->results, 1)))
						bc_err(BC_ERR_EXEC_STACK);
				}
#endif // BC_PROG_NO_STACK_CHECK

				assert(BC_PROG_STACK(&p->results, 1));

				bc_vec_pop(&p->results);

				BC_PROG_JUMP(inst, code, ip);
			}

			BC_PROG_LBL(BC_INST_SWAP):
			{
				BcResult *ptr2;

				// Check the stack.
				if (BC_ERR(!BC_PROG_STACK(&p->results, 2)))
					bc_err(BC_ERR_EXEC_STACK);

				assert(BC_PROG_STACK(&p->results, 2));

				// Get the two items.
				ptr = bc_vec_item_rev(&p->results, 0);
				ptr2 = bc_vec_item_rev(&p->results, 1);

				// Swap. It's just easiest to do it this way.
				memcpy(&r, ptr, sizeof(BcResult));
				memcpy(ptr, ptr2, sizeof(BcResult));
				memcpy(ptr2, &r, sizeof(BcResult));

				BC_PROG_JUMP(inst, code, ip);
			}

			BC_PROG_LBL(BC_INST_MODEXP):
			{
				bc_program_modexp(p);
				BC_PROG_JUMP(inst, code, ip);
			}

			BC_PROG_LBL(BC_INST_DIVMOD):
			{
				bc_program_divmod(p);
				BC_PROG_JUMP(inst, code, ip);
			}

			BC_PROG_LBL(BC_INST_PRINT_STREAM):
			{
				bc_program_printStream(p);
				BC_PROG_JUMP(inst, code, ip);
			}

#if DC_ENABLED
			BC_PROG_LBL(BC_INST_POP_EXEC):
			{
				// If this fails, the dc parser got something wrong.
				assert(BC_PROG_STACK(&p->stack, 2));

				// Pop the execution stack and tail call stack.
				bc_vec_pop(&p->stack);
				bc_vec_pop(&p->tail_calls);

				// Because we changed the execution stack and where we are
				// executing, we have to update all of this.
				ip = bc_vec_top(&p->stack);
				func = bc_vec_item(&p->fns, ip->func);
				code = func->code.v;
				bc_program_setVecs(p, func);

				BC_PROG_JUMP(inst, code, ip);
			}

			BC_PROG_LBL(BC_INST_EXECUTE):
			BC_PROG_LBL(BC_INST_EXEC_COND):
			{
				cond = (inst == BC_INST_EXEC_COND);

				bc_program_execStr(p, code, &ip->idx, cond, func->code.len);

				// Because we changed the execution stack and where we are
				// executing, we have to update all of this.
				ip = bc_vec_top(&p->stack);
				func = bc_vec_item(&p->fns, ip->func);
				code = func->code.v;
				bc_program_setVecs(p, func);

				BC_PROG_JUMP(inst, code, ip);
			}

			BC_PROG_LBL(BC_INST_PRINT_STACK):
			{
				bc_program_printStack(p);
				BC_PROG_JUMP(inst, code, ip);
			}

			BC_PROG_LBL(BC_INST_CLEAR_STACK):
			{
				bc_vec_popAll(&p->results);
				BC_PROG_JUMP(inst, code, ip);
			}

			BC_PROG_LBL(BC_INST_REG_STACK_LEN):
			{
				bc_program_regStackLen(p, code, &ip->idx);
				BC_PROG_JUMP(inst, code, ip);
			}

			BC_PROG_LBL(BC_INST_STACK_LEN):
			{
				bc_program_stackLen(p);
				BC_PROG_JUMP(inst, code, ip);
			}

			BC_PROG_LBL(BC_INST_DUPLICATE):
			{
				// Check the stack.
				if (BC_ERR(!BC_PROG_STACK(&p->results, 1)))
					bc_err(BC_ERR_EXEC_STACK);

				assert(BC_PROG_STACK(&p->results, 1));

				// Get the top of the stack.
				ptr = bc_vec_top(&p->results);

				BC_SIG_LOCK;

				// Copy and push.
				bc_result_copy(&r, ptr);
				bc_vec_push(&p->results, &r);

				BC_SIG_UNLOCK;

				BC_PROG_JUMP(inst, code, ip);
			}

			BC_PROG_LBL(BC_INST_LOAD):
			BC_PROG_LBL(BC_INST_PUSH_VAR):
			{
				bool copy = (inst == BC_INST_LOAD);
				bc_program_pushVar(p, code, &ip->idx, true, copy);
				BC_PROG_JUMP(inst, code, ip);
			}

			BC_PROG_LBL(BC_INST_PUSH_TO_VAR):
			{
				idx = bc_program_index(code, &ip->idx);
				bc_program_copyToVar(p, idx, BC_TYPE_VAR, true);
				BC_PROG_JUMP(inst, code, ip);
			}

			BC_PROG_LBL(BC_INST_QUIT):
			BC_PROG_LBL(BC_INST_NQUIT):
			{
				bc_program_nquit(p, inst);

				// Because we changed the execution stack and where we are
				// executing, we have to update all of this.
				ip = bc_vec_top(&p->stack);
				func = bc_vec_item(&p->fns, ip->func);
				code = func->code.v;
				bc_program_setVecs(p, func);

				BC_PROG_JUMP(inst, code, ip);
			}

			BC_PROG_LBL(BC_INST_EXEC_STACK_LEN):
			{
				bc_program_execStackLen(p);
				BC_PROG_JUMP(inst, code, ip);
			}
#endif // DC_ENABLED

#if BC_HAS_COMPUTED_GOTO
			BC_PROG_LBL(BC_INST_INVALID):
			{
				return;
			}
#else // BC_HAS_COMPUTED_GOTO
			default:
			{
				BC_UNREACHABLE
#ifndef NDEBUG
				abort();
#endif // NDEBUG
			}
#endif // BC_HAS_COMPUTED_GOTO
		}

#if !BC_HAS_COMPUTED_GOTO
#ifndef NDEBUG
		// This is to allow me to use a debugger to see the last instruction,
		// which will point to which function was the problem. But it's also a
		// good smoke test for error handling changes.
		assert(jmp_bufs_len == vm.jmp_bufs.len);
#endif // NDEBUG
#endif // !BC_HAS_COMPUTED_GOTO
	}
}

#if BC_DEBUG_CODE
#if BC_ENABLED && DC_ENABLED
void bc_program_printStackDebug(BcProgram *p) {
	bc_file_puts(&vm.fout, bc_flush_err, "-------------- Stack ----------\n");
	bc_program_printStack(p);
	bc_file_puts(&vm.fout, bc_flush_err, "-------------- Stack End ------\n");
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

	s = *((char**) bc_vec_item(p->strs, idx));

	bc_vm_printf(" (\"%s\") ", s);
}

void bc_program_printInst(const BcProgram *p, const char *restrict code,
                          size_t *restrict bgn)
{
	uchar inst = (uchar) code[(*bgn)++];

	bc_vm_printf("Inst[%zu]: %s [%lu]; ", *bgn - 1,
	             bc_inst_names[inst], (unsigned long) inst);

	if (inst == BC_INST_VAR || inst == BC_INST_ARRAY_ELEM ||
	    inst == BC_INST_ARRAY)
	{
		bc_program_printIndex(code, bgn);
	}
	else if (inst == BC_INST_STR) bc_program_printStr(p, code, bgn);
	else if (inst == BC_INST_NUM) {
		size_t idx = bc_program_index(code, bgn);
		BcConst *c = bc_vec_item(p->consts, idx);
		bc_vm_printf("(%s)", c->val);
	}
	else if (inst == BC_INST_CALL ||
	         (inst > BC_INST_STR && inst <= BC_INST_JUMP_ZERO))
	{
		bc_program_printIndex(code, bgn);
		if (inst == BC_INST_CALL) bc_program_printIndex(code, bgn);
	}

	bc_vm_putchar('\n', bc_flush_err);
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
		bc_file_puts(&vm.fout, bc_flush_err, "\n\n");
	}
}
#endif // BC_ENABLED && DC_ENABLED
#endif // BC_DEBUG_CODE
