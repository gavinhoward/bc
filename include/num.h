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

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include <sys/types.h>

#include <status.h>

typedef signed char BcDig;

typedef struct BcNum {
	BcDig *restrict num;
	size_t rdx;
	size_t len;
	size_t cap;
	bool neg;
} BcNum;

#define BC_NUM_MIN_BASE ((unsigned long) 2)
#define BC_NUM_MAX_POSIX_IBASE ((unsigned long) 16)
#define BC_NUM_MAX_IBASE ((unsigned long) 36)
// This is the max base allowed by bc_num_parseChar().
#define BC_NUM_MAX_LBASE ('Z' + 10 + 1)
#define BC_NUM_DEF_SIZE (16)
#define BC_NUM_PRINT_WIDTH (69)

#ifndef BC_NUM_KARATSUBA_LEN
#define BC_NUM_KARATSUBA_LEN (32)
#elif BC_NUM_KARATSUBA_LEN < 2
#error BC_NUM_KARATSUBA_LEN must be at least 2
#endif // BC_NUM_KARATSUBA_LEN

// A crude, but always big enough, calculation of
// the size required for ibase and obase BcNum's.
#define BC_NUM_LONG_LOG10 ((CHAR_BIT * sizeof(unsigned long) + 1) / 2 + 1)

#define BC_NUM_NEG(n, neg) ((((ssize_t) (n)) ^ -((ssize_t) (neg))) + (neg))
#define BC_NUM_NONZERO(n) ((n)->len)
#define BC_NUM_ZERO(n) (!BC_NUM_NONZERO(n))
#define BC_NUM_ONE(n) ((n)->len == 1 && (n)->rdx == 0 && (n)->num[0] == 1)
#define BC_NUM_INT(n) ((n)->len - (n)->rdx)
#define BC_NUM_CMP_ZERO(a) (BC_NUM_NEG((a)->len != 0, (a)->neg))
#define BC_NUM_PREQ(a, b) ((a)->len + (b)->len + 1)
#define BC_NUM_SHREQ(a) ((a)->len)

#define BC_NUM_NUM_LETTER(c) ((c) - 'A' + 10)

typedef BcStatus (*BcNumBinaryOp)(BcNum*, BcNum*, BcNum*, size_t);
typedef size_t (*BcNumBinaryOpReq)(BcNum*, BcNum*, size_t);
typedef void (*BcNumDigitOp)(size_t, size_t, bool);

void bc_num_init(BcNum *n, size_t req);
void bc_num_setup(BcNum *n, BcDig *num, size_t cap);
void bc_num_expand(BcNum *n, size_t req);
void bc_num_copy(BcNum *d, const BcNum *s);
void bc_num_createCopy(BcNum *d, const BcNum *s);
void bc_num_createFromUlong(BcNum *n, unsigned long val);
void bc_num_free(void *num);

BcStatus bc_num_ulong(const BcNum *n, unsigned long *result);
void bc_num_ulong2num(BcNum *n, unsigned long val);

BcStatus bc_num_add(BcNum *a, BcNum *b, BcNum *c, size_t scale);
BcStatus bc_num_sub(BcNum *a, BcNum *b, BcNum *c, size_t scale);
BcStatus bc_num_mul(BcNum *a, BcNum *b, BcNum *c, size_t scale);
BcStatus bc_num_div(BcNum *a, BcNum *b, BcNum *c, size_t scale);
BcStatus bc_num_mod(BcNum *a, BcNum *b, BcNum *c, size_t scale);
BcStatus bc_num_pow(BcNum *a, BcNum *b, BcNum *c, size_t scale);
#if BC_ENABLE_EXTRA_MATH
BcStatus bc_num_places(BcNum *a, BcNum *b, BcNum *c, size_t scale);
BcStatus bc_num_lshift(BcNum *a, BcNum *b, BcNum *c, size_t scale);
BcStatus bc_num_rshift(BcNum *a, BcNum *b, BcNum *c, size_t scale);
#endif // BC_ENABLE_EXTRA_MATH
BcStatus bc_num_sqrt(BcNum *restrict a, BcNum *restrict b, size_t scale);
BcStatus bc_num_divmod(BcNum *a, BcNum *b, BcNum *c, BcNum *d, size_t scale);

size_t bc_num_addReq(BcNum *a, BcNum *b, size_t scale);

size_t bc_num_mulReq(BcNum *a, BcNum *b, size_t scale);
size_t bc_num_powReq(BcNum *a, BcNum *b, size_t scale);
#if BC_ENABLE_EXTRA_MATH
size_t bc_num_shiftReq(BcNum *a, BcNum *b, size_t scale);
#endif // BC_ENABLE_EXTRA_MATH

void bc_num_truncate(BcNum *restrict n, size_t places);
ssize_t bc_num_cmp(const BcNum *a, const BcNum *b);

#if DC_ENABLED
BcStatus bc_num_modexp(BcNum *a, BcNum *b, BcNum *c, BcNum *restrict d);
#endif // DC_ENABLED

void bc_num_one(BcNum *restrict n);
void bc_num_ten(BcNum *restrict n);

BcStatus bc_num_parse(BcNum *restrict n, const char *restrict val,
                      BcNum *restrict base, size_t base_t, bool letter);
BcStatus bc_num_print(BcNum *restrict n, BcNum *restrict base,
                      size_t base_t, bool newline);
#if DC_ENABLED
BcStatus bc_num_stream(BcNum *restrict n, BcNum *restrict base);
#endif // DC_ENABLED

extern const char bc_num_hex_digits[];

#endif // BC_NUM_H
