/*
 * Copyright 2018 Contributors
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.

 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.

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
 *******************************************************************************
 *
 * A special license exemption is granted to the Toybox project and to the
 * Android Open Source Project to use this source under the following BSD
 * 0-Clause License:
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
 *******************************************************************************
 *
 * Definitions for all of bc.
 *
 */

#ifndef BC_H
#define BC_H

#include <stdint.h>
#include <stdlib.h>

#define BC_FLAG_WARN (1<<0)
#define BC_FLAG_VERSION (1<<1)
#define BC_FLAG_STANDARD (1<<2)
#define BC_FLAG_QUIET (1<<3)
#define BC_FLAG_MATHLIB (1<<4)
#define BC_FLAG_INTERACTIVE (1<<5)
#define BC_FLAG_HELP (1<<6)
#define BC_FLAG_CODE (1<<7)

typedef void (*BcFreeFunc)(void*);

#define BC_INVALID_IDX ((size_t) -1)

#define BC_BASE_MAX_DEF (99)
#define BC_DIM_MAX_DEF (2048)
#define BC_SCALE_MAX_DEF (99)
#define BC_STRING_MAX_DEF (1024)

typedef enum BcStatus {

  BC_STATUS_SUCCESS,

  BC_STATUS_MALLOC_FAIL,
  BC_STATUS_IO_ERR,

  BC_STATUS_INVALID_PARAM,

  BC_STATUS_INVALID_OPTION,

  BC_STATUS_NO_LIMIT,
  BC_STATUS_INVALID_LIMIT,

  BC_STATUS_VECO_OUT_OF_BOUNDS,
  BC_STATUS_VECO_ITEM_EXISTS,

  BC_STATUS_LEX_INVALID_TOKEN,
  BC_STATUS_LEX_NO_STRING_END,
  BC_STATUS_LEX_NO_COMMENT_END,
  BC_STATUS_LEX_EOF,

  BC_STATUS_PARSE_INVALID_TOKEN,
  BC_STATUS_PARSE_INVALID_EXPR,
  BC_STATUS_PARSE_INVALID_PRINT,
  BC_STATUS_PARSE_INVALID_FUNC,
  BC_STATUS_PARSE_INVALID_ASSIGN,
  BC_STATUS_PARSE_NO_AUTO,
  BC_STATUS_PARSE_LIMITS,
  BC_STATUS_PARSE_QUIT,
  BC_STATUS_PARSE_MISMATCH_NUM_FUNCS,
  BC_STATUS_PARSE_EOF,
  BC_STATUS_PARSE_BUG,

  BC_STATUS_VM_FILE_ERR,
  BC_STATUS_VM_DIVIDE_BY_ZERO,
  BC_STATUS_VM_NEG_SQRT,
  BC_STATUS_VM_MISMATCHED_PARAMS,
  BC_STATUS_VM_UNDEFINED_FUNC,
  BC_STATUS_VM_UNDEFINED_VAR,
  BC_STATUS_VM_UNDEFINED_ARRAY,
  BC_STATUS_VM_FILE_NOT_EXECUTABLE,
  BC_STATUS_VM_SIGACTION_FAIL,
  BC_STATUS_VM_INVALID_SCALE,
  BC_STATUS_VM_INVALID_IBASE,
  BC_STATUS_VM_INVALID_OBASE,
  BC_STATUS_VM_INVALID_STMT,
  BC_STATUS_VM_INVALID_EXPR,
  BC_STATUS_VM_INVALID_STRING,
  BC_STATUS_VM_STRING_LEN,
  BC_STATUS_VM_INVALID_NAME,
  BC_STATUS_VM_ARRAY_LENGTH,
  BC_STATUS_VM_INVALID_TEMP,
  BC_STATUS_VM_INVALID_READ_EXPR,
  BC_STATUS_VM_PRINT_ERR,
  BC_STATUS_VM_HALT,

  BC_STATUS_POSIX_NAME_LEN,
  BC_STATUS_POSIX_SCRIPT_COMMENT,
  BC_STATUS_POSIX_INVALID_KEYWORD,
  BC_STATUS_POSIX_RETURN_PARENS,
  BC_STATUS_POSIX_BOOL_OPS,
  BC_STATUS_POSIX_REL_OUTSIDE,
  BC_STATUS_POSIX_MULTIPLE_REL,
  BC_STATUS_POSIX_MISSING_FOR_INIT,
  BC_STATUS_POSIX_MISSING_FOR_COND,
  BC_STATUS_POSIX_MISSING_FOR_UPDATE,
  BC_STATUS_POSIX_FUNC_HEADER_LEFT_BRACE,

} BcStatus;

BcStatus bc_exec(unsigned int flags, unsigned int filec, const char *filev[]);

void bc_error(BcStatus status);
void bc_error_file(BcStatus status, const char* file, uint32_t line);

BcStatus bc_posix_error(BcStatus status, const char* file,
                        uint32_t line, const char* msg);

extern long bc_code;
extern long bc_interactive;
extern long bc_std;
extern long bc_warn;

extern long bc_signal;

extern const char* const bc_mathlib;
extern const char* const bc_version;

#endif // BC_H
