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

#define BC_PROG_ONE_CAP (1)

typedef struct BcProgram {

	size_t scale;

	BcNum ib;
	size_t ib_t;
	BcNum ob;
	size_t ob_t;

#if DC_ENABLED
	BcNum strmb;
#endif // DC_ENABLED

	BcVec results;
	BcVec stack;

	BcVec fns;
#if BC_ENABLED
	BcVec fn_map;
#endif // BC_ENABLED

	BcVec vars;
	BcVec var_map;

	BcVec arrs;
	BcVec arr_map;

#if BC_ENABLED
	BcNum one;
	BcNum last;
#endif // BC_ENABLED

	BcDig ib_num[BC_NUM_LONG_LOG10];
	BcDig ob_num[BC_NUM_LONG_LOG10];
#if DC_ENABLED
	// This uses BC_NUM_LONG_LOG10 because it is used in bc_num_ulong2num(),
	// which attempts to realloc, unless it is big enough. This is big enough.
	BcDig strmb_num[BC_NUM_LONG_LOG10];
#endif // DC_ENABLED
#if BC_ENABLED
	BcDig one_num[BC_PROG_ONE_CAP];
#endif // BC_ENABLED

} BcProgram;

#define BC_PROG_STACK(s, n) ((s)->len >= ((size_t) (n)))

#define BC_PROG_MAIN (0)
#define BC_PROG_READ (1)

#if DC_ENABLED
#define BC_PROG_REQ_FUNCS (2)
#if !BC_ENABLED
// For dc only, last is always true.
#define bc_program_copyToVar(p, name, t, last) \
	bc_program_copyToVar(p, name, t)
#endif // !BC_ENABLED
#else
// For bc, 'pop' and 'copy' are always false.
#define bc_program_pushVar(p, code, bgn, pop, copy) \
	bc_program_pushVar(p, code, bgn)
#ifdef NDEBUG
#define BC_PROG_NO_STACK_CHECK
#endif // NDEBUG
#endif // DC_ENABLED

#define BC_PROG_STR(n) (!(n)->num && !(n)->cap)
#define BC_PROG_NUM(r, n) \
	((r)->t != BC_RESULT_ARRAY && (r)->t != BC_RESULT_STR && !BC_PROG_STR(n))

typedef void (*BcProgramUnary)(BcResult*, BcNum*);

// ** Exclude start. **
// ** Busybox exclude start. **
void bc_program_init(BcProgram *p);
void bc_program_free(BcProgram *p);

#ifndef NDEBUG
#if BC_ENABLED && DC_ENABLED
void bc_program_code(BcProgram *p);
void bc_program_printInst(BcProgram *p, const char *code,
                          size_t *restrict bgn);
#endif // BC_ENABLED && DC_ENABLED
#endif // NDEBUG
// ** Busybox exclude end. **
// ** Exclude end. **

void bc_program_addFunc(BcProgram *p, BcFunc *f, const char* name);
size_t bc_program_insertFunc(BcProgram *p, char *name);
BcStatus bc_program_reset(BcProgram *p, BcStatus s);
BcStatus bc_program_exec(BcProgram *p);

unsigned long bc_program_scale(const BcNum *restrict n);
unsigned long bc_program_len(const BcNum *restrict n);

void bc_program_negate(BcResult *r, BcNum *n);
void bc_program_not(BcResult *r, BcNum *n);
#if BC_ENABLE_EXTRA_MATH
void bc_program_trunc(BcResult *r, BcNum *n);
#endif // BC_ENABLE_EXTRA_MATH

// ** Exclude start. **
// ** Busybox exclude start. **
extern const BcNumBinaryOp bc_program_ops[];
extern const BcProgramBuiltIn bc_program_builtins[];
extern const BcProgramUnary bc_program_unarys[];
extern const char bc_program_exprs_name[];
extern const char bc_program_stdin_name[];
extern const char bc_program_ready_msg[];
extern const char bc_program_esc_chars[];
extern const char bc_program_esc_seqs[];
// ** Busybox exclude end. **
// ** Exclude end. **

#endif // BC_PROGRAM_H
