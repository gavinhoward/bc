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
 * Code for the number type.
 *
 */

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <limits.h>

#include <status.h>
#include <num.h>
#include <vm.h>

#if BC_DEBUG_CODE
static void bc_num_printDecimal(const BcNum *restrict a);

static void bc_num_printDebug(const BcNum *n, const char *name, bool emptyline)
{
	printf("%s: ", name);
	bc_num_printDecimal(n);
	printf("\n");
	if (emptyline) printf("\n");
	vm->nchars = 0;
}

static void bc_num_printDigs(const BcNum *n, const char *name, bool emptyline) {

	size_t i;

	printf("%s len: %zu, rdx: %zu, scale: %zu\n",
	       name, n->len, n->rdx, n->scale);

	for (i = n->len - 1; i < n->len; --i)
		printf(" %0*d", BC_BASE_POWER, n->num[i]);

	printf("\n");
	if (emptyline) printf("\n");
	vm->nchars = 0;
}

static void bc_num_dump(const char *varname, const BcNum *n) {

	unsigned long i, scale = n->scale;

	fprintf(stderr, "\n%s = %s", varname, n->len ? (n->neg ? "-" : "+") : "0 ");

	for (i = n->len -1; i < n->len; i--) {

		if (i + 1 == n->rdx) fprintf(stderr, ". ");

		if (scale / BC_BASE_POWER != n->rdx - i)
			fprintf(stderr, "%0*d ", BC_BASE_POWER, n->num[i]);
		else {

			size_t mod = scale % BC_BASE_POWER;
			BcDig div;

			if (mod != 0) {
				size_t temp = scale / BC_BASE_POWER;
				div = n->num[i] / (BcDig) bc_num_pow10[BC_BASE_POWER - temp];
				fprintf(stderr, "%0*d", (int) mod, div);
			}

			div = n->num[i] / (BcDig) bc_num_pow10[mod];
			fprintf(stderr, " ' %0*d ", BC_BASE_POWER - (int) mod, div);
		}
	}

	fprintf(stderr, "(%zu | %zu.%zu/%zu) %p\n",
		n->scale, n->len, n->rdx, n->cap, (void*) n->num);
}
#endif // BC_DEBUG_CODE

static BcStatus bc_num_m(BcNum *a, BcNum *b, BcNum *restrict c, size_t scale);

static ssize_t bc_num_neg(size_t n, bool neg) {
	return (((ssize_t) n) ^ -((ssize_t) neg)) + neg;
}

ssize_t bc_num_cmpZero(const BcNum *n) {
	return bc_num_neg((n)->len != 0, (n)->neg);
}

static size_t bc_num_int(const BcNum *n) {
	return n->len ? n->len - n->rdx : 0;
}

static bool bc_num_isOne(const BcNum *n) {
	return n->len == 1 && n->rdx == 0 && n->num[0] == 1;
}

static void bc_num_expand(BcNum *restrict n, size_t req) {
	assert(n);
	req = req >= BC_NUM_DEF_SIZE ? req : BC_NUM_DEF_SIZE;
	if (req > n->cap) {
		n->num = bc_vm_realloc(n->num, BC_NUM_SIZE(req));
		n->cap = req;
	}
}

static void bc_num_setToZero(BcNum *restrict n, size_t scale) {
	assert(n);
	n->scale = scale;
	n->len = n->rdx = 0;
	n->neg = false;
}

static void bc_num_zero(BcNum *restrict n) {
	bc_num_setToZero(n, 0);
}

void bc_num_one(BcNum *restrict n) {
	bc_num_setToZero(n, 0);
	n->len = 1;
	n->num[0] = 1;
}

void bc_num_ten(BcNum *restrict n) {
	assert(n);
	bc_num_setToZero(n, 0);
#if BC_BASE_DIG == 10
	n->len = 2;
	n->num[0] = 0;
	n->num[1] = 1;
#else // BC_BASE_DIG == 10
	n->len = 1;
	n->num[0] = BC_BASE;
#endif // BC_BASE_DIG == 10
}

static size_t bc_num_log10(size_t i) {
	size_t len;
	for (len = 1; i; i /= BC_BASE, ++len);
	return len - 1;
}

static size_t bc_num_int_digits(const BcNum *n) {

	size_t digits;

	digits = bc_num_int(n) * BC_BASE_POWER;

	if (digits > 0)
		digits -= BC_BASE_POWER - bc_num_log10((size_t) n->num[n->len - 1]);

	return digits;
}

static size_t bc_num_nonzeroLen(const BcNum *restrict n) {
	size_t i, len = n->len;
	assert(len == n->rdx);
	for (i = len - 1; i < n->len && !n->num[i]; --len, --i);
	assert(len > 0);
	return len;
}

static unsigned long bc_num_addDigit(BcDig *restrict num, unsigned long d,
                                    unsigned long c)
{
	d += c;
	*num = (BcDig) (d % BC_BASE_DIG);
	assert(*num >= 0 && *num < BC_BASE_DIG);
	return d / BC_BASE_DIG;
}

static BcStatus bc_num_addArrays(BcDig *restrict a, const BcDig *restrict b,
                                 size_t len)
{
	size_t i;
	unsigned long carry = 0;

	for (i = 0; BC_NO_SIG && i < len; ++i) {
		unsigned long in = ((unsigned long) a[i]) + ((unsigned long) b[i]);
		carry = bc_num_addDigit(a + i, in, carry);
	}

	for (; BC_NO_SIG && carry; ++i)
		carry = bc_num_addDigit(a + i, (unsigned long) a[i], carry);

	return BC_SIG ? BC_STATUS_SIGNAL : BC_STATUS_SUCCESS;
}

static BcStatus bc_num_subArrays(BcDig *restrict a, const BcDig *restrict b,
                                 size_t len)
{
	size_t i, j;

	for (i = 0; BC_NO_SIG && i < len; ++i) {

		for (a[i] -= b[i], j = 0; BC_NO_SIG && a[i + j] < 0;) {
			assert(a[i + j] >= -BC_BASE_DIG);
			a[i + j++] += BC_BASE_DIG;
			a[i + j] -= 1;
			assert(a[i + j - 1] >= 0 && a[i + j - 1] < BC_BASE_DIG);
		}
	}

	return BC_SIG ? BC_STATUS_SIGNAL : BC_STATUS_SUCCESS;
}

static ssize_t bc_num_compare(const BcDig *restrict a, const BcDig *restrict b,
                              size_t len)
{
	size_t i;
	long c = 0;
	for (i = len - 1; BC_NO_SIG && i < len && !(c = a[i] - b[i]); --i);
	return BC_SIG ? BC_NUM_SSIZE_MIN : bc_num_neg(i + 1, c < 0);
}

ssize_t bc_num_cmp(const BcNum *a, const BcNum *b) {

	size_t i, min, a_int, b_int, diff;
	BcDig *max_num, *min_num;
	bool a_max, neg = false;
	ssize_t cmp;

	assert(a && b);

	if (a == b) return 0;
	if (BC_NUM_ZERO(a)) return bc_num_neg(b->len != 0, !b->neg);
	if (BC_NUM_ZERO(b)) return bc_num_cmpZero(a);
	if (a->neg) {
		if (b->neg) neg = true;
		else return -1;
	}
	else if (b->neg) return 1;

	a_int = bc_num_int(a);
	b_int = bc_num_int(b);
	a_int -= b_int;
	a_max = (a->rdx > b->rdx);

	if (a_int) return (ssize_t) a_int;

	if (a_max) {
		min = b->rdx;
		diff = a->rdx - b->rdx;
		max_num = a->num + diff;
		min_num = b->num;
	}
	else {
		min = a->rdx;
		diff = b->rdx - a->rdx;
		max_num = b->num + diff;
		min_num = a->num;
	}

	cmp = bc_num_compare(max_num, min_num, b_int + min);
	if (cmp == BC_NUM_SSIZE_MIN) return cmp;
	if (cmp) return bc_num_neg((size_t) cmp, !a_max == !neg);

	for (max_num -= diff, i = diff - 1; BC_NO_SIG && i < diff; --i) {
		if (max_num[i]) return bc_num_neg(1, !a_max == !neg);
	}

	return BC_SIG ? BC_NUM_SSIZE_MIN : 0;
}

static void bc_num_clean(BcNum *restrict n) {
	while (BC_NUM_NONZERO(n) && !n->num[n->len - 1]) --n->len;
	if (BC_NUM_ZERO(n)) n->neg = false;
	else if (n->len < n->rdx) n->len = n->rdx;
}

void bc_num_truncate(BcNum *restrict n, size_t places) {

	size_t places_rdx;

	if (!places) return;

	places_rdx = n->rdx - BC_NUM_RDX(n->scale - places);
	assert(places <= n->scale && (BC_NUM_ZERO(n) || places_rdx <= n->len));

	n->scale -= places;
	n->rdx -= places_rdx;

	if (BC_NUM_NONZERO(n)) {

		size_t pow;

		pow = n->scale % BC_BASE_POWER;
		pow = pow ? BC_BASE_POWER - pow : 0;
		pow = bc_num_pow10[pow];

		n->len -= places_rdx;
		memmove(n->num, n->num + places_rdx, BC_NUM_SIZE(n->len));

		// Clear the lower part of the last digit.
		if (BC_NUM_NONZERO(n)) n->num[0] -= n->num[0] % (BcDig) pow;

		bc_num_clean(n);
	}
}

