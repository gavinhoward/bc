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

#ifndef BC_DATA_H
#define BC_DATA_H

#include <stdbool.h>

#include <bc.h>
#include <vector.h>
#include <num.h>

#define BC_PROGRAM_MAX_STMTS (128)

#define BC_PROGRAM_DEF_SIZE (16)

typedef enum BcExprType {

  BC_EXPR_INC_PRE,
  BC_EXPR_DEC_PRE,

  BC_EXPR_INC_POST,
  BC_EXPR_DEC_POST,

  BC_EXPR_NEGATE,

  BC_EXPR_POWER,

  BC_EXPR_MULTIPLY,
  BC_EXPR_DIVIDE,
  BC_EXPR_MODULUS,

  BC_EXPR_PLUS,
  BC_EXPR_MINUS,

  BC_EXPR_REL_EQUAL,
  BC_EXPR_REL_LESS_EQ,
  BC_EXPR_REL_GREATER_EQ,
  BC_EXPR_REL_NOT_EQ,
  BC_EXPR_REL_LESS,
  BC_EXPR_REL_GREATER,

  BC_EXPR_BOOL_NOT,

  BC_EXPR_BOOL_OR,
  BC_EXPR_BOOL_AND,

  BC_EXPR_ASSIGN_POWER,
  BC_EXPR_ASSIGN_MULTIPLY,
  BC_EXPR_ASSIGN_DIVIDE,
  BC_EXPR_ASSIGN_MODULUS,
  BC_EXPR_ASSIGN_PLUS,
  BC_EXPR_ASSIGN_MINUS,
  BC_EXPR_ASSIGN,

  BC_EXPR_NUMBER,
  BC_EXPR_VAR,
  BC_EXPR_ARRAY_ELEM,

  BC_EXPR_FUNC_CALL,

  BC_EXPR_SCALE_FUNC,
  BC_EXPR_SCALE,
  BC_EXPR_IBASE,
  BC_EXPR_OBASE,
  BC_EXPR_LAST,
  BC_EXPR_LENGTH,
  BC_EXPR_READ,
  BC_EXPR_SQRT,

  BC_EXPR_PRINT,

} BcExprType;

typedef struct BcEntry {

  char *name;
  size_t idx;

} BcEntry;

typedef union BcLocal {

  BcNum num;
  BcVec array;

} BcLocal;

typedef struct BcAuto {

  char *name;
  bool var;

} BcAuto;

typedef struct BcFunc {

  BcVec code;

  BcVec labels;

  BcVec params;

  BcVec autos;

} BcFunc;

typedef BcNum BcVar;

typedef BcVec BcArray;

typedef enum BcResultType {

  BC_RESULT_INTERMEDIATE,

  BC_RESULT_CONSTANT,

  BC_RESULT_AUTO_VAR,
  BC_RESULT_AUTO_ARRAY,

  BC_RESULT_LOCAL_VAR,
  BC_RESULT_LOCAL_ARRAY,

  BC_RESULT_VAR,
  BC_RESULT_ARRAY_ELEM,

  BC_RESULT_SCALE,
  BC_RESULT_LAST,
  BC_RESULT_IBASE,
  BC_RESULT_OBASE,

  BC_RESULT_ONE,

} BcResultType;

typedef struct BcResult {

  BcResultType type;

  union {

    BcLocal local;

    struct {

      char *name;
      size_t idx;

    } id;

  } data;

} BcResult;

typedef struct BcInstPtr {

  size_t func;
  size_t idx;
  size_t len;

} BcInstPtr;

typedef BcStatus (*BcDataInitFunc)(void*);

// ** Exclude start. **
BcStatus bc_func_init(BcFunc *func);
BcStatus bc_func_insert(BcFunc *func, char *name, bool var, BcVec *vec);
void bc_func_free(void *func);

BcStatus bc_var_init(void *var);
void bc_var_free(void *var);

BcStatus bc_array_init(void *array);
BcStatus bc_array_copy(void *dest, void *src);
BcStatus bc_array_zero(BcArray *a);
BcStatus bc_array_expand(BcArray *a, size_t len);
void bc_array_free(void *array);

void bc_string_free(void *string);

int bc_entry_cmp(void *entry1, void*entry2);
void bc_entry_free(void *entry);

void bc_result_free(void *result);

void bc_constant_free(void *constant);
// ** Exclude end. **

void bc_auto_init(void *auto1, char *name, bool var);
void bc_auto_free(void *auto1);

extern const char *bc_lang_func_main;
extern const char *bc_lang_func_read;

#endif // BC_DATA_H
