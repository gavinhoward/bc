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
 * Definitions for the num type.
 *
 */

#ifndef BC_NUM_H
#define BC_NUM_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <bc.h>

#define BC_NUM_MIN_BASE (2)

#define BC_NUM_MAX_INPUT_BASE (16)

#define BC_NUM_MAX_OUTPUT_BASE (99)

#define BC_NUM_DEF_SIZE (16)

#define BC_NUM_FROM_CHAR(c) ((c) -'0')

#define BC_NUM_TO_CHAR(n) ((n) + '0')

#define BC_NUM_SCALE(n) ((n)->len - (n)->rdx)

#define BC_NUM_PRINT_WIDTH (68)

typedef struct BcNum {

  char* num;
  size_t rdx;
  size_t len;
  size_t cap;
  bool neg;

} BcNum;

typedef BcStatus (*BcUnaryFunc)(BcNum* a, BcNum* res, size_t scale);
typedef BcStatus (*BcBinaryFunc)(BcNum* a, BcNum* b, BcNum* res, size_t scale);

BcStatus bc_num_init(BcNum* n, size_t request);

BcStatus bc_num_expand(BcNum* n, size_t request);

void bc_num_free(BcNum* n);

BcStatus bc_num_copy(BcNum* d, BcNum* s);

BcStatus bc_num_parse(BcNum* n, const char* val,
                       size_t base, size_t scale);

BcStatus bc_num_print(BcNum* n, size_t base);
BcStatus bc_num_fprint(BcNum* n, size_t base, FILE* f);

BcStatus bc_num_long(BcNum* n, long* result);
BcStatus bc_num_ulong(BcNum* n, unsigned long* result);

BcStatus bc_num_long2num(BcNum* n, long val);
BcStatus bc_num_ulong2num(BcNum* n, unsigned long val);

BcStatus bc_num_add(BcNum* a, BcNum* b, BcNum* result, size_t scale);
BcStatus bc_num_sub(BcNum* a, BcNum* b, BcNum* result, size_t scale);
BcStatus bc_num_mul(BcNum* a, BcNum* b, BcNum* result, size_t scale);
BcStatus bc_num_div(BcNum* a, BcNum* b, BcNum* result, size_t scale);
BcStatus bc_num_mod(BcNum* a, BcNum* b, BcNum* result, size_t scale);
BcStatus bc_num_pow(BcNum* a, BcNum* b, BcNum* result, size_t scale);

BcStatus bc_num_sqrt(BcNum* a, BcNum* result, size_t scale);

bool bc_num_isInteger(BcNum* num);

int bc_num_compare(BcNum* a, BcNum* b);

#endif // BC_NUM_H