static void bc_num_extend(BcNum *restrict n, size_t places) {

	size_t places_rdx;

	if (!places) return;

	places_rdx = BC_NUM_RDX(places + n->scale) - n->rdx;

	bc_num_expand(n, n->len + places_rdx);
	memmove(n->num + places_rdx, n->num, BC_NUM_SIZE(n->len));
	memset(n->num, 0, BC_NUM_SIZE(places_rdx));

	n->rdx += places_rdx;
	n->scale += places;
	n->len += places_rdx;

	assert(n->rdx == BC_NUM_RDX(n->scale));
}

static void bc_num_roundPlaces(BcNum *restrict n, size_t places) {

	size_t rdx, place, i;
	BcDig p10, sum;

	if (places >= n->scale || places >= n->rdx * BC_BASE_POWER) {
		bc_num_extend(n, places - n->scale);
		return;
	}

	rdx = n->rdx - BC_NUM_RDX(places + 1);
	place = BC_BASE_POWER - (places % BC_BASE_POWER + 1);

	for (i = 0; i < rdx; i++) n->num[i] = 0;

	p10 = (BcDig) bc_num_pow10[place];
	sum = n->num[rdx] + (BC_BASE / 2) * p10;
	sum = sum - sum % (BC_BASE * p10);

	if (sum < BC_BASE_DIG) n->num[rdx] = sum;
	else {

		sum -= BC_BASE_DIG;
		n->num[rdx] = sum;

		do {
			rdx += 1;

			if (n->len <= rdx) {
				bc_num_expand(n, rdx + 1);
				n->num[rdx] = 0;
				n->len = rdx +1;
			}

			sum = n->num[rdx] + 1;
			n->num[rdx] = sum < BC_BASE_DIG ? sum : sum - BC_BASE_DIG;
		} while (sum >= BC_BASE_DIG);
	}

	bc_num_truncate(n, n->scale - places);
}

static void bc_num_retireMul(BcNum *restrict n, size_t scale,
                             bool neg1, bool neg2)
{
	if (n->scale < scale) bc_num_extend(n, scale - n->scale);
	else bc_num_truncate(n, n->scale - scale);

	bc_num_clean(n);
	if (BC_NUM_NONZERO(n)) n->neg = (!neg1 != !neg2);
}

static void bc_num_split(const BcNum *restrict n, size_t idx,
                         BcNum *restrict a, BcNum *restrict b)
{
	if (idx < n->len) {

		b->len = n->len - idx;
		a->len = idx;
		a->scale = a->rdx = b->scale = b->rdx = 0;

		memcpy(b->num, n->num + idx, BC_NUM_SIZE(b->len));
		memcpy(a->num, n->num, BC_NUM_SIZE(idx));

		bc_num_clean(b);
	}
	else bc_num_copy(a, n);

	bc_num_clean(a);
}

static size_t bc_num_shiftZero(BcNum *restrict n) {

	size_t i;

	assert(!n->rdx || BC_NUM_ZERO(n));

	for (i = 0; i < n->len && !n->num[i]; ++i);

	n->len -= i;
	n->num += i;

	return i;
}

static void bc_num_unshiftZero(BcNum *restrict n, size_t places_rdx) {
	n->len += places_rdx;
	n->num -= places_rdx;
}

static BcStatus bc_num_shift(BcNum *restrict n, unsigned long dig) {

	size_t i, len = n->len;
	unsigned long carry = 0, pow;
	BcDig *ptr = n->num;

	assert(dig < BC_BASE_POWER);

	pow = bc_num_pow10[dig];
	dig = bc_num_pow10[BC_BASE_POWER - dig];

	for (i = len - 1; BC_NO_SIG && i < len; --i) {
		unsigned long in, temp;
		in = ((unsigned long) ptr[i]);
		temp = carry * dig;
		carry = in % pow;
		ptr[i] = ((BcDig) (in / pow)) + (BcDig) temp;
	}

	assert(!carry);

	return BC_SIG ? BC_STATUS_SIGNAL : BC_STATUS_SUCCESS;
}

static BcStatus bc_num_shiftLeft(BcNum *restrict n, size_t places) {

	BcStatus s = BC_STATUS_SUCCESS;
	unsigned long dig;
	size_t places_rdx;
	bool shift;

	if (!places) return s;
	if (BC_NUM_ZERO(n)) {
		if (n->scale >= places) n->scale -= places;
		else n->scale = 0;
		return s;
	}

	dig = (unsigned long) (places % BC_BASE_POWER);
	shift = (dig != 0);

	if (!n->scale) places_rdx = BC_NUM_RDX(places);
	else {

		places_rdx = BC_NUM_RDX(places);

		if (n->rdx >= places_rdx) {

			size_t mod = n->scale % BC_BASE_POWER, revdig;

			mod = mod ? mod : BC_BASE_POWER;
			revdig = dig ? BC_BASE_POWER - dig : 0;

			if (mod + revdig > BC_BASE_POWER) places_rdx = 1;
			else places_rdx = 0;
		}
		else places_rdx -= n->rdx;
	}

	if (places_rdx) {
		bc_num_expand(n, n->len + places_rdx);
		memmove(n->num + places_rdx, n->num, BC_NUM_SIZE(n->len));
		memset(n->num, 0, BC_NUM_SIZE(places_rdx));
		n->len += places_rdx;
	}

	if (places > n->scale) n->scale = n->rdx = 0;
	else {
		n->scale -= places;
		n->rdx = BC_NUM_RDX(n->scale);
	}

	if (shift) s = bc_num_shift(n, BC_BASE_POWER - dig);

	bc_num_clean(n);

	return BC_SIG && !s ? BC_STATUS_SIGNAL : s;
}

static BcStatus bc_num_shiftRight(BcNum *restrict n, size_t places) {

	BcStatus s = BC_STATUS_SUCCESS;
	unsigned long dig;
	size_t places_rdx, scale, scale_mod, int_len, expand;
	bool shift;

	if (!places) return s;
	if (BC_NUM_ZERO(n)) {
		n->scale += places;
		bc_num_expand(n, BC_NUM_RDX(n->scale));
		return s;
	}

	dig = (unsigned long) (places % BC_BASE_POWER);
	shift = (dig != 0);
	scale = n->scale;
	scale_mod = scale % BC_BASE_POWER;
	scale_mod = scale_mod ? scale_mod : BC_BASE_POWER;
	int_len = bc_num_int(n);
	places_rdx = BC_NUM_RDX(places);

	if (scale_mod + dig > BC_BASE_POWER) {
		expand = places_rdx - 1;
		places_rdx = 1;
	}
	else {
		expand = places_rdx;
		places_rdx = 0;
	}

	if (expand > int_len) expand -= int_len;
	else expand = 0;

	bc_num_extend(n, places_rdx * BC_BASE_POWER);
	bc_num_expand(n, expand + n->len);
	memset(n->num + n->len, 0, BC_NUM_SIZE(expand));
	n->len += expand;
	n->scale = n->rdx = 0;

	if (shift) s = bc_num_shift(n, dig);

	n->scale = scale + places;
	n->rdx = BC_NUM_RDX(n->scale);

	bc_num_clean(n);

	assert(n->rdx <= n->len && n->len <= n->cap);
	assert(n->rdx == BC_NUM_RDX(n->scale));

	return BC_SIG && !s ? BC_STATUS_SIGNAL : s;
}

static BcStatus bc_num_inv(BcNum *a, BcNum *b, size_t scale) {

	BcNum one;
	BcDig num[2];

	assert(BC_NUM_NONZERO(a));

	one.cap = 2;
	one.num = num;
	bc_num_one(&one);

	return bc_num_div(&one, a, b, scale);
}

#if BC_ENABLE_EXTRA_MATH
static BcStatus bc_num_intop(const BcNum *a, const BcNum *b, BcNum *restrict c,
                             unsigned long *v)
{
	if (BC_ERR(b->rdx)) return bc_vm_err(BC_ERROR_MATH_NON_INTEGER);
	bc_num_copy(c, a);
	return bc_num_ulong(b, v);
}
#endif // BC_ENABLE_EXTRA_MATH

