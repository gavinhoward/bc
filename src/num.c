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
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <status.h>
#include <num.h>
#include <vm.h>

void bc_num_zero(BcNum *n) {
	if (!n) return;
	memset(n->num, 0, n->cap * sizeof(char));
	n->neg = false;
	n->len = 0;
	n->rdx = 0;
}

void bc_num_one(BcNum *n) {
	if (!n) return;
	bc_num_zero(n);
	n->len = 1;
	n->num[0] = 1;
}

void bc_num_ten(BcNum *n) {
	if (!n) return;
	bc_num_zero(n);
	n->len = 2;
	n->num[0] = 0;
	n->num[1] = 1;
}

BcStatus bc_num_subArrays(BcDigit *n1, BcDigit *n2, size_t len) {
	size_t i, j;
	for (i = 0; !bcg.signe && i < len; ++i) {
		for (n1[i] -= n2[i], j = 0; !bcg.signe && n1[i + j] < 0;) {
			n1[i + j++] += 10;
			n1[i + j] -= 1;
		}
	}
	return bcg.signe ? BC_STATUS_EXEC_SIGNAL : BC_STATUS_SUCCESS;
}

ssize_t bc_num_compare(BcDigit *n1, BcDigit *n2, size_t len) {
	size_t i;
	BcDigit c = 0;
	for (i = len - 1; !bcg.signe && i < len && !(c = n1[i] - n2[i]); --i);
	return (c < 0 ? -1 : 1) * (ssize_t) (i + 1);
}

ssize_t bc_num_cmp(BcNum *a, BcNum *b) {

	size_t i, min, a_int, b_int, diff;
	BcDigit *max_num, *min_num;
	bool a_max;
	ssize_t cmp, neg = 1;

	if (!a) return !b ? 0 : !b->neg * -2 + 1;
	else if (!b) return a->neg * -2 + 1;

	if (!a->len) return (!b->neg * -2 + 1) * !!b->len;
	else if (!b->len) return a->neg * -2 + 1;

	if (a->neg) {
		if (b->neg) neg = -1;
		else return -1;
	}
	else if (b->neg) return 1;

	a_int = BC_NUM_INT(a);
	b_int = BC_NUM_INT(b);
	a_int -= b_int;

	if (a_int) return (ssize_t) a_int;

	a_max = a->rdx > b->rdx;

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
	if (cmp) return cmp * (!a_max * -2 + 1) * neg;

	for (max_num -= diff, i = diff - 1; !bcg.signe && i < diff; --i) {
		if (max_num[i]) return neg * (!a_max * -2 + 1);
	}

	return 0;
}

void bc_num_truncate(BcNum *n, size_t places) {

	BcDigit *ptr;

	assert(places <= n->rdx);

	if (!places) return;

	ptr = n->num + places;
	n->len -= places;
	n->rdx -= places;

	memmove(n->num, ptr, n->len * sizeof(BcDigit));
	memset(n->num + n->len, 0, sizeof(BcDigit) * (n->cap - n->len));
}

BcStatus bc_num_extend(BcNum *n, size_t places) {

	BcStatus s;
	BcDigit *ptr;
	size_t len;

	if (!places) return BC_STATUS_SUCCESS;

	len = n->len + places;
	if (n->cap < len && (s = bc_num_expand(n, len))) return s;

	ptr = n->num + places;
	memmove(ptr, n->num, sizeof(BcDigit) * n->len);
	memset(n->num, 0, sizeof(BcDigit) * places);

	n->len += places;
	n->rdx += places;

	return BC_STATUS_SUCCESS;
}

void bc_num_clean(BcNum *n) {
	while (n->len > 0 && !n->num[n->len - 1]) --n->len;
	if (n->len == 0) n->rdx = n->neg = 0;
	else if (n->len < n->rdx) n->len = n->rdx;
}

BcStatus bc_num_retireMul(BcNum *n, size_t scale) {

	BcStatus s = BC_STATUS_SUCCESS;

	if (n->rdx < scale) s = bc_num_extend(n, scale - n->rdx);
	else bc_num_truncate(n, n->rdx - scale);

	bc_num_clean(n);

	return s;
}

BcStatus bc_num_inv(BcNum *a, BcNum *b, size_t scale) {

	BcStatus s;
	BcNum one;

	if ((s = bc_num_init(&one, BC_NUM_DEF_SIZE))) return s;

	bc_num_one(&one);
	s = bc_num_div(&one, a, b, scale);
	bc_num_free(&one);

	return s;
}

