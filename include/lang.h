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
 * Definitions for program data.
 *
 */

#ifndef BC_LANG_H
#define BC_LANG_H

#include <stdbool.h>

#include <status.h>
#include <vector.h>
#include <num.h>

typedef enum BcInst {

#if BC_ENABLED
	BC_INST_INC_POST = 0,
	BC_INST_DEC_POST,
	BC_INST_INC_PRE,
	BC_INST_DEC_PRE,
#endif // BC_ENABLED

	BC_INST_NEG,
	BC_INST_BOOL_NOT,
#if BC_ENABLE_EXTRA_MATH
	BC_INST_TRUNC,
#endif // BC_ENABLE_EXTRA_MATH

	BC_INST_POWER,
	BC_INST_MULTIPLY,
	BC_INST_DIVIDE,
	BC_INST_MODULUS,
	BC_INST_PLUS,
	BC_INST_MINUS,

#if BC_ENABLE_EXTRA_MATH
	BC_INST_PLACES,

	BC_INST_LSHIFT,
	BC_INST_RSHIFT,
#endif // BC_ENABLE_EXTRA_MATH

	BC_INST_REL_EQ,
	BC_INST_REL_LE,
	BC_INST_REL_GE,
	BC_INST_REL_NE,
	BC_INST_REL_LT,
	BC_INST_REL_GT,

	BC_INST_BOOL_OR,
	BC_INST_BOOL_AND,

#if BC_ENABLED
	BC_INST_ASSIGN_POWER,
	BC_INST_ASSIGN_MULTIPLY,
	BC_INST_ASSIGN_DIVIDE,
	BC_INST_ASSIGN_MODULUS,
	BC_INST_ASSIGN_PLUS,
	BC_INST_ASSIGN_MINUS,
#if BC_ENABLE_EXTRA_MATH
	BC_INST_ASSIGN_PLACES,
	BC_INST_ASSIGN_LSHIFT,
	BC_INST_ASSIGN_RSHIFT,
#endif // BC_ENABLE_EXTRA_MATH
#endif // BC_ENABLED
	BC_INST_ASSIGN,

	BC_INST_NUM,
	BC_INST_VAR,
	BC_INST_ARRAY_ELEM,
	BC_INST_ARRAY,

#if BC_ENABLED
	BC_INST_LAST,
#endif // BC_ENABLED
	BC_INST_IBASE,
	BC_INST_OBASE,
	BC_INST_SCALE,
	BC_INST_LENGTH,
	BC_INST_SCALE_FUNC,
	BC_INST_SQRT,
	BC_INST_ABS,
	BC_INST_READ,

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
	BC_INST_POP_EXEC,

#if DC_ENABLED
	BC_INST_MODEXP,
	BC_INST_DIVMOD,

	BC_INST_EXECUTE,
	BC_INST_EXEC_COND,

	BC_INST_ASCIIFY,
	BC_INST_PRINT_STREAM,

	BC_INST_PRINT_STACK,
	BC_INST_CLEAR_STACK,
	BC_INST_STACK_LEN,
	BC_INST_DUPLICATE,
	BC_INST_SWAP,

	BC_INST_LOAD,
	BC_INST_PUSH_VAR,
	BC_INST_PUSH_TO_VAR,

	BC_INST_QUIT,
	BC_INST_NQUIT,

	BC_INST_INVALID = UCHAR_MAX,
#endif // DC_ENABLED

} BcInst;

typedef struct BcId {
	char *name;
	size_t idx;
} BcId;

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
	BC_RESULT_ARRAY,

	BC_RESULT_STR,

	BC_RESULT_CONSTANT,
	BC_RESULT_TEMP,

#if BC_ENABLED
	BC_RESULT_VOID,
	BC_RESULT_ONE,
	BC_RESULT_LAST,
#endif // BC_ENABLED
	BC_RESULT_IBASE,
	BC_RESULT_OBASE,
	BC_RESULT_SCALE,

} BcResultType;

typedef union BcResultData {
	BcNum n;
	BcVec v;
	BcId id;
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
#if BC_ENABLE_REFERENCES
	BC_TYPE_REF,
#endif // BC_ENABLE_REFERENCES
} BcType;

void bc_func_init(BcFunc *f, const char* name);
BcStatus bc_func_insert(BcFunc *f, char *name, BcType type, size_t line);
void bc_func_reset(BcFunc *f);
void bc_func_free(void *func);

void bc_array_init(BcVec *a, bool nums);
void bc_array_copy(BcVec *d, const BcVec *s);

void bc_string_free(void *string);
void bc_id_free(void *id);
void bc_result_copy(BcResult *d, BcResult *src);
void bc_result_free(void *result);

void bc_array_expand(BcVec *a, size_t len);
int bc_id_cmp(const BcId *e1, const BcId *e2);

#ifndef NDEBUG
extern const char bc_inst_chars[];
#endif // NDEBUG

extern const char bc_func_main[];
extern const char bc_func_read[];

#endif // BC_LANG_H