static BcStatus bc_num_a(BcNum *a, BcNum *b, BcNum *restrict c, size_t sub) {

	BcDig *ptr, *ptr_a, *ptr_b, *ptr_c;
	size_t i, max, min_rdx, min_int, diff, a_int, b_int;
	unsigned long carry;

	// Because this function doesn't need to use scale (per the bc spec),
	// I am hijacking it to say whether it's doing an add or a subtract.

	if (BC_NUM_ZERO(a)) {
		bc_num_copy(c, b);
		if (sub && BC_NUM_NONZERO(c)) c->neg = !c->neg;
		return BC_STATUS_SUCCESS;
	}
	if (BC_NUM_ZERO(b)) {
		bc_num_copy(c, a);
		return BC_STATUS_SUCCESS;
	}

	c->neg = a->neg;
	c->rdx = BC_MAX(a->rdx, b->rdx);
	c->scale = BC_MAX(a->scale, b->scale);
	min_rdx = BC_MIN(a->rdx, b->rdx);

	if (a->rdx > b->rdx) {
		diff = a->rdx - b->rdx;
		ptr = a->num;
		ptr_a = a->num + diff;
		ptr_b = b->num;
	}
	else {
		diff = b->rdx - a->rdx;
		ptr = b->num;
		ptr_a = a->num;
		ptr_b = b->num + diff;
	}

	for (ptr_c = c->num, i = 0; i < diff; ++i) ptr_c[i] = ptr[i];

	c->len = diff;
	ptr_c += diff;
	a_int = bc_num_int(a);
	b_int = bc_num_int(b);

	if (a_int > b_int) {
		min_int = b_int;
		max = a_int;
		ptr = ptr_a;
	}
	else {
		min_int = a_int;
		max = b_int;
		ptr = ptr_b;
	}

	for (carry = 0, i = 0; BC_NO_SIG && i < min_rdx + min_int; ++i) {
		unsigned long in;
		in = ((unsigned long) ptr_a[i]) + ((unsigned long) ptr_b[i]);
		carry = bc_num_addDigit(ptr_c + i, in, carry);
	}

	for (; BC_NO_SIG && i < max + min_rdx; ++i)
		carry = bc_num_addDigit(ptr_c + i, (unsigned long) ptr[i], carry);

	c->len += i;

	if (carry) c->num[c->len++] = (BcDig) carry;

	return BC_SIG ? BC_STATUS_SIGNAL : BC_STATUS_SUCCESS;
}

static BcStatus bc_num_s(BcNum *a, BcNum *b, BcNum *restrict c, size_t sub) {

	BcStatus s;
	ssize_t cmp;
	BcNum *minuend, *subtrahend;
	size_t start;
	bool aneg, bneg, neg;

	// Because this function doesn't need to use scale (per the bc spec),
	// I am hijacking it to say whether it's doing an add or a subtract.

	if (BC_NUM_ZERO(a)) {
		bc_num_copy(c, b);
		if (sub && BC_NUM_NONZERO(c)) c->neg = !c->neg;
		return BC_STATUS_SUCCESS;
	}
	if (BC_NUM_ZERO(b)) {
		bc_num_copy(c, a);
		return BC_STATUS_SUCCESS;
	}

	aneg = a->neg;
	bneg = b->neg;
	a->neg = b->neg = false;

	cmp = bc_num_cmp(a, b);
	if (cmp == BC_NUM_SSIZE_MIN) return BC_STATUS_SIGNAL;

	a->neg = aneg;
	b->neg = bneg;

	if (!cmp) {
		bc_num_setToZero(c, BC_MAX(a->rdx, b->rdx));
		return BC_STATUS_SUCCESS;
	}

	if (cmp > 0) {
		neg = a->neg;
		minuend = a;
		subtrahend = b;
	}
	else {
		neg = b->neg;
		if (sub) neg = !neg;
		minuend = b;
		subtrahend = a;
	}

	bc_num_copy(c, minuend);
	c->neg = neg;

	if (c->scale < subtrahend->scale) {
		bc_num_extend(c, subtrahend->scale - c->scale);
		start = 0;
	}
	else start = c->rdx - subtrahend->rdx;

	s = bc_num_subArrays(c->num + start, subtrahend->num, subtrahend->len);

	bc_num_clean(c);

	return s;
}

static BcStatus bc_num_m_simp(const BcNum *a, const BcNum *b, BcNum *restrict c)
{
	size_t i, alen = a->len, blen = b->len, clen;
	BcDig *ptr_a = a->num, *ptr_b = b->num, *ptr_c;
	unsigned long sum = 0, carry = 0;

	assert(sizeof(sum) >= sizeof(BcDig) * 2);
	assert(!a->rdx && !b->rdx);

	clen = bc_vm_growSize(alen, blen);
	bc_num_expand(c, clen + 1);

	ptr_c = c->num;
	memset(ptr_c, 0, BC_NUM_SIZE(c->cap));

	for (i = 0; BC_NO_SIG && i < clen; ++i) {

		ssize_t sidx = (ssize_t) (i - blen + 1);
		size_t j = (size_t) BC_MAX(0, sidx), k = BC_MIN(i, blen - 1);

		for (; BC_NO_SIG && j < alen && k < blen; ++j, --k) {

			sum += ((unsigned long) ptr_a[j]) * ((unsigned long) ptr_b[k]);

			if (sum >= BC_BASE_DIG) {
				carry += sum / BC_BASE_DIG;
				sum %= BC_BASE_DIG;
			}
		}

		ptr_c[i] = (BcDig) sum;
		assert(ptr_c[i] < BC_BASE_DIG);
		sum = carry;
		carry = 0;
	}

	if (sum) {
		assert(sum < BC_BASE_DIG);
		ptr_c[clen] = (BcDig) sum;
		clen += 1;
	}

	c->len = clen;

	return BC_SIG ? BC_STATUS_SIGNAL : BC_STATUS_SUCCESS;
}

static BcStatus bc_num_shiftAddSub(BcNum *restrict n, const BcNum *restrict a,
                                   size_t shift, BcNumShiftAddOp op)
{
	assert(n->len >= shift + a->len);
	assert(!n->rdx && !a->rdx);
	return op(n->num + shift, a->num, a->len);
}

static BcStatus bc_num_k(BcNum *a, BcNum *b, BcNum *restrict c) {

	BcStatus s;
	size_t max, max2, total;
	BcNum l1, h1, l2, h2, m2, m1, z0, z1, z2, temp;
	BcDig *digs, *dig_ptr;
	BcNumShiftAddOp op;
	bool aone = bc_num_isOne(a);

	assert(BC_NUM_ZERO(c));

	// This is here because the function is recursive.
 	if (BC_SIG) return BC_STATUS_SIGNAL;
	if (BC_NUM_ZERO(a) || BC_NUM_ZERO(b)) {
		bc_num_zero(c);
		return BC_STATUS_SUCCESS;
	}
	if (aone || bc_num_isOne(b)) {
		bc_num_copy(c, aone ? b : a);
		return BC_STATUS_SUCCESS;
	}

	if (a->len + b->len < BC_NUM_KARATSUBA_LEN ||
	    a->len < BC_NUM_KARATSUBA_LEN || b->len < BC_NUM_KARATSUBA_LEN)
	{
		return bc_num_m_simp(a, b, c);
	}

	max = BC_MAX(a->len, b->len);
	max = BC_MAX(max, BC_NUM_DEF_SIZE);
	max2 = (max + 1) / 2;

	total = bc_vm_arraySize(BC_NUM_KARATSUBA_ALLOCS, max);
	digs = dig_ptr = bc_vm_malloc(BC_NUM_SIZE(total));

	bc_num_setup(&l1, dig_ptr, max);
	dig_ptr += max;
	bc_num_setup(&h1, dig_ptr, max);
	dig_ptr += max;
	bc_num_setup(&l2, dig_ptr, max);
	dig_ptr += max;
	bc_num_setup(&h2, dig_ptr, max);
	dig_ptr += max;
	bc_num_setup(&m1, dig_ptr, max);
	dig_ptr += max;
	bc_num_setup(&m2, dig_ptr, max);
	max = bc_vm_growSize(max, 1);
	bc_num_init(&z0, max);
	bc_num_init(&z1, max);
	bc_num_init(&z2, max);
	max = bc_vm_growSize(max, max) + 1;
	bc_num_init(&temp, max);

	bc_num_split(a, max2, &l1, &h1);
	bc_num_clean(&l1);
	bc_num_clean(&h1);
	bc_num_split(b, max2, &l2, &h2);
	bc_num_clean(&l2);
	bc_num_clean(&h2);

	bc_num_expand(c, max);
	c->len = max;
	memset(c->num, 0, BC_NUM_SIZE(c->len));

	s = bc_num_sub(&h1, &l1, &m1, 0);
	if (BC_ERR(s)) goto err;
	s = bc_num_sub(&l2, &h2, &m2, 0);
	if (BC_ERR(s)) goto err;

	if (BC_NUM_NONZERO(&h1) && BC_NUM_NONZERO(&h2)) {

		s = bc_num_m(&h1, &h2, &z2, 0);
		if (BC_ERR(s)) goto err;
		bc_num_clean(&z2);

		s = bc_num_shiftAddSub(c, &z2, max2 * 2, bc_num_addArrays);
		if (BC_ERR(s)) goto err;
		s = bc_num_shiftAddSub(c, &z2, max2, bc_num_addArrays);
		if (BC_ERR(s)) goto err;
	}

	if (BC_NUM_NONZERO(&l1) && BC_NUM_NONZERO(&l2)) {

		s = bc_num_m(&l1, &l2, &z0, 0);
		if (BC_ERR(s)) goto err;
		bc_num_clean(&z0);

		s = bc_num_shiftAddSub(c, &z0, max2, bc_num_addArrays);
		if (BC_ERR(s)) goto err;
		s = bc_num_shiftAddSub(c, &z0, 0, bc_num_addArrays);
		if (BC_ERR(s)) goto err;
	}

	if (BC_NUM_NONZERO(&m1) && BC_NUM_NONZERO(&m2)) {

		s = bc_num_m(&m1, &m2, &z1, 0);
		if (BC_ERR(s)) goto err;
		bc_num_clean(&z1);

		op = (m1.neg != m2.neg) ? bc_num_subArrays : bc_num_addArrays;
		s = bc_num_shiftAddSub(c, &z1, max2, op);
		if (BC_ERR(s)) goto err;
	}

err:
	free(digs);
	bc_num_free(&temp);
	bc_num_free(&z2);
	bc_num_free(&z1);
	bc_num_free(&z0);
	return s;
}

