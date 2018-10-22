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
#include <stddef.h>
#include <stdio.h>

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
#define BC_NUM_MAX_IBASE ((unsigned long) 16)
#define BC_NUM_DEF_SIZE (16)
#define BC_NUM_PRINT_WIDTH (69)

#ifndef BC_NUM_KARATSUBA_LEN
#define BC_NUM_KARATSUBA_LEN (32)
#elif BC_NUM_KARATSUBA_LEN < 2
#error BC_NUM_KARATSUBA_LEN must be at least 2
#endif // BC_NUM_KARATSUBA_LEN

#define BC_NUM_NEG(n, neg) ((((ssize_t) (n)) ^ -((ssize_t) (neg))) + (neg))
#define BC_NUM_ONE(n) ((n)->len == 1 && (n)->rdx == 0 && (n)->num[0] == 1)
#define BC_NUM_INT(n) ((n)->len - (n)->rdx)
#define BC_NUM_AREQ(a, b) \
	(BC_MAX((a)->rdx, (b)->rdx) + BC_MAX(BC_NUM_INT(a), BC_NUM_INT(b)) + 1)
#define BC_NUM_MREQ(a, b, scale) \
	(BC_NUM_INT(a) + BC_NUM_INT(b) + BC_MAX((scale), (a)->rdx + (b)->rdx) + 1)

typedef BcStatus (*BcNumBinaryOp)(BcNum*, BcNum*, BcNum*, size_t);
typedef BcStatus (*BcNumDigitOp)(size_t, size_t, bool, size_t*, size_t);

BcStatus bc_num_init(BcNum *n, size_t request);
BcStatus bc_num_expand(BcNum *n, size_t req);
BcStatus bc_num_copy(BcNum *d, BcNum *s);
void bc_num_free(void *num);

BcStatus bc_num_ulong(BcNum *n, unsigned long *result);
BcStatus bc_num_ulong2num(BcNum *n, unsigned long val);

// ** Exclude start. **

BcStatus bc_num_subArrays(BcDig *restrict n1, BcDig *restrict n2, size_t len);
ssize_t bc_num_compare(BcDig *restrict n1, BcDig *restrict n2, size_t len);

void bc_num_truncate(BcNum *n, size_t places);
BcStatus bc_num_extend(BcNum *n, size_t places);

void bc_num_clean(BcNum *n);
BcStatus bc_num_retireMul(BcNum *n, size_t scale, bool neg1, bool neg2);
BcStatus bc_num_splitAt(BcNum *restrict n, size_t idx, BcNum *restrict a,
                        BcNum *restrict b);

BcStatus bc_num_shift(BcNum *n, size_t places);
BcStatus bc_num_inv(BcNum *a, BcNum *b, size_t scale);

BcStatus bc_num_alg_a(BcNum *a, BcNum *b, BcNum *restrict c, size_t sub);
BcStatus bc_num_alg_s(BcNum *a, BcNum *b, BcNum *restrict c, size_t sub);
BcStatus bc_num_alg_k(BcNum *restrict a, BcNum *restrict b, BcNum *restrict c);
BcStatus bc_num_alg_m(BcNum *a, BcNum *b, BcNum *restrict c, size_t scale);
BcStatus bc_num_alg_d(BcNum *a, BcNum *b, BcNum *restrict c, size_t scale);
BcStatus bc_num_alg_rem(BcNum *a, BcNum *b, BcNum *restrict c, size_t scale);
BcStatus bc_num_alg_p(BcNum *a, BcNum *b, BcNum *restrict c, size_t scale);

BcStatus bc_num_binary(BcNum *a, BcNum *b, BcNum *c, size_t scale,
                       BcNumBinaryOp op, size_t req);

bool bc_num_strValid(const char *val, size_t base);
BcStatus bc_num_parseDecimal(BcNum *n, const char *val);
BcStatus bc_num_parseBase(BcNum *n, const char *val, BcNum *base);
#ifdef DC_ENABLED
BcStatus bc_num_printChar(size_t num, size_t width, bool radix,
                          size_t *nchars, size_t line_len);
#endif // DC_ENABLED
BcStatus bc_num_printDigits(size_t num, size_t width, bool radix,
                            size_t *nchars, size_t line_len);
BcStatus bc_num_printHex(size_t num, size_t width, bool radix,
                         size_t *nchars, size_t line_len);
BcStatus bc_num_printDecimal(BcNum *n, size_t *nchars, size_t len);
BcStatus bc_num_printNum(BcNum *n, BcNum *base, size_t width, size_t *nchars,
                         size_t line_len, BcNumDigitOp print);
BcStatus bc_num_printBase(BcNum *n, BcNum *base, size_t base_t,
                          size_t *nchars, size_t line_len);

BcStatus bc_num_printNewline(size_t *nchars, size_t line_len);

ssize_t bc_num_cmp(BcNum *a, BcNum *b);

// ** Exclude end. **

BcStatus bc_num_add(BcNum *a, BcNum *b, BcNum *c, size_t scale);
BcStatus bc_num_sub(BcNum *a, BcNum *b, BcNum *c, size_t scale);
BcStatus bc_num_mul(BcNum *a, BcNum *b, BcNum *c, size_t scale);
BcStatus bc_num_div(BcNum *a, BcNum *b, BcNum *c, size_t scale);
BcStatus bc_num_rem(BcNum *a, BcNum *b, BcNum *c, size_t scale);
BcStatus bc_num_pow(BcNum *a, BcNum *b, BcNum *c, size_t scale);
BcStatus bc_num_sqrt(BcNum *a, BcNum *b, size_t scale);
BcStatus bc_num_divmod(BcNum *a, BcNum *b, BcNum *c, BcNum *d, size_t scale);

// ** Exclude start. **

#ifdef DC_ENABLED
BcStatus bc_num_modexp(BcNum *a, BcNum *b, BcNum *c, BcNum *d, size_t scale);
#endif // DC_ENABLED

void bc_num_zero(BcNum *n);
void bc_num_one(BcNum *n);
void bc_num_ten(BcNum *n);

BcStatus bc_num_parse(BcNum *n, const char *val, BcNum *base, size_t base_t);
BcStatus bc_num_print(BcNum *n, BcNum *base, size_t base_t, bool newline,
                      size_t *nchars, size_t line_len);
BcStatus bc_num_stream(BcNum *n, BcNum *base, size_t *nchars, size_t len);

// ** Exclude end. **

extern const char bc_num_hex_digits[];

#endif // BC_NUM_H