BcStatus bc_num_alg_a(BcNum *a, BcNum *b, BcNum *c, size_t sub) {

	BcDigit *ptr, *ptr_a, *ptr_b, *ptr_c;
	size_t i, max, min_rdx, min_int, diff, a_int, b_int;
	BcDigit carry;

	// Because this function doesn't need to use scale (per the bc spec),
	// I am hijacking it to say whether it's doing an add or a subtract.

	if (!a->len) {
		BcStatus s = bc_num_copy(c, b);
		if (sub) c->neg = !c->neg;
		return s;
	}
	else if (!b->len) return bc_num_copy(c, a);

	c->neg = a->neg;
	memset(c->num, 0, c->cap * sizeof(BcDigit));

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

	for (carry = 0, i = 0; !bcg.signe && i < min_rdx + min_int; ++i, ++c->len) {
		ptr_c[i] = ptr_a[i] + ptr_b[i] + carry;
		carry = ptr_c[i] / 10;
		ptr_c[i] %= 10;
	}

	for (; !bcg.signe && i < max + min_rdx; ++i, ++c->len) {
		ptr_c[i] += ptr[i] + carry;
		carry = ptr_c[i] / 10;
		ptr_c[i] %= 10;
	}

	if (bcg.signe) return BC_STATUS_EXEC_SIGNAL;

	if (carry) c->num[c->len++] = carry;

	return BC_STATUS_SUCCESS;
}

