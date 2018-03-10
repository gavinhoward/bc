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

typedef signed char BcDigit;

typedef struct BcNum {

  BcDigit *num;
  size_t rdx;
  size_t len;
  size_t cap;
  bool neg;

} BcNum;

#define BC_NUM_MIN_BASE (2)

#define BC_NUM_MAX_INPUT_BASE (16)

#define BC_NUM_MAX_OUTPUT_BASE (99)

#define BC_NUM_DEF_SIZE (16)

#define BC_NUM_FROM_CHAR(c) ((c) -'0')

#define BC_NUM_TO_CHAR(n) ((n) + '0')

#define BC_NUM_PRINT_WIDTH (69)

#define BC_NUM_ZERO(n) (!(n)->len)

#define BC_NUM_ONE(n) ((n)->len == 1 && (n)->rdx == 0 && (n)->num[0] == 1)

#define BC_NUM_POS_ONE(n) (BC_NUM_ONE(n) && !(n)->neg)
#define BC_NUM_NEG_ONE(n) (BC_NUM_ONE(n) && (n)->neg)

typedef BcStatus (*BcNumUnaryFunc)(BcNum*, BcNum*, size_t);
typedef BcStatus (*BcNumBinaryFunc)(BcNum*, BcNum*, BcNum*, size_t);

typedef BcStatus (*BcNumDigitFunc)(unsigned long, size_t, size_t*, FILE*);

BcStatus bc_num_init(BcNum *n, size_t request);

BcStatus bc_num_expand(BcNum *n, size_t request);

void bc_num_free(void *num);

BcStatus bc_num_copy(void *dest, void *src);

BcStatus bc_num_parse(BcNum *n, const char *val, BcNum *base, size_t base_t);

BcStatus bc_num_print(BcNum *n, BcNum *base, size_t base_t, bool newline);
BcStatus bc_num_fprint(BcNum *n, BcNum *base, size_t base_t,
                       bool newline, FILE *f);

BcStatus bc_num_long(BcNum *n, long *result);
BcStatus bc_num_ulong(BcNum *n, unsigned long *result);

BcStatus bc_num_long2num(BcNum *n, long val);
BcStatus bc_num_ulong2num(BcNum *n, unsigned long val);

BcStatus bc_num_truncate(BcNum *n);

BcStatus bc_num_add(BcNum *a, BcNum *b, BcNum *result, size_t scale);
BcStatus bc_num_sub(BcNum *a, BcNum *b, BcNum *result, size_t scale);
BcStatus bc_num_mul(BcNum *a, BcNum *b, BcNum *result, size_t scale);
BcStatus bc_num_div(BcNum *a, BcNum *b, BcNum *result, size_t scale);
BcStatus bc_num_mod(BcNum *a, BcNum *b, BcNum *result, size_t scale);
BcStatus bc_num_pow(BcNum *a, BcNum *b, BcNum *result, size_t scale);

BcStatus bc_num_sqrt(BcNum *a, BcNum *result, size_t scale);

int bc_num_compare(BcNum *a, BcNum *b);

void bc_num_zero(BcNum *n);
void bc_num_one(BcNum *n);
void bc_num_ten(BcNum *n);

#endif // BC_NUM_H
