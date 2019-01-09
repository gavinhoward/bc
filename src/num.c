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

void bc_num_setToZero(BcNum *restrict n, size_t scale) {
	assert(n);
	n->len = 0;
	n->neg = false;
	n->rdx = scale;
}

void bc_num_zero(BcNum *restrict n) {
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
	n->len = 2;
	n->num[0] = 0;
	n->num[1] = 1;
}

size_t bc_num_log10(size_t i) {
	size_t len;
	for (len = 1; i; i /= 10, ++len);
	return len;
}

BcStatus bc_num_subArrays(BcDig *restrict a, const BcDig *restrict b, size_t len)
{
	size_t i, j;
	for (i = 0; !BC_SIGNAL && i < len; ++i) {
		for (a[i] -= b[i], j = 0; !BC_SIGNAL && a[i + j] < 0;) {
			a[i + j++] += 10;
			a[i + j] -= 1;
			assert(a[i + j - 1] >= 0 && a[i + j - 1] < 10);
		}
	}
	return BC_SIGNAL ? BC_STATUS_SIGNAL : BC_STATUS_SUCCESS;
}

ssize_t bc_num_compare(const BcDig *restrict a, const BcDig *restrict b,
                       size_t len)
{
	size_t i;
	int c = 0;
	for (i = len - 1; !BC_SIGNAL && i < len && !(c = a[i] - b[i]); --i);
	return BC_NUM_NEG(i + 1, c < 0);
}

ssize_t bc_num_cmp(const BcNum *a, const BcNum *b) {

	size_t i, min, a_int, b_int, diff;
	BcDig *max_num, *min_num;
	bool a_max, neg = false;
	ssize_t cmp;

	assert(a && b);

	if (a == b) return 0;
	if (BC_NUM_ZERO(a)) return BC_NUM_NEG(!!b->len, !b->neg);
	if (BC_NUM_ZERO(b)) return BC_NUM_CMP_ZERO(a);
	if (a->neg) {
		if (b->neg) neg = true;
		else return -1;
	}
	else if (b->neg) return 1;

	a_int = BC_NUM_INT(a);
	b_int = BC_NUM_INT(b);
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
	if (cmp) return BC_NUM_NEG(cmp, (!a_max) != neg);

	for (max_num -= diff, i = diff - 1; !BC_SIGNAL && i < diff; --i) {
		if (max_num[i]) return BC_NUM_NEG(1, (!a_max) != neg);
	}

	return 0;
}

void bc_num_truncate(BcNum *restrict n, size_t places) {

	assert(places <= n->rdx && (BC_NUM_ZERO(n) || places <= n->len));

	if (!places) return;

	n->rdx -= places;

	if (BC_NUM_NONZERO(n)) {
		n->len -= places;
		memmove(n->num, n->num + places, n->len * sizeof(BcDig));
	}
}

void bc_num_extend(BcNum *restrict n, size_t places) {

	size_t len = n->len + places;

	if (!places) return;

	if (n->cap < len) bc_num_expand(n, len);

	memmove(n->num + places, n->num, sizeof(BcDig) * n->len);
	memset(n->num, 0, sizeof(BcDig) * places);

	if (n->len) n->len += places;

	n->rdx += places;
}

void bc_num_clean(BcNum *restrict n) {
	while (BC_NUM_NONZERO(n) && !n->num[n->len - 1]) --n->len;
	if (BC_NUM_ZERO(n)) n->neg = false;
	else if (n->len < n->rdx) n->len = n->rdx;
}

void bc_num_retireMul(BcNum *restrict n, size_t scale, bool neg1, bool neg2) {

	if (n->rdx < scale) bc_num_extend(n, scale - n->rdx);
	else bc_num_truncate(n, n->rdx - scale);

	bc_num_clean(n);
	if (BC_NUM_NONZERO(n)) n->neg = (!neg1 != !neg2);
}

void bc_num_split(const BcNum *restrict n, size_t idx, BcNum *restrict a, BcNum *restrict b) {

	if (idx < n->len) {

		b->len = n->len - idx;
		a->len = idx;
		a->rdx = b->rdx = 0;

		memcpy(b->num, n->num + idx, b->len * sizeof(BcDig));
		memcpy(a->num, n->num, idx * sizeof(BcDig));

		bc_num_clean(b);
	}
	else bc_num_copy(a, n);

	bc_num_clean(a);
}

BcStatus bc_num_shift(BcNum *restrict n, size_t places) {

	if (!places || BC_NUM_ZERO(n)) return BC_STATUS_SUCCESS;
	if (places + n->len > BC_MAX_NUM)
		return bc_vm_verr(BC_ERROR_MATH_OVERFLOW, "shifted left too far");

	if (n->rdx >= places) n->rdx -= places;
	else {
		bc_num_extend(n, places - n->rdx);
		n->rdx = 0;
	}

	bc_num_clean(n);

	return BC_STATUS_SUCCESS;
}

