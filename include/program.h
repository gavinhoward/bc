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
#include <rand.h>

/// The index of ibase in the globals array.
#define BC_PROG_GLOBALS_IBASE (0)

/// The index of obase in the globals array.
#define BC_PROG_GLOBALS_OBASE (1)

/// The index of scale in the globals array.
#define BC_PROG_GLOBALS_SCALE (2)

#if BC_ENABLE_EXTRA_MATH

/// The index of the rand max in the maxes array.
#define BC_PROG_MAX_RAND (3)

#endif // BC_ENABLE_EXTRA_MATH

/// The length of the globals array.
#define BC_PROG_GLOBALS_LEN (3 + BC_ENABLE_EXTRA_MATH)

typedef struct BcProgram {

	/// The array of globals values.
	BcBigDig globals[BC_PROG_GLOBALS_LEN];

	/// The array of globals stacks.
	BcVec globals_v[BC_PROG_GLOBALS_LEN];

#if BC_ENABLE_EXTRA_MATH && BC_ENABLE_RAND

	/// The pseudo-random number generator.
	BcRNG rng;

#endif // BC_ENABLE_EXTRA_MATH && BC_ENABLE_RAND

	/// The results stack.
	BcVec results;

	/// The execution stack.
	BcVec stack;

	/// A pointer to the current function's constants.
	BcVec *consts;

	/// A pointer to the current function's strings.
	BcVec *strs;

	/// The array of functions.
	BcVec fns;

	/// The map of functions to go with fns.
	BcVec fn_map;

	/// The array of variables.
	BcVec vars;

	/// The map of variables to go with vars.
	BcVec var_map;

	/// The array of arrays.
	BcVec arrs;

	/// The map of arrays to go with arrs.
	BcVec arr_map;

#if DC_ENABLED

	/// An array of strings. This is for dc only because dc does not store
	/// strings in functions.
	BcVec strs_v;

	/// A vector of tail calls. These are just integers, which are the number of
	/// tail calls that have been executed for each function (string) on the
	/// stack for dc. This is to prevent dc from constantly growing memory use
	/// because of pushing more and more string executions on the stack.
	BcVec tail_calls;

	/// A BcNum that has the proper base for asciify for dc.
	BcNum strmb;

#endif // DC_ENABLED

#if BC_ENABLED

	/// The last printed value for bc.
	BcNum last;

#endif // BC_ENABLED

#if DC_ENABLED

	// The BcDig array for strmb. This uses BC_NUM_LONG_LOG10 because it is used
	// in bc_num_ulong2num(), which attempts to realloc, unless it is big
	// enough. This is big enough.
	BcDig strmb_num[BC_NUM_BIGDIG_LOG10];

#endif // DC_ENABLED

} BcProgram;

/**
 * Returns true if the stack @a s has at least @a n items, false otherwise.
 * @param s  The stack to check.
 * @param n  The number of items the stack must have.
 * @return   True if @a s has at least @a n items, false otherwise.
 */
#define BC_PROG_STACK(s, n) ((s)->len >= ((size_t) (n)))

/**
 * Get a pointer to the top value in a global value stack.
 * @param v  The global value stack.
 * @return   A pointer to the top value in @a v.
 */
#define BC_PROG_GLOBAL_PTR(v) (bc_vec_top(v))

/**
 * Get the top value in a global value stack.
 * @param v  The global value stack.
 * @return   The top value in @a v.
 */
#define BC_PROG_GLOBAL(v) (*((BcBigDig*) BC_PROG_GLOBAL_PTR(v)))

/**
 * Returns the current value of ibase.
 * @param p  The program.
 * @return   The current ibase.
 */
#define BC_PROG_IBASE(p) ((p)->globals[BC_PROG_GLOBALS_IBASE])

/**
 * Returns the current value of obase.
 * @param p  The program.
 * @return   The current obase.
 */
#define BC_PROG_OBASE(p) ((p)->globals[BC_PROG_GLOBALS_OBASE])

/**
 * Returns the current value of scale.
 * @param p  The program.
 * @return   The current scale.
 */
#define BC_PROG_SCALE(p) ((p)->globals[BC_PROG_GLOBALS_SCALE])

/// The index for the main function in the functions array.//
#define BC_PROG_MAIN (0)

/// The index for the read function in the functions array.
#define BC_PROG_READ (1)

/**
 * Retires (completes the execution of) an instruction. Some instructions
 * require special retirement, but most can use this. This basically pops the
 * operands while preserving the result (which we assumed was pushed before the
 * actual operation).
 * @param p     The program.
 * @param nres  The number of results returned by the instruction.
 * @param nops  The number of operands used by the instruction.
 */
#define bc_program_retire(p, nres, nops) \
	(bc_vec_npopAt(&(p)->results, (nops), (p)->results.len - (nres + nops)))

#if DC_ENABLED

/// A constant that tells how many functions are required in dc.
#define BC_PROG_REQ_FUNCS (2)

#if !BC_ENABLED

/// This define disappears the parameter last because for dc only, last is
/// always true.
#define bc_program_copyToVar(p, name, t, last) \
	bc_program_copyToVar(p, name, t)

#endif // !BC_ENABLED

#else // DC_ENABLED

/// This define disappears pop and copy because for bc, 'pop' and 'copy' are
/// always false.
#define bc_program_pushVar(p, code, bgn, pop, copy) \
	bc_program_pushVar(p, code, bgn)