static BcStatus bc_num_m(BcNum *a, BcNum *b, BcNum *restrict c, size_t scale) {

	BcStatus s;
	BcNum cpa, cpb;
	size_t ascale, bscale, ardx, brdx, azero = 0, bzero = 0, zero, len, rscale;

	bc_num_setToZero(c, 0);

	ascale = a->scale;
	bscale = b->scale;
	scale = BC_MAX(scale, ascale);
	scale = BC_MAX(scale, bscale);

	rscale = ascale + bscale;
	scale = BC_MIN(rscale, scale);

	bc_num_createCopy(&cpa, a);
	bc_num_createCopy(&cpb, b);

	cpa.neg = cpb.neg = false;

	ardx = cpa.rdx * BC_BASE_POWER;
	s = bc_num_shiftLeft(&cpa, ardx);
	if (BC_ERROR_SIGNAL_ONLY(s)) goto err;
	bc_num_clean(&cpa);
	azero = bc_num_shiftZero(&cpa);

	brdx = cpb.rdx * BC_BASE_POWER;
	s = bc_num_shiftLeft(&cpb, brdx);
	if (BC_ERROR_SIGNAL_ONLY(s)) goto err;
	bzero = bc_num_shiftZero(&cpb);
	bc_num_clean(&cpb);

	s = bc_num_k(&cpa, &cpb, c);
	if (BC_ERROR_SIGNAL_ONLY(s)) goto err;

	zero = bc_vm_growSize(azero, bzero);
	len = bc_vm_growSize(c->len, zero);

	bc_num_expand(c, len);
	s = bc_num_shiftLeft(c, (len - c->len) * BC_BASE_POWER);
	if (BC_ERROR_SIGNAL_ONLY(s)) goto err;
	s = bc_num_shiftRight(c, ardx + brdx);
	if (BC_ERROR_SIGNAL_ONLY(s)) goto err;

	bc_num_retireMul(c, scale, a->neg, b->neg);

err:
	bc_num_unshiftZero(&cpb, bzero);
	bc_num_free(&cpb);
	bc_num_unshiftZero(&cpa, azero);
	bc_num_free(&cpa);
	return s;
}

static BcStatus bc_num_d(BcNum *a, BcNum *b, BcNum *c, size_t scale) {

	BcStatus s = BC_STATUS_SUCCESS;
	size_t rdx, rscale, req;
	ssize_t cmp;
	BcNum cpa, cpb, two, factor, factor2, *fi, *fnext, *temp;
	BcDig two_digs[2];
	bool aneg, bneg;

	aneg = a->neg;
	bneg = b->neg;

	if (BC_NUM_ZERO(b)) return bc_vm_err(BC_ERROR_MATH_DIVIDE_BY_ZERO);
	if (BC_NUM_ZERO(a)) {
		bc_num_setToZero(c, scale);
		return BC_STATUS_SUCCESS;
	}
	if (bc_num_isOne(b)) {
		bc_num_copy(c, a);
		goto exit;
	}

	a->neg = b->neg = false;
	cmp = bc_num_cmp(a, b);

	if (cmp == BC_NUM_SSIZE_MIN) return BC_STATUS_SIGNAL;
	if (!cmp) {
		bc_num_one(c);
		goto exit;
	}

	bc_num_createCopy(&cpa, a);
	bc_num_createCopy(&cpb, b);

	req = bc_num_int(a) + BC_NUM_RDX(scale) + 1;
	bc_num_init(&factor, req);
	bc_num_init(&factor2, req);

	bc_num_setup(&two, two_digs, sizeof(two_digs) / sizeof(BcDig));
	bc_num_one(&two);
	two.num[0] = 2;

	if (b->rdx == b->len) {
		rdx = cpb.len - bc_num_nonzeroLen(&cpb);
		rscale = rdx * BC_BASE_POWER;
		s = bc_num_shiftLeft(&cpa, rscale);
		if (BC_ERROR_SIGNAL_ONLY(s)) goto err;
		s = bc_num_shiftLeft(&cpb, rscale);
	}
	else {
		rdx = b->len - b->rdx;
		rscale = rdx * BC_BASE_POWER;
		s = bc_num_shiftRight(&cpa, rscale);
		if (BC_ERROR_SIGNAL_ONLY(s)) goto err;
		s = bc_num_shiftRight(&cpb, rscale);
	}

	if (BC_ERROR_SIGNAL_ONLY(s)) goto err;

	req = bc_num_int(&cpa);
	if (!req) req = cpa.len - bc_num_nonzeroLen(&cpa);
	req = BC_BASE_POWER * (req + 1);
	req += b->scale % BC_BASE_POWER == 0 ? BC_BASE_POWER : 0;
	req += BC_NUM_RDX(scale) * BC_BASE_POWER + 2;

	rscale += req;
	bc_num_extend(&cpa, req);
	bc_num_extend(&cpb, req);

	fi = &factor;
	fnext = &factor2;
	cmp = 1;

	while (!bc_num_isOne(fi) && cmp) {

		s = bc_num_sub(&two, &cpb, fi, rscale);
		if (BC_ERROR_SIGNAL_ONLY(s)) goto err;
		s = bc_num_mul(&cpa, fi, &cpa, rscale);
		if (BC_ERROR_SIGNAL_ONLY(s)) goto err;
		s = bc_num_mul(&cpb, fi, &cpb, rscale);
		if (BC_ERROR_SIGNAL_ONLY(s)) goto err;

		temp = fi;
		fi = fnext;
		fnext = temp;

		cmp = bc_num_cmp(fi, fnext);
		if (cmp == BC_NUM_SSIZE_MIN) {
			s = BC_STATUS_SIGNAL;
			goto err;
		}
	}

	// We round to 2 extra places here. The reason is because without rounding,
	// the Goldschmidt algorithm can produce numbers that are 1 digit off in the
	// last place, because of the nature of the algorithm. If we only shift by
	// one place, the last digit in the number gets rounded, which is wrong
	// according to the bc spec (it needs to be truncated).
	// Note: I would really prefer an exact division algorithm here, even if
	// it's slightly slower than this one.
	bc_num_roundPlaces(&cpa, scale + 2);
	bc_num_copy(c, &cpa);

err:
	bc_num_free(&factor2);
	bc_num_free(&factor);
	bc_num_free(&cpb);
	bc_num_free(&cpa);
exit:
	a->neg = aneg;
	b->neg = bneg;
	if (BC_SIG) s = BC_STATUS_SIGNAL;
	if (BC_NO_ERR(!s)) bc_num_retireMul(c, scale, a->neg, b->neg);
	return s;
}

static BcStatus bc_num_r(BcNum *a, BcNum *b, BcNum *restrict c,
                         BcNum *restrict d, size_t scale, size_t ts)
{
	BcStatus s;
	BcNum temp;
	bool neg;

	if (BC_NUM_ZERO(b)) return bc_vm_err(BC_ERROR_MATH_DIVIDE_BY_ZERO);
	if (BC_NUM_ZERO(a)) {
		bc_num_setToZero(c, ts);
		bc_num_setToZero(d, ts);
		return BC_STATUS_SUCCESS;
	}

	bc_num_init(&temp, d->cap);
	s = bc_num_d(a, b, c, scale);
	assert(!s || s == BC_STATUS_SIGNAL);
	if (BC_ERROR_SIGNAL_ONLY(s)) goto err;

	if (scale) scale = ts + 1;

	s = bc_num_m(c, b, &temp, scale);
	if (BC_ERROR_SIGNAL_ONLY(s)) goto err;
	s = bc_num_sub(a, &temp, d, scale);
	if (BC_ERROR_SIGNAL_ONLY(s)) goto err;

	if (ts > d->scale && BC_NUM_NONZERO(d)) bc_num_extend(d, ts - d->scale);

	neg = d->neg;
	bc_num_retireMul(d, ts, a->neg, b->neg);
	d->neg = BC_NUM_NONZERO(d) ? neg : false;

err:
	bc_num_free(&temp);
	return s;
}

