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
 * A special license exemption is granted to the Toybox project to use this
 * source under the following BSD 0-Clause License:
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
 * Definitions for the num type.
 *
 */

#ifndef BC_NUM_H
#define BC_NUM_H

// For C++ compatibility.
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <bc/bc.h>

#define BC_NUM_MIN_BASE (2)

#define BC_NUM_MAX_INPUT_BASE (16)

#define BC_NUM_MAX_OUTPUT_BASE (99)

#define BC_NUM_DEF_SIZE (16)

#define BC_NUM_FROM_CHAR(c) ((c) -'0')

#define BC_NUM_TO_CHAR(n) ((n) + '0')

#define BC_NUM_SCALE(n) ((n)->len - (n)->radix)

typedef struct BcNum {

  char* num;
  size_t radix;
  size_t len;
  size_t unused;
  bool neg;

} BcNum;

typedef BcStatus (*BcUnaryFunc)(BcNum* a, BcNum* res, size_t scale);
typedef BcStatus (*BcBinaryFunc)(BcNum* a, BcNum* b, BcNum* res, size_t scale);

BcStatus bc_num_construct(BcNum* num, size_t request);

BcStatus bc_num_expand(BcNum* num, size_t request);

void bc_num_destruct(BcNum* num);

BcStatus bc_num_copy(BcNum* dest, BcNum* src);

BcStatus bc_num_parse(BcNum* num, const char* val,
                       size_t base, size_t scale);

BcStatus bc_num_print(BcNum* num, size_t base);
BcStatus bc_num_fprint(BcNum* num, size_t base, FILE* f);

BcStatus bc_num_add(BcNum* a, BcNum* b, BcNum* result, size_t scale);
BcStatus bc_num_sub(BcNum* a, BcNum* b, BcNum* result, size_t scale);
BcStatus bc_num_mul(BcNum* a, BcNum* b, BcNum* result, size_t scale);
BcStatus bc_num_div(BcNum* a, BcNum* b, BcNum* result, size_t scale);
BcStatus bc_num_mod(BcNum* a, BcNum* b, BcNum* result, size_t scale);
BcStatus bc_num_pow(BcNum* a, BcNum* b, BcNum* result, size_t scale);

BcStatus bc_num_sqrt(BcNum* a, BcNum* result, size_t scale);

bool bc_num_isInteger(BcNum* num);

int bc_num_compare(BcNum* a, BcNum* b);

#ifdef __cplusplus
}
#endif

#endif // BC_NUM_H