BcStatus bc_num_alg_s(BcNum *a, BcNum *b, BcNum *c, size_t sub) {

	BcStatus s;
	ssize_t cmp;
	BcNum *minuend, *subtrahend;
	size_t start;
	bool aneg, bneg, neg;

	// Because this function doesn't need to use scale (per the bc spec),
	// I am hijacking it to say whether it's doing an add or a subtract.

	if (!a->len) {
		s = bc_num_copy(c, b);
		if (sub) c->neg = !b->neg;
		return s;
	}
	else if (!b->len) return bc_num_copy(c, a);

	aneg = a->neg;
	bneg = b->neg;
	a->neg = b->neg = false;

	cmp = bc_num_cmp(a, b);

	a->neg = aneg;
	b->neg = bneg;

	if (!cmp) {
		bc_num_zero(c);
		return BC_STATUS_SUCCESS;
	}
	else if (cmp > 0) {
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

	if ((s = bc_num_copy(c, minuend))) return s;
	c->neg = neg;

	if (c->rdx < subtrahend->rdx) {
		if ((s = bc_num_extend(c, subtrahend->rdx - c->rdx))) return s;
		start = 0;
	}
	else start = c->rdx - subtrahend->rdx;

	s = bc_num_subArrays(c->num + start, subtrahend->num, subtrahend->len);

	bc_num_clean(c);

	return s;
}

BcStatus bc_num_alg_m(BcNum *a, BcNum *b, BcNum *c, size_t scale) {

	BcStatus s;
	BcDigit carry;
	size_t i, j, len;
	bool aone = BC_NUM_ONE(a);

	c->rdx = a->rdx + b->rdx;
	scale = BC_MAX(scale, a->rdx);
	scale = BC_MAX(scale, b->rdx);
	scale = BC_MIN(scale, c->rdx);

	if (!a->len || !b->len) {
		bc_num_zero(c);
		return BC_STATUS_SUCCESS;
	}
	else if (aone || BC_NUM_ONE(b)) {

		if (aone) {
			if ((s = bc_num_copy(c, b))) return s;
			if (a->neg) c->neg = !c->neg;
		}
		else {
			if ((s = bc_num_copy(c, a))) return s;
			if (b->neg) c->neg = !c->neg;
		}

		return bc_num_retireMul(c, scale);
	}

	memset(c->num, 0, sizeof(BcDigit) * c->cap);
	c->len = carry = len = 0;

	for (i = 0; !bcg.signe && i < b->len; ++i) {

		for (j = 0; !bcg.signe && j < a->len; ++j) {
			c->num[i + j] += a->num[j] * b->num[i] + carry;
			carry = c->num[i + j] / 10;
			c->num[i + j] %= 10;
		}

		if (bcg.signe) return BC_STATUS_EXEC_SIGNAL;

		if (carry) {
			c->num[i + j] += carry;
			carry = 0;
			len = BC_MAX(len, i + j + 1);
		}
		else len = BC_MAX(len, i + j);
	}

	if (bcg.signe) return BC_STATUS_EXEC_SIGNAL;

	c->len = BC_MAX(len, c->rdx);
	c->neg = !a->neg != !b->neg;

	return bc_num_retireMul(c, scale);
}

BcStatus bc_num_alg_d(BcNum *a, BcNum *b, BcNum *c, size_t scale) {

	BcStatus s;
	BcDigit *n, *bptr, q;
	size_t len, end, i;
	BcNum cp;

	if (!b->len) return BC_STATUS_MATH_DIVIDE_BY_ZERO;
	else if (!a->len) {
		bc_num_zero(c);
		return BC_STATUS_SUCCESS;
	}
	else if (BC_NUM_ONE(b)) {
		if ((s = bc_num_copy(c, a))) return s;
		if (b->neg) c->neg = !c->neg;
		return bc_num_retireMul(c, scale);
	}

	if ((s = bc_num_init(&cp, BC_NUM_MREQ(a, b, scale)))) return s;
	if ((s = bc_num_copy(&cp, a))) goto err;

	if ((len = b->len) > cp.len) {
		if ((s = bc_num_expand(&cp, len + 2))) goto err;
		if ((s = bc_num_extend(&cp, len - cp.len))) goto err;
	}

	if (b->rdx > cp.rdx && (s = bc_num_extend(&cp, b->rdx - cp.rdx))) goto err;
	cp.rdx -= b->rdx;
	if (scale > cp.rdx && (s = bc_num_extend(&cp, scale - cp.rdx))) goto err;

	if (b->rdx == b->len) {

		bool zero = true;
		for (i = 0; zero && i < len; ++i) zero = !b->num[len - i - 1];

		if (i == len && zero) {
			s = BC_STATUS_MATH_DIVIDE_BY_ZERO;
			goto err;
		}

		len -= i - 1;
	}

	if (cp.cap == cp.len && (s = bc_num_expand(&cp, cp.len + 1))) goto err;

	// We want an extra zero in front to make things simpler.
	cp.num[cp.len++] = 0;
	end = cp.len - len;

	if ((s = bc_num_expand(c, cp.len))) goto err;

	bc_num_zero(c);
	c->rdx = cp.rdx;
	c->len = cp.len;
	bptr = b->num;

	for (i = end - 1; !bcg.signe  && !s && i < end; --i) {

		n = cp.num + i;

		for (q = 0; (!s && n[len]) || bc_num_compare(n, bptr, len) >= 0; ++q)
			s = bc_num_subArrays(n, bptr, len);

		c->num[i] = q;
	}

	if (s) goto err;

	c->neg = !a->neg != !b->neg;
	s = bc_num_retireMul(c, scale);

err:
	bc_num_free(&cp);
	return s;
}

BcStatus bc_num_alg_rem(BcNum *a, BcNum *b, BcNum *c, size_t scale) {

	BcStatus s;
	BcNum c1, c2;
	size_t ts = BC_MAX(scale + b->rdx, a->rdx), len = BC_NUM_MREQ(a, b, ts);

	if (!b->len) return BC_STATUS_MATH_DIVIDE_BY_ZERO;

	if (!a->len) {
		bc_num_zero(c);
		return BC_STATUS_SUCCESS;
	}

	if ((s = bc_num_init(&c1, len))) return s;
	if ((s = bc_num_init(&c2, len))) goto c2_err;
	if ((s = bc_num_div(a, b, &c1, scale))) goto err;

	if (scale) scale = ts;

	if ((s = bc_num_mul(&c1, b, &c2, scale))) goto err;
	if ((s = bc_num_sub(a, &c2, c, scale))) goto err;

	if (ts > c->rdx && c->len) s = bc_num_extend(c, ts - c->rdx);

err:
	bc_num_free(&c2);
c2_err:
	bc_num_free(&c1);
	return s;
}

BcStatus bc_num_alg_p(BcNum *a, BcNum *b, BcNum *c, size_t scale) {

	BcStatus s;
	BcNum copy;
	unsigned long pow;
	size_t i, powrdx, resrdx;
	bool neg, zero;

	if (b->rdx) return BC_STATUS_MATH_NON_INTEGER;

	if (!b->len) {
		bc_num_one(c);
		return BC_STATUS_SUCCESS;
	}
	else if (!a->len) {
		bc_num_zero(c);
		return BC_STATUS_SUCCESS;
	}
	else if (BC_NUM_ONE(b)) {

		if (!b->neg) s = bc_num_copy(c, a);
		else s = bc_num_inv(a, c, scale);

		return s;
	}

	neg = b->neg;
	b->neg = false;

	if ((s = bc_num_ulong(b, &pow))) return s;
	if ((s = bc_num_init(&copy, a->len))) return s;
	if ((s = bc_num_copy(&copy, a))) goto err;

	if (!neg) scale = BC_MIN(a->rdx * pow, BC_MAX(scale, a->rdx));

	b->neg = neg;

	for (powrdx = a->rdx; !bcg.signe && !(pow & 1); pow >>= 1) {
		powrdx <<= 1;
		if ((s = bc_num_mul(&copy, &copy, &copy, powrdx))) goto err;
	}

	if ((s = bc_num_copy(c, &copy))) goto err;
	if (bcg.signe) {
		s = BC_STATUS_EXEC_SIGNAL;
		goto err;
	}

	for (resrdx = powrdx, pow >>= 1; !bcg.signe && pow != 0; pow >>= 1) {

		powrdx <<= 1;

		if ((s = bc_num_mul(&copy, &copy, &copy, powrdx))) goto err;

		if (pow & 1) {
			resrdx += powrdx;
			if ((s = bc_num_mul(c, &copy, c, resrdx))) goto err;
		}
	}

	if (neg && (s = bc_num_inv(c, c, scale))) goto err;
	if (bcg.signe) {
		s = BC_STATUS_EXEC_SIGNAL;
		goto err;
	}

	if (c->rdx > scale) bc_num_truncate(c, c->rdx - scale);

	for (zero = true, i = 0; zero && i < c->len; ++i) zero = !c->num[i];
	if (zero) bc_num_zero(c);

err:
	bc_num_free(&copy);
	return s;
}

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

	if (init) s = bc_num_init(c, req);
	else s = bc_num_expand(c, req);

	if (!s) s = op(ptr_a, ptr_b, c, scale);

	if (init) bc_num_free(&num2);
	return s;
}

