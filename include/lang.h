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
 * Definitions for program data.
 *
 */

#ifndef BC_LANG_H
#define BC_LANG_H

#include <stdbool.h>

#include <status.h>
#include <vector.h>
#include <num.h>

/// The instructions for bytecode.
typedef enum BcInst {

#if BC_ENABLED

	/// Postfix increment and decrement. Prefix are translated into
	/// BC_INST_ONE with either BC_INST_ASSIGN_PLUS or BC_INST_ASSIGN_MINUS.
	BC_INST_INC = 0,
	BC_INST_DEC,
#endif // BC_ENABLED

	/// Unary negation.
	BC_INST_NEG,

	/// Boolean not.
	BC_INST_BOOL_NOT,
#if BC_ENABLE_EXTRA_MATH
	/// Truncation operator.
	BC_INST_TRUNC,
#endif // BC_ENABLE_EXTRA_MATH

	/// These should be self-explanatory.
	BC_INST_POWER,
	BC_INST_MULTIPLY,
	BC_INST_DIVIDE,
	BC_INST_MODULUS,
	BC_INST_PLUS,
	BC_INST_MINUS,

#if BC_ENABLE_EXTRA_MATH

	/// Places operator.
	BC_INST_PLACES,

	/// Shift operators.
	BC_INST_LSHIFT,
	BC_INST_RSHIFT,
#endif // BC_ENABLE_EXTRA_MATH

	/// Comparison operators.
	BC_INST_REL_EQ,
	BC_INST_REL_LE,
	BC_INST_REL_GE,
	BC_INST_REL_NE,
	BC_INST_REL_LT,
	BC_INST_REL_GT,

	/// Boolean or and and.
	BC_INST_BOOL_OR,
	BC_INST_BOOL_AND,

#if BC_ENABLED
	/// Same as the normal operators, but assigment. So ^=, *=, /=, etc.
	BC_INST_ASSIGN_POWER,
	BC_INST_ASSIGN_MULTIPLY,
	BC_INST_ASSIGN_DIVIDE,
	BC_INST_ASSIGN_MODULUS,
	BC_INST_ASSIGN_PLUS,
	BC_INST_ASSIGN_MINUS,
#if BC_ENABLE_EXTRA_MATH
	/// Places and shift assignment operators.
	BC_INST_ASSIGN_PLACES,
	BC_INST_ASSIGN_LSHIFT,
	BC_INST_ASSIGN_RSHIFT,
#endif // BC_ENABLE_EXTRA_MATH

	/// Normal assignment.
	BC_INST_ASSIGN,

	/// bc and dc detect when the value from an assignment is not necessary.
	/// For example, a plain assignment statement means the value is never used.
	/// In those cases, we can get lots of performance back by not even creating
	/// a copy at all. In fact, it saves a copy, a push onto the results stack,
	/// a pop from the results stack, and a free. Definitely worth it to detect.
	BC_INST_ASSIGN_POWER_NO_VAL,
	BC_INST_ASSIGN_MULTIPLY_NO_VAL,
	BC_INST_ASSIGN_DIVIDE_NO_VAL,
	BC_INST_ASSIGN_MODULUS_NO_VAL,
	BC_INST_ASSIGN_PLUS_NO_VAL,
	BC_INST_ASSIGN_MINUS_NO_VAL,
#if BC_ENABLE_EXTRA_MATH
	/// Same as above.
	BC_INST_ASSIGN_PLACES_NO_VAL,
	BC_INST_ASSIGN_LSHIFT_NO_VAL,
	BC_INST_ASSIGN_RSHIFT_NO_VAL,
#endif // BC_ENABLE_EXTRA_MATH
#endif // BC_ENABLED

	/// Normal assignment that pushes no value on the stack.
	BC_INST_ASSIGN_NO_VAL,

	/// Push a constant onto the results stack.
	BC_INST_NUM,

	/// Push a variable onto the results stack.
	BC_INST_VAR,

	/// Push an array element onto the results stack.
	BC_INST_ARRAY_ELEM,
#if BC_ENABLED
	/// Push an array onto the results stack. This is different from pushing an
	/// array *element* onto the results stack; it pushes a reference to the
	/// whole array. This is needed in bc for function arguments that are
	/// arrays.
	BC_INST_ARRAY,
#endif // BC_ENABLED

	/// Push a zero or a one onto the stack. These are special cased because it
	/// does help performance, particularly for one since inc/dec operators
	/// use it.
	BC_INST_ZERO,
	BC_INST_ONE,

#if BC_ENABLED
	/// Push the last printed value onto the stack.
	BC_INST_LAST,
#endif // BC_ENABLED

	/// Push the value of any of the globals onto the stack.
	BC_INST_IBASE,
	BC_INST_OBASE,
	BC_INST_SCALE,

#if BC_ENABLE_EXTRA_MATH && BC_ENABLE_RAND
	/// Push the value of the seed global onto the stack.
	BC_INST_SEED,
#endif // BC_ENABLE_EXTRA_MATH && BC_ENABLE_RAND

	/// These are builtin functions.
	BC_INST_LENGTH,
	BC_INST_SCALE_FUNC,
	BC_INST_SQRT,
	BC_INST_ABS,

#if BC_ENABLE_EXTRA_MATH && BC_ENABLE_RAND
	/// Another builtin function.
	BC_INST_IRAND,
#endif // BC_ENABLE_EXTRA_MATH && BC_ENABLE_RAND

	/// Another builtin function.
	BC_INST_READ,

#if BC_ENABLE_EXTRA_MATH && BC_ENABLE_RAND
	/// Another builtin function.
	BC_INST_RAND,
#endif // BC_ENABLE_EXTRA_MATH && BC_ENABLE_RAND

	/// Return the max for the various globals.
	BC_INST_MAXIBASE,
	BC_INST_MAXOBASE,
	BC_INST_MAXSCALE,
#if BC_ENABLE_EXTRA_MATH && BC_ENABLE_RAND
	/// Return the max value returned by rand().
	BC_INST_MAXRAND,
#endif // BC_ENABLE_EXTRA_MATH && BC_ENABLE_RAND

	/// This is slightly misnamed versus BC_INST_PRINT_POP. Well, it is in bc.
	/// dc uses this instruction to print, but not pop. That's valid in dc.
	/// However, in bc, it is *never* valid to print without popping.
	BC_INST_PRINT,
	BC_INST_PRINT_POP,
	BC_INST_STR,
	BC_INST_PRINT_STR,

#if BC_ENABLED
	BC_INST_JUMP,
	BC_INST_JUMP_ZERO,

	BC_INST_CALL,

	BC_INST_RET,
	BC_INST_RET0,
	BC_INST_RET_VOID,

	BC_INST_HALT,
#endif // BC_ENABLED

	BC_INST_POP,

#if DC_ENABLED
	BC_INST_POP_EXEC,
	BC_INST_MODEXP,
	BC_INST_DIVMOD,

	BC_INST_EXECUTE,
	BC_INST_EXEC_COND,

	BC_INST_ASCIIFY,
	BC_INST_PRINT_STREAM,

	BC_INST_PRINT_STACK,
	BC_INST_CLEAR_STACK,
	BC_INST_REG_STACK_LEN,
	BC_INST_STACK_LEN,
	BC_INST_DUPLICATE,
	BC_INST_SWAP,

	BC_INST_LOAD,
	BC_INST_PUSH_VAR,
	BC_INST_PUSH_TO_VAR,

	BC_INST_QUIT,
	BC_INST_NQUIT,
#endif // DC_ENABLED

	BC_INST_INVALID = UCHAR_MAX,

} BcInst;

