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

#include <stdbool.h>
#include <stdint.h>

#include <bc.h>
#include <lang.h>

#define BC_PROGRAM_BUF_SIZE (1024)

typedef struct BcProgram {

  BcVec ip_stack;

  size_t scale;

  BcNum ibase;
  size_t ibase_t;
  BcNum obase;
  size_t obase_t;

  long base_max;
  long dim_max;
  long scale_max;
  long string_max;

  BcVec expr_stack;

  BcVec stack;

  BcVec funcs;

  BcVecO func_map;

  BcVec vars;

  BcVecO var_map;

  BcVec arrays;

  BcVecO array_map;

  BcVec strings;

  BcVec constants;

  const char *file;

  BcNum last;

  BcNum zero;
  BcNum one;

  char *num_buf;
  size_t buf_size;

} BcProgram;

#define BC_PROGRAM_CHECK_STACK(p) ((p)->stack.len > 1)
#define BC_PROGRAM_CHECK_EXPR_STACK(p, l) ((p)->expr_stack.len >= (l))

#define BC_PROGRAM_MAIN_FUNC (0)
#define BC_PROGRAM_READ_FUNC (1)

#define BC_PROGRAM_SEARCH_VAR (1<<0)
#define BC_PROGRAM_SEARCH_ARRAY_ONLY (1<<1)

typedef BcStatus (*BcProgramExecFunc)(BcProgram*);
typedef unsigned long (*BcProgramBuiltInFunc)(BcNum*);
typedef void (*BcNumInitFunc)(BcNum*);

BcStatus bc_program_init(BcProgram *p);
void bc_program_limits(BcProgram *p);
BcStatus bc_program_func_add(BcProgram *p, char *name, size_t *idx);
BcStatus bc_program_var_add(BcProgram *p, char *name, size_t *idx);
BcStatus bc_program_array_add(BcProgram *p, char *name, size_t *idx);
BcStatus bc_program_exec(BcProgram *p);
BcStatus bc_program_print(BcProgram *p);
void bc_program_free(BcProgram *program);

extern const char *bc_program_byte_fmt;
extern const BcNumBinaryFunc bc_program_math_ops[];
extern const char *bc_program_stdin_name;
extern const char *bc_program_ready_prompt;
extern const char *bc_program_sigint_msg;

#endif // BC_PROGRAM_H
