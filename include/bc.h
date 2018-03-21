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
 * Definitions for all of bc.
 *
 */

#ifndef BC_H
#define BC_H

#include <stdint.h>
#include <stdlib.h>
#include <limits.h>

#define BC_FLAG_WARN (1<<0)
#define BC_FLAG_STANDARD (1<<1)
#define BC_FLAG_QUIET (1<<2)
#define BC_FLAG_MATHLIB (1<<3)
#define BC_FLAG_INTERACTIVE (1<<4)
#define BC_FLAG_CODE (1<<5)

#define BC_MAX(a, b) ((a) > (b) ? (a) : (b))

#define BC_MIN(a, b) ((a) < (b) ? (a) : (b))

#define BC_INVALID_IDX ((size_t) -1)

#define BC_BASE_MAX_DEF (999)
#define BC_DIM_MAX_DEF (INT_MAX)
#define BC_SCALE_MAX_DEF (LONG_MAX)
#define BC_STRING_MAX_DEF (INT_MAX)

#define BC_ERR_IDX_BC (0)
#define BC_ERR_IDX_VEC (1)
#define BC_ERR_IDX_LEX (2)
#define BC_ERR_IDX_PARSE (3)
#define BC_ERR_IDX_MATH (4)
#define BC_ERR_IDX_EXEC (5)
#define BC_ERR_IDX_POSIX (6)

typedef enum BcStatus {

  BC_STATUS_SUCCESS,

  BC_STATUS_QUIT,
  BC_STATUS_LIMITS,

  BC_STATUS_MALLOC_FAIL,
  BC_STATUS_IO_ERR,

  BC_STATUS_INVALID_ARG,

  BC_STATUS_NO_LIMIT,
  BC_STATUS_BAD_LIMIT,

  BC_STATUS_VEC_OUT_OF_BOUNDS,
  BC_STATUS_VEC_ITEM_EXISTS,

  BC_STATUS_LEX_BAD_CHARACTER,
  BC_STATUS_LEX_NO_STRING_END,
  BC_STATUS_LEX_NO_COMMENT_END,
  BC_STATUS_LEX_EOF,

  BC_STATUS_PARSE_BAD_TOKEN,
  BC_STATUS_PARSE_BAD_EXPR,
  BC_STATUS_PARSE_BAD_PRINT,
  BC_STATUS_PARSE_BAD_FUNC,
  BC_STATUS_PARSE_BAD_ASSIGN,
  BC_STATUS_PARSE_NO_AUTO,
  BC_STATUS_PARSE_MISMATCH_NUM_FUNCS,
  BC_STATUS_PARSE_DUPLICATE_LOCAL,
  BC_STATUS_PARSE_BUG,

  BC_STATUS_MATH_NEGATIVE,
  BC_STATUS_MATH_NON_INTEGER,
  BC_STATUS_MATH_OVERFLOW,
  BC_STATUS_MATH_DIVIDE_BY_ZERO,
  BC_STATUS_MATH_NEG_SQRT,
  BC_STATUS_MATH_BAD_STRING,
  BC_STATUS_MATH_BAD_TRUNCATE,

  BC_STATUS_EXEC_FILE_ERR,
  BC_STATUS_EXEC_MISMATCHED_PARAMS,
  BC_STATUS_EXEC_UNDEFINED_FUNC,
  BC_STATUS_EXEC_UNDEFINED_VAR,
  BC_STATUS_EXEC_UNDEFINED_ARRAY,
  BC_STATUS_EXEC_FILE_NOT_EXECUTABLE,
  BC_STATUS_EXEC_SIGACTION_FAIL,
  BC_STATUS_EXEC_BAD_SCALE,
  BC_STATUS_EXEC_BAD_IBASE,
  BC_STATUS_EXEC_BAD_OBASE,
  BC_STATUS_EXEC_BAD_STMT,
  BC_STATUS_EXEC_BAD_EXPR,
  BC_STATUS_EXEC_BAD_STRING,
  BC_STATUS_EXEC_STRING_LEN,
  BC_STATUS_EXEC_ARRAY_LEN,
  BC_STATUS_EXEC_BAD_READ_EXPR,
  BC_STATUS_EXEC_RECURSIVE_READ,
  BC_STATUS_EXEC_BAD_CONSTANT,
  BC_STATUS_EXEC_BAD_RETURN,
  BC_STATUS_EXEC_BAD_LABEL,
  BC_STATUS_EXEC_BAD_TYPE,
  BC_STATUS_EXEC_BAD_STACK,

  BC_STATUS_POSIX_NAME_LEN,
  BC_STATUS_POSIX_SCRIPT_COMMENT,
  BC_STATUS_POSIX_BAD_KEYWORD,
  BC_STATUS_POSIX_DOT_LAST,
  BC_STATUS_POSIX_RETURN_PARENS,
  BC_STATUS_POSIX_BOOL_OPS,
  BC_STATUS_POSIX_REL_OUTSIDE,
  BC_STATUS_POSIX_MULTIPLE_REL,
  BC_STATUS_POSIX_MISSING_FOR_INIT,
  BC_STATUS_POSIX_MISSING_FOR_COND,
  BC_STATUS_POSIX_MISSING_FOR_UPDATE,
  BC_STATUS_POSIX_FUNC_HEADER_LEFT_BRACE,

  // This is last so I can remove it for toybox.
  BC_STATUS_INVALID_OPTION,

} BcStatus;

// ** Exclude start. **
typedef struct BcGlobals {

  long bc_interactive;
  long bc_std;
  long bc_warn;

  long bc_sig;

} BcGlobals;

BcStatus bc_main(unsigned int flags, unsigned int filec, char *filev[]);
// ** Exclude end. **

BcStatus bc_error(BcStatus st);
BcStatus bc_error_file(BcStatus st, const char *file, size_t line);

BcStatus bc_posix_error(BcStatus st, const char *file,
                        size_t line, const char *msg);

extern BcGlobals bcg;

extern const char bc_lib[];
extern const char *bc_lib_name;

extern const char *bc_header;
extern const char *bc_err_types[];
extern const uint8_t bc_err_type_indices[];
extern const char *bc_err_descs[];

#endif // BC_H
