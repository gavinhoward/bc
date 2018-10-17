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
 * Definitions for bc programs.
 *
 */

#ifndef BC_PROGRAM_H
#define BC_PROGRAM_H

#include <stddef.h>

#include <status.h>
#include <parse.h>
#include <lang.h>
#include <num.h>

typedef struct BcProgram {

	size_t len;

	size_t scale;

	BcNum ib;
	size_t ib_t;
	BcNum ob;
	size_t ob_t;

	BcNum hexb;

#ifdef DC_ENABLED
	BcNum strmb;
#endif // DC_ENABLED

	BcVec results;
	BcVec stack;

	BcVec fns;
	BcVecO fn_map;

	BcVec vars;
	BcVecO var_map;

	BcVec arrs;
	BcVecO arr_map;

	BcVec strs;
	BcVec consts;

	const char *file;

	BcNum last;
	BcNum zero;
	BcNum one;

	size_t nchars;

	// ** Exclude start. **
	BcParseInit parse_init;
	BcParseExpr parse_expr;
	// ** Exclude end. **

} BcProgram;

#define BC_PROG_STACK(s, n) ((s)->len >= ((size_t) n))

#define BC_PROG_MAIN (0)
#define BC_PROG_READ (1)

#ifdef DC_ENABLED
#define BC_PROG_REQ_FUNCS (2)
#endif // DC_ENABLED

#define BC_PROG_STR(n) (!(n)->num && !(n)->cap)
#define BC_PROG_NUM(r, n) \
	((r)->t != BC_RESULT_ARRAY && (r)->t != BC_RESULT_STR && !BC_PROG_STR(n))

typedef unsigned long (*BcProgramBuiltIn)(BcNum*);

// ** Exclude start. **
BcStatus bc_program_init(BcProgram *p, size_t line_len,
                         BcParseInit init, BcParseExpr expr);
void bc_program_free(BcProgram *program);

BcStatus bc_program_search(BcProgram *p, char *name, BcVec **ret, bool var);
BcStatus bc_program_num(BcProgram *p, BcResult *r, BcNum **num, bool hex);
BcStatus bc_program_binOpPrep(BcProgram *p, BcResult **l, BcNum **ln,
                              BcResult **r, BcNum **rn, bool assign);
BcStatus bc_program_binOpRetire(BcProgram *p, BcResult *r);
BcStatus bc_program_prep(BcProgram *p, BcResult **r, BcNum **n);
BcStatus bc_program_retire(BcProgram *p, BcResult *r, BcResultType t);
BcStatus bc_program_op(BcProgram *p, uint8_t inst);
BcStatus bc_program_read(BcProgram *p);
size_t bc_program_index(char *code, size_t *bgn);
char* bc_program_name(char *code, size_t *bgn);
BcStatus bc_program_printString(const char *str, size_t *nchars);
BcStatus bc_program_print(BcProgram *p, uint8_t inst, size_t idx);
BcStatus bc_program_negate(BcProgram *p);
BcStatus bc_program_logical(BcProgram *p, uint8_t inst);

#ifdef DC_ENABLED
BcStatus bc_program_assignStr(BcProgram *p, BcResult *r, BcVec *v, bool push);
#endif // DC_ENABLED

BcStatus bc_program_copyToVar(BcProgram *p, char *name, bool var);
BcStatus bc_program_assign(BcProgram *p, uint8_t inst);
BcStatus bc_program_pushVar(BcProgram *p, char *code, size_t *bgn,
                            bool pop, bool copy);
BcStatus bc_program_pushArray(BcProgram *p, char *code,
                              size_t *bgn, uint8_t inst);

#ifdef BC_ENABLED
BcStatus bc_program_incdec(BcProgram *p, uint8_t inst);
BcStatus bc_program_call(BcProgram *p, char *code, size_t *idx);
BcStatus bc_program_return(BcProgram *p, uint8_t inst);
#endif // BC_ENABLED

unsigned long bc_program_scale(BcNum *n);
unsigned long bc_program_len(BcNum *n);
BcStatus bc_program_builtin(BcProgram *p, uint8_t inst);

#ifdef DC_ENABLED
BcStatus bc_program_divmod(BcProgram *p);
BcStatus bc_program_modexp(BcProgram *p);
BcStatus bc_program_stackLen(BcProgram *p);
BcStatus bc_program_asciify(BcProgram *p);
BcStatus bc_program_printStream(BcProgram *p);
BcStatus bc_program_nquit(BcProgram *p);
BcStatus bc_program_execStr(BcProgram *p, char *code, size_t *bgn, bool cond);
#endif // DC_ENABLED

BcStatus bc_program_pushGlobal(BcProgram *p, uint8_t inst);

#ifndef NDEBUG
BcStatus bc_program_code(BcProgram *p);
BcStatus bc_program_printInst(BcProgram *p, char *code, size_t *bgn);
#endif // NDEBUG
// ** Exclude end. **

BcStatus bc_program_addFunc(BcProgram *p, char *name, size_t *idx);
BcStatus bc_program_reset(BcProgram *p, BcStatus s);
BcStatus bc_program_exec(BcProgram *p);

extern const BcNumBinaryOp bc_program_ops[];
extern const char bc_program_stdin_name[];
extern const char bc_program_ready_msg[];

#endif // BC_PROGRAM_H