static BcStatus bc_num_rem(BcNum *a, BcNum *b, BcNum *restrict c, size_t scale)
{
	BcStatus s;
	BcNum c1;
	size_t ts;

	ts = bc_vm_growSize(scale, b->scale);
	ts = BC_MAX(ts, a->scale);

	bc_num_init(&c1, bc_num_mulReq(a, b, ts));
	s = bc_num_r(a, b, &c1, c, scale, ts);
	bc_num_free(&c1);

	return s;
}

static BcStatus bc_num_p(BcNum *a, BcNum *b, BcNum *restrict c, size_t scale) {

	BcStatus s = BC_STATUS_SUCCESS;
	BcNum copy;
	unsigned long pow = 0;
	size_t i, powrdx, resrdx;
	bool neg, zero;

	if (BC_ERR(b->rdx)) return bc_vm_err(BC_ERROR_MATH_NON_INTEGER);

	if (BC_NUM_ZERO(b)) {
		bc_num_one(c);
		return BC_STATUS_SUCCESS;
	}
	if (BC_NUM_ZERO(a)) {
		bc_num_setToZero(c, scale);
		return BC_STATUS_SUCCESS;
	}
	if (bc_num_isOne(b)) {
		if (!b->neg) bc_num_copy(c, a);
		else s = bc_num_inv(a, c, scale);
		return s;
	}

	neg = b->neg;
	b->neg = false;
	s = bc_num_ulong(b, &pow);
	b->neg = neg;
	if (s) return s;

	bc_num_createCopy(&copy, a);

	if (!neg) scale = BC_MIN(a->scale * pow, BC_MAX(scale, a->scale));

	for (powrdx = a->scale; BC_NO_SIG && !(pow & 1); pow >>= 1) {
		powrdx <<= 1;
		s = bc_num_mul(&copy, &copy, &copy, powrdx);
		if (BC_ERROR_SIGNAL_ONLY(s)) goto err;
	}

	if (BC_SIG) goto sig_err;

	bc_num_copy(c, &copy);
	resrdx = powrdx;

	while (BC_NO_SIG && (pow >>= 1)) {

		powrdx <<= 1;
		s = bc_num_mul(&copy, &copy, &copy, powrdx);
		if (BC_ERROR_SIGNAL_ONLY(s)) goto err;

		if (pow & 1) {
			resrdx += powrdx;
			s = bc_num_mul(c, &copy, c, resrdx);
			if (BC_ERROR_SIGNAL_ONLY(s)) goto err;
		}
	}

	if (BC_SIG) goto sig_err;
	if (neg) {
		s = bc_num_inv(c, c, scale);
		if (BC_ERROR_SIGNAL_ONLY(s)) goto err;
	}

	if (c->scale > scale) bc_num_truncate(c, c->scale - scale);

	// We can't use bc_num_clean() here.
	for (zero = true, i = 0; zero && i < c->len; ++i) zero = !c->num[i];
	if (zero) bc_num_setToZero(c, scale);

sig_err:
	if (BC_NO_ERR(!s) && BC_SIG) s = BC_STATUS_SIGNAL;
err:
	bc_num_free(&copy);
	return s;
}

#if BC_ENABLE_EXTRA_MATH
static BcStatus bc_num_place(BcNum *a, BcNum *b, BcNum *restrict c,
                             size_t scale)
{
	BcStatus s = BC_STATUS_SUCCESS;
	unsigned long val = 0;

	BC_UNUSED(scale);

	s = bc_num_intop(a, b, c, &val);
	if (BC_ERR(s)) return s;

	if (val < c->scale) bc_num_truncate(c, c->scale - val);
	else if (val > c->scale) bc_num_extend(c, val - c->scale);

	return s;
}

static BcStatus bc_num_left(BcNum *a, BcNum *b, BcNum *restrict c, size_t scale)
{
	BcStatus s = BC_STATUS_SUCCESS;
	unsigned long val = 0;

	BC_UNUSED(scale);

	s = bc_num_intop(a, b, c, &val);
	if (BC_ERR(s)) return s;

	return bc_num_shiftLeft(c, (size_t) val);
}

static BcStatus bc_num_right(BcNum *a, BcNum *b, BcNum *restrict c,
                             size_t scale)
{
	BcStatus s = BC_STATUS_SUCCESS;
	unsigned long val = 0;

	BC_UNUSED(scale);

	s = bc_num_intop(a, b, c, &val);
	if (BC_ERR(s)) return s;

	if (BC_NUM_ZERO(c)) return s;

	return bc_num_shiftRight(c, (size_t) val);
}
#endif // BC_ENABLE_EXTRA_MATH

static BcStatus bc_num_binary(BcNum *a, BcNum *b, BcNum *c, size_t scale,
                              BcNumBinaryOp op, size_t req)
{
	BcStatus s;
	BcNum num2, *ptr_a, *ptr_b;
	bool init = false;

	assert(a && b && c && op);

	if (c == a) {
		ptr_a = &num2;
		memcpy(ptr_a, c, sizeof(BcNum));
		init = true;
	}
	else ptr_a = a;

	if (c == b) {
		ptr_b = &num2;
		if (c != a) {
			memcpy(ptr_b, c, sizeof(BcNum));
			init = true;
		}
	}
	else ptr_b = b;

	if (init) bc_num_init(c, req);
	else bc_num_expand(c, req);

	s = op(ptr_a, ptr_b, c, scale);

	assert(!c->neg || BC_NUM_NONZERO(c));
	assert(c->rdx <= c->len || !c->len || s);

	if (init) bc_num_free(&num2);

	return s;
}

#ifndef NDEBUG
static bool bc_num_strValid(const char *val) {

	bool radix = false;
	size_t i, len = strlen(val);

	if (!len) return true;

	for (i = 0; i < len; ++i) {

		BcDig c = val[i];

		if (c == '.') {

			if (radix) return false;

			radix = true;
			continue;
		}

		if (!(isdigit(c) || isupper(c))) return false;
	}

	return true;
}
#endif // NDEBUG

static unsigned long bc_num_parseChar(char c, size_t base_t) {

	// TODO: Check this function.

	if (isupper(c)) {
		c = BC_NUM_NUM_LETTER(c);
		c = ((size_t) c) >= base_t ? (char) base_t - 1 : c;
	}
	else c -= '0';

	return (unsigned long) (uchar) c;
}

static void bc_num_parseDecimal(BcNum *restrict n, const char *restrict val) {

	size_t len, i, temp, mod;
	const char *ptr;
	bool zero = true, rdx;

	for (i = 0; val[i] == '0'; ++i);

	val += i;
	assert(isalnum(val[0]) || val[0] == '.');
	len = strlen(val);

	ptr = strchr(val, '.');
	rdx = (ptr != NULL);

	for (i = 0; i < len && (zero = (val[i] == '0' || val[i] == '.')); ++i);

	n->scale = (size_t) (rdx * ((val + len) - (ptr + 1)));
	n->rdx = BC_NUM_RDX(n->scale);

	i = len - (ptr == val ? 0 : i) - rdx;
	temp = BC_NUM_ROUND_POW(i);
	mod = n->scale % BC_BASE_POWER;
	i = mod ? BC_BASE_POWER - mod : 0;
	n->len = ((temp + i) / BC_BASE_POWER);

	bc_num_expand(n, n->len);
	memset(n->num, 0, BC_NUM_SIZE(n->len));

	if (zero) n->len = n->rdx = 0;
	else {

		unsigned long exp, pow;

		exp = i;
		pow = bc_num_pow10[exp];

		for (i = len - 1; i < len; --i, ++exp) {

			char c = val[i];

			if (c == '.') exp -= 1;
			else {

				size_t idx = exp / BC_BASE_POWER;

				if (isupper(c)) c = '9';
				n->num[idx] += (((unsigned long) c) - '0') * pow;

				if ((exp + 1) % BC_BASE_POWER == 0) pow = 1;
				else pow *= BC_BASE;
			}
		}
	}
}