// In debug mode, we want bc to check the stack, but otherwise, we don't because
// the bc language implicitly mandates that the stack should always have enough
// items.
#ifdef NDEBUG
#define BC_PROG_NO_STACK_CHECK
#endif // NDEBUG

#endif // DC_ENABLED

/**
 * Returns true if the BcNum @a n is acting as a string.
 * @param n  The BcNum to test.
 * @return   True if @a n is acting as a string, false otherwise.
 */
#define BC_PROG_STR(n) ((n)->num == NULL && !(n)->cap)

#if BC_ENABLED

/**
 * Returns true if the result @a r and @a n is a number.
 * @param r  The result.
 * @param n  The number corresponding to the result.
 * @return   True if the result holds a number, false otherwise.
 */
#define BC_PROG_NUM(r, n) \
	((r)->t != BC_RESULT_ARRAY && (r)->t != BC_RESULT_STR && !BC_PROG_STR(n))

#else // BC_ENABLED

/**
 * Returns true if the result @a r and @a n is a number.
 * @param r  The result.
 * @param n  The number corresponding to the result.
 * @return   True if the result holds a number, false otherwise.
 */
#define BC_PROG_NUM(r, n) ((r)->t != BC_RESULT_STR && !BC_PROG_STR(n))

/// This define removes the inst parameter because for dc, inst is always
/// BC_INST_ARRAY_ELEM.
#define bc_program_pushArray(p, code, bgn, inst) \
	bc_program_pushArray(p, code, bgn)

#endif // BC_ENABLED

/**
 * This is a function type for unary operations. Currently, these include
 * boolean not, negation, and truncation with extra math.
 * @param r  The BcResult to store the result into.
 * @param n  The parameter to the unary operation.
 */
typedef void (*BcProgramUnary)(BcResult *r, BcNum *n);

/**
 * Initializes the BcProgram.
 * @param p  The program to initialize.
 */
void bc_program_init(BcProgram *p);

#ifndef NDEBUG

/**
 * Frees a BcProgram. This is only used in debug builds because a BcProgram is
 * only freed on program exit, and we don't care about freeing resources on
 * exit.
 * @param p  The program to initialize.
 */
void bc_program_free(BcProgram *p);

#endif // NDEBUG

#if BC_DEBUG_CODE
#if BC_ENABLED && DC_ENABLED

/**
 * Prints the bytecode in a function. This is a debug-only function.
 * @param p  The program.
 */
void bc_program_code(const BcProgram *p);

/**
 * Prints an instruction. This is a debug-only function.
 * @param p  The program.
 * @param code  The bytecode array.
 * @param bgn   A pointer to the current index. It is also updated to the next
 *              index.
 */
void bc_program_printInst(const BcProgram *p, const char *code,
                          size_t *restrict bgn);

/**
 * Prints the stack. This is a debug-only function.
 * @param p  The program.
 */
void bc_program_printStackDebug(BcProgram* p);

#endif // BC_ENABLED && DC_ENABLED
#endif // BC_DEBUG_CODE

/**
 * Returns the index of the variable or array in their respective arrays.
 * @param p    The program.
 * @param id   The BcId of the variable or array.
 * @param var  True if the search should be for a variable, false for an array.
 * @return     The index of the variable or array in the correct array.
 */
size_t bc_program_search(BcProgram *p, const char* id, bool var);

/**
 * Inserts a function into the program and returns the index of the function in
 * the fns array.
 * @param p     The program.
 * @param name  The name of the function.
 * @return      The index of the function after insertion.
 */
size_t bc_program_insertFunc(BcProgram *p, const char *name);

/**
 * Resets a program, usually because of resetting after an error.
 * @param p  The program to reset.
 */
void bc_program_reset(BcProgram *p);

/**
 * Executes bc or dc code in the BcProgram.
 * @param p  The program.
 */
void bc_program_exec(BcProgram *p);

/**
 * Negates a copy of a BcNum. This is a BcProgramUnary function.
 * @param r  The BcResult to store the result into.
 * @param n  The parameter to the unary operation.
 */
void bc_program_negate(BcResult *r, BcNum *n);

/**
 * Returns a boolean not of a BcNum. This is a BcProgramUnary function.
 * @param r  The BcResult to store the result into.
 * @param n  The parameter to the unary operation.
 */
void bc_program_not(BcResult *r, BcNum *n);

#if BC_ENABLE_EXTRA_MATH

/**
 * Truncates a copy of a BcNum. This is a BcProgramUnary function.
 * @param r  The BcResult to store the result into.
 * @param n  The parameter to the unary operation.
 */
void bc_program_trunc(BcResult *r, BcNum *n);

#endif // BC_ENABLE_EXTRA_MATH

/// A reference to an array of binary operator functions.
extern const BcNumBinaryOp bc_program_ops[];

/// A reference to an array of binary operator allocation request functions.
extern const BcNumBinaryOpReq bc_program_opReqs[];

/// A reference to an array of unary operator functions.
extern const BcProgramUnary bc_program_unarys[];

/// A reference to a filename for command-line expressions.
extern const char bc_program_exprs_name[];

/// A reference to a filename for stdin.
extern const char bc_program_stdin_name[];

/// A reference to the ready message printed on SIGINT.
extern const char bc_program_ready_msg[];

/// A reference to the length of the ready message.
extern const size_t bc_program_ready_msg_len;

/// A reference to an array of escape characters for the print statement.
extern const char bc_program_esc_chars[];

/// A reference to an array of the characters corresponding to the escape
/// characters in bc_program_esc_chars.
extern const char bc_program_esc_seqs[];

#endif // BC_PROGRAM_H
