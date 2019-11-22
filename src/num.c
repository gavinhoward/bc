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

static void bc_num_expand(BcNum *restrict n, size_t req) {
	assert(n != NULL);
	req = req >= BC_NUM_DEF_SIZE ? req : BC_NUM_DEF_SIZE;
	if (req > n->cap) {
		n->num = bc_vm_realloc(n->num, BC_NUM_SIZE(req));
		n->cap = req;
	}
}

static void bc_num_setToZero(BcNum *restrict n, size_t scale) {
	assert(n != NULL);
	n->scale = scale;
	n->len = n->rdx = 0;
	n->neg = false;
}

static void bc_num_zero(BcNum *restrict n) {
	bc_num_setToZero(n, 0);
}

void bc_num_one(BcNum *restrict n) {
	bc_num_zero(n);
	n->len = 1;
	n->num[0] = 1;
}

static void bc_num_clean(BcNum *restrict n) {
	while (BC_NUM_NONZERO(n) && !n->num[n->len - 1]) n->len -= 1;
	if (BC_NUM_ZERO(n)) {
		n->neg = false;
		n->rdx = 0;
	}
	else if (n->len < n->rdx) n->len = n->rdx;
}

static size_t bc_num_log10(size_t i) {
	size_t len;
	for (len = 1; i; i /= BC_BASE, ++len);
	assert(len - 1 <= BC_BASE_DIGS + 1);
	return len - 1;
}

static size_t bc_num_zeroDigits(const BcDig *n) {
	return BC_BASE_DIGS - bc_num_log10((size_t) *n);
}

static size_t bc_num_intDigits(const BcNum *n) {
	size_t digits = bc_num_int(n) * BC_BASE_DIGS;
	if (digits > 0) digits -= bc_num_zeroDigits(n->num + n->len - 1);
	return digits;
}

static size_t bc_num_nonzeroLen(const BcNum *restrict n) {
	size_t i, len = n->len;
	assert(len == n->rdx);
	for (i = len - 1; i < len && !n->num[i]; --i);
	assert(i + 1 > 0);
	return i + 1;
}

static BcDig bc_num_addDigits(BcDig a, BcDig b, bool *carry) {

	assert(((BcBigDig) BC_BASE_POW) * 2 == ((BcDig) BC_BASE_POW) * 2);
	assert(a < BC_BASE_POW);
	assert(b < BC_BASE_POW);

	a += b + *carry;
	*carry = (a >= BC_BASE_POW);
	if (*carry) a -= BC_BASE_POW;

	assert(a >= 0);
	assert(a < BC_BASE_POW);

	return a;
}

static BcDig bc_num_subDigits(BcDig a, BcDig b, bool *carry) {

	assert(a < BC_BASE_POW);
	assert(b < BC_BASE_POW);

	b += *carry;
	*carry = (a < b);
	if (*carry) a += BC_BASE_POW;

	assert(a - b >= 0);
	assert(a - b < BC_BASE_POW);

	return a - b;
}

static BcStatus bc_num_addArrays(BcDig *restrict a, const BcDig *restrict b,
                                 size_t len)
{
	size_t i;
	bool carry = false;

	for (i = 0; BC_NO_SIG && i < len; ++i)
		a[i] = bc_num_addDigits(a[i], b[i], &carry);

	for (; BC_NO_SIG && carry; ++i)
		a[i] = bc_num_addDigits(a[i], 0, &carry);

	return BC_SIG ? BC_STATUS_SIGNAL : BC_STATUS_SUCCESS;
}

static BcStatus bc_num_subArrays(BcDig *restrict a, const BcDig *restrict b,
                                 size_t len)
{
	size_t i;
	bool carry = false;

	for (i = 0; BC_NO_SIG && i < len; ++i)
		a[i] = bc_num_subDigits(a[i], b[i], &carry);

	for (; BC_NO_SIG && carry; ++i)
		a[i] = bc_num_subDigits(a[i], 0, &carry);

	return BC_SIG ? BC_STATUS_SIGNAL : BC_STATUS_SUCCESS;
}

static BcStatus bc_num_mulArray(const BcNum *restrict a, BcBigDig b,
                                BcNum *restrict c)
{
	size_t i;
	BcBigDig carry = 0;

	assert(b <= BC_BASE_POW);

	if (a->len + 1 > c->cap) bc_num_expand(c, a->len + 1);

	memset(c->num, 0, BC_NUM_SIZE(c->cap));

	for (i = 0; BC_NO_SIG && i < a->len; ++i) {
		BcBigDig in = ((BcBigDig) a->num[i]) * b + carry;
		c->num[i] = in % BC_BASE_POW;
		carry = in / BC_BASE_POW;
	}

	if (BC_NO_SIG) {
		assert(carry < BC_BASE_POW);
		c->num[i] = (BcDig) carry;
		c->len = a->len;
		c->len += (carry != 0);
	}

	bc_num_clean(c);

	assert(!c->neg || BC_NUM_NONZERO(c));
	assert(c->rdx <= c->len || !c->len || BC_SIG);
	assert(!c->len || c->num[c->len - 1] || c->rdx == c->len);

	return BC_SIG ? BC_STATUS_SIGNAL : BC_STATUS_SUCCESS;
}

static BcStatus bc_num_divArray(const BcNum *restrict a, BcBigDig b,
                                BcNum *restrict c, BcBigDig *rem)
{
	size_t i;
	BcBigDig carry = 0;

	assert(c->cap >= a->len);

	for (i = a->len - 1; BC_NO_SIG && i < a->len; --i) {
		BcBigDig in = ((BcBigDig) a->num[i]) + carry * BC_BASE_POW;
		assert(in / b < BC_BASE_POW);
		c->num[i] = (BcDig) (in / b);
		carry = in % b;
	}

	c->len = a->len;
	bc_num_clean(c);
	*rem = carry;

	assert(!c->neg || BC_NUM_NONZERO(c));
	assert(c->rdx <= c->len || !c->len || BC_SIG);
	assert(!c->len || c->num[c->len - 1] || c->rdx == c->len);

	return BC_SIG ? BC_STATUS_SIGNAL : BC_STATUS_SUCCESS;
}

static ssize_t bc_num_compare(const BcDig *restrict a, const BcDig *restrict b,
                              size_t len)
{
	size_t i;
	BcDig c = 0;
	for (i = len - 1; BC_NO_SIG && i < len && !(c = a[i] - b[i]); --i);
	return BC_SIG ? BC_NUM_CMP_SIGNAL_VAL : bc_num_neg(i + 1, c < 0);
}

ssize_t bc_num_cmp(const BcNum *a, const BcNum *b) {

	size_t i, min, a_int, b_int, diff;
	BcDig *max_num, *min_num;
	bool a_max, neg = false;
	ssize_t cmp;

	assert(a != NULL && b != NULL);

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

	if (a_int) return neg ? -((ssize_t) a_int) : (ssize_t) a_int;

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

#if BC_ENABLE_SIGNALS
	if (BC_NUM_CMP_SIGNAL(cmp)) return cmp;
#endif // BC_ENABLE_SIGNALS

	if (cmp) return bc_num_neg((size_t) cmp, !a_max == !neg);

	for (max_num -= diff, i = diff - 1; BC_NO_SIG && i < diff; --i) {
		if (max_num[i]) return bc_num_neg(1, !a_max == !neg);
	}

	return BC_SIG ? BC_NUM_CMP_SIGNAL_VAL : 0;
}

