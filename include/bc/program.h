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
 * Definitions for bc programs.
 *
 */

#ifndef BC_PROGRAM_H
#define BC_PROGRAM_H

#include <stdbool.h>
#include <stdint.h>

#include <bc/bc.h>
#include <bc/data.h>

#include <arbprec/arbprec.h>

#define BC_PROGRAM_BUF_SIZE (1024)

typedef BcStatus (*BcMathOpFunc)(BcNum*, BcNum*, BcNum*, size_t);

typedef struct BcProgram {

  BcVec ip_stack;

  size_t scale;
  size_t ibase;
  size_t obase;

  long base_max;
  long dim_max;
  long scale_max;
  long string_max;

  BcVec expr_stack;

  BcVec stack;

  BcVec locals;

  BcVec temps;

  BcVec funcs;

  BcVecO func_map;

  BcVec vars;

  BcVecO var_map;

  BcVec arrays;

  BcVecO array_map;

  BcVec strings;

  BcVec constants;

  const char* file;

  BcNum last;

  BcNum zero;
  BcNum one;

  char* num_buf;
  size_t buf_size;

} BcProgram;

BcStatus bc_program_init(BcProgram* p);
void bc_program_limits(BcProgram* p);
BcStatus bc_program_func_add(BcProgram* p, char* name, size_t* idx);
BcStatus bc_program_var_add(BcProgram* p, char *name, size_t *idx);
BcStatus bc_program_array_add(BcProgram* p, char *name, size_t *idx);
BcStatus bc_program_exec(BcProgram* p);
void bc_program_printCode(BcProgram* p);
void bc_program_free(BcProgram* program);

#endif // BC_PROGRAM_H