typedef struct BcId {
	char *name;
	size_t idx;
} BcId;

typedef struct BcLoc {
	size_t loc;
	size_t idx;
} BcLoc;

typedef struct BcConst {
	char *val;
	BcBigDig base;
	BcNum num;
} BcConst;

typedef struct BcFunc {

	BcVec code;
#if BC_ENABLED
	BcVec labels;
	BcVec autos;
	size_t nparams;
#endif // BC_ENABLED

	BcVec strs;
	BcVec consts;

	const char *name;
#if BC_ENABLED
	bool voidfn;
#endif // BC_ENABLED

} BcFunc;

typedef enum BcResultType {

	BC_RESULT_VAR,
	BC_RESULT_ARRAY_ELEM,
#if BC_ENABLED
	BC_RESULT_ARRAY,
#endif // BC_ENABLED

	BC_RESULT_STR,

	BC_RESULT_TEMP,

	BC_RESULT_ZERO,
	BC_RESULT_ONE,

#if BC_ENABLED
	BC_RESULT_LAST,
	BC_RESULT_VOID,
#endif // BC_ENABLED
	BC_RESULT_IBASE,
	BC_RESULT_OBASE,
	BC_RESULT_SCALE,
#if BC_ENABLE_EXTRA_MATH
	BC_RESULT_SEED,
#endif // BC_ENABLE_EXTRA_MATH

} BcResultType;