static BcStatus bc_num_parseBase(BcNum *restrict n, const char *restrict val,
                                 BcNum *restrict base, size_t base_t)
{
	// TODO: Check this function.
	BcStatus s = BC_STATUS_SUCCESS;
	BcNum temp, mult, result;
	char c = 0;
	bool zero = true;
	unsigned long v;
	size_t i, digs, len = strlen(val);

	for (i = 0; zero && i < len; ++i) zero = (val[i] == '.' || val[i] == '0');
	if (zero) return BC_STATUS_SUCCESS;

	bc_num_init(&temp, BC_NUM_LONG_LOG10);
	bc_num_init(&mult, BC_NUM_LONG_LOG10);

	for (i = 0; i < len && (c = val[i]) && c != '.'; ++i) {

		v = bc_num_parseChar(c, base_t);

		s = bc_num_mul(n, base, &mult, 0);
		if (BC_ERROR_SIGNAL_ONLY(s)) goto int_err;
		bc_num_ulong2num(&temp, v);
		s = bc_num_add(&mult, &temp, n, 0);
		if (BC_ERROR_SIGNAL_ONLY(s)) goto int_err;
	}

	if (i == len && !(c = val[i])) goto int_err;

	assert(c == '.');
	bc_num_init(&result, base->len);
	bc_num_one(&mult);

	for (i += 1, digs = 0; BC_NO_SIG && i < len && (c = val[i]); ++i, ++digs)
	{
		v = bc_num_parseChar(c, base_t);

		s = bc_num_mul(&result, base, &result, 0);
		if (BC_ERROR_SIGNAL_ONLY(s)) goto err;

		bc_num_ulong2num(&temp, v);
		s = bc_num_add(&result, &temp, &result, 0);
		if (BC_ERROR_SIGNAL_ONLY(s)) goto err;
		s = bc_num_mul(&mult, base, &mult, 0);
		if (BC_ERROR_SIGNAL_ONLY(s)) goto err;
	}

	if (BC_SIG) {
		s = BC_STATUS_SIGNAL;
		goto err;
	}

	// This one cannot be a divide by 0 because mult starts out at 1, then is
	// multiplied by base, and base cannot be 0, so mult cannot be 0.
	s = bc_num_div(&result, &mult, &result, digs * 2);
	assert(!s || s == BC_STATUS_SIGNAL);
	if (BC_ERROR_SIGNAL_ONLY(s)) goto err;
	bc_num_truncate(&result, digs);
	s = bc_num_add(n, &result, n, digs);
	if (BC_ERROR_SIGNAL_ONLY(s)) goto err;

	if (BC_NUM_NONZERO(n)) {
		if (n->scale < digs) bc_num_extend(n, digs - n->scale);
	}
	else bc_num_zero(n);

err:
	bc_num_free(&result);
int_err:
	bc_num_free(&mult);
	bc_num_free(&temp);
	return s;
}

static void bc_num_printNewline() {
	if (vm->nchars >= (size_t) (vm->line_len - 1)) {
		bc_vm_putchar('\\');
		bc_vm_putchar('\n');
		vm->nchars = 0;
	}
}

#if DC_ENABLED
static void bc_num_printChar(size_t n, size_t len, bool rdx) {
	BC_UNUSED(rdx);
	bc_vm_putchar((uchar) n);
	vm->nchars += len;
}
#endif // DC_ENABLED

static void bc_num_printDigits(size_t n, size_t len, bool rdx) {

	// TODO: Check this function.

	size_t exp, pow;

	bc_num_printNewline();
	bc_vm_putchar(rdx ? '.' : ' ');
	++vm->nchars;

	bc_num_printNewline();
	for (exp = 0, pow = 1; exp < len - 1; ++exp, pow *= BC_BASE);

	for (exp = 0; exp < len; pow /= BC_BASE, ++vm->nchars, ++exp) {
		size_t dig;
		bc_num_printNewline();
		dig = n / pow;
		n -= dig * pow;
		bc_vm_putchar(((uchar) dig) + '0');
	}
}

static void bc_num_printHex(size_t n, size_t len, bool rdx) {

	assert(len == 1);

	if (rdx) {
		bc_num_printNewline();
		bc_vm_putchar('.');
		vm->nchars += 1;
	}

	bc_num_printNewline();
	bc_vm_putchar(bc_num_hex_digits[n]);
	vm->nchars += len;
}

static void bc_num_printDecimal(const BcNum *restrict n) {

	size_t i, j, rdx = n->rdx;
	bool zero = true;
	size_t buffer[BC_BASE_POWER];

	if (n->neg) bc_vm_putchar('-');
	vm->nchars += n->neg;

	for (i = n->len - 1; i < n->len; --i) {

		BcDig n9 = n->num[i];
		size_t temp;
		bool irdx = (i == rdx - 1);

		zero = (zero & !irdx);
		temp = n->scale % BC_BASE_POWER;
		temp = i || !temp ? 0 : BC_BASE_POWER - temp;

		memset(buffer, 0, BC_BASE_POWER * sizeof(size_t));

		for (j = 0; n9 && j < BC_BASE_POWER; ++j) {
			buffer[j] = n9 % BC_BASE;
			n9 /= BC_BASE;
		}

		for (j = BC_BASE_POWER - 1; j < BC_BASE_POWER && j >= temp; --j) {
			bool print_rdx = (irdx & (j == BC_BASE_POWER - 1));
			zero = (zero && buffer[j] == 0);
			if (!zero) bc_num_printHex(buffer[j], 1, print_rdx);
		}
	}
}

#if BC_ENABLE_EXTRA_MATH
static BcStatus bc_num_printExponent(const BcNum *restrict n, bool eng) {

	// TODO: Check this function.

	BcStatus s = BC_STATUS_SUCCESS;
	bool neg = (n->len <= n->rdx);
	BcNum temp, exp;
	size_t places, mod;
	BcDig digs[BC_NUM_LONG_LOG10];

	bc_num_createCopy(&temp, n);

	if (neg) {

		size_t i, idx = bc_num_nonzeroLen(n) - 1;

		places = 1;

		for (i = BC_BASE_POWER - 1; i < BC_BASE_POWER; --i) {
			if (bc_num_pow10[i] > (unsigned long) n->num[idx])
				places += 1;
			else break;
		}

		places += (n->rdx - (idx + 1)) * BC_BASE_POWER;

		mod = places % 3;

		if (eng && mod != 0) places += 3 - mod;
		s = bc_num_shiftLeft(&temp, places);
		if (BC_ERROR_SIGNAL_ONLY(s)) goto exit;
	}
	else {
		places = bc_num_int_digits(n) - 1;
		mod = places % 3;
		if (eng && mod != 0) places -= 3 - (3 - mod);
		s = bc_num_shiftRight(&temp, places);
		if (BC_ERROR_SIGNAL_ONLY(s)) goto exit;
	}

	bc_num_printDecimal(&temp);

	bc_num_printNewline();
	bc_vm_putchar('e');

	if (!places) {
		bc_num_printHex(0, 1, false);
		goto exit;
	}

	if (neg) {
		bc_num_printNewline();
		bc_vm_putchar('-');
	}

	bc_num_setup(&exp, digs, BC_NUM_LONG_LOG10);
	bc_num_ulong2num(&exp, (unsigned long) places);

	bc_num_printDecimal(&exp);

exit:
	bc_num_free(&temp);
	return BC_SIG ? BC_STATUS_SIGNAL : BC_STATUS_SUCCESS;
}
#endif // BC_ENABLE_EXTRA_MATH

static BcStatus bc_num_printNum(BcNum *restrict n, BcNum *restrict base,
                                size_t len, BcNumDigitOp print)
{
	// TODO: Check this function.
	BcStatus s;
	BcVec stack;
	BcNum intp, fracp, digit, frac_len;
	unsigned long dig, *ptr;
	size_t i;
	bool radix;

	assert(BC_NUM_NONZERO(base));

	if (BC_NUM_ZERO(n)) {
		print(0, len, false);
		return BC_STATUS_SUCCESS;
	}

	bc_vec_init(&stack, sizeof(unsigned long), NULL);
	bc_num_init(&fracp, n->rdx);
	bc_num_init(&digit, len);
	bc_num_init(&frac_len, bc_num_int(n));
	bc_num_one(&frac_len);
	bc_num_createCopy(&intp, n);

	bc_num_truncate(&intp, intp.rdx);
	s = bc_num_sub(n, &intp, &fracp, 0);
	if (BC_ERROR_SIGNAL_ONLY(s)) goto err;

	while (BC_NO_SIG && BC_NUM_NONZERO(&intp)) {

		// Dividing by base cannot be divide by 0 because base cannot be 0.
		s = bc_num_divmod(&intp, base, &intp, &digit, 0);
		if (BC_ERROR_SIGNAL_ONLY(s)) goto err;

		// Will never fail (except for signals) because digit is
		// guaranteed to be non-negative and small enough.
		s = bc_num_ulong(&digit, &dig);
		if (BC_ERROR_SIGNAL_ONLY(s)) goto err;

		bc_vec_push(&stack, &dig);
	}

	if (BC_SIG) goto sig_err;

	for (i = 0; BC_NO_SIG && i < stack.len; ++i) {
		ptr = bc_vec_item_rev(&stack, i);
		assert(ptr);
		print(*ptr, len, false);
	}

	if (BC_SIG) goto sig_err;
	if (!n->rdx) goto err;

	for (radix = true; BC_NO_SIG && frac_len.len <= n->rdx; radix = false) {

		s = bc_num_mul(&fracp, base, &fracp, n->rdx);
		if (BC_ERROR_SIGNAL_ONLY(s)) goto err;

		// Will never fail (except for signals) because fracp is
		// guaranteed to be non-negative and small enough.
		s = bc_num_ulong(&fracp, &dig);
		if (BC_ERROR_SIGNAL_ONLY(s)) goto err;

		bc_num_ulong2num(&intp, dig);
		s = bc_num_sub(&fracp, &intp, &fracp, 0);
		if (BC_ERROR_SIGNAL_ONLY(s)) goto err;

		print(dig, len, radix);
		s = bc_num_mul(&frac_len, base, &frac_len, 0);
		if (BC_ERROR_SIGNAL_ONLY(s)) goto err;
	}

sig_err:
	if (BC_NO_ERR(!s) && BC_SIG) s = BC_STATUS_SIGNAL;
err:
	bc_num_free(&frac_len);
	bc_num_free(&digit);
	bc_num_free(&fracp);
	bc_num_free(&intp);
	bc_vec_free(&stack);
	return s;
}