BcStatus bc_num_inv(BcNum *a, BcNum *b, size_t scale) {

	BcNum one;
	BcDig num[2];

	one.cap = 2;
	one.num = num;
	bc_num_one(&one);

	return bc_num_div(&one, a, b, scale);
}

#if BC_ENABLE_EXTRA_MATH
BcStatus bc_num_intop(const BcNum *a, const BcNum *b, BcNum *restrict c,
                      unsigned long *v)
{
	if (b->rdx) return bc_vm_err(BC_ERROR_MATH_NON_INTEGER);
	bc_num_copy(c, a);
	return bc_num_ulong(b, v);
}
#endif // BC_ENABLE_EXTRA_MATH

BcStatus bc_num_a(BcNum *a, BcNum *b, BcNum *restrict c, size_t sub) {

	BcDig *ptr, *ptr_a, *ptr_b, *ptr_c;
	size_t i, max, min_rdx, min_int, diff, a_int, b_int;
	unsigned int carry, in;

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
	min_rdx = BC_MIN(a->rdx, b->rdx);
	c->len = 0;

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

	for (ptr_c = c->num, i = 0; i < diff; ++i, ++c->len) ptr_c[i] = ptr[i];

	ptr_c += diff;
	a_int = BC_NUM_INT(a);
	b_int = BC_NUM_INT(b);

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

	for (carry = 0, i = 0; !BC_SIGNAL && i < min_rdx + min_int; ++i, ++c->len) {
		in = ((unsigned int) ptr_a[i]) + ((unsigned int) ptr_b[i]) + carry;
		carry = in / 10;
		assert(carry >= 0 && carry < 10);
		ptr_c[i] = (BcDig) (in % 10);
		assert(ptr_c[i] >= 0 && ptr_c[i] < 10);
	}

	for (; !BC_SIGNAL && i < max + min_rdx; ++i, ++c->len) {
		in = ((unsigned int) ptr[i]) + carry;
		carry = in / 10;
		assert(carry >= 0 && carry < 10);
		ptr_c[i] = (BcDig) (in % 10);
		assert(ptr_c[i] >= 0 && ptr_c[i] < 10);
	}

	if (carry) c->num[c->len++] = (BcDig) carry;

	return BC_SIGNAL ? BC_STATUS_SIGNAL : BC_STATUS_SUCCESS;
}

BcStatus bc_num_s(BcNum *a, BcNum *b, BcNum *restrict c, size_t sub) {

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

	if (c->rdx < subtrahend->rdx) {
		bc_num_extend(c, subtrahend->rdx - c->rdx);
		start = 0;
	}
	else start = c->rdx - subtrahend->rdx;

	s = bc_num_subArrays(c->num + start, subtrahend->num, subtrahend->len);

	bc_num_clean(c);

	return s;
}