void bc_num_truncate(BcNum *restrict n, size_t places) {

	size_t places_rdx;

	if (!places) return;

	places_rdx = n->rdx ? n->rdx - BC_NUM_RDX(n->scale - places) : 0;
	assert(places <= n->scale && (BC_NUM_ZERO(n) || places_rdx <= n->len));

	n->scale -= places;
	n->rdx -= places_rdx;

	if (BC_NUM_NONZERO(n)) {

		size_t pow;

		pow = n->scale % BC_BASE_DIGS;
		pow = pow ? BC_BASE_DIGS - pow : 0;
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
	if (BC_NUM_ZERO(n)) {
		n->scale += places;
		return;
	}

	places_rdx = BC_NUM_RDX(places + n->scale) - n->rdx;

	if (places_rdx) {
		bc_num_expand(n, bc_vm_growSize(n->len, places_rdx));
		memmove(n->num + places_rdx, n->num, BC_NUM_SIZE(n->len));
		memset(n->num, 0, BC_NUM_SIZE(places_rdx));
	}

	n->rdx += places_rdx;
	n->scale += places;
	n->len += places_rdx;

	assert(n->rdx == BC_NUM_RDX(n->scale));
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
	assert(BC_NUM_ZERO(a));
	assert(BC_NUM_ZERO(b));

	if (idx < n->len) {

		b->len = n->len - idx;
		a->len = idx;
		a->scale = a->rdx = b->scale = b->rdx = 0;

		assert(a->cap >= a->len);
		assert(b->cap >= b->len);

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

static BcStatus bc_num_shift(BcNum *restrict n, BcBigDig dig) {

	size_t i, len = n->len;
	BcBigDig carry = 0, pow;
	BcDig *ptr = n->num;

	assert(dig < BC_BASE_DIGS);

	pow = bc_num_pow10[dig];
	dig = bc_num_pow10[BC_BASE_DIGS - dig];

	for (i = len - 1; BC_NO_SIG && i < len; --i) {
		BcBigDig in, temp;
		in = ((BcBigDig) ptr[i]);
		temp = carry * dig;
		carry = in % pow;
		ptr[i] = ((BcDig) (in / pow)) + (BcDig) temp;
	}

	assert(!carry);

	return BC_SIG ? BC_STATUS_SIGNAL : BC_STATUS_SUCCESS;
}

static BcStatus bc_num_shiftLeft(BcNum *restrict n, size_t places) {

	BcStatus s = BC_STATUS_SUCCESS;
	BcBigDig dig;
	size_t places_rdx;
	bool shift;

	if (!places) return s;
	if (places > n->scale) {
		size_t size = bc_vm_growSize(BC_NUM_RDX(places - n->scale), n->len);
		if (size > SIZE_MAX - 1) return bc_vm_err(BC_ERROR_MATH_OVERFLOW);
	}
	if (BC_NUM_ZERO(n)) {
		if (n->scale >= places) n->scale -= places;
		else n->scale = 0;
		return s;
	}

	dig = (BcBigDig) (places % BC_BASE_DIGS);
	shift = (dig != 0);
	places_rdx = BC_NUM_RDX(places);

	if (n->scale) {

		if (n->rdx >= places_rdx) {

			size_t mod = n->scale % BC_BASE_DIGS, revdig;

			mod = mod ? mod : BC_BASE_DIGS;
			revdig = dig ? BC_BASE_DIGS - dig : 0;

			if (mod + revdig > BC_BASE_DIGS) places_rdx = 1;
			else places_rdx = 0;
		}
		else places_rdx -= n->rdx;
	}

	if (places_rdx) {
		bc_num_expand(n, bc_vm_growSize(n->len, places_rdx));
		memmove(n->num + places_rdx, n->num, BC_NUM_SIZE(n->len));
		memset(n->num, 0, BC_NUM_SIZE(places_rdx));
		n->len += places_rdx;
	}

	if (places > n->scale) n->scale = n->rdx = 0;
	else {
		n->scale -= places;
		n->rdx = BC_NUM_RDX(n->scale);
	}

	if (shift) s = bc_num_shift(n, BC_BASE_DIGS - dig);

	bc_num_clean(n);

	return BC_SIG && !s ? BC_STATUS_SIGNAL : s;
}

static BcStatus bc_num_shiftRight(BcNum *restrict n, size_t places) {

	BcStatus s = BC_STATUS_SUCCESS;
	BcBigDig dig;
	size_t places_rdx, scale, scale_mod, int_len, expand;
	bool shift;

	if (!places) return s;
	if (BC_NUM_ZERO(n)) {
		n->scale += places;
		bc_num_expand(n, BC_NUM_RDX(n->scale));
		return s;
	}

	dig = (BcBigDig) (places % BC_BASE_DIGS);
	shift = (dig != 0);
	scale = n->scale;
	scale_mod = scale % BC_BASE_DIGS;
	scale_mod = scale_mod ? scale_mod : BC_BASE_DIGS;
	int_len = bc_num_int(n);
	places_rdx = BC_NUM_RDX(places);

	if (scale_mod + dig > BC_BASE_DIGS) {
		expand = places_rdx - 1;
		places_rdx = 1;
	}
	else {
		expand = places_rdx;
		places_rdx = 0;
	}

	if (expand > int_len) expand -= int_len;
	else expand = 0;

	bc_num_extend(n, places_rdx * BC_BASE_DIGS);
	bc_num_expand(n, bc_vm_growSize(expand, n->len));
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

	bc_num_setup(&one, num, sizeof(num) / sizeof(BcDig));
	bc_num_one(&one);

	return bc_num_div(&one, a, b, scale);
}

#if BC_ENABLE_EXTRA_MATH
static BcStatus bc_num_intop(const BcNum *a, const BcNum *b, BcNum *restrict c,
                             BcBigDig *v)
{
	if (BC_ERR(b->rdx)) return bc_vm_err(BC_ERROR_MATH_NON_INTEGER);
	bc_num_copy(c, a);
	return bc_num_bigdig(b, v);
}
#endif // BC_ENABLE_EXTRA_MATH

static BcStatus bc_num_as(BcNum *a, BcNum *b, BcNum *restrict c, size_t sub) {

	BcDig *ptr_c, *ptr_l, *ptr_r;
	size_t i, min_rdx, max_rdx, diff, a_int, b_int, min_len, max_len, max_int;
	size_t len_l, len_r;
	bool b_neg, do_sub, do_rev_sub, carry;

	// Because this function doesn't need to use scale (per the bc spec),
	// I am hijacking it to say whether it's doing an add or a subtract.
	// Convert substraction to addition of negative second operand.

	if (BC_NUM_ZERO(b)) {
		bc_num_copy(c, a);
		return BC_STATUS_SUCCESS;
	}
	if (BC_NUM_ZERO(a)) {
		bc_num_copy(c, b);
		c->neg = (b->neg != sub);
		return BC_STATUS_SUCCESS;
	}

	b_neg = (b->neg != sub);
	do_sub = (a->neg != b_neg);

	a_int = bc_num_int(a);
	b_int = bc_num_int(b);
	max_int = BC_MAX(a_int, b_int);

	min_rdx = BC_MIN(a->rdx, b->rdx);
	max_rdx = BC_MAX(a->rdx, b->rdx);
	diff = max_rdx - min_rdx;

	max_len = max_int + max_rdx;

	if (do_sub) {
		if (a_int != b_int) do_rev_sub = (a_int < b_int);
		else if (a->rdx > b->rdx)
			do_rev_sub = (bc_num_compare(a->num + diff, b->num, b->len) < 0);
		else
			do_rev_sub = (bc_num_compare(a->num, b->num + diff, a->len) <= 0);
	}
	else {
		max_len += 1;
		do_rev_sub = false;
	}

	assert(max_len <= c->cap);

	if (do_rev_sub) {
		ptr_l = b->num;
		ptr_r = a->num;
		len_l = b->len;
		len_r = a->len;
	}
	else {
		ptr_l = a->num;
		ptr_r = b->num;
		len_l = a->len;
		len_r = b->len;
	}

	ptr_c = c->num;
	carry = false;

	if (diff) {

		if ((a->rdx > b->rdx) != do_rev_sub) {
			memcpy(ptr_c, ptr_l, BC_NUM_SIZE(diff));
			ptr_l += diff;
			len_l -= diff;
		}
		else {

			if (do_sub) {

				for (i = 0; BC_NO_SIG && i < diff; i++)
					ptr_c[i] = bc_num_subDigits(0, ptr_r[i], &carry);

				if (BC_SIG) return BC_STATUS_SIGNAL;
			}
			else memcpy(ptr_c, ptr_r, BC_NUM_SIZE(diff));

			ptr_r += diff;
			len_r -= diff;
		}

		ptr_c += diff;
	}

	min_len = BC_MIN(len_l, len_r);

	if (do_sub) {
		for (i = 0; BC_NO_SIG && i < min_len; ++i)
			ptr_c[i] = bc_num_subDigits(ptr_l[i], ptr_r[i], &carry);
		for (; BC_NO_SIG && i < len_l; ++i)
			ptr_c[i] = bc_num_subDigits(ptr_l[i], 0, &carry);
		for (; BC_NO_SIG && i < len_r; ++i)
			ptr_c[i] = bc_num_subDigits(0, ptr_r[i], &carry);
		for (; BC_NO_SIG && i < max_len - diff; ++i)
			ptr_c[i] = bc_num_subDigits(0, 0, &carry);
	}
	else {
		for (i = 0; BC_NO_SIG && i < min_len; ++i)
			ptr_c[i] = bc_num_addDigits(ptr_l[i], ptr_r[i], &carry);
		for (; BC_NO_SIG && i < len_l; ++i)
			ptr_c[i] = bc_num_addDigits(ptr_l[i], 0, &carry);
		for (; BC_NO_SIG && i < len_r; ++i)
			ptr_c[i] = bc_num_addDigits(0, ptr_r[i], &carry);
		for (; BC_NO_SIG && i < max_len - diff; ++i)
			ptr_c[i] = bc_num_addDigits(0, 0, &carry);
	}

	if (BC_NO_SIG) {

		assert(carry == false);

		// The result has the same sign as a, unless the operation was a
		// reverse subtraction (b - a).
		c->neg = (a->neg != do_rev_sub);
		c->len = max_len;
		c->rdx = max_rdx;
		c->scale = BC_MAX(a->scale, b->scale);

		bc_num_clean(c);
	}

	return BC_SIG ? BC_STATUS_SIGNAL : BC_STATUS_SUCCESS;
}

static BcStatus bc_num_m_simp(const BcNum *a, const BcNum *b, BcNum *restrict c)
{
	size_t i, alen = a->len, blen = b->len, clen;
	BcDig *ptr_a = a->num, *ptr_b = b->num, *ptr_c;
	BcBigDig sum = 0, carry = 0;

	assert(sizeof(sum) >= sizeof(BcDig) * 2);
	assert(!a->rdx && !b->rdx);

	clen = bc_vm_growSize(alen, blen);
	bc_num_expand(c, bc_vm_growSize(clen, 1));

	ptr_c = c->num;
	memset(ptr_c, 0, BC_NUM_SIZE(c->cap));

	for (i = 0; BC_NO_SIG && i < clen; ++i) {

		ssize_t sidx = (ssize_t) (i - blen + 1);
		size_t j = (size_t) BC_MAX(0, sidx), k = BC_MIN(i, blen - 1);

		for (; BC_NO_SIG && j < alen && k < blen; ++j, --k) {

			sum += ((BcBigDig) ptr_a[j]) * ((BcBigDig) ptr_b[k]);

			if (sum >= ((BcBigDig) BC_BASE_POW) * BC_BASE_POW) {
				carry += sum / BC_BASE_POW;
				sum %= BC_BASE_POW;
			}
		}

		if (sum >= BC_BASE_POW) {
			carry += sum / BC_BASE_POW;
			sum %= BC_BASE_POW;
		}

		ptr_c[i] = (BcDig) sum;
		assert(ptr_c[i] < BC_BASE_POW);
		sum = carry;
		carry = 0;
	}

	if (sum) {
		assert(sum < BC_BASE_POW);
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
	bool aone = BC_NUM_ONE(a);

	assert(BC_NUM_ZERO(c));

	// This is here because the function is recursive.
	if (BC_SIG) return BC_STATUS_SIGNAL;
	if (BC_NUM_ZERO(a) || BC_NUM_ZERO(b)) return BC_STATUS_SUCCESS;
	if (aone || BC_NUM_ONE(b)) {
		bc_num_copy(c, aone ? b : a);
		if ((aone && a->neg) || b->neg) c->neg = !c->neg;
		return BC_STATUS_SUCCESS;
	}
	if (a->len < BC_NUM_KARATSUBA_LEN || b->len < BC_NUM_KARATSUBA_LEN)
		return bc_num_m_simp(a, b, c);

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
	bc_num_split(b, max2, &l2, &h2);

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

	bc_num_zero(c);
	ascale = a->scale;
	bscale = b->scale;
	scale = BC_MAX(scale, ascale);
	scale = BC_MAX(scale, bscale);

	rscale = ascale + bscale;
	scale = BC_MIN(rscale, scale);

	if ((a->len == 1 || b->len == 1) && !a->rdx && !b->rdx) {

		BcNum *operand;
		BcBigDig dig;

		if (a->len == 1) {
			dig = (BcBigDig) a->num[0];
			operand = b;
		}
		else {
			dig = (BcBigDig) b->num[0];
			operand = a;
		}

		s = bc_num_mulArray(operand, dig, c);
		if (BC_ERROR_SIGNAL_ONLY(s)) return s;

		if (BC_NUM_NONZERO(c)) c->neg = (a->neg != b->neg);

		return s;
	}

	bc_num_init(&cpa, a->len + a->rdx);
	bc_num_init(&cpb, b->len + b->rdx);
	bc_num_copy(&cpa, a);
	bc_num_copy(&cpb, b);

	cpa.neg = cpb.neg = false;

	ardx = cpa.rdx * BC_BASE_DIGS;
	s = bc_num_shiftLeft(&cpa, ardx);
	if (BC_ERROR_SIGNAL_ONLY(s)) goto err;
	bc_num_clean(&cpa);
	azero = bc_num_shiftZero(&cpa);

	brdx = cpb.rdx * BC_BASE_DIGS;
	s = bc_num_shiftLeft(&cpb, brdx);
	if (BC_ERROR_SIGNAL_ONLY(s)) goto err;
	bzero = bc_num_shiftZero(&cpb);
	bc_num_clean(&cpb);

	s = bc_num_k(&cpa, &cpb, c);
	if (BC_ERROR_SIGNAL_ONLY(s)) goto err;

	zero = bc_vm_growSize(azero, bzero);
	len = bc_vm_growSize(c->len, zero);

	bc_num_expand(c, len);
	s = bc_num_shiftLeft(c, (len - c->len) * BC_BASE_DIGS);
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

static bool bc_num_nonZeroDig(BcDig *restrict a, size_t len) {
	size_t i;
	bool nonzero = false;
	for (i = len - 1; !nonzero && i < len; --i) nonzero = (a[i] != 0);
	return nonzero;
}

static ssize_t bc_num_divCmp(const BcDig *a, const BcNum *b, size_t len) {

	ssize_t cmp;

	if (b->len > len && a[len]) cmp = bc_num_compare(a, b->num, len + 1);
	else if (b->len <= len) {
		if (a[len]) cmp = 1;
		else cmp = bc_num_compare(a, b->num, len);
	}
	else cmp = -1;

	return cmp;
}

static BcStatus bc_num_divExtend(BcNum *restrict a, BcNum *restrict b,
                                 BcBigDig divisor)
{
	BcStatus s;
	size_t pow;

	pow = BC_BASE_DIGS - bc_num_log10((size_t) divisor);

	s = bc_num_shiftLeft(a, pow);
	if (BC_ERROR_SIGNAL_ONLY(s)) return s;

	return bc_num_shiftLeft(b, pow);
}

static BcStatus bc_num_d_long(BcNum *restrict a, BcNum *restrict b,
                              BcNum *restrict c, size_t scale)
{
	BcStatus s = BC_STATUS_SUCCESS;
	BcBigDig divisor;
	size_t len, end, i, rdx;
	BcNum cpb;
	bool nonzero = false;

	assert(b->len < a->len);
	len = b->len;
	end = a->len - len;
	assert(len >= 1);

	bc_num_expand(c, a->len);
	memset(c->num, 0, c->cap * sizeof(BcDig));

	c->rdx = a->rdx;
	c->scale = a->scale;
	c->len = a->len;

	divisor = (BcBigDig) b->num[len - 1];

	if (len > 1 && bc_num_nonZeroDig(b->num, len - 1)) {

		nonzero = (divisor > 1 << ((10 * BC_BASE_DIGS) / 6 + 1));

		if (!nonzero) {

			s = bc_num_divExtend(a, b, divisor);
			if (BC_ERROR_SIGNAL_ONLY(s)) return s;

			len = BC_MAX(a->len, b->len);
			bc_num_expand(a, len + 1);

			if (len + 1 > a->len) a->len = len + 1;

			len = b->len;
			end = a->len - len;
			divisor = (BcBigDig) b->num[len - 1];

			nonzero = bc_num_nonZeroDig(b->num, len - 1);
		}
	}

	divisor += nonzero;

	bc_num_expand(c, a->len);
	memset(c->num, 0, BC_NUM_SIZE(c->cap));

	assert(c->scale >= scale);
	rdx = c->rdx - BC_NUM_RDX(scale);

	bc_num_init(&cpb, len + 1);

	i = end - 1;

	for (; BC_NO_SIG && i < end && i >= rdx && BC_NUM_NONZERO(a); --i) {

		ssize_t cmp;
		BcDig *n;
		BcBigDig q, result;

		n = a->num + i;
		assert(n >= a->num);
		result = q = 0;

		cmp = bc_num_divCmp(n, b, len);

#if BC_ENABLE_SIGNALS
		if (BC_NUM_CMP_SIGNAL(cmp)) goto err;
#endif // BC_ENABLE_SIGNALS

		while (cmp >= 0) {

			BcBigDig n1, dividend;

			n1 = (BcBigDig) n[len];
			dividend = n1 * BC_BASE_POW + (BcBigDig) n[len - 1];
			q = (dividend / divisor);

			if (q <= 1) {
				q = 1;
				s = bc_num_subArrays(n, b->num, len);
				if (BC_ERROR_SIGNAL_ONLY(s)) goto err;
			}
			else {

				assert(q <= BC_BASE_POW);

				s = bc_num_mulArray(b, (BcBigDig) q, &cpb);
				if (BC_ERROR_SIGNAL_ONLY(s)) goto err;

				s = bc_num_subArrays(n, cpb.num, cpb.len);
				if (BC_ERROR_SIGNAL_ONLY(s)) goto err;
			}

			result += q;
			assert(result <= BC_BASE_POW);

			if (nonzero) {
				cmp = bc_num_divCmp(n, b, len);
#if BC_ENABLE_SIGNALS
				if (BC_NUM_CMP_SIGNAL(cmp)) goto err;
#endif // BC_ENABLE_SIGNALS
			}
			else cmp = -1;
		}

		assert(result < BC_BASE_POW);

		c->num[i] = (BcDig) result;
	}

err:
	if (BC_NO_ERR(!s) && BC_SIG) s = BC_STATUS_SIGNAL;
	bc_num_free(&cpb);
	return s;
}

static BcStatus bc_num_d(BcNum *a, BcNum *b, BcNum *restrict c, size_t scale) {

	BcStatus s = BC_STATUS_SUCCESS;
	size_t len;
	BcNum cpa, cpb;

	if (BC_NUM_ZERO(b)) return bc_vm_err(BC_ERROR_MATH_DIVIDE_BY_ZERO);
	if (BC_NUM_ZERO(a)) {
		bc_num_setToZero(c, scale);
		return BC_STATUS_SUCCESS;
	}
	if (BC_NUM_ONE(b)) {
		bc_num_copy(c, a);
		bc_num_retireMul(c, scale, a->neg, b->neg);
		return BC_STATUS_SUCCESS;
	}
	if (!a->rdx && !b->rdx && b->len == 1 && !scale) {
		BcBigDig rem;
		s = bc_num_divArray(a, (BcBigDig) b->num[0], c, &rem);
		bc_num_retireMul(c, scale, a->neg, b->neg);
		return s;
	}

	len = bc_num_mulReq(a, b, scale);
	bc_num_init(&cpa, len);
	bc_num_copy(&cpa, a);
	bc_num_createCopy(&cpb, b);

	len = b->len;

	if (len > cpa.len) {
		bc_num_expand(&cpa, bc_vm_growSize(len, 2));
		bc_num_extend(&cpa, (len - cpa.len) * BC_BASE_DIGS);
	}

	cpa.scale = cpa.rdx * BC_BASE_DIGS;

	bc_num_extend(&cpa, b->scale);
	cpa.rdx -= BC_NUM_RDX(b->scale);
	cpa.scale = cpa.rdx * BC_BASE_DIGS;

	if (scale > cpa.scale) {
		bc_num_extend(&cpa, scale);
		cpa.scale = cpa.rdx * BC_BASE_DIGS;
	}

	if (cpa.cap == cpa.len) bc_num_expand(&cpa, bc_vm_growSize(cpa.len, 1));

	// We want an extra zero in front to make things simpler.
	cpa.num[cpa.len++] = 0;

	if (cpa.rdx == cpa.len) cpa.len = bc_num_nonzeroLen(&cpa);
	if (cpb.rdx == cpb.len) cpb.len = bc_num_nonzeroLen(&cpb);
	cpb.scale = cpb.rdx = 0;

	s = bc_num_d_long(&cpa, &cpb, c, scale);

	if (BC_NO_ERR(!s)) {
		if (BC_SIG) s = BC_STATUS_SIGNAL;
		else bc_num_retireMul(c, scale, a->neg, b->neg);
	}

	bc_num_free(&cpb);
	bc_num_free(&cpa);

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
	BcBigDig pow = 0;
	size_t i, powrdx, resrdx;
	bool neg, zero;

	if (BC_ERR(b->rdx)) return bc_vm_err(BC_ERROR_MATH_NON_INTEGER);

	if (BC_NUM_ZERO(b)) {
		bc_num_one(c);
		return BC_STATUS_SUCCESS;
	}
	if (BC_NUM_ZERO(a)) {
		if (b->neg) return bc_vm_err(BC_ERROR_MATH_DIVIDE_BY_ZERO);
		bc_num_setToZero(c, scale);
		return BC_STATUS_SUCCESS;
	}
	if (BC_NUM_ONE(b)) {
		if (!b->neg) bc_num_copy(c, a);
		else s = bc_num_inv(a, c, scale);
		return s;
	}

	neg = b->neg;
	b->neg = false;
	s = bc_num_bigdig(b, &pow);
	b->neg = neg;
	if (s) return s;

	bc_num_createCopy(&copy, a);

	if (!neg) {
		size_t max = BC_MAX(scale, a->scale), scalepow = a->scale * pow;
		scale = BC_MIN(scalepow, max);
	}

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
	BcBigDig val = 0;

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
	BcBigDig val = 0;

	BC_UNUSED(scale);

	s = bc_num_intop(a, b, c, &val);
	if (BC_ERR(s)) return s;

	return bc_num_shiftLeft(c, (size_t) val);
}

static BcStatus bc_num_right(BcNum *a, BcNum *b, BcNum *restrict c,
                             size_t scale)
{
	BcStatus s = BC_STATUS_SUCCESS;
	BcBigDig val = 0;

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

	assert(a != NULL && b != NULL && c != NULL && op != NULL);

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
	assert(!c->len || c->num[c->len - 1] || c->rdx == c->len);

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

static BcBigDig bc_num_parseChar(char c, size_t base_t) {

	if (isupper(c)) {
		c = BC_NUM_NUM_LETTER(c);
		c = ((size_t) c) >= base_t ? (char) base_t - 1 : c;
	}
	else c -= '0';

	return (BcBigDig) (uchar) c;
}

static void bc_num_parseDecimal(BcNum *restrict n, const char *restrict val) {

	size_t len, i, temp, mod;
	const char *ptr;
	bool zero = true, rdx;

	for (i = 0; val[i] == '0'; ++i);

	val += i;
	assert(!val[0] || isalnum(val[0]) || val[0] == '.');

	// All 0's. We can just return, since this
	// procedure expects a virgin (already 0) BcNum.
	if (!val[0]) return;

	len = strlen(val);

	ptr = strchr(val, '.');
	rdx = (ptr != NULL);

	for (i = 0; i < len && (zero = (val[i] == '0' || val[i] == '.')); ++i);

	n->scale = (size_t) (rdx * ((val + len) - (ptr + 1)));
	n->rdx = BC_NUM_RDX(n->scale);

	i = len - (ptr == val ? 0 : i) - rdx;
	temp = BC_NUM_ROUND_POW(i);
	mod = n->scale % BC_BASE_DIGS;
	i = mod ? BC_BASE_DIGS - mod : 0;
	n->len = ((temp + i) / BC_BASE_DIGS);

	bc_num_expand(n, n->len);
	memset(n->num, 0, BC_NUM_SIZE(n->len));

	if (zero) n->len = n->rdx = 0;
	else {

		BcBigDig exp, pow;

		assert(i <= BC_NUM_BIGDIG_MAX);

		exp = (BcBigDig) i;
		pow = bc_num_pow10[exp];

		for (i = len - 1; i < len; --i, ++exp) {

			char c = val[i];

			if (c == '.') exp -= 1;
			else {

				size_t idx = exp / BC_BASE_DIGS;

				if (isupper(c)) c = '9';
				n->num[idx] += (((BcBigDig) c) - '0') * pow;

				if ((exp + 1) % BC_BASE_DIGS == 0) pow = 1;
				else pow *= BC_BASE;
			}
		}
	}
}

static BcStatus bc_num_parseBase(BcNum *restrict n, const char *restrict val,
                                 BcBigDig base)
{
	BcStatus s = BC_STATUS_SUCCESS;
	BcNum temp, mult1, mult2, result1, result2, *m1, *m2, *ptr;
	char c = 0;
	bool zero = true;
	BcBigDig v;
	size_t i, digs, len = strlen(val);

	for (i = 0; zero && i < len; ++i) zero = (val[i] == '.' || val[i] == '0');
	if (zero) return BC_STATUS_SUCCESS;

	bc_num_init(&temp, BC_NUM_BIGDIG_LOG10);
	bc_num_init(&mult1, BC_NUM_BIGDIG_LOG10);

	for (i = 0; i < len && (c = val[i]) && c != '.'; ++i) {

		v = bc_num_parseChar(c, base);

		s = bc_num_mulArray(n, base, &mult1);
		if (BC_ERROR_SIGNAL_ONLY(s)) goto int_err;
		bc_num_bigdig2num(&temp, v);
		s = bc_num_add(&mult1, &temp, n, 0);
		if (BC_ERROR_SIGNAL_ONLY(s)) goto int_err;
	}

	if (i == len && !(c = val[i])) goto int_err;

	assert(c == '.');
	bc_num_init(&mult2, BC_NUM_BIGDIG_LOG10);
	bc_num_init(&result1, BC_NUM_DEF_SIZE);
	bc_num_init(&result2, BC_NUM_DEF_SIZE);
	bc_num_one(&mult1);

	m1 = &mult1;
	m2 = &mult2;

	for (i += 1, digs = 0; BC_NO_SIG && i < len && (c = val[i]); ++i, ++digs) {

		v = bc_num_parseChar(c, base);

		s = bc_num_mulArray(&result1, base, &result2);
		if (BC_ERROR_SIGNAL_ONLY(s)) goto err;

		bc_num_bigdig2num(&temp, v);
		s = bc_num_add(&result2, &temp, &result1, 0);
		if (BC_ERROR_SIGNAL_ONLY(s)) goto err;
		s = bc_num_mulArray(m1, base, m2);
		if (BC_ERROR_SIGNAL_ONLY(s)) goto err;

		if (m2->len < m2->rdx) m2->len = m2->rdx;

		ptr = m1;
		m1 = m2;
		m2 = ptr;
	}

	if (BC_SIG) {
		s = BC_STATUS_SIGNAL;
		goto err;
	}

	// This one cannot be a divide by 0 because mult starts out at 1, then is
	// multiplied by base, and base cannot be 0, so mult cannot be 0.
	s = bc_num_div(&result1, m1, &result2, digs * 2);
	assert(!s || s == BC_STATUS_SIGNAL);
	if (BC_ERROR_SIGNAL_ONLY(s)) goto err;
	bc_num_truncate(&result2, digs);
	s = bc_num_add(n, &result2, n, digs);
	if (BC_ERROR_SIGNAL_ONLY(s)) goto err;

	if (BC_NUM_NONZERO(n)) {
		if (n->scale < digs) bc_num_extend(n, digs - n->scale);
	}
	else bc_num_zero(n);

err:
	bc_num_free(&result2);
	bc_num_free(&result1);
	bc_num_free(&mult2);
int_err:
	bc_num_free(&mult1);
	bc_num_free(&temp);
	return s;
}

static void bc_num_printNewline(void) {
	if (vm->nchars >= (size_t) (vm->line_len - 1)) {
		bc_vm_putchar('\\');
		bc_vm_putchar('\n');
	}
}

static void bc_num_putchar(int c) {
	if (c != '\n') bc_num_printNewline();
	bc_vm_putchar(c);
}

#if DC_ENABLED
static void bc_num_printChar(size_t n, size_t len, bool rdx) {
	BC_UNUSED(rdx);
	BC_UNUSED(len);
	assert(len == 1);
	bc_vm_putchar((uchar) n);
}
#endif // DC_ENABLED

static void bc_num_printDigits(size_t n, size_t len, bool rdx) {

	size_t exp, pow;

	bc_num_putchar(rdx ? '.' : ' ');

	for (exp = 0, pow = 1; exp < len - 1; ++exp, pow *= BC_BASE);

	for (exp = 0; exp < len; pow /= BC_BASE, ++exp) {
		size_t dig = n / pow;
		n -= dig * pow;
		bc_num_putchar(((uchar) dig) + '0');
	}
}

static void bc_num_printHex(size_t n, size_t len, bool rdx) {

	BC_UNUSED(len);

	assert(len == 1);

	if (rdx) bc_num_putchar('.');

	bc_num_putchar(bc_num_hex_digits[n]);
}

static void bc_num_printDecimal(const BcNum *restrict n) {

	size_t i, j, rdx = n->rdx;
	bool zero = true;
	size_t buffer[BC_BASE_DIGS];

	if (n->neg) bc_num_putchar('-');

	for (i = n->len - 1; i < n->len; --i) {

		BcDig n9 = n->num[i];
		size_t temp;
		bool irdx = (i == rdx - 1);

		zero = (zero & !irdx);
		temp = n->scale % BC_BASE_DIGS;
		temp = i || !temp ? 0 : BC_BASE_DIGS - temp;

		memset(buffer, 0, BC_BASE_DIGS * sizeof(size_t));

		for (j = 0; n9 && j < BC_BASE_DIGS; ++j) {
			buffer[j] = n9 % BC_BASE;
			n9 /= BC_BASE;
		}

		for (j = BC_BASE_DIGS - 1; j < BC_BASE_DIGS && j >= temp; --j) {
			bool print_rdx = (irdx & (j == BC_BASE_DIGS - 1));
			zero = (zero && buffer[j] == 0);
			if (!zero) bc_num_printHex(buffer[j], 1, print_rdx);
		}
	}
}

#if BC_ENABLE_EXTRA_MATH
static BcStatus bc_num_printExponent(const BcNum *restrict n, bool eng) {

	BcStatus s = BC_STATUS_SUCCESS;
	bool neg = (n->len <= n->rdx);
	BcNum temp, exp;
	size_t places, mod;
	BcDig digs[BC_NUM_BIGDIG_LOG10];

	bc_num_createCopy(&temp, n);

	if (neg) {

		size_t i, idx = bc_num_nonzeroLen(n) - 1;

		places = 1;

		for (i = BC_BASE_DIGS - 1; i < BC_BASE_DIGS; --i) {
			if (bc_num_pow10[i] > (BcBigDig) n->num[idx]) places += 1;
			else break;
		}

		places += (n->rdx - (idx + 1)) * BC_BASE_DIGS;
		mod = places % 3;

		if (eng && mod != 0) places += 3 - mod;
		s = bc_num_shiftLeft(&temp, places);
		if (BC_ERROR_SIGNAL_ONLY(s)) goto exit;
	}
	else {
		places = bc_num_intDigits(n) - 1;
		mod = places % 3;
		if (eng && mod != 0) places -= 3 - (3 - mod);
		s = bc_num_shiftRight(&temp, places);
		if (BC_ERROR_SIGNAL_ONLY(s)) goto exit;
	}

	bc_num_printDecimal(&temp);
	bc_num_putchar('e');

	if (!places) {
		bc_num_printHex(0, 1, false);
		goto exit;
	}

	if (neg) bc_num_putchar('-');

	bc_num_setup(&exp, digs, BC_NUM_BIGDIG_LOG10);
	bc_num_bigdig2num(&exp, (BcBigDig) places);

	bc_num_printDecimal(&exp);

exit:
	bc_num_free(&temp);
	return BC_SIG ? BC_STATUS_SIGNAL : BC_STATUS_SUCCESS;
}
#endif // BC_ENABLE_EXTRA_MATH

static BcStatus bc_num_printFixup(BcNum *restrict n, BcBigDig rem,
                                  BcBigDig pow, size_t idx)
{
	size_t i, len = n->len - idx;
	BcBigDig acc;
	BcDig *a = n->num + idx;

	if (len < 2) return BC_STATUS_SUCCESS;

	for (i = len - 1; BC_NO_SIG && i > 0; --i) {

		acc = ((BcBigDig) a[i]) * rem + ((BcBigDig) a[i - 1]);
		a[i - 1] = (BcDig) (acc % pow);
		acc /= pow;
		acc += (BcBigDig) a[i];

		if (acc >= BC_BASE_POW) {

			if (i == len - 1) {
				len = bc_vm_growSize(len, 1);
				bc_num_expand(n, bc_vm_growSize(len, idx));
				a = n->num + idx;
				a[len - 1] = 0;
			}

			a[i + 1] += acc / BC_BASE_POW;
			acc %= BC_BASE_POW;
		}

		assert(acc < BC_BASE_POW);
		a[i] = (BcDig) acc;
	}

	n->len = len + idx;

	return BC_SIG ? BC_STATUS_SIGNAL : BC_STATUS_SUCCESS;
}

static BcStatus bc_num_printPrepare(BcNum *restrict n, BcBigDig rem,
                                    BcBigDig pow)
{
	BcStatus s = BC_STATUS_SUCCESS;
	size_t i;

	for (i = 0; BC_NO_SIG && BC_NO_ERR(!s) && i < n->len; ++i)
		s = bc_num_printFixup(n, rem, pow, i);

	if (BC_ERR(s)) return s;

	for (i = 0; BC_NO_SIG && i < n->len; ++i) {

		assert(pow == ((BcBigDig) ((BcDig) pow)));

		if (n->num[i] >= (BcDig) pow) {

			if (i + 1 == n->len) {
				n->len = bc_vm_growSize(n->len, 1);
				bc_num_expand(n, n->len);
				n->num[i + 1] = 0;
			}

			assert(pow < BC_BASE_POW);
			n->num[i + 1] += n->num[i] / ((BcDig) pow);
			n->num[i] %= (BcDig) pow;
		}
	}

	return BC_NO_ERR(!s) && BC_SIG ? BC_STATUS_SIGNAL : BC_STATUS_SUCCESS;
}

static BcStatus bc_num_printNum(BcNum *restrict n, BcBigDig base,
                                size_t len, BcNumDigitOp print)
{
	BcStatus s;
	BcVec stack;
	BcNum intp, fracp1, fracp2, digit, flen1, flen2, *n1, *n2, *temp;
	BcBigDig dig = 0, *ptr, acc, exp;
	size_t i, j;
	bool radix;
	BcDig digit_digs[BC_NUM_BIGDIG_LOG10 + 1];

	assert(base > 1);

	if (BC_NUM_ZERO(n)) {
		print(0, len, false);
		return BC_STATUS_SUCCESS;
	}

	// This function uses an algorithm that Stefan Esser <se@freebsd.org> came
	// up with to print the integer part of a number. What it does is convert
	// intp into a number of the specified base, but it does it directly,
	// instead of just doing a series of divisions and printing the remainders
	// in reverse order.
	//
	// Let me explain in a bit more detail:
	//
	// The algorithm takes the current least significant digit (after intp has
	// been converted to an integer) and the next to least significant digit,
	// and it converts the least significant digit into one of the specified
	// base, putting any overflow into the next to least significant digit. It
	// iterates through the whole number, from least significant to most
	// significant, doing this conversion. At the end of that iteration, the
	// least significant digit is converted, but the others are not, so it
	// iterates again, starting at the next to least significant digit. It keeps
	// doing that conversion, skipping one more digit than the last time, until
	// all digits have been converted. Then it prints them in reverse order.
	//
	// That is the gist of the algorithm. It leaves out several things, such as
	// the fact that digits are not always converted into the specified base,
	// but into something close, basically a power of the specified base. In
	// Stefan's words, "You could consider BcDigs to be of base 10^BC_BASE_DIGS
	// in the normal case and obase^N for the largest value of N that satisfies
	// obase^N <= 10^BC_BASE_DIGS. [This means that] the result is not in base
	// "obase", but in base "obase^N", which happens to be printable as a number
	// of base "obase" without consideration for neighbouring BcDigs." This fact
	// is what necessitates the existence of the loop later in this function.
	//
	// The conversion happens in bc_num_printPrepare() where the outer loop
	// happens and bc_num_printFixup() where the inner loop, or actual
	// conversion, happens.

	bc_vec_init(&stack, sizeof(BcBigDig), NULL);
	bc_num_init(&fracp1, n->rdx);

	bc_num_createCopy(&intp, n);
	bc_num_truncate(&intp, intp.scale);

	s = bc_num_sub(n, &intp, &fracp1, 0);
	if (BC_ERROR_SIGNAL_ONLY(s)) goto err;

	if (base != vm->last_base) {

		vm->last_pow = 1;
		vm->last_exp = 0;

		while (vm->last_pow * base <= BC_BASE_POW) {
			vm->last_pow *= base;
			vm->last_exp += 1;
		}

		vm->last_rem = BC_BASE_POW - vm->last_pow;
		vm->last_base = base;
	}

	exp = vm->last_exp;

	if (vm->last_rem != 0) {
		s = bc_num_printPrepare(&intp, vm->last_rem, vm->last_pow);
		if (BC_ERROR_SIGNAL_ONLY(s)) goto err;
	}

	for (i = 0; BC_NO_SIG && i < intp.len; ++i) {

		acc = (BcBigDig) intp.num[i];

		for (j = 0; BC_NO_SIG && j < exp && (i < intp.len - 1 || acc != 0); ++j)
		{
			if (j != exp - 1) {
				dig = acc % base;
				acc /= base;
			}
			else {
				dig = acc;
				acc = 0;
			}

			assert(dig < base);

			bc_vec_push(&stack, &dig);
		}

		assert(acc == 0 || BC_SIG);
	}

	if (BC_SIG) goto sig_err;

	for (i = 0; BC_NO_SIG && i < stack.len; ++i) {
		ptr = bc_vec_item_rev(&stack, i);
		assert(ptr != NULL);
		print(*ptr, len, false);
	}

	if (BC_SIG) goto sig_err;
	if (!n->scale) goto err;

	bc_num_init(&fracp2, n->rdx);
	bc_num_setup(&digit, digit_digs, sizeof(digit_digs) / sizeof(BcDig));
	bc_num_init(&flen1, BC_NUM_BIGDIG_LOG10 + 1);
	bc_num_init(&flen2, BC_NUM_BIGDIG_LOG10 + 1);
	bc_num_one(&flen1);

	radix = true;
	n1 = &flen1;
	n2 = &flen2;

	fracp2.scale = n->scale;
	fracp2.rdx = BC_NUM_RDX(fracp2.scale);

	while (BC_NO_SIG && bc_num_intDigits(n1) < n->scale + 1) {

		bc_num_expand(&fracp2, fracp1.len + 1);
		s = bc_num_mulArray(&fracp1, base, &fracp2);
		if (BC_ERROR_SIGNAL_ONLY(s)) goto frac_err;
		if (fracp2.len < fracp2.rdx) fracp2.len = fracp2.rdx;

		// Will never fail (except for signals) because fracp is
		// guaranteed to be non-negative and small enough.
		s = bc_num_bigdig(&fracp2, &dig);
		if (BC_ERROR_SIGNAL_ONLY(s)) goto frac_err;

		bc_num_bigdig2num(&digit, dig);
		s = bc_num_sub(&fracp2, &digit, &fracp1, 0);
		if (BC_ERROR_SIGNAL_ONLY(s)) goto frac_err;

		print(dig, len, radix);
		s = bc_num_mulArray(n1, base, n2);
		if (BC_ERROR_SIGNAL_ONLY(s)) goto frac_err;

		radix = false;
		temp = n1;
		n1 = n2;
		n2 = temp;
	}

frac_err:
	bc_num_free(&flen2);
	bc_num_free(&flen1);
	bc_num_free(&fracp2);
sig_err:
	if (BC_NO_ERR(!s) && BC_SIG) s = BC_STATUS_SIGNAL;
err:
	bc_num_free(&fracp1);
	bc_num_free(&intp);
	bc_vec_free(&stack);
	return s;
}

static BcStatus bc_num_printBase(BcNum *restrict n, BcBigDig base) {

	BcStatus s;
	size_t width;
	BcNumDigitOp print;
	bool neg = n->neg;

	if (neg) bc_num_putchar('-');

	n->neg = false;

	if (base <= BC_NUM_MAX_POSIX_IBASE) {
		width = 1;
		print = bc_num_printHex;
	}
	else {
		width = bc_num_log10(base - 1);
		print = bc_num_printDigits;
	}

	s = bc_num_printNum(n, base, width, print);
	n->neg = neg;

	return s;
}

#if DC_ENABLED
BcStatus bc_num_stream(BcNum *restrict n, BcBigDig base) {
	return bc_num_printNum(n, base, 1, bc_num_printChar);
}
#endif // DC_ENABLED

void bc_num_setup(BcNum *restrict n, BcDig *restrict num, size_t cap) {
	assert(n != NULL);
	n->num = num;
	n->cap = cap;
	bc_num_zero(n);
}

void bc_num_init(BcNum *restrict n, size_t req) {
	assert(n != NULL);
	req = req >= BC_NUM_DEF_SIZE ? req : BC_NUM_DEF_SIZE;
	bc_num_setup(n, bc_vm_malloc(BC_NUM_SIZE(req)), req);
}

void bc_num_free(void *num) {
	assert(num != NULL);
	free(((BcNum*) num)->num);
}

void bc_num_copy(BcNum *d, const BcNum *s) {
	assert(d != NULL && s != NULL);
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

void bc_num_createFromBigdig(BcNum *n, BcBigDig val) {
	bc_num_init(n, (BC_NUM_BIGDIG_LOG10 - 1) / BC_BASE_DIGS + 1);
	bc_num_bigdig2num(n, val);
}

size_t bc_num_scale(const BcNum *restrict n) {
	return n->scale;
}

size_t bc_num_len(const BcNum *restrict n) {

	size_t len = n->len;

	if (BC_NUM_ZERO(n)) return 0;

	if (n->rdx == len) {

		size_t zero, scale;

		len = bc_num_nonzeroLen(n);

		scale = n->scale % BC_BASE_DIGS;
		scale = scale ? scale : BC_BASE_DIGS;

		zero = bc_num_zeroDigits(n->num + len - 1);

		len = len * BC_BASE_DIGS - zero - (BC_BASE_DIGS - scale);
	}
	else len = bc_num_intDigits(n) + n->scale;

	return len;
}

BcStatus bc_num_parse(BcNum *restrict n, const char *restrict val,
                      BcBigDig base, bool letter)
{
	BcStatus s = BC_STATUS_SUCCESS;

	assert(n != NULL && val != NULL && base);
	assert(base >= BC_NUM_MIN_BASE && base <= vm->maxes[BC_PROG_GLOBALS_IBASE]);
	assert(bc_num_strValid(val));

	if (letter) {
		BcBigDig dig = bc_num_parseChar(val[0], BC_NUM_MAX_LBASE);
		bc_num_bigdig2num(n, dig);
	}
	else if (base == BC_BASE) bc_num_parseDecimal(n, val);
	else s = bc_num_parseBase(n, val, base);

	return s;
}

BcStatus bc_num_print(BcNum *restrict n, BcBigDig base, bool newline) {

	BcStatus s = BC_STATUS_SUCCESS;

	assert(n != NULL);
	assert(BC_ENABLE_EXTRA_MATH || base >= BC_NUM_MIN_BASE);

	bc_num_printNewline();

	if (BC_NUM_ZERO(n)) bc_num_printHex(0, 1, false);
	else if (base == BC_BASE) bc_num_printDecimal(n);
#if BC_ENABLE_EXTRA_MATH
	else if (base == 0 || base == 1)
		s = bc_num_printExponent(n, base != 0);
#endif // BC_ENABLE_EXTRA_MATH
	else s = bc_num_printBase(n, base);

	if (BC_NO_ERR(!s) && newline) bc_num_putchar('\n');

	return s;
}

BcStatus bc_num_bigdig(const BcNum *restrict n, BcBigDig *result) {

	size_t i;
	BcBigDig r;

	assert(n != NULL && result != NULL);

	if (BC_ERR(n->neg)) return bc_vm_err(BC_ERROR_MATH_NEGATIVE);

	for (r = 0, i = n->len; i > n->rdx;) {

		BcBigDig prev = r * BC_BASE_POW;

		if (BC_ERR(prev / BC_BASE_POW != r))
			return bc_vm_err(BC_ERROR_MATH_OVERFLOW);

		r = prev + (BcBigDig) n->num[--i];

		if (BC_ERR(r < prev)) return bc_vm_err(BC_ERROR_MATH_OVERFLOW);
	}

	*result = r;

	return BC_STATUS_SUCCESS;
}

void bc_num_bigdig2num(BcNum *restrict n, BcBigDig val) {

	BcDig *ptr;
	size_t i;

	assert(n != NULL);

	bc_num_zero(n);

	if (!val) return;

	bc_num_expand(n, BC_NUM_BIGDIG_LOG10);

	for (ptr = n->num, i = 0; val; ++i, val /= BC_BASE_POW)
		ptr[i] = val % BC_BASE_POW;

	n->len = i;
}

size_t bc_num_addReq(const BcNum *a, const BcNum *b, size_t scale) {

	size_t aint, bint, ardx, brdx;

	BC_UNUSED(scale);

	ardx = a->rdx;
	aint = bc_num_int(a);
	assert(aint <= a->len && ardx <= a->len);

	brdx = b->rdx;
	bint = bc_num_int(b);
	assert(bint <= b->len && brdx <= b->len);

	ardx = BC_MAX(ardx, brdx);
	aint = BC_MAX(aint, bint);

	return bc_vm_growSize(bc_vm_growSize(ardx, aint), 1);
}

size_t bc_num_mulReq(const BcNum *a, const BcNum *b, size_t scale) {
	size_t max, rdx;
	rdx = bc_vm_growSize(a->rdx, b->rdx);
	max = BC_NUM_RDX(scale);
	max = bc_vm_growSize(BC_MAX(max, rdx), 1);
	rdx = bc_vm_growSize(bc_vm_growSize(bc_num_int(a), bc_num_int(b)), max);
	return rdx;
}

size_t bc_num_powReq(const BcNum *a, const BcNum *b, size_t scale) {
	BC_UNUSED(scale);
	return bc_vm_growSize(bc_vm_growSize(a->len, b->len), 1);
}

#if BC_ENABLE_EXTRA_MATH
size_t bc_num_placesReq(const BcNum *a, const BcNum *b, size_t scale) {
	BC_UNUSED(scale);
	return a->len + b->len - a->rdx - b->rdx;
}
#endif // BC_ENABLE_EXTRA_MATH

BcStatus bc_num_add(BcNum *a, BcNum *b, BcNum *c, size_t scale) {
	return bc_num_binary(a, b, c, false, bc_num_as, bc_num_addReq(a, b, scale));
}

BcStatus bc_num_sub(BcNum *a, BcNum *b, BcNum *c, size_t scale) {
	return bc_num_binary(a, b, c, true, bc_num_as, bc_num_addReq(a, b, scale));
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
	size_t req = bc_num_placesReq(a, b, scale);
	return bc_num_binary(a, b, c, scale, bc_num_place, req);
}

BcStatus bc_num_lshift(BcNum *a, BcNum *b, BcNum *c, size_t scale) {
	size_t req = bc_num_placesReq(a, b, scale);
	return bc_num_binary(a, b, c, scale, bc_num_left, req);
}

BcStatus bc_num_rshift(BcNum *a, BcNum *b, BcNum *c, size_t scale) {
	size_t req = bc_num_placesReq(a, b, scale);
	return bc_num_binary(a, b, c, scale, bc_num_right, req);
}
#endif // BC_ENABLE_EXTRA_MATH

BcStatus bc_num_sqrt(BcNum *restrict a, BcNum *restrict b, size_t scale) {

	BcStatus s = BC_STATUS_SUCCESS;
	BcNum num1, num2, half, f, fprime, *x0, *x1, *temp;
	size_t pow, len, rdx, req, digs, digs1, digs2, resscale;
	BcDig half_digs[1];

	assert(a != NULL && b != NULL && a != b);

	if (BC_ERR(a->neg)) return bc_vm_err(BC_ERROR_MATH_NEGATIVE);

	if (a->scale > scale) scale = a->scale;
	len = bc_vm_growSize(bc_num_intDigits(a), 1);
	rdx = BC_NUM_RDX(scale);
	req = bc_vm_growSize(BC_MAX(rdx, a->rdx), len >> 1);
	bc_num_init(b, bc_vm_growSize(req, 1));

	if (BC_NUM_ZERO(a)) {
		bc_num_setToZero(b, scale);
		return BC_STATUS_SUCCESS;
	}
	if (BC_NUM_ONE(a)) {
		bc_num_one(b);
		bc_num_extend(b, scale);
		return BC_STATUS_SUCCESS;
	}

	rdx = BC_NUM_RDX(scale);
	rdx = BC_MAX(rdx, a->rdx);
	len = bc_vm_growSize(a->len, rdx);

	bc_num_init(&num1, len);
	bc_num_init(&num2, len);
	bc_num_setup(&half, half_digs, sizeof(half_digs) / sizeof(BcDig));

	bc_num_one(&half);
	half.num[0] = BC_BASE_POW / 2;
	half.len = 1;
	half.rdx = 1;
	half.scale = 1;

	bc_num_init(&f, len);
	bc_num_init(&fprime, len);

	x0 = &num1;
	x1 = &num2;

	bc_num_one(x0);
	pow = bc_num_intDigits(a);

	if (pow) {

		if (pow & 1) x0->num[0] = 2;
		else x0->num[0] = 6;

		pow -= 2 - (pow & 1);
		s = bc_num_shiftLeft(x0, pow / 2);
		if (BC_ERROR_SIGNAL_ONLY(s)) goto err;
	}

	x0->scale = x0->rdx = digs = digs1 = digs2 = 0;
	resscale = (scale + BC_BASE_DIGS) + 2;

	while (BC_NO_SIG && bc_num_cmp(x1, x0)) {

		assert(BC_NUM_NONZERO(x0));

		s = bc_num_div(a, x0, &f, resscale);
		assert(!s || s == BC_STATUS_SIGNAL);
		if (BC_ERROR_SIGNAL_ONLY(s)) goto err;
		s = bc_num_add(x0, &f, &fprime, resscale);
		if (BC_ERROR_SIGNAL_ONLY(s)) goto err;
		s = bc_num_mul(&fprime, &half, x1, resscale);
		if (BC_ERROR_SIGNAL_ONLY(s)) goto err;

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

	assert(!b->neg || BC_NUM_NONZERO(b));
	assert(b->rdx <= b->len || !b->len);
	assert(!b->len || b->num[b->len - 1] || b->rdx == b->len);

err:
	if (BC_ERR(s)) bc_num_free(b);
	bc_num_free(&fprime);
	bc_num_free(&f);
	bc_num_free(&num2);
	bc_num_free(&num1);
	return s;
}

BcStatus bc_num_divmod(BcNum *a, BcNum *b, BcNum *c, BcNum *d, size_t scale) {

	BcStatus s;
	BcNum num2, *ptr_a;
	bool init = false;
	size_t ts, len;

	ts = BC_MAX(scale + b->scale, a->scale);
	len = bc_num_mulReq(a, b, ts);

	assert(a != NULL && b != NULL && c != NULL && d != NULL);
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

	if (BC_NUM_NONZERO(a) && !a->rdx && !b->rdx && b->len == 1 && !scale) {

		BcBigDig rem;

		s = bc_num_divArray(ptr_a, (BcBigDig) b->num[0], c, &rem);

		assert(rem < BC_BASE_POW);

		d->num[0] = (BcDig) rem;
		d->len = (rem != 0);
	}
	else s = bc_num_r(ptr_a, b, c, d, scale, ts);

	assert(!c->neg || BC_NUM_NONZERO(c));
	assert(c->rdx <= c->len || !c->len);
	assert(!c->len || c->num[c->len - 1] || c->rdx == c->len);
	assert(!d->neg || BC_NUM_NONZERO(d));
	assert(d->rdx <= d->len || !d->len);
	assert(!d->len || d->num[d->len - 1] || d->rdx == d->len);

	if (init) bc_num_free(&num2);

	return s;
}

#if DC_ENABLED
BcStatus bc_num_modexp(BcNum *a, BcNum *b, BcNum *c, BcNum *restrict d) {

	BcStatus s;
	BcNum base, exp, two, temp;
	BcDig two_digs[2];

	assert(a != NULL && b != NULL && c != NULL && d != NULL);
	assert(a != d && b != d && c != d);

	if (BC_ERR(BC_NUM_ZERO(c)))
		return bc_vm_err(BC_ERROR_MATH_DIVIDE_BY_ZERO);
	if (BC_ERR(b->neg)) return bc_vm_err(BC_ERROR_MATH_NEGATIVE);
	if (BC_ERR(a->rdx || b->rdx || c->rdx))
		return bc_vm_err(BC_ERROR_MATH_NON_INTEGER);

	bc_num_expand(d, c->len);
	bc_num_init(&base, c->len);
	bc_num_setup(&two, two_digs, sizeof(two_digs) / sizeof(BcDig));
	bc_num_init(&temp, b->len + 1);

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

		if (BC_NUM_ONE(&temp) && !temp.neg) {

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
	assert(!d->len || d->num[d->len - 1] || d->rdx == d->len);
	return s;
}
#endif // DC_ENABLED

#if BC_DEBUG_CODE
void bc_num_printDebug(const BcNum *n, const char *name, bool emptyline) {
	printf("%s: ", name);
	bc_num_printDecimal(n);
	printf("\n");
	if (emptyline) printf("\n");
	vm->nchars = 0;
}

void bc_num_printDigs(const BcDig *n, size_t len, bool emptyline) {

	size_t i;

	for (i = len - 1; i < len; --i) printf(" %0*d", BC_BASE_DIGS, n[i]);

	printf("\n");
	if (emptyline) printf("\n");
	vm->nchars = 0;
}

void bc_num_printWithDigs(const BcNum *n, const char *name, bool emptyline) {
	printf("%s len: %zu, rdx: %zu, scale: %zu\n",
	       name, n->len, n->rdx, n->scale);
	bc_num_printDigs(n->num, n->len, emptyline);
}

void bc_num_dump(const char *varname, const BcNum *n) {

	ulong i, scale = n->scale;

	fprintf(stderr, "\n%s = %s", varname, n->len ? (n->neg ? "-" : "+") : "0 ");

	for (i = n->len - 1; i < n->len; --i) {

		if (i + 1 == n->rdx) fprintf(stderr, ". ");

		if (scale / BC_BASE_DIGS != n->rdx - i - 1)
			fprintf(stderr, "%0*d ", BC_BASE_DIGS, n->num[i]);
		else {

			int mod = scale % BC_BASE_DIGS;
			int d = BC_BASE_DIGS - mod;
			BcDig div;

			if (mod != 0) {
				div = n->num[i] / ((BcDig) bc_num_pow10[(ulong) d]);
				fprintf(stderr, "%0*d", (int) mod, div);
			}

			div = n->num[i] % ((BcDig) bc_num_pow10[(ulong) d]);
			fprintf(stderr, " ' %0*d ", d, div);
		}
	}

	fprintf(stderr, "(%zu | %zu.%zu / %zu) %p\n",
	        n->scale, n->len, n->rdx, n->cap, (void*) n->num);
}
#endif // BC_DEBUG_CODE