bool bc_num_strValid(const char *val, size_t base) {

	BcDigit b;
	bool small, radix = false;
	size_t i, len = strlen(val);

	if (!len) return true;

	small = base <= 10;
	b = (BcDigit) (small ? base + '0' : base - 9 + 'A');

	for (i = 0; i < len; ++i) {

		BcDigit c = val[i];

		if (c == '.') {

			if (radix) return false;

			radix = true;
			continue;
		}

		if (c < '0' || (small && c >= b) || (c > '9' && (c < 'A' || c >= b)))
			return false;
	}

	return true;
}

BcStatus bc_num_parseDecimal(BcNum *n, const char *val) {

	BcStatus s;
	size_t len, i;
	const char *ptr;
	bool zero = true;

	for (i = 0; val[i] == '0'; ++i);

	val += i;
	len = strlen(val);
	bc_num_zero(n);

	if (len) {
		for (i = 0; zero && i < len; ++i) zero = val[i] == '0' || val[i] == '.';
		if ((s = bc_num_expand(n, len))) return s;
	}

	if (zero) {
		memset(n->num, 0, sizeof(BcDigit) * n->cap);
		n->neg = false;
		return BC_STATUS_SUCCESS;
	}

	ptr = strchr(val, '.');

	// Explicitly test for NULL here to produce either a 0 or 1.
	n->rdx = (size_t) ((ptr != NULL) * ((val + len) - (ptr + 1)));

	for (i = len - 1; i < len; ++n->len, i -= 1 + (i && val[i - 1] == '.'))
		n->num[n->len] = val[i] - '0';

	return BC_STATUS_SUCCESS;
}

BcStatus bc_num_parseBase(BcNum *n, const char *val, BcNum *base) {

	BcStatus s;
	BcNum temp, mult, result;
	BcDigit c = '\0';
	bool zero;
	unsigned long v;
	size_t i, digits, len = strlen(val);

	bc_num_zero(n);

	for (zero = true, i = 0; zero && i < len; ++i)
		zero = (val[i] == '.' || val[i] == '0');
	if (zero) return BC_STATUS_SUCCESS;

	if ((s = bc_num_init(&temp, BC_NUM_DEF_SIZE))) return s;
	if ((s = bc_num_init(&mult, BC_NUM_DEF_SIZE))) goto mult_err;

	for (i = 0; i < len && (c = val[i]) != '.'; ++i) {

		v = (unsigned long) (c <= '9' ? c - '0' : c - 'A' + 10);

		if ((s = bc_num_mul(n, base, &mult, 0))) goto int_err;
		if ((s = bc_num_ulong2num(&temp, v))) goto int_err;
		if ((s = bc_num_add(&mult, &temp, n, 0))) goto int_err;
	}

	if (i == len && !(c = val[i])) goto int_err;
	assert(c == '.');
	if ((s = bc_num_init(&result, base->len))) goto int_err;

	bc_num_zero(&result);
	bc_num_one(&mult);

	for (i += 1, digits = 0; i < len && (c = val[i]); ++i, ++digits) {

		v = (unsigned long) (c <= '9' ? c - '0' : c - 'A' + 10);

		if ((s = bc_num_mul(&result, base, &result, 0))) goto err;
		if ((s = bc_num_ulong2num(&temp, v))) goto err;
		if ((s = bc_num_add(&result, &temp, &result, 0))) goto err;
		if ((s = bc_num_mul(&mult, base, &mult, 0))) goto err;
	}

	if ((s = bc_num_div(&result, &mult, &result, digits))) goto err;
	if ((s = bc_num_add(n, &result, n, digits))) goto err;

	if (n->len) {
		if (n->rdx < digits && n->len) s = bc_num_extend(n, digits - n->rdx);
	}
	else bc_num_zero(n);

err:
	bc_num_free(&result);
int_err:
	bc_num_free(&mult);
mult_err:
	bc_num_free(&temp);
	return s;
}