BcStatus bc_num_k(const BcNum *a, const BcNum *b, BcNum *restrict c) {

	BcStatus s;
	size_t max = BC_MAX(a->len, b->len), max2 = (max + 1) / 2;
	BcNum l1, h1, l2, h2, m2, m1, z0, z1, z2, temp;
	bool aone = BC_NUM_ONE(a);

	// This is here because the function is recursive.
	if (BC_SIGNAL) return BC_STATUS_SIGNAL;
	if (BC_NUM_ZERO(a) || BC_NUM_ZERO(b)) {
		bc_num_zero(c);
		return BC_STATUS_SUCCESS;
	}
	if (aone || BC_NUM_ONE(b)) {
		bc_num_copy(c, aone ? b : a);
		return BC_STATUS_SUCCESS;
	}

	if (a->len + b->len < BC_NUM_KARATSUBA_LEN ||
	    a->len < BC_NUM_KARATSUBA_LEN || b->len < BC_NUM_KARATSUBA_LEN)
	{
		size_t i, j, len;
		unsigned int carry;

		bc_num_expand(c, a->len + b->len + 1);

		memset(c->num, 0, sizeof(BcDig) * c->cap);
		c->len = len = 0;

		for (i = 0; !BC_SIGNAL && i < b->len; ++i) {

			carry = 0;

			for (j = 0; !BC_SIGNAL && j < a->len; ++j) {
				unsigned int in = c->num[i + j];
				in += ((unsigned int) a->num[j]) * ((unsigned int) b->num[i]);
				in += carry;
				carry = in / 10;
				assert(carry >= 0 && carry < 10);
				c->num[i + j] = (BcDig) (in % 10);
				assert(c->num[i + j] >= 0 && c->num[i + j] < 10);
			}

			c->num[i + j] += (BcDig) carry;
			assert(c->num[i + j] >= 0 && c->num[i + j] < 10);
			len = BC_MAX(len, i + j + !!carry);
		}

		c->len = len;

		return BC_SIGNAL ? BC_STATUS_SIGNAL : BC_STATUS_SUCCESS;
	}

	bc_num_init(&l1, max);
	bc_num_init(&h1, max);
	bc_num_init(&l2, max);
	bc_num_init(&h2, max);
	bc_num_init(&m1, max);
	bc_num_init(&m2, max);
	bc_num_init(&z0, max);
	bc_num_init(&z1, max);
	bc_num_init(&z2, max);
	bc_num_init(&temp, max + max);

	bc_num_split(a, max2, &l1, &h1);
	bc_num_split(b, max2, &l2, &h2);

	s = bc_num_add(&h1, &l1, &m1, 0);
	if (s) goto err;
	s = bc_num_add(&h2, &l2, &m2, 0);
	if (s) goto err;

	s = bc_num_k(&h1, &h2, &z0);
	if (s) goto err;
	s = bc_num_k(&m1, &m2, &z1);
	if (s) goto err;
	s = bc_num_k(&l1, &l2, &z2);
	if (s) goto err;

	s = bc_num_sub(&z1, &z0, &temp, 0);
	if (s) goto err;
	s = bc_num_sub(&temp, &z2, &z1, 0);
	if (s) goto err;

	s = bc_num_shift(&z0, max2 * 2);
	if (s) goto err;
	s = bc_num_shift(&z1, max2);
	if (s) goto err;
	s = bc_num_add(&z0, &z1, &temp, 0);
	if (s) goto err;
	s = bc_num_add(&temp, &z2, c, 0);

err:
	bc_num_free(&temp);
	bc_num_free(&z2);
	bc_num_free(&z1);
	bc_num_free(&z0);
	bc_num_free(&m2);
	bc_num_free(&m1);
	bc_num_free(&h2);
	bc_num_free(&l2);
	bc_num_free(&h1);
	bc_num_free(&l1);
	return s;
}

BcStatus bc_num_m(BcNum *a, BcNum *b, BcNum *restrict c, size_t scale) {

	BcStatus s;
	BcNum cpa, cpb;
	size_t maxrdx = BC_MAX(a->rdx, b->rdx);

	scale = BC_MAX(scale, a->rdx);
	scale = BC_MAX(scale, b->rdx);
	scale = BC_MIN(a->rdx + b->rdx, scale);
	maxrdx = BC_MAX(maxrdx, scale);

	bc_num_init(&cpa, a->len);
	bc_num_init(&cpb, b->len);

	bc_num_copy(&cpa, a);
	bc_num_copy(&cpb, b);
	cpa.neg = cpb.neg = false;

	s = bc_num_shift(&cpa, maxrdx);
	if (s) goto err;
	s = bc_num_shift(&cpb, maxrdx);
	if (s) goto err;
	s = bc_num_k(&cpa, &cpb, c);
	if (s) goto err;

	maxrdx += scale;
	bc_num_expand(c, c->len + maxrdx);

	if (c->len < maxrdx) {
		memset(c->num + c->len, 0, (c->cap - c->len) * sizeof(BcDig));
		c->len += maxrdx;
	}

	c->rdx = maxrdx;
	bc_num_retireMul(c, scale, a->neg, b->neg);

err:
	bc_num_free(&cpb);
	bc_num_free(&cpa);
	return s;
}

BcStatus bc_num_d(BcNum *a, BcNum *b, BcNum *restrict c, size_t scale) {

	BcStatus s = BC_STATUS_SUCCESS;
	BcDig *n, *p, q;
	size_t len, end, i;
	BcNum cp;
	bool zero = true;

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

	bc_num_init(&cp, BC_NUM_MREQ(a, b, scale));
	bc_num_copy(&cp, a);
	len = b->len;

	if (len > cp.len) {
		bc_num_expand(&cp, len + 2);
		bc_num_extend(&cp, len - cp.len);
	}

	if (b->rdx > cp.rdx) bc_num_extend(&cp, b->rdx - cp.rdx);
	cp.rdx -= b->rdx;
	if (scale > cp.rdx) bc_num_extend(&cp, scale - cp.rdx);

	if (b->rdx == b->len) {
		for (i = 0; zero && i < len; ++i) zero = !b->num[len - i - 1];
		assert(i != len || !zero);
		len -= i - 1;
	}

	if (cp.cap == cp.len) bc_num_expand(&cp, cp.len + 1);

	// We want an extra zero in front to make things simpler.
	cp.num[cp.len++] = 0;
	end = cp.len - len;

	bc_num_expand(c, cp.len);

	memset(c->num + end, 0, (c->cap - end) * sizeof(BcDig));
	c->rdx = cp.rdx;
	c->len = cp.len;
	p = b->num;

	for (i = end - 1; !BC_SIGNAL && !s && i < end; --i) {
		n = cp.num + i;
		for (q = 0; !s && (n[len] || bc_num_compare(n, p, len) >= 0); ++q)
			s = bc_num_subArrays(n, p, len);
		c->num[i] = q;
	}

	if (!s) bc_num_retireMul(c, scale, a->neg, b->neg);
	bc_num_free(&cp);

	return s;
}