static BcStatus bc_num_printBase(BcNum *restrict n, BcNum *restrict base,
                                 size_t base_t)
{
	// TODO: Check this function.
	BcStatus s;
	size_t width;
	BcNumDigitOp print;
	bool neg = n->neg;

	if (neg) bc_vm_putchar('-');
	vm->nchars += neg;

	n->neg = false;

	if (base_t <= BC_NUM_MAX_POSIX_IBASE) {
		width = 1;
		print = bc_num_printHex;
	}
	else {
		width = bc_num_log10(base_t - 1) - 1;
		print = bc_num_printDigits;
	}

	s = bc_num_printNum(n, base, width, print);
	n->neg = neg;

	return s;
}

#if DC_ENABLED
BcStatus bc_num_stream(BcNum *restrict n, BcNum *restrict base) {
	return bc_num_printNum(n, base, 1, bc_num_printChar);
}
#endif // DC_ENABLED

void bc_num_setup(BcNum *restrict n, BcDig *restrict num, size_t cap) {
	assert(n);
	n->num = num;
	n->cap = cap;
	n->rdx = n->scale = n->len = 0;
	n->neg = false;
}

void bc_num_init(BcNum *restrict n, size_t req) {
	assert(n);
	req = req >= BC_NUM_DEF_SIZE ? req : BC_NUM_DEF_SIZE;
	bc_num_setup(n, bc_vm_malloc(BC_NUM_SIZE(req)), req);
}

void bc_num_free(void *num) {
	assert(num);
	free(((BcNum*) num)->num);
}

void bc_num_copy(BcNum *d, const BcNum *s) {
	assert(d && s);
	if (d == s) return;
	bc_num_expand(d, s->len);
	d->len = s->len;
	d->neg = s->neg;
	d->rdx = s->rdx;
	d->scale = s->scale;
	memcpy(d->num, s->num, BC_NUM_SIZE(d->len));
}

void bc_num_createCopy(BcNum *d, const BcNum *s) {
	bc_num_init(d, s->len);
	bc_num_copy(d, s);
}

void bc_num_createFromUlong(BcNum *n, unsigned long val) {
	bc_num_init(n, (BC_NUM_LONG_LOG10 - 1) / BC_BASE_POWER + 1);
	bc_num_ulong2num(n, val);
}

size_t bc_num_scale(const BcNum *restrict n) {
	return n->scale;
}

size_t bc_num_len(const BcNum *restrict n) {

	size_t i, pow, scale, len = n->len;
	BcDig dig;

	if (BC_NUM_ZERO(n)) return 0;
	if (n->rdx == len) len = bc_num_nonzeroLen(n);

	dig = n->num[len - 1];
	pow = BC_BASE_DIG;
	i = BC_BASE_POWER + 1;

	while (pow && (dig % (BcDig) pow == dig)) {
		i -= 1;
		pow /= BC_BASE;
	}

	scale = n->scale % BC_BASE_POWER;
	scale = scale ? scale : BC_BASE_POWER;

	return (len - 1) * BC_BASE_POWER + i - (BC_BASE_POWER - scale);
}

BcStatus bc_num_parse(BcNum *restrict n, const char *restrict val,
                      BcNum *restrict base, size_t base_t, bool letter)
{
	BcStatus s = BC_STATUS_SUCCESS;

	assert(n && val && base);
	assert(BC_ENABLE_EXTRA_MATH ||
	       (base_t >= BC_NUM_MIN_BASE && base_t <= vm->max_ibase));
	assert(bc_num_strValid(val));

	if (letter) bc_num_ulong2num(n, bc_num_parseChar(val[0], BC_NUM_MAX_LBASE));
	else if (base_t == BC_BASE) bc_num_parseDecimal(n, val);
	else s = bc_num_parseBase(n, val, base, base_t);

	return s;
}

BcStatus bc_num_print(BcNum *restrict n, BcNum *restrict base,
                      size_t base_t, bool newline)
{
	BcStatus s = BC_STATUS_SUCCESS;

	assert(n && base);
	assert(BC_ENABLE_EXTRA_MATH || base_t >= BC_NUM_MIN_BASE);

	bc_num_printNewline();

	if (BC_NUM_ZERO(n)) bc_num_printHex(0, 1, false);
	else if (base_t == BC_BASE) bc_num_printDecimal(n);
#if BC_ENABLE_EXTRA_MATH
	else if (base_t == 0 || base_t == 1)
		s = bc_num_printExponent(n, base_t != 0);
#endif // BC_ENABLE_EXTRA_MATH
	else s = bc_num_printBase(n, base, base_t);

	if (BC_NO_ERR(!s) && newline) {
		bc_vm_putchar('\n');
		vm->nchars = 0;
	}

	return s;
}

BcStatus bc_num_ulong(const BcNum *restrict n, unsigned long *result) {

	size_t i;
	unsigned long r;

	assert(n && result);

	*result = 0;

	if (BC_ERR(n->neg)) return bc_vm_err(BC_ERROR_MATH_NEGATIVE);

	for (r = 0, i = n->len; i > n->rdx;) {

		unsigned long prev = r * BC_BASE_DIG;

		if (BC_ERR(prev == SIZE_MAX || prev / BC_BASE_DIG != r))
			return bc_vm_err(BC_ERROR_MATH_OVERFLOW);

		r = prev + (unsigned long) n->num[--i];

		if (BC_ERR(r == SIZE_MAX || r < prev))
			return bc_vm_err(BC_ERROR_MATH_OVERFLOW);
	}

	*result = r;

	return BC_STATUS_SUCCESS;
}

void bc_num_ulong2num(BcNum *restrict n, unsigned long val) {

	BcDig *ptr;
	unsigned long i;

	assert(n);

	bc_num_zero(n);

	if (!val) return;

	bc_num_expand(n, bc_num_log10(ULONG_MAX));

	for (ptr = n->num, i = 0; val; ++i, ++n->len, val /= BC_BASE_DIG)
		ptr[i] = val % BC_BASE_DIG;
}

size_t bc_num_addReq(BcNum *a, BcNum *b, size_t scale) {

	size_t aint, bint, ardx, brdx;

	BC_UNUSED(scale);

	ardx = a->rdx;
	brdx = b->rdx;
	aint = bc_num_int(a);
	bint = bc_num_int(b);
	ardx = BC_MAX(ardx, brdx);
	aint = BC_MAX(aint, bint);

	return bc_vm_growSize(bc_vm_growSize(ardx, aint), 1);
}

size_t bc_num_mulReq(BcNum *a, BcNum *b, size_t scale) {
	size_t max, rdx;
	rdx = bc_vm_growSize(a->rdx, b->rdx);
	max = bc_vm_growSize(BC_MAX(BC_NUM_RDX(scale), rdx), 1);
	rdx = bc_vm_growSize(bc_vm_growSize(bc_num_int(a), bc_num_int(b)), max);
	return rdx;
}

size_t bc_num_powReq(BcNum *a, BcNum *b, size_t scale) {
	BC_UNUSED(scale);
	return bc_vm_growSize(bc_vm_growSize(a->len, b->len), 1);
}

#if BC_ENABLE_EXTRA_MATH
size_t bc_num_shiftReq(BcNum *a, BcNum *b, size_t scale) {
	BC_UNUSED(b);
	BC_UNUSED(scale);
	return BC_NUM_SHREQ(a);
}
#endif // BC_ENABLE_EXTRA_MATH

BcStatus bc_num_add(BcNum *a, BcNum *b, BcNum *c, size_t scale) {
	BcNumBinaryOp op = (!a->neg == !b->neg) ? bc_num_a : bc_num_s;
	BC_UNUSED(scale);
	return bc_num_binary(a, b, c, false, op, bc_num_addReq(a, b, scale));
}

BcStatus bc_num_sub(BcNum *a, BcNum *b, BcNum *c, size_t scale) {
	BcNumBinaryOp op = (!a->neg == !b->neg) ? bc_num_s : bc_num_a;
	BC_UNUSED(scale);
	return bc_num_binary(a, b, c, true, op, bc_num_addReq(a, b, scale));
}

BcStatus bc_num_mul(BcNum *a, BcNum *b, BcNum *c, size_t scale) {
	return bc_num_binary(a, b, c, scale, bc_num_m, bc_num_mulReq(a, b, scale));
}

BcStatus bc_num_div(BcNum *a, BcNum *b, BcNum *c, size_t scale) {
	return bc_num_binary(a, b, c, scale, bc_num_d, bc_num_mulReq(a, b, scale));
}

BcStatus bc_num_mod(BcNum *a, BcNum *b, BcNum *c, size_t scale) {
	size_t req = bc_num_mulReq(a, b, scale);
	return bc_num_binary(a, b, c, scale, bc_num_rem, req);
}