typedef union BcResultData {
	BcNum n;
	BcVec v;
	BcLoc loc;
} BcResultData;

typedef struct BcResult {
	BcResultType t;
	BcResultData d;
} BcResult;

typedef struct BcInstPtr {
	size_t func;
	size_t idx;
	size_t len;
} BcInstPtr;

typedef enum BcType {
	BC_TYPE_VAR,
	BC_TYPE_ARRAY,
#if BC_ENABLED
	BC_TYPE_REF,
#endif // BC_ENABLED
} BcType;

#if BC_ENABLED
typedef struct BcAuto {

	size_t idx;

	BcType type;

} BcAuto;
#endif // BC_ENABLED

struct BcProgram;

void bc_func_init(BcFunc *f, const char* name);
void bc_func_insert(BcFunc *f, struct BcProgram* p, char* name,
                    BcType type, size_t line);
void bc_func_reset(BcFunc *f);
void bc_func_free(void *func);

void bc_array_init(BcVec *a, bool nums);
void bc_array_copy(BcVec *d, const BcVec *s);

void bc_string_free(void *string);
void bc_const_free(void *constant);
void bc_id_free(void *id);
void bc_result_clear(BcResult *r);
void bc_result_copy(BcResult *d, BcResult *src);
void bc_result_free(void *result);

void bc_array_expand(BcVec *a, size_t len);
int bc_id_cmp(const BcId *e1, const BcId *e2);

/// Returns non-zero if the instruction i is an assignment instruction.
#if BC_ENABLED
#define BC_INST_IS_ASSIGN(i) \
	((i) == BC_INST_ASSIGN || (i) == BC_INST_ASSIGN_NO_VAL)
#define BC_INST_USE_VAL(i) ((i) <= BC_INST_ASSIGN)
#else // BC_ENABLED
#define BC_INST_IS_ASSIGN(i) ((i) == BC_INST_ASSIGN_NO_VAL)
#define BC_INST_USE_VAL(i) (false)
#endif // BC_ENABLED

// In bc, functions are never freed until the end. So we only free them if
// dc is enabled or in debug mode.
#ifndef NDEBUG
#define BC_ENABLE_FUNC_FREE (1)
#else // NDEBUG
#define BC_ENABLE_FUNC_FREE DC_ENABLED
#endif // NDEBUG

#if BC_DEBUG_CODE
extern const char* bc_inst_names[];
#endif // BC_DEBUG_CODE

extern const char bc_func_main[];
extern const char bc_func_read[];

#endif // BC_LANG_H