BcStatus bc_num_printNewline(size_t *nchars, size_t line_len) {
	if (*nchars == line_len - 1) {
		if (putchar('\\') == EOF) return BC_STATUS_IO_ERR;
		if (putchar('\n') == EOF) return BC_STATUS_IO_ERR;
		*nchars = 0;
	}
	return BC_STATUS_SUCCESS;
}

BcStatus bc_num_printDigits(size_t num, size_t width, bool radix,
                            size_t *nchars, size_t line_len)
{
	BcStatus s;
	size_t exp, pow, div;

	if ((s = bc_num_printNewline(nchars, line_len))) return s;

	if (putchar(radix ? '.' : ' ') == EOF) return BC_STATUS_IO_ERR;
	++(*nchars);

	if ((s = bc_num_printNewline(nchars, line_len))) return s;

	for (exp = 0, pow = 1; exp < width - 1; ++exp, pow *= 10);

	for (exp = 0; exp < width; pow /= 10, ++(*nchars), ++exp) {

		if ((s = bc_num_printNewline(nchars, line_len))) return s;

		div = num / pow;
		num -= div * pow;

		if (putchar(((char) div) + '0') == EOF) return BC_STATUS_IO_ERR;
	}

	return BC_STATUS_SUCCESS;
}

BcStatus bc_num_printHex(size_t num, size_t width, bool radix,
                         size_t *nchars, size_t line_len)
{
	BcStatus s;

	assert(width == 1);

	if (radix) {
		if ((s = bc_num_printNewline(nchars, line_len))) return s;
		if (putchar('.') == EOF) return BC_STATUS_IO_ERR;
		*nchars += 1;
	}

	if ((s = bc_num_printNewline(nchars, line_len))) return s;
	if (putchar(bc_num_hex_digits[num]) == EOF) return BC_STATUS_IO_ERR;
	*nchars = *nchars + width;

	return BC_STATUS_SUCCESS;
}

BcStatus bc_num_printDecimal(BcNum *n, size_t *nchars, size_t len) {

	BcStatus s = BC_STATUS_SUCCESS;
	size_t i, rdx = n->rdx - 1;

	if (n->neg && putchar('-') == EOF) return BC_STATUS_IO_ERR;
	(*nchars) += n->neg;

	for (i = n->len - 1; !s && i < n->len; --i)
		s = bc_num_printHex((size_t) n->num[i], 1, i == rdx, nchars, len);

	return s;
}

BcStatus bc_num_printBase(BcNum *n, BcNum *base, size_t base_t,
                          size_t *nchars, size_t line_len)
{
	BcStatus s;
	BcVec stack;
	BcNum intp, fracp, digit, frac_len;
	size_t width, i;
	BcNumDigitOp print;
	unsigned long dig, *ptr;
	bool radix, neg = n->neg;

	if (neg && putchar('-') == EOF) return BC_STATUS_IO_ERR;
	(*nchars) += neg;

	n->neg = false;

	if (base_t <= BC_NUM_MAX_IBASE) {
		width = 1;
		print = bc_num_printHex;
	}
	else {
		width = (size_t) floor(log10((double) (base_t - 1)) + 1.0);
		print = bc_num_printDigits;
	}

	if ((s = bc_vec_init(&stack, sizeof(long), NULL))) goto stack_err;
	if ((s = bc_num_init(&intp, n->len))) goto int_err;
	if ((s = bc_num_init(&fracp, n->rdx))) goto frac_err;
	if ((s = bc_num_init(&digit, width))) goto digit_err;
	if ((s = bc_num_init(&frac_len, BC_NUM_INT(n)))) goto frac_len_err;
	if ((s = bc_num_copy(&intp, n))) goto err;
	bc_num_one(&frac_len);

	bc_num_truncate(&intp, intp.rdx);
	if ((s = bc_num_sub(n, &intp, &fracp, 0))) goto err;

	while (intp.len) {
		if ((s = bc_num_rem(&intp, base, &digit, 0))) goto err;
		if ((s = bc_num_ulong(&digit, &dig))) goto err;
		if ((s = bc_vec_push(&stack, &dig))) goto err;
		if ((s = bc_num_div(&intp, base, &intp, 0))) goto err;
	}

	for (i = 0; i < stack.len; ++i) {
		ptr = bc_vec_item_rev(&stack, i);
		assert(ptr);
		if ((s = print(*ptr, width, false, nchars, line_len))) goto err;
	}

	if (!n->rdx) goto err;

	for (radix = true; frac_len.len <= n->rdx; radix = false) {
		if ((s = bc_num_mul(&fracp, base, &fracp, n->rdx))) goto err;
		if ((s = bc_num_ulong(&fracp, &dig))) goto err;
		if ((s = bc_num_ulong2num(&intp, dig))) goto err;
		if ((s = bc_num_sub(&fracp, &intp, &fracp, 0))) goto err;
		if ((s = print(dig, width, radix, nchars, line_len))) goto err;
		if ((s = bc_num_mul(&frac_len, base, &frac_len, 0))) goto err;
	}

err:
	bc_num_free(&frac_len);
frac_len_err:
	bc_num_free(&digit);
digit_err:
	bc_num_free(&fracp);
frac_err:
	bc_num_free(&intp);
int_err:
	bc_vec_free(&stack);
stack_err:
	n->neg = neg;
	return s;
}

