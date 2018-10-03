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
	BcParseExpr parse_exp;
	// ** Exclude end. **

} BcProgram;

#define BC_PROG_CHECK_STACK(s, n) ((s)->len >= (n))

#define BC_PROG_MAIN (0)
#define BC_PROG_READ (1)

#ifdef DC_ENABLED
#	define BC_PROG_STR_VAR(n) (!(n)->num && !n->cap)
#endif // DC_ENABLED

typedef unsigned long (*BcProgramBuiltIn)(BcNum*);

// ** Exclude start. **
BcStatus bc_program_init(BcProgram *p, size_t line_len,
                         BcParseInit parse_init, BcParseExpr parse_expr);
void bc_program_free(BcProgram *program);
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
