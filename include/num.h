/*
 * *****************************************************************************
 *
 * Copyright (c) 2018-2019 Gavin D. Howard and contributors.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
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
#include <stdint.h>
#include <stdio.h>

#include <sys/types.h>

#include <status.h>

#ifndef BC_ENABLE_EXTRA_MATH
#define BC_ENABLE_EXTRA_MATH (1)
#endif // BC_ENABLE_EXTRA_MATH

#define BC_BASE (10)

#if LONG_BIT >= 64

typedef int_least32_t BcDig;

#define BC_BASE_DIG (1000000000)
#define BC_BASE_POWER (9)

#elif LONG_BIT >= 32

typedef int_least16_t BcDig;

#define BC_BASE_DIG (10000)
#define BC_BASE_POWER (4)

#else

typedef int_least8_t BcDig;

#if LONG_BIT >= 16

#define BC_BASE_DIG (100)
#define BC_BASE_POWER (2)

#elif LONG_BIT >= 8

#define BC_BASE_DIG (10)
#define BC_BASE_POWER (1)

#else // LONG_BIT >= 8

#error long must be at least 8 bits

#endif // LONG_BIT >= 16
#endif // LONG_BIT >= 64

typedef struct BcNum {
	BcDig *restrict num;
	size_t rdx;
	size_t scale;
	size_t len;
	size_t cap;
	bool neg;
} BcNum;

#define BC_NUM_MIN_BASE ((unsigned long) 2)
#define BC_NUM_MAX_POSIX_IBASE ((unsigned long) 16)
#define BC_NUM_MAX_IBASE ((unsigned long) 36)
// This is the max base allowed by bc_num_parseChar().
#define BC_NUM_MAX_LBASE ('Z' + BC_BASE + 1)
#define BC_NUM_DEF_SIZE (2)
#define BC_NUM_PRINT_WIDTH (69)

#ifndef BC_NUM_KARATSUBA_LEN
#define BC_NUM_KARATSUBA_LEN (128)
#elif BC_NUM_KARATSUBA_LEN < 16
#error BC_NUM_KARATSUBA_LEN must be at least 16
#endif // BC_NUM_KARATSUBA_LEN

// A crude, but always big enough, calculation of
// the size required for ibase and obase BcNum's.
#define BC_NUM_LONG_LOG10 ((CHAR_BIT * sizeof(unsigned long) + 1) / 2 + 1)

#define BC_NUM_NONZERO(n) ((n)->len)
#define BC_NUM_ZERO(n) (!BC_NUM_NONZERO(n))
#define BC_NUM_SHREQ(a) ((a)->len)

#define BC_NUM_NUM_LETTER(c) ((c) - 'A' + BC_BASE)

#define BC_NUM_KARATSUBA_ALLOCS (6)

#define BC_NUM_SSIZE_MIN (~SSIZE_MAX)

#define BC_NUM_ROUND_POW(s) ((s) + (BC_BASE_POWER - 1))
#define BC_NUM_RDX(s) (BC_NUM_ROUND_POW(s) / BC_BASE_POWER)

#define BC_NUM_SIZE(n) ((n) * sizeof(BcDig))

#if BC_DEBUG_CODE
#define BC_NUM_PRINT(x) fprintf(stderr, "%s = %lu\n", #x, (unsigned long)(x))
#endif // BC_DEBUG_CODE

typedef BcStatus (*BcNumBinaryOp)(BcNum*, BcNum*, BcNum*, size_t);
typedef size_t (*BcNumBinaryOpReq)(BcNum*, BcNum*, size_t);
typedef void (*BcNumDigitOp)(size_t, size_t, bool);
typedef BcStatus (*BcNumShiftAddOp)(BcDig*, const BcDig*, size_t);

void bc_num_init(BcNum *restrict n, size_t req);
void bc_num_setup(BcNum *restrict n, BcDig *restrict num, size_t cap);
void bc_num_copy(BcNum *d, const BcNum *s);
void bc_num_createCopy(BcNum *d, const BcNum *s);
void bc_num_createFromUlong(BcNum *n, unsigned long val);
void bc_num_free(void *num);

size_t bc_num_scale(const BcNum *restrict n);
size_t bc_num_len(const BcNum *restrict n);

BcStatus bc_num_ulong(const BcNum *restrict n, unsigned long *result);
void bc_num_ulong2num(BcNum *restrict n, unsigned long val);

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
ssize_t bc_num_cmpZero(const BcNum *n);

BcStatus bc_num_parse(BcNum *restrict n, const char *restrict val,
                      BcNum *restrict base, size_t base_t, bool letter);
BcStatus bc_num_print(BcNum *restrict n, BcNum *restrict base,
                      size_t base_t, bool newline);
#if DC_ENABLED
BcStatus bc_num_stream(BcNum *restrict n, BcNum *restrict base);
#endif // DC_ENABLED

extern const char bc_num_hex_digits[];
extern const unsigned long bc_num_pow10[BC_BASE_POWER + 1];

#endif // BC_NUM_H