BcStatus bc_num_init(BcNum *n, size_t request) {

	assert(n);

	request = request >= BC_NUM_DEF_SIZE ? request : BC_NUM_DEF_SIZE;
	memset(n, 0, sizeof(BcNum));
	if (!(n->num = malloc(request))) return BC_STATUS_ALLOC_ERR;

	n->cap = request;

	return BC_STATUS_SUCCESS;
}

BcStatus bc_num_expand(BcNum *n, size_t request) {

	BcDigit *temp;

	assert(n && request);

	if (request <= n->cap) return BC_STATUS_SUCCESS;
	if (!(temp = realloc(n->num, request))) return BC_STATUS_ALLOC_ERR;

	memset(temp + n->cap, 0, sizeof(char) * (request - n->cap));
	n->num = temp;
	n->cap = request;

	return BC_STATUS_SUCCESS;
}

void bc_num_free(void *num) {
	BcNum *n = (BcNum*) num;
	assert(n);
	if (n->num) free(n->num);
}

BcStatus bc_num_copy(BcNum *d, BcNum *s) {

	BcStatus status;

	assert(d && s);

	if (d == s) return BC_STATUS_SUCCESS;
	if ((status = bc_num_expand(d, s->cap))) return status;

	d->len = s->len;
	d->neg = s->neg;
	d->rdx = s->rdx;

	memcpy(d->num, s->num, sizeof(BcDigit) * d->len);
	memset(d->num + d->len, 0, sizeof(BcDigit) * (d->cap - d->len));

	return BC_STATUS_SUCCESS;
}

BcStatus bc_num_parse(BcNum *n, const char *val, BcNum *base, size_t base_t) {

	BcStatus s;

	assert(n && val && base);
	assert(base_t >= BC_NUM_MIN_BASE && base_t <= BC_NUM_MAX_IBASE);

	if (!bc_num_strValid(val, base_t)) return BC_STATUS_MATH_BAD_STRING;

	if (base_t == 10) s = bc_num_parseDecimal(n, val);
	else s = bc_num_parseBase(n, val, base);

	return s;
}

BcStatus bc_num_print(BcNum *n, BcNum *base, size_t base_t, bool newline,
                      size_t *nchars, size_t line_len)
{
	BcStatus s = BC_STATUS_SUCCESS;

	assert(n && base && nchars);
	assert(base_t >= BC_NUM_MIN_BASE && base_t <= BC_MAX_OBASE);

	if (*nchars >= line_len) {
		if (putchar('\\') == EOF) return BC_STATUS_IO_ERR;
		if (putchar('\n') == EOF) return BC_STATUS_IO_ERR;
		*nchars = 0;
	}

	if (!n->len) {
		if (putchar('0') == EOF) return BC_STATUS_IO_ERR;
		++(*nchars);
	}
	else if (base_t == 10) s = bc_num_printDecimal(n, nchars, line_len);
	else s = bc_num_printBase(n, base, base_t, nchars, line_len);

	if (s) return s;

	if (newline) {
		if (putchar('\n') == EOF) return BC_STATUS_IO_ERR;
		*nchars = 0;
	}

	return s;
}

BcStatus bc_num_ulong(BcNum *n, unsigned long *result) {

	size_t i;
	unsigned long pow;

	assert(n && result);

	if (n->neg) return BC_STATUS_MATH_NEGATIVE;

	for (*result = 0, pow = 1, i = n->rdx; i < n->len; ++i) {

		unsigned long prev = *result, powprev = pow;
		*result += ((unsigned long) n->num[i]) * pow;
		pow *= 10;

		if (*result < prev || pow < powprev) return BC_STATUS_MATH_OVERFLOW;
	}

	return BC_STATUS_SUCCESS;
}

