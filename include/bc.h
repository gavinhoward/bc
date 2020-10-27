/*
 * *****************************************************************************
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2018-2020 Gavin D. Howard and contributors.
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
 * The public header for libbc.
 *
 */

#ifndef LIBBC_H
#define LIBBC_H

#include <stdbool.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>
#include <sys/types.h>

typedef enum BcError {

	BC_ERROR_SUCCESS,

	BC_ERROR_SIGNAL,

	BC_ERROR_MATH_NEGATIVE,
	BC_ERROR_MATH_NON_INTEGER,
	BC_ERROR_MATH_OVERFLOW,
	BC_ERROR_MATH_DIVIDE_BY_ZERO,

	BC_ERROR_PARSE_INVALID_NUM,

	BC_ERROR_FATAL_ALLOC_ERR,
	BC_ERROR_FATAL_UNKNOWN_ERR,

	BC_ERROR_NELEMS,

} BcError;

#define BC_ERROR_IDX_MATH (0)
#define BC_ERROR_IDX_FATAL (1)
#define BC_ERROR_IDX_NELEMS (2)

// For some reason, LONG_BIT is not defined in some versions of gcc.
// I define it here to the minimum accepted value in the POSIX standard.
#ifndef LONG_BIT
#define LONG_BIT (32)
#endif // LONG_BIT

#ifndef BC_LONG_BIT
#define BC_LONG_BIT LONG_BIT
#endif // BC_LONG_BIT

#if BC_LONG_BIT > LONG_BIT
#error BC_LONG_BIT cannot be greater than LONG_BIT
#endif // BC_LONG_BIT > LONG_BIT

#if BC_LONG_BIT >= 64

typedef int_least32_t BcDig;
typedef uint64_t BcBigDig;

typedef uint64_t BcRandInt;

#elif BC_LONG_BIT >= 32

typedef int_least16_t BcDig;
typedef uint32_t BcBigDig;

typedef uint32_t BcRandInt;

#else

#error BC_LONG_BIT must be at least 32

#endif // BC_LONG_BIT >= 64

struct BcNum;
struct BcRNG;

BcError libbc_init(bool abortOnFatal);
void libbc_dtor(void);

void libbc_abortOnFatalError(bool abrt);

void libbc_handleSignal(void);

BcError libbc_num_init(struct BcNum *restrict n);
BcError libbc_num_initReq(struct BcNum *restrict n, size_t req);
BcError libbc_num_copy(struct BcNum *d, const struct BcNum *s);
BcError libbc_num_copy_create(struct BcNum *d, const struct BcNum *s);
void libbc_num_clear(struct BcNum *restrict n);
void libbc_num_free(struct BcNum *num);
void libbc_num_dtor(void *num);

size_t libbc_num_scale(const struct BcNum *restrict n);
size_t libbc_num_len(const struct BcNum *restrict n);

BcError libbc_num_bigdig(const struct BcNum *restrict n, BcBigDig *result);
BcError libbc_num_bigdig2num_create(struct BcNum *n, BcBigDig val);
BcError libbc_num_bigdig2num(struct BcNum *restrict n, BcBigDig val);

BcError libbc_num_add_create(struct BcNum *a, struct BcNum *b,
                             struct BcNum *c, size_t scale);
BcError libbc_num_add(struct BcNum *a, struct BcNum *b,
                      struct BcNum *c, size_t scale);

BcError libbc_num_sub_create(struct BcNum *a, struct BcNum *b,
                             struct BcNum *c, size_t scale);
BcError libbc_num_sub(struct BcNum *a, struct BcNum *b,
                      struct BcNum *c, size_t scale);

BcError libbc_num_mul_create(struct BcNum *a, struct BcNum *b,
                             struct BcNum *c, size_t scale);
BcError libbc_num_mul(struct BcNum *a, struct BcNum *b,
                      struct BcNum *c, size_t scale);

BcError libbc_num_div_create(struct BcNum *a, struct BcNum *b,
                             struct BcNum *c, size_t scale);
BcError libbc_num_div(struct BcNum *a, struct BcNum *b,
                      struct BcNum *c, size_t scale);

BcError libbc_num_mod_create(struct BcNum *a, struct BcNum *b,
                             struct BcNum *c, size_t scale);
BcError libbc_num_mod(struct BcNum *a, struct BcNum *b,
                      struct BcNum *c, size_t scale);