BcStatus bc_num_r(BcNum *a, BcNum *b, BcNum *restrict c,
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
	bc_num_d(a, b, c, scale);

	if (scale) scale = ts;

	s = bc_num_m(c, b, &temp, scale);
	if (s) goto err;
	s = bc_num_sub(a, &temp, d, scale);
	if (s) goto err;

	if (ts > d->rdx && BC_NUM_NONZERO(d)) bc_num_extend(d, ts - d->rdx);

	neg = d->neg;
	bc_num_retireMul(d, ts, a->neg, b->neg);
	d->neg = neg;

err:
	bc_num_free(&temp);
	return s;
}

BcStatus bc_num_rem(BcNum *a, BcNum *b, BcNum *restrict c, size_t scale) {

	BcStatus s;
	BcNum c1;
	size_t ts = BC_MAX(scale + b->rdx, a->rdx), len = BC_NUM_MREQ(a, b, ts);

	bc_num_init(&c1, len);
	s = bc_num_r(a, b, &c1, c, scale, ts);
	bc_num_free(&c1);

	return s;
}

BcStatus bc_num_p(BcNum *a, BcNum *b, BcNum *restrict c, size_t scale) {

	BcStatus s = BC_STATUS_SUCCESS;
	BcNum copy;
	unsigned long pow = 0;
	size_t i, powrdx, resrdx;
	bool neg, zero;

	if (b->rdx) return bc_vm_err(BC_ERROR_MATH_NON_INTEGER);

	if (BC_NUM_ZERO(b)) {
		bc_num_one(c);
		return BC_STATUS_SUCCESS;
	}
	if (BC_NUM_ZERO(a)) {
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
	s = bc_num_ulong(b, &pow);
	b->neg = neg;
	if (s) return s;

	bc_num_init(&copy, a->len);
	bc_num_copy(&copy, a);

	if (!neg) scale = BC_MIN(a->rdx * pow, BC_MAX(scale, a->rdx));

	for (powrdx = a->rdx; !BC_SIGNAL && !(pow & 1); pow >>= 1) {
		powrdx <<= 1;
		s = bc_num_mul(&copy, &copy, &copy, powrdx);
		if (s) goto err;
	}

	if (BC_SIGNAL) {
		s = BC_STATUS_SIGNAL;
		goto err;
	}

	bc_num_copy(c, &copy);
	resrdx = powrdx;

	while (!BC_SIGNAL && (pow >>= 1)) {

		powrdx <<= 1;
		s = bc_num_mul(&copy, &copy, &copy, powrdx);
		if (s) goto err;

		if (pow & 1) {
			resrdx += powrdx;
			s = bc_num_mul(c, &copy, c, resrdx);
			if (s) goto err;
		}
	}

	if (neg) {
		s = bc_num_inv(c, c, scale);
		if (s) goto err;
	}

	if (BC_SIGNAL) {
		s = BC_STATUS_SIGNAL;
		goto err;
	}

	if (c->rdx > scale) bc_num_truncate(c, c->rdx - scale);

	// We can't use bc_num_clean() here.
	for (zero = true, i = 0; zero && i < c->len; ++i) zero = !c->num[i];
	if (zero) bc_num_setToZero(c, scale);

err:
	bc_num_free(&copy);
	return s;
}

#if BC_ENABLE_EXTRA_MATH
BcStatus bc_num_place(BcNum *a, BcNum *b, BcNum *restrict c, size_t scale) {

	BcStatus s = BC_STATUS_SUCCESS;
	unsigned long val = 0;

	BC_UNUSED(scale);

	s = bc_num_intop(a, b, c, &val);
	if (s) return s;

	if (val < c->rdx) bc_num_truncate(c, c->rdx - val);
	else if (val > c->rdx) bc_num_extend(c, val - c->rdx);

	return s;
}

BcStatus bc_num_left(BcNum *a, BcNum *b, BcNum *restrict c, size_t scale) {

	BcStatus s = BC_STATUS_SUCCESS;
	unsigned long val = 0;

	BC_UNUSED(scale);

	s = bc_num_intop(a, b, c, &val);
	if (s) return s;

	bc_num_shift(c, val);

	return s;
}

BcStatus bc_num_right(BcNum *a, BcNum *b, BcNum *restrict c, size_t scale) {

	BcStatus s = BC_STATUS_SUCCESS;
	unsigned long val = 0;
	size_t len;

	BC_UNUSED(scale);

	s = bc_num_intop(a, b, c, &val);
	if (s) return s;

	len = c->rdx + val;

	if (len > c->len) {

		if (len > BC_MAX_NUM) return bc_vm_err(BC_ERROR_MATH_UNDERFLOW);
		if (len > c->cap) bc_num_expand(c, len);

		memset(c->num + c->len, 0, len - c->len);
		c->len = len;
	}

	c->rdx += val;

	assert(c->rdx <= c->len && c->len <= c->cap);

	return s;
}
#endif // BC_ENABLE_EXTRA_MATH

BcStatus bc_num_binary(BcNum *a, BcNum *b, BcNum *c, size_t scale,
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
	assert(c->rdx <= c->len || !c->len);

	if (init) bc_num_free(&num2);

	return s;
}

#ifndef NDEBUG
bool bc_num_strValid(const char *val) {

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

unsigned long bc_num_parseChar(char c, size_t base_t) {

	if (isupper(c)) {
		c = BC_NUM_NUM_LETTER(c);
		c = ((size_t) c) >= base_t ? (char) base_t - 1 : c;
	}
	else c -= '0';

	return (unsigned long) (uchar) c;
}

void bc_num_parseDecimal(BcNum *restrict n, const char *restrict val) {

	size_t len, i;
	const char *ptr;
	bool zero = true;

	for (i = 0; val[i] == '0'; ++i);

	val += i;
	len = strlen(val);

	if (len) {
		for (i = 0; zero && i < len; ++i) zero = (val[i] == '0') || val[i] == '.';
		bc_num_expand(n, len);
	}

	ptr = strchr(val, '.');

	// Explicitly test for NULL here to produce either a 0 or 1.
	n->rdx = (size_t) ((ptr != NULL) * ((val + len) - (ptr + 1)));

	if (!zero) {
		for (i = len - 1; i < len; ++n->len, --i) {

			char c = val[i];

			if (c == '.') n->len -= 1;
			else {
				if (isupper(c)) c = '9';
				n->num[n->len] = c - '0';
			}
		}
	}
}

BcStatus bc_num_parseBase(BcNum *restrict n, const char *restrict val, BcNum *restrict base, size_t base_t) {

	BcStatus s;
	BcNum temp, mult, result;
	BcDig c;
	bool zero = true;
	unsigned long v;
	size_t i, digits, len = strlen(val);

	for (i = 0; zero && i < len; ++i) zero = (val[i] == '.' || val[i] == '0');
	if (zero) return BC_STATUS_SUCCESS;

	bc_num_init(&temp, BC_NUM_DEF_SIZE);
	bc_num_init(&mult, BC_NUM_DEF_SIZE);

	for (i = 0; i < len && (c = val[i]) && c != '.'; ++i) {

		v = bc_num_parseChar(c, base_t);

		s = bc_num_mul(n, base, &mult, 0);
		if (s) goto int_err;
		bc_num_ulong2num(&temp, v);
		s = bc_num_add(&mult, &temp, n, 0);
		if (s) goto int_err;
	}

	if (i == len && !(c = val[i])) goto int_err;

	assert(c == '.');
	bc_num_init(&result, base->len);
	bc_num_one(&mult);

	for (i += 1, digits = 0; i < len && (c = val[i]); ++i, ++digits) {

		v = bc_num_parseChar(c, base_t);

		s = bc_num_mul(&result, base, &result, 0);
		if (s) goto err;
		bc_num_ulong2num(&temp, v);
		s = bc_num_add(&result, &temp, &result, 0);
		if (s) goto err;
		s = bc_num_mul(&mult, base, &mult, 0);
		if (s) goto err;
	}

	s = bc_num_div(&result, &mult, &result, digits);
	if (s) goto err;
	s = bc_num_add(n, &result, n, digits);
	if (s) goto err;

	if (BC_NUM_NONZERO(n)) {
		if (n->rdx < digits) bc_num_extend(n, digits - n->rdx);
	}
	else bc_num_zero(n);

err:
	bc_num_free(&result);
int_err:
	bc_num_free(&mult);
	bc_num_free(&temp);
	return s;
}

void bc_num_printNewline() {
	if (vm->nchars >= (size_t) (vm->line_len - 1)) {
		bc_vm_putchar('\\');
		bc_vm_putchar('\n');
		vm->nchars = 0;
	}
}

#if DC_ENABLED
void bc_num_printChar(size_t n, size_t len, bool rdx)
{
	BC_UNUSED(rdx);
	bc_vm_putchar((uchar) n);
	vm->nchars += len;
}
#endif // DC_ENABLED

void bc_num_printDigits(size_t n, size_t len, bool rdx)
{
	size_t exp, pow;

	bc_num_printNewline();
	bc_vm_putchar(rdx ? '.' : ' ');
	++vm->nchars;

	bc_num_printNewline();
	for (exp = 0, pow = 1; exp < len - 1; ++exp, pow *= 10);

	for (exp = 0; exp < len; pow /= 10, ++vm->nchars, ++exp) {
		size_t dig;
		bc_num_printNewline();
		dig = n / pow;
		n -= dig * pow;
		bc_vm_putchar(((uchar) dig) + '0');
	}
}

void bc_num_printHex(size_t n, size_t len, bool rdx) {

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

void bc_num_printDecimal(const BcNum *restrict n) {

	size_t i, rdx = n->rdx - 1;

	if (n->neg) bc_vm_putchar('-');
	vm->nchars += n->neg;

	for (i = n->len - 1; i < n->len; --i)
		bc_num_printHex((size_t) n->num[i], 1, i == rdx);
}

BcStatus bc_num_printNum(BcNum *restrict n, BcNum *restrict base, size_t len, BcNumDigitOp print)
{
	BcStatus s;
	BcVec stack;
	BcNum intp, fracp, digit, frac_len;
	unsigned long dig, *ptr;
	size_t i;
	bool radix;

	if (BC_NUM_ZERO(n)) {
		print(0, len, false);
		return BC_STATUS_SUCCESS;
	}

	bc_vec_init(&stack, sizeof(unsigned long), NULL);
	bc_num_init(&intp, n->len);
	bc_num_init(&fracp, n->rdx);
	bc_num_init(&digit, len);
	bc_num_init(&frac_len, BC_NUM_INT(n));
	bc_num_copy(&intp, n);
	bc_num_one(&frac_len);

	bc_num_truncate(&intp, intp.rdx);
	s = bc_num_sub(n, &intp, &fracp, 0);
	if (s) goto err;

	while (BC_NUM_NONZERO(&intp)) {
		s = bc_num_divmod(&intp, base, &intp, &digit, 0);
		if (s) goto err;
		s = bc_num_ulong(&digit, &dig);
		if (s) goto err;
		bc_vec_push(&stack, &dig);
	}

	for (i = 0; i < stack.len; ++i) {
		ptr = bc_vec_item_rev(&stack, i);
		assert(ptr);
		print(*ptr, len, false);
	}

	if (!n->rdx) goto err;

	for (radix = true; frac_len.len <= n->rdx; radix = false) {
		s = bc_num_mul(&fracp, base, &fracp, n->rdx);
		if (s) goto err;
		s = bc_num_ulong(&fracp, &dig);
		if (s) goto err;
		bc_num_ulong2num(&intp, dig);
		s = bc_num_sub(&fracp, &intp, &fracp, 0);
		if (s) goto err;
		print(dig, len, radix);
		s = bc_num_mul(&frac_len, base, &frac_len, 0);
		if (s) goto err;
	}

err:
	bc_num_free(&frac_len);
	bc_num_free(&digit);
	bc_num_free(&fracp);
	bc_num_free(&intp);
	bc_vec_free(&stack);
	return s;
}

BcStatus bc_num_printBase(BcNum *restrict n, BcNum *restrict base, size_t base_t) {

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
	n->rdx = n->len = 0;
	n->neg = false;
}

void bc_num_init(BcNum *restrict n, size_t req) {
	assert(n);
	req = req >= BC_NUM_DEF_SIZE ? req : BC_NUM_DEF_SIZE;
	bc_num_setup(n, bc_vm_malloc(req), req);
}

void bc_num_expand(BcNum *restrict n, size_t req) {
	assert(n);
	req = req >= BC_NUM_DEF_SIZE ? req : BC_NUM_DEF_SIZE;
	if (req > n->cap) {
		n->num = bc_vm_realloc(n->num, req);
		n->cap = req;
	}
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
	memcpy(d->num, s->num, sizeof(BcDig) * d->len);
}

BcStatus bc_num_parse(BcNum *restrict n, const char *restrict val,
                      BcNum *restrict base, size_t base_t, bool letter)
{
	BcStatus s = BC_STATUS_SUCCESS;

	assert(n && val && base);
	assert(base_t >= BC_NUM_MIN_BASE && base_t <= vm->max_ibase);
	assert(bc_num_strValid(val));

	if (letter) bc_num_ulong2num(n, bc_num_parseChar(val[0], BC_NUM_MAX_LBASE));
	else if (base_t == 10) bc_num_parseDecimal(n, val);
	else s = bc_num_parseBase(n, val, base, base_t);

	return s;
}

BcStatus bc_num_print(BcNum *restrict n, BcNum *restrict base,
                      size_t base_t, bool newline)
{
	BcStatus s = BC_STATUS_SUCCESS;

	assert(n && base);
	assert(base_t >= BC_NUM_MIN_BASE && base_t <= BC_MAX_OBASE);

	bc_num_printNewline();

	if (BC_NUM_ZERO(n)) bc_num_printHex(0, 1, false);
	else if (base_t == 10) bc_num_printDecimal(n);
	else s = bc_num_printBase(n, base, base_t);

	if (!s && newline) {
		bc_vm_putchar('\n');
		vm->nchars = 0;
	}

	return s;
}

BcStatus bc_num_ulong(const BcNum *restrict n, unsigned long *result) {

	size_t i;
	unsigned long pow, r;

	assert(n && result);

	*result = 0;

	if (n->neg) return bc_vm_err(BC_ERROR_MATH_NEGATIVE);

	for (r = 0, pow = 1, i = n->rdx; i < n->len; ++i) {

		unsigned long prev = r, powprev = pow;

		r += ((unsigned long) n->num[i]) * pow;
		pow *= 10;

		if (r < prev || pow < powprev)
			return bc_vm_verr(BC_ERROR_MATH_OVERFLOW, "number cannot fit");
	}

	*result = r;

	return BC_STATUS_SUCCESS;
}

void bc_num_ulong2num(BcNum *restrict n, unsigned long val) {

	size_t len;
	BcDig *ptr;
	unsigned long i;

	assert(n);

	bc_num_zero(n);

	if (!val) return;

	len = bc_num_log10(ULONG_MAX);
	bc_num_expand(n, len);
	for (ptr = n->num, i = 0; val; ++i, ++n->len, val /= 10) ptr[i] = val % 10;
}

BcStatus bc_num_add(BcNum *a, BcNum *b, BcNum *c, size_t scale) {
	BcNumBinaryOp op = (!a->neg == !b->neg) ? bc_num_a : bc_num_s;
	BC_UNUSED(scale);
	return bc_num_binary(a, b, c, false, op, BC_NUM_AREQ(a, b));
}

BcStatus bc_num_sub(BcNum *a, BcNum *b, BcNum *c, size_t scale) {
	BcNumBinaryOp op = (!a->neg == !b->neg) ? bc_num_s : bc_num_a;
	BC_UNUSED(scale);
	return bc_num_binary(a, b, c, true, op, BC_NUM_AREQ(a, b));
}

BcStatus bc_num_mul(BcNum *a, BcNum *b, BcNum *c, size_t scale) {
	size_t req = BC_NUM_MREQ(a, b, scale);
	return bc_num_binary(a, b, c, scale, bc_num_m, req);
}

BcStatus bc_num_div(BcNum *a, BcNum *b, BcNum *c, size_t scale) {
	size_t req = BC_NUM_MREQ(a, b, scale);
	return bc_num_binary(a, b, c, scale, bc_num_d, req);
}

BcStatus bc_num_mod(BcNum *a, BcNum *b, BcNum *c, size_t scale) {
	size_t req = BC_NUM_MREQ(a, b, scale);
	return bc_num_binary(a, b, c, scale, bc_num_rem, req);
}

BcStatus bc_num_pow(BcNum *a, BcNum *b, BcNum *c, size_t scale) {
	return bc_num_binary(a, b, c, scale, bc_num_p, a->len * b->len + 1);
}

#if BC_ENABLE_EXTRA_MATH
BcStatus bc_num_places(BcNum *a, BcNum *b, BcNum *c, size_t scale) {
	return bc_num_binary(a, b, c, scale, bc_num_place, a->len);
}

BcStatus bc_num_lshift(BcNum *a, BcNum *b, BcNum *c, size_t scale) {
	return bc_num_binary(a, b, c, scale, bc_num_left, a->len);
}

BcStatus bc_num_rshift(BcNum *a, BcNum *b, BcNum *c, size_t scale) {
	return bc_num_binary(a, b, c, scale, bc_num_right, a->len);
}
#endif // BC_ENABLE_EXTRA_MATH

BcStatus bc_num_sqrt(BcNum *restrict a, BcNum *restrict b, size_t scale) {

	BcStatus s;
	BcNum num1, num2, half, f, fprime, *x0, *x1, *temp;
	size_t pow, len, digs, digs1, resrdx, req, times = 0;
	ssize_t cmp = 1, cmp1 = SSIZE_MAX, cmp2 = SSIZE_MAX;
	BcDig half_digs[2];

	assert(a && b && a != b);

	req = BC_MAX(scale, a->rdx) + ((BC_NUM_INT(a) + 1) >> 1) + 1;
	bc_num_expand(b, req);

	if (BC_NUM_ZERO(a)) {
		bc_num_setToZero(b, scale);
		return BC_STATUS_SUCCESS;
	}
	if (a->neg) return bc_vm_err(BC_ERROR_MATH_NEGATIVE);
	if (BC_NUM_ONE(a)) {
		bc_num_one(b);
		bc_num_extend(b, scale);
		return BC_STATUS_SUCCESS;
	}

	scale = BC_MAX(scale, a->rdx) + 1;
	len = a->len + scale;

	bc_num_init(&num1, len);
	bc_num_init(&num2, len);
	bc_num_setup(&half, half_digs, sizeof(half_digs) / sizeof(BcDig));

	bc_num_one(&half);
	half.num[0] = 5;
	half.rdx = 1;

	bc_num_init(&f, len);
	bc_num_init(&fprime, len);

	x0 = &num1;
	x1 = &num2;

	bc_num_one(x0);
	pow = BC_NUM_INT(a);

	if (pow) {

		if (pow & 1) x0->num[0] = 2;
		else x0->num[0] = 6;

		pow -= 2 - (pow & 1);

		bc_num_extend(x0, pow);

		// Make sure to move the radix back.
		x0->rdx -= pow;
	}

	x0->rdx = digs = digs1 = 0;
	resrdx = scale + 2;
	len = BC_NUM_INT(x0) + resrdx - 1;

	while (!BC_SIGNAL && (cmp || digs < len)) {

		s = bc_num_div(a, x0, &f, resrdx);
		if (s) goto err;
		s = bc_num_add(x0, &f, &fprime, resrdx);
		if (s) goto err;
		s = bc_num_mul(&fprime, &half, x1, resrdx);
		if (s) goto err;

		cmp = bc_num_cmp(x1, x0);
		digs = x1->len - (unsigned long long) llabs(cmp);

		if (cmp == cmp2 && digs == digs1) times += 1;
		else times = 0;

		resrdx += times > 4;

		cmp2 = cmp1;
		cmp1 = cmp;
		digs1 = digs;

		temp = x0;
		x0 = x1;
		x1 = temp;
	}

	if (BC_SIGNAL) {
		s = BC_STATUS_SIGNAL;
		goto err;
	}

	bc_num_copy(b, x0);
	scale -= 1;
	if (b->rdx > scale) bc_num_truncate(b, b->rdx - scale);

err:
	bc_num_free(&fprime);
	bc_num_free(&f);
	bc_num_free(&num2);
	bc_num_free(&num1);
	assert(!b->neg || BC_NUM_NONZERO(b));
	assert(b->rdx <= b->len || !b->len);
	return s;
}

BcStatus bc_num_divmod(BcNum *a, BcNum *b, BcNum *c, BcNum *d, size_t scale) {

	BcStatus s;
	BcNum num2, *ptr_a;
	bool init = false;
	size_t ts = BC_MAX(scale + b->rdx, a->rdx), len = BC_NUM_MREQ(a, b, ts);

	assert(c != d && a != b && a != d && b != d && b != c);

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

	BcStatus s;
	BcNum base, exp, two, temp;
	BcDig two_digs[2];

	assert(a && b && c && d && a != d && b != d && c != d);

	if (BC_NUM_ZERO(c)) return bc_vm_err(BC_ERROR_MATH_DIVIDE_BY_ZERO);
	if (b->neg) return bc_vm_err(BC_ERROR_MATH_NEGATIVE);
	if (a->rdx || b->rdx || c->rdx)
		return bc_vm_err(BC_ERROR_MATH_NON_INTEGER);

	bc_num_expand(d, c->len);
	bc_num_init(&base, c->len);
	bc_num_init(&exp, b->len);
	bc_num_setup(&two, two_digs, sizeof(two_digs) / sizeof(BcDig));
	bc_num_init(&temp, b->len);

	bc_num_one(&two);
	two.num[0] = 2;
	bc_num_one(d);

	s = bc_num_rem(a, c, &base, 0);
	if (s) goto err;
	bc_num_copy(&exp, b);

	while (BC_NUM_NONZERO(&exp)) {

		s = bc_num_divmod(&exp, &two, &exp, &temp, 0);
		if (s) goto err;

		if (BC_NUM_ONE(&temp)) {
			s = bc_num_mul(d, &base, &temp, 0);
			if (s) goto err;
			s = bc_num_rem(&temp, c, d, 0);
			if (s) goto err;
		}

		s = bc_num_mul(&base, &base, &temp, 0);
		if (s) goto err;
		s = bc_num_rem(&temp, c, &base, 0);
		if (s) goto err;
	}

err:
	bc_num_free(&temp);
	bc_num_free(&exp);
	bc_num_free(&base);
	assert(!d->neg || d->len);
	return s;
}
#endif // DC_ENABLED