BcStatus bc_num_ulong2num(BcNum *n, unsigned long val) {

	BcStatus s;
	size_t len, i;
	BcDigit *ptr;

	assert(n);

	bc_num_zero(n);

	if (!val) {
		memset(n->num, 0, sizeof(char) * n->cap);
		return BC_STATUS_SUCCESS;
	}

	len = (size_t) ceil(log10(((double) ULONG_MAX) + 1.0));

	if ((s = bc_num_expand(n, len))) return s;

	for (ptr = n->num, i = 0; val; ++i, ++n->len, val /= 10)
		ptr[i] = (char) (val % 10);

	return BC_STATUS_SUCCESS;
}

BcStatus bc_num_add(BcNum *a, BcNum *b, BcNum *c, size_t scale) {
	(void) scale;
	BcNumBinaryOp op = (!a->neg == !b->neg) ? bc_num_alg_a : bc_num_alg_s;
	return bc_num_binary(a, b, c, false, op, BC_NUM_AREQ(a, b));
}

BcStatus bc_num_sub(BcNum *a, BcNum *b, BcNum *c, size_t scale) {
	(void) scale;
	BcNumBinaryOp op = (!a->neg == !b->neg) ? bc_num_alg_s : bc_num_alg_a;
	return bc_num_binary(a, b, c, true, op, BC_NUM_AREQ(a, b));
}

BcStatus bc_num_mul(BcNum *a, BcNum *b, BcNum *c, size_t scale) {
	size_t req = BC_NUM_MREQ(a, b, scale);
	return bc_num_binary(a, b, c, scale, bc_num_alg_m, req);
}

BcStatus bc_num_div(BcNum *a, BcNum *b, BcNum *c, size_t scale) {
	size_t req = BC_NUM_MREQ(a, b, scale);
	return bc_num_binary(a, b, c, scale, bc_num_alg_d, req);
}

BcStatus bc_num_rem(BcNum *a, BcNum *b, BcNum *c, size_t scale) {
	size_t req = BC_NUM_MREQ(a, b, scale);
	return bc_num_binary(a, b, c, scale, bc_num_alg_rem, req);
}

BcStatus bc_num_pow(BcNum *a, BcNum *b, BcNum *c, size_t scale) {
	return bc_num_binary(a, b, c, scale, bc_num_alg_p, a->len * b->len + 1);
}

BcStatus bc_num_sqrt(BcNum *a, BcNum *b, size_t scale) {

	BcStatus s;
	BcNum a2, *ptr_a, num1, num2, half, f, fprime, *x0, *x1, *temp;
	size_t pow, len, digits, digits1, resrdx, req, times = 0;
	ssize_t cmp = 1, cmp1 = SSIZE_MAX, cmp2 = SSIZE_MAX;

	assert(a && b);

	req = BC_MAX(scale, a->rdx) + ((BC_NUM_INT(a)+ 1) >> 1) + 1;

	if (b == a) {
		memcpy(&a2, b, sizeof(BcNum));
		ptr_a = &a2;
		s = bc_num_init(b, req);
	}
	else {
		ptr_a = a;
		s = bc_num_expand(b, req);
	}

	if (s) goto init_err;

	if (!ptr_a->len) {
		bc_num_zero(b);
		goto init_err;
	}
	else if (ptr_a->neg) {
		s = BC_STATUS_MATH_NEG_SQRT;
		goto init_err;
	}
	else if (BC_NUM_ONE(a)) {
		bc_num_one(b);
		s = bc_num_extend(b, scale);
		goto init_err;
	}

	memset(b->num, 0, b->cap * sizeof(BcDigit));
	len = ptr_a->len;

	scale = BC_MAX(scale, ptr_a->rdx) + 1;

	if ((s = bc_num_init(&num1, len))) goto init_err;
	if ((s = bc_num_init(&num2, num1.len))) goto num2_err;
	if ((s = bc_num_init(&half, BC_NUM_DEF_SIZE))) goto two_err;

	bc_num_one(&half);
	half.num[0] = 5;
	half.rdx = 1;

	len += scale;

	if ((s = bc_num_init(&f, len))) goto f_err;
	if ((s = bc_num_init(&fprime, len + scale))) goto fprime_err;

	x0 = &num1;
	x1 = &num2;

	bc_num_one(x0);

	if ((pow = BC_NUM_INT(ptr_a))) {

		if (pow & 1) x0->num[0] = 2;
		else x0->num[0] = 6;

		pow -= 2 - (pow & 1);

		if ((s = bc_num_extend(x0, pow))) goto err;

		// Make sure to move the radix back.
		x0->rdx -= pow;
	}

	x0->rdx = digits = digits1 = 0;
	resrdx = scale + 2;
	len = BC_NUM_INT(x0) + resrdx - 1;

	while (!bcg.signe && cmp && digits <= len) {

		if ((s = bc_num_div(a, x0, &f, resrdx))) goto err;
		if ((s = bc_num_add(x0, &f, &fprime, resrdx))) goto err;
		if ((s = bc_num_mul(&fprime, &half, x1, resrdx))) goto err;

		cmp = bc_num_cmp(x1, x0);
		digits = x1->len - (unsigned long long) llabs(cmp);

		// Break if we're going nowhere.
		if ((times += (cmp == cmp2 && digits == digits1)) > 2 * len) break;

		cmp2 = cmp1;
		cmp1 = cmp;
		digits1 = digits;

		temp = x0;
		x0 = x1;
		x1 = temp;
	}

	if (bcg.signe) {
		s = BC_STATUS_EXEC_SIGNAL;
		goto err;
	}

	if ((s = bc_num_copy(b, x0))) goto err;

	if (b->rdx > --scale) bc_num_truncate(b, b->rdx - scale);
	else if (b->rdx < scale) s = bc_num_extend(b, scale - b->rdx);

err:
	bc_num_free(&fprime);
fprime_err:
	bc_num_free(&f);
f_err:
	bc_num_free(&half);
two_err:
	bc_num_free(&num2);
num2_err:
	bc_num_free(&num1);
init_err:
	if (b == a) bc_num_free(&a2);
	return s;
}