BcStatus bc_num_pow(BcNum *a, BcNum *b, BcNum *c, size_t scale) {
	return bc_num_binary(a, b, c, scale, bc_num_p, bc_num_powReq(a, b, scale));
}

#if BC_ENABLE_EXTRA_MATH
BcStatus bc_num_places(BcNum *a, BcNum *b, BcNum *c, size_t scale) {
	return bc_num_binary(a, b, c, scale, bc_num_place, BC_NUM_SHREQ(a));
}

BcStatus bc_num_lshift(BcNum *a, BcNum *b, BcNum *c, size_t scale) {
	return bc_num_binary(a, b, c, scale, bc_num_left, BC_NUM_SHREQ(a));
}

BcStatus bc_num_rshift(BcNum *a, BcNum *b, BcNum *c, size_t scale) {
	return bc_num_binary(a, b, c, scale, bc_num_right, BC_NUM_SHREQ(a));
}
#endif // BC_ENABLE_EXTRA_MATH

BcStatus bc_num_sqrt(BcNum *restrict a, BcNum *restrict b, size_t scale) {

	// TODO: Check this function.

	BcStatus s = BC_STATUS_SUCCESS;
	BcNum num1, num2, half, f, fprime, *x0, *x1, *temp;
	size_t pow, len, rdx, req, digs, digs1, digs2, resscale, times = 0;
	ssize_t cmp = 1, cmp1 = SSIZE_MAX, cmp2 = SSIZE_MAX;
	BcDig half_digs[1];

	assert(a && b && a != b);

	if (BC_ERR(a->neg)) return bc_vm_err(BC_ERROR_MATH_NEGATIVE);

	if (a->scale > scale) scale = a->scale;
	len = bc_vm_growSize(bc_num_int_digits(a), 1);
	req = bc_vm_growSize(BC_MAX(BC_NUM_RDX(scale), a->rdx), len >> 1);
	bc_num_init(b, bc_vm_growSize(req, 1));

	if (BC_NUM_ZERO(a)) {
		bc_num_setToZero(b, scale);
		return BC_STATUS_SUCCESS;
	}
	if (bc_num_isOne(a)) {
		bc_num_one(b);
		bc_num_extend(b, scale);
		return BC_STATUS_SUCCESS;
	}

	rdx = BC_MAX(BC_NUM_RDX(scale), a->rdx);
	len = bc_vm_growSize(a->len, rdx);

	bc_num_init(&num1, len);
	bc_num_init(&num2, len);
	bc_num_setup(&half, half_digs, sizeof(half_digs) / sizeof(BcDig));

	bc_num_one(&half);
	half.num[0] = BC_BASE_DIG / 2;
	half.len = 1;
	half.rdx = 1;
	half.scale = 1;

	bc_num_init(&f, len);
	bc_num_init(&fprime, len);

	x0 = &num1;
	x1 = &num2;

	bc_num_one(x0);
	pow = bc_num_int_digits(a);

	if (pow) {

		if (pow & 1) x0->num[0] = 2;
		else x0->num[0] = 6;

		pow -= 2 - (pow & 1);
		bc_num_shiftLeft(x0, pow / 2);

		// Make sure to move the radix back.
		x0->scale -= pow;
		x0->rdx = BC_NUM_RDX(x0->scale);
	}

	x0->rdx = digs = digs1 = digs2 = 0;
	x0->scale = 0;
	resscale = (scale + BC_BASE_POWER) * 2;

	len = BC_NUM_RDX(bc_num_int_digits(x0) + resscale - 1); // <se> apply -1 after BC_NUM_RDX???

	while (BC_NO_SIG && (cmp || digs < len)) {

		assert(BC_NUM_NONZERO(x0));

		s = bc_num_div(a, x0, &f, resscale);
		assert(!s || s == BC_STATUS_SIGNAL);
		if (BC_ERROR_SIGNAL_ONLY(s)) goto err;
		s = bc_num_add(x0, &f, &fprime, resscale);
		if (BC_ERROR_SIGNAL_ONLY(s)) goto err;
		s = bc_num_mul(&fprime, &half, x1, resscale);
		if (BC_ERROR_SIGNAL_ONLY(s)) goto err;

		cmp = bc_num_cmp(x1, x0);
		if (cmp == BC_NUM_SSIZE_MIN) {
			s = BC_STATUS_SIGNAL;
			break;
		}

		digs = x1->len - (unsigned long long) llabs(cmp);

		if (cmp == cmp2 && digs == digs1) times += 1;
		else times = 0;

		resscale += times > 2;

		cmp2 = cmp1;
		cmp1 = cmp;
		digs1 = digs;

		temp = x0;
		x0 = x1;
		x1 = temp;
	}

	if (BC_SIG) {
		s = BC_STATUS_SIGNAL;
		goto err;
	}

	bc_num_copy(b, x0);
	if (b->scale > scale) bc_num_truncate(b, b->scale - scale);

err:
	if (BC_ERR(s)) bc_num_free(b);
	bc_num_free(&fprime);
	bc_num_free(&f);
	bc_num_free(&num2);
	bc_num_free(&num1);
	assert(!b->neg || BC_NUM_NONZERO(b));
	assert(b->rdx <= b->len || !b->len);
	return s;
}

BcStatus bc_num_divmod(BcNum *a, BcNum *b, BcNum *c, BcNum *d, size_t scale) {

	// TODO: Check this function.

	BcStatus s;
	BcNum num2, *ptr_a;
	bool init = false;
	size_t ts, len;

	ts = BC_MAX(scale + b->scale, a->scale);
	len = bc_num_mulReq(a, b, ts);

	assert(c != d && a != d && b != d && b != c);

	if (c == a) {
		memcpy(&num2, c, sizeof(BcNum));
		ptr_a = &num2;
		bc_num_init(c, len);
		init = true;
	}
	else {
		ptr_a = a;
		bc_num_expand(c, len);
	}

	s = bc_num_r(ptr_a, b, c, d, scale, ts);

	assert(!c->neg || BC_NUM_NONZERO(c));
	assert(c->rdx <= c->len || !c->len);
	assert(!d->neg || BC_NUM_NONZERO(d));
	assert(d->rdx <= d->len || !d->len);

	if (init) bc_num_free(&num2);

	return s;
}

#if DC_ENABLED
BcStatus bc_num_modexp(BcNum *a, BcNum *b, BcNum *c, BcNum *restrict d) {

	// TODO: Check this function.

	BcStatus s;
	BcNum base, exp, two, temp;
	BcDig two_digs[2];

	assert(a && b && c && d && a != d && b != d && c != d);

	if (BC_ERR(BC_NUM_ZERO(c)))
		return bc_vm_err(BC_ERROR_MATH_DIVIDE_BY_ZERO);
	if (BC_ERR(b->neg)) return bc_vm_err(BC_ERROR_MATH_NEGATIVE);
	if (BC_ERR(a->rdx || b->rdx || c->rdx))
		return bc_vm_err(BC_ERROR_MATH_NON_INTEGER);

	bc_num_expand(d, c->len);
	bc_num_init(&base, c->len);
	bc_num_setup(&two, two_digs, sizeof(two_digs) / sizeof(BcDig));
	bc_num_init(&temp, b->len);

	bc_num_one(&two);
	two.num[0] = 2;
	bc_num_one(d);

	// We already checked for 0.
	s = bc_num_rem(a, c, &base, 0);
	assert(!s || s == BC_STATUS_SIGNAL);
	if (BC_ERROR_SIGNAL_ONLY(s)) goto rem_err;
	bc_num_createCopy(&exp, b);

	while (BC_NO_SIG && BC_NUM_NONZERO(&exp)) {

		// Num two cannot be 0, so no errors.
		s = bc_num_divmod(&exp, &two, &exp, &temp, 0);
		if (BC_ERROR_SIGNAL_ONLY(s)) goto err;

		if (bc_num_isOne(&temp)) {
			s = bc_num_mul(d, &base, &temp, 0);
			if (BC_ERROR_SIGNAL_ONLY(s)) goto err;

			// We already checked for 0.
			s = bc_num_rem(&temp, c, d, 0);
			assert(!s || s == BC_STATUS_SIGNAL);
			if (BC_ERROR_SIGNAL_ONLY(s)) goto err;
		}

		s = bc_num_mul(&base, &base, &temp, 0);
		if (BC_ERROR_SIGNAL_ONLY(s)) goto err;

		// We already checked for 0.
		s = bc_num_rem(&temp, c, &base, 0);
		assert(!s || s == BC_STATUS_SIGNAL);
		if (BC_ERROR_SIGNAL_ONLY(s)) goto err;
	}

	if (BC_NO_ERR(!s) && BC_SIG) s = BC_STATUS_SIGNAL;

err:
	bc_num_free(&exp);
rem_err:
	bc_num_free(&temp);
	bc_num_free(&base);
	assert(!d->neg || d->len);
	return s;
}
#endif // DC_ENABLED
