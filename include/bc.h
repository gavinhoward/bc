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

typedef size_t BcNumber;

typedef struct BcMaybe {

	union {

		BcError err;
		BcNumber num;

	} data;

	bool err;

} BcMaybe;

BcError libbc_init(bool abortOnFatal);
void libbc_dtor(void);

size_t libbc_scale(void);
void libbc_setScale(size_t scale);
size_t libbc_ibase(void);
void libbc_setIbase(size_t ibase);
size_t libbc_obase(void);
void libbc_setObase(size_t obase);

bool libbc_abortOnFatalError(void);
void libbc_setAbortOnFatalError(bool abrt);

void libbc_handleSignal(void);

BcMaybe libbc_num_init(void);
BcMaybe libbc_num_initReq(size_t req);
BcError libbc_num_copy(const BcNumber d, const BcNumber s);
BcMaybe libbc_num_dup(const BcNumber s);
void libbc_num_free(const BcNumber n);

size_t libbc_num_scale(const BcNumber n);
size_t libbc_num_len(const BcNumber n);

BcError libbc_num_bigdig(const BcNumber n, BcBigDig *result);
BcMaybe libbc_num_bigdig2num_create(const BcBigDig val);
BcError libbc_num_bigdig2num(const BcNumber n, const BcBigDig val);

BcMaybe libbc_num_add_create(const BcNumber a, const BcNumber b);
BcError libbc_num_add(const BcNumber a, const BcNumber b, const BcNumber c);

BcMaybe libbc_num_sub_create(const BcNumber a, const BcNumber b);
BcError libbc_num_sub(const BcNumber a, const BcNumber b, const BcNumber c);

BcMaybe libbc_num_mul_create(const BcNumber a, const BcNumber b);
BcError libbc_num_mul(const BcNumber a, const BcNumber b, const BcNumber c);

BcMaybe libbc_num_div_create(const BcNumber a, const BcNumber b);
BcError libbc_num_div(const BcNumber a, const BcNumber b, const BcNumber c);

BcMaybe libbc_num_mod_create(const BcNumber a, const BcNumber b);
BcError libbc_num_mod(const BcNumber a, const BcNumber b, const BcNumber c);

BcMaybe libbc_num_pow_create(const BcNumber a, const BcNumber b);
BcError libbc_num_pow(const BcNumber a, const BcNumber b, const BcNumber c);

#if BC_ENABLE_EXTRA_MATH
BcMaybe libbc_num_places_create(const BcNumber a, const BcNumber b);
BcError libbc_num_places(const BcNumber a, const BcNumber b, const BcNumber c);

BcMaybe libbc_num_lshift_create(const BcNumber a, const BcNumber b);
BcError libbc_num_lshift(const BcNumber a, const BcNumber b, const BcNumber c);

BcMaybe libbc_num_rshift_create(const BcNumber a, const BcNumber b);
BcError libbc_num_rshift(const BcNumber a, const BcNumber b, const BcNumber c);
#endif // BC_ENABLE_EXTRA_MATH

BcMaybe libbc_num_sqrt_create(const BcNumber a);
BcError libbc_num_sqrt(const BcNumber a, const BcNumber b);

BcError libbc_num_divmod_create(const BcNumber a, const BcNumber b,
                                BcNumber *c, BcNumber *d);
BcError libbc_num_divmod(const BcNumber a, const BcNumber b,
                         const BcNumber c, const BcNumber d);

BcMaybe libbc_num_modexp_create(const BcNumber a, const BcNumber b,
                                const BcNumber c);
BcError libbc_num_modexp(const BcNumber a, const BcNumber b,
                         const BcNumber c, const BcNumber d);

size_t libbc_num_addReq(const BcNumber a, const BcNumber b);
size_t libbc_num_mulReq(const BcNumber a, const BcNumber b);
size_t libbc_num_divReq(const BcNumber a, const BcNumber b);
size_t libbc_num_powReq(const BcNumber a, const BcNumber b);

#if BC_ENABLE_EXTRA_MATH
size_t libbc_num_placesReq(const BcNumber a, const BcNumber b);
#endif // BC_ENABLE_EXTRA_MATH

BcError libbc_num_setScale(const BcNumber n, const size_t scale);
ssize_t libbc_num_cmp(const BcNumber a, const BcNumber b);

void libbc_num_one(const BcNumber n);
ssize_t libbc_num_cmpZero(const BcNumber n);

BcMaybe libbc_num_parse_create(const char *restrict val, const BcBigDig base);
BcError libbc_num_parse(const BcNumber n, const char *restrict val,
                        const BcBigDig base);
char* libbc_num_string(const BcNumber n, const BcBigDig base);

#if BC_ENABLE_EXTRA_MATH
BcMaybe libbc_num_irand_create(const BcNumber a);
BcError libbc_num_irand(BcNumber a, BcNumber b);

BcMaybe libbc_num_frand_create(size_t places);
BcError libbc_num_frand(const BcNumber n, size_t places);

BcMaybe libbc_num_ifrand_create(const BcNumber a, size_t places);
BcError libbc_num_ifrand(const BcNumber a, size_t places, const BcNumber b);

BcError libbc_num_seedWithNum(const BcNumber n);
BcError libbc_num_seedWithUlongs(unsigned long state1, unsigned long state2,
                                 unsigned long inc1, unsigned long inc2);
BcError libbc_num_reseed(void);
BcMaybe libbc_num_seed2num_create(void);
BcError libbc_num_seed2num(BcNumber n);

BcRandInt libbc_rand_int(void);
BcRandInt libbc_rand_bounded(BcRandInt bound);
#endif // BC_ENABLE_EXTRA_MATH

#endif // LIBBC_H
