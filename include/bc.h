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
 * The public header for the bc library.
 *
 */

#ifndef LIBBC_H
#define LIBBC_H

#include <stdbool.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>
#include <sys/types.h>

#define BC_SEED_ULONGS (4)
#define BC_SEED_SIZE (sizeof(long) * BC_SEED_ULONGS)

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

typedef uint64_t BcBigDig;
typedef uint64_t BcRandInt;

#elif BC_LONG_BIT >= 32

typedef uint32_t BcBigDig;
typedef uint32_t BcRandInt;

#else

#error BC_LONG_BIT must be at least 32

#endif // BC_LONG_BIT >= 64

typedef enum BcError {

	BC_ERROR_SUCCESS,

	BC_ERROR_INVALID_NUM,
	BC_ERROR_INVALID_CONTEXT,
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

typedef size_t BcNumber;

struct BcCtxt;

typedef struct BcCtxt* BcContext;

BcError bcl_init(void);
void bcl_free(void);

bool bcl_abortOnFatalError(void);
void bcl_setAbortOnFatalError(bool abrt);

void bcl_gc(void);

BcError bcl_pushContext(BcContext ctxt);
void bcl_popContext(void);
BcContext bcl_context(void);

void bcl_ctxt_free(BcContext ctxt);
void bcl_ctxt_freeAll(BcContext ctxt);

size_t bcl_ctxt_scale(BcContext ctxt);
void bcl_setScale(BcContext ctxt, size_t scale);
size_t bcl_ibase(BcContext ctxt);
void bcl_setIbase(BcContext ctxt, size_t ibase);
size_t bcl_obase(BcContext ctxt);
void bcl_setObase(BcContext ctxt, size_t obase);

BcError bcl_num_error(const BcNumber n);

void bcl_handleSignal(void);

BcNumber bcl_num_init(void);
BcNumber bcl_num_initReq(size_t req);
BcError bcl_num_copy(const BcNumber d, const BcNumber s);
BcNumber bcl_num_dup(const BcNumber s);
void bcl_num_free(BcNumber n);

bool bcl_num_neg(const BcNumber n);
size_t bcl_num_scale(const BcNumber n);
size_t bcl_num_len(const BcNumber n);

BcError bcl_num_bigdig(const BcNumber n, BcBigDig *result);
BcNumber bcl_num_bigdig2num(const BcBigDig val);
BcError bcl_num_bigdig2num_err(const BcNumber n, const BcBigDig val);

BcNumber bcl_num_add(const BcNumber a, const BcNumber b);
BcError bcl_num_add_err(const BcNumber a, const BcNumber b, const BcNumber c);

BcNumber bcl_num_sub(const BcNumber a, const BcNumber b);
BcError bcl_num_sub_err(const BcNumber a, const BcNumber b, const BcNumber c);

BcNumber bcl_num_mul(const BcNumber a, const BcNumber b);
BcError bcl_num_mul_err(const BcNumber a, const BcNumber b, const BcNumber c);

BcNumber bcl_num_div(const BcNumber a, const BcNumber b);
BcError bcl_num_div_err(const BcNumber a, const BcNumber b, const BcNumber c);

BcNumber bcl_num_mod(const BcNumber a, const BcNumber b);
BcError bcl_num_mod_err(const BcNumber a, const BcNumber b, const BcNumber c);

BcNumber bcl_num_pow(const BcNumber a, const BcNumber b);
BcError bcl_num_pow_err(const BcNumber a, const BcNumber b, const BcNumber c);

#if BC_ENABLE_EXTRA_MATH
BcNumber bcl_num_places(const BcNumber a, const BcNumber b);
BcError bcl_num_places_err(const BcNumber a, const BcNumber b,
                           const BcNumber c);

BcNumber bcl_num_lshift(const BcNumber a, const BcNumber b);
BcError bcl_num_lshift_err(const BcNumber a, const BcNumber b,
                           const BcNumber c);

BcNumber bcl_num_rshift(const BcNumber a, const BcNumber b);
BcError bcl_num_rshift_err(const BcNumber a, const BcNumber b,
                           const BcNumber c);
#endif // BC_ENABLE_EXTRA_MATH

BcNumber bcl_num_sqrt(const BcNumber a);
BcError bcl_num_sqrt_err(const BcNumber a, const BcNumber b);

BcError bcl_num_divmod(const BcNumber a, const BcNumber b,
                       BcNumber *c, BcNumber *d);
BcError bcl_num_divmod_err(const BcNumber a, const BcNumber b,
                           const BcNumber c, const BcNumber d);

BcNumber bcl_num_modexp(const BcNumber a, const BcNumber b, const BcNumber c);
BcError bcl_num_modexp_err(const BcNumber a, const BcNumber b,
                           const BcNumber c, const BcNumber d);

size_t bcl_num_addReq(const BcNumber a, const BcNumber b);
size_t bcl_num_mulReq(const BcNumber a, const BcNumber b);
size_t bcl_num_divReq(const BcNumber a, const BcNumber b);
size_t bcl_num_powReq(const BcNumber a, const BcNumber b);

#if BC_ENABLE_EXTRA_MATH
size_t bcl_num_placesReq(const BcNumber a, const BcNumber b);
#endif // BC_ENABLE_EXTRA_MATH

BcError bcl_num_setScale(const BcNumber n, const size_t scale);
ssize_t bcl_num_cmp(const BcNumber a, const BcNumber b);

void bcl_num_zero(const BcNumber n);
void bcl_num_one(const BcNumber n);
ssize_t bcl_num_cmpZero(const BcNumber n);

BcNumber bcl_num_parse(const char *restrict val, const BcBigDig base);
BcError bcl_num_parse_err(const BcNumber n, const char *restrict val,
                          const BcBigDig base);
char* bcl_num_string(const BcNumber n, const BcBigDig base);
BcError bcl_num_string_err(const BcNumber n, const BcBigDig base, char **str);

#if BC_ENABLE_EXTRA_MATH
BcNumber bcl_num_irand(const BcNumber a);
BcError bcl_num_irand_err(BcNumber a, BcNumber b);

BcNumber bcl_num_frand(size_t places);
BcError bcl_num_frand_err(const BcNumber n, size_t places);

BcNumber bcl_num_ifrand(const BcNumber a, size_t places);
BcError bcl_num_ifrand_err(const BcNumber a, size_t places, const BcNumber b);

BcError bcl_num_seedWithNum(const BcNumber n);
BcError bcl_num_seed(unsigned char seed[BC_SEED_SIZE]);
BcError bcl_num_reseed(void);
BcNumber bcl_num_seed2num(void);
BcError bcl_num_seed2num_err(BcNumber n);

BcRandInt bcl_rand_int(void);
BcRandInt bcl_rand_bounded(BcRandInt bound);
#endif // BC_ENABLE_EXTRA_MATH

#endif // LIBBC_H