BcError libbc_num_pow_create(struct BcNum *a, struct BcNum *b,
                             struct BcNum *c, size_t scale);
BcError libbc_num_pow(struct BcNum *a, struct BcNum *b,
                      struct BcNum *c, size_t scale);

#if BC_ENABLE_EXTRA_MATH
BcError libbc_num_places_create(struct BcNum *a, struct BcNum *b,
                                struct BcNum *c, size_t scale);
BcError libbc_num_places(struct BcNum *a, struct BcNum *b,
                         struct BcNum *c, size_t scale);

BcError libbc_num_lshift_create(struct BcNum *a, struct BcNum *b,
                                struct BcNum *c, size_t scale);
BcError libbc_num_lshift(struct BcNum *a, struct BcNum *b,
                         struct BcNum *c, size_t scale);

BcError libbc_num_rshift_create(struct BcNum *a, struct BcNum *b,
                                struct BcNum *c, size_t scale);
BcError libbc_num_rshift(struct BcNum *a, struct BcNum *b,
                         struct BcNum *c, size_t scale);
#endif // BC_ENABLE_EXTRA_MATH

BcError libbc_num_sqrt_create(struct BcNum *restrict a,
                              struct BcNum *restrict b, size_t scale);
BcError libbc_num_sqrt(struct BcNum *restrict a,
                       struct BcNum *restrict b, size_t scale);

BcError libbc_num_divmod_create(struct BcNum *a, struct BcNum *b,
                                struct BcNum *c, struct BcNum *d, size_t scale);
BcError libbc_num_divmod(struct BcNum *a, struct BcNum *b, struct BcNum *c,
                         struct BcNum *d, size_t scale);

BcError libbc_num_modexp_create(struct BcNum *a, struct BcNum *b,
                                struct BcNum *c, struct BcNum *restrict d);
BcError libbc_num_modexp(struct BcNum *a, struct BcNum *b, struct BcNum *c,
                         struct BcNum *restrict d);

size_t libbc_num_addReq(const struct BcNum* a, const struct BcNum* b,
                        size_t scale);
size_t libbc_num_mulReq(const struct BcNum *a, const struct BcNum *b,
                        size_t scale);
size_t libbc_num_divReq(const struct BcNum *a, const struct BcNum *b,
                        size_t scale);
size_t libbc_num_powReq(const struct BcNum *a, const struct BcNum *b,
                        size_t scale);

#if BC_ENABLE_EXTRA_MATH
size_t libbc_num_placesReq(const struct BcNum *a, const struct BcNum *b,
                           size_t scale);
#endif // BC_ENABLE_EXTRA_MATH

BcError libbc_num_setScale(struct BcNum *restrict n, size_t scale);
ssize_t libbc_num_cmp(const struct BcNum *a, const struct BcNum *b);

void libbc_num_one(struct BcNum *restrict n);
ssize_t libbc_num_cmpZero(const struct BcNum *n);

BcError libbc_num_parse_create(struct BcNum *restrict n,
                               const char *restrict val, BcBigDig base);
BcError libbc_num_parse(struct BcNum *restrict n, const char *restrict val,
                        BcBigDig base);
BcError libbc_num_string(struct BcNum *restrict n, BcBigDig base, char **str);

#if BC_ENABLE_EXTRA_MATH
BcError libbc_num_irand_create(const struct BcNum *restrict a,
                               struct BcNum *restrict b);
BcError libbc_num_irand(const struct BcNum *restrict a,
                        struct BcNum *restrict b);

BcError libbc_num_frand_create(struct BcNum *restrict b, size_t places);
BcError libbc_num_frand(struct BcNum *restrict b, size_t places);

BcError libbc_num_ifrand_create(struct BcNum *restrict a,
                                struct BcNum *restrict b, size_t places);
BcError libbc_num_ifrand(struct BcNum *restrict a,
                         struct BcNum *restrict b, size_t places);

BcError libbc_num_seedWithNum(const struct BcNum *restrict n);
BcError libbc_num_seedWithUlongs(unsigned long state1, unsigned long state2,
                                 unsigned long inc1, unsigned long inc2);
BcError libbc_num_reseed(void);
BcError libbc_num_seed2num_create(struct BcNum *restrict n);
BcError libbc_num_seed2num(struct BcNum *restrict n);

BcRandInt libbc_rand_int(void);
BcRandInt libbc_rand_bounded(BcRandInt bound);
#endif // BC_ENABLE_EXTRA_MATH

#endif // LIBBC_H