#ifdef DC_ENABLED
BcStatus bc_num_modexp(BcNum *a, BcNum *b, BcNum *c, BcNum *d, size_t scale) {

	BcStatus s;
	BcNum num2, *ptr_a, *ptr_b, *ptr_c, base, exp, two, temp;
	bool init = false;

	assert(a && b && c && d);

	scale = 0;

	if (d == a) {
		memcpy(&num2, d, sizeof(BcNum));
		ptr_a = &num2;
		init = true;
	}
	else ptr_a = a;

	if (d == b) {

		ptr_b = &num2;

		if (d != a) {
			memcpy(ptr_b, d, sizeof(BcNum));
			init = true;
		}
	}
	else ptr_b = b;

	if (d == c) {

		ptr_c = &num2;

		if (d != a && d != b) {
			memcpy(ptr_c, d, sizeof(BcNum));
			init = true;
		}
	}
	else ptr_c = c;

	if (init) s = bc_num_init(d, ptr_c->len);
	else s = bc_num_expand(d, ptr_c->len);

	if (s) return s;

	if (!ptr_c->len) {
		s = BC_STATUS_MATH_DIVIDE_BY_ZERO;
		goto base_err;
	}

	if (ptr_a->rdx || ptr_b->rdx || ptr_c->rdx) {
		s = BC_STATUS_MATH_NON_INTEGER;
		goto base_err;
	}

	if (ptr_b->neg) {
		s = BC_STATUS_MATH_NEGATIVE;
		goto base_err;
	}

	if ((s = bc_num_init(&base, ptr_c->len))) goto base_err;
	if ((s = bc_num_init(&exp, ptr_b->len))) goto exp_err;
	if ((s = bc_num_init(&two, BC_NUM_DEF_SIZE))) goto two_err;
	if ((s = bc_num_init(&temp, ptr_b->len))) goto temp_err;

	bc_num_one(&two);
	two.num[0] = 2;
	bc_num_one(d);

	if ((s = bc_num_rem(ptr_a, ptr_c, &base, scale))) goto err;
	if ((s = bc_num_copy(&exp, ptr_b))) goto err;

	while (exp.len) {

		if ((s = bc_num_rem(&exp, &two, &temp, scale))) goto err;

		if (BC_NUM_ONE(&temp)) {
			if ((s = bc_num_mul(d, &base, &temp, scale))) goto err;
			if ((s = bc_num_rem(&temp, ptr_c, d, scale))) goto err;
		}

		if ((s = bc_num_div(&exp, &two, &exp, scale))) goto err;
		if ((s = bc_num_mul(&base, &base, &temp, scale))) goto err;
		if ((s = bc_num_rem(&temp, ptr_c, &base, scale))) goto err;
	}

err:
	bc_num_free(&temp);
temp_err:
	bc_num_free(&two);
two_err:
	bc_num_free(&exp);
exp_err:
	bc_num_free(&base);
base_err:
	if (init) bc_num_free(&num2);
	return s;
}
#endif // DC_ENABLED
