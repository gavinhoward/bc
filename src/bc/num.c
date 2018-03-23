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
#include <stdbool.h>
#include <string.h>

#include <math.h>

#include <limits.h>

#include <status.h>
#include <num.h>
#include <vector.h>
#include <bc.h>

static void bc_num_subArrays(BcDigit *n1, BcDigit *n2,
                             size_t len, size_t extra)
{
  size_t i, j;

  for (i = 0; i < len; ++i) {

    n1[i] -= n2[i];

    for (j = 0; n1[i + j] < 0;) {
      assert(j < len + extra + 1);
      n1[i + j++] += 10;
      n1[i + j] -= 1;
    }
  }
}

static int bc_num_compare(BcDigit *n1, BcDigit *n2, size_t len, size_t *digits)
{
  size_t i, digs;
  BcDigit c;

  for (digs = 0, i = len - 1; i < len; ++digs, --i) {
    if ((c = n1[i] - n2[i])) break;
  }

  if (digits) *digits = digs;

  return c;
}

int bc_num_cmp(BcNum *a, BcNum *b, size_t *digits) {

  size_t i, min, a_int, b_int, diff;
  BcDigit *max_num;
  BcDigit *min_num;
  bool a_max;
  int cmp, neg;

  if (digits) *digits = 0;

  if (!a) return !b ? 0 : !b->neg * -2 + 1;
  else if (!b) return a->neg * -2 + 1;

  neg = 1;

  if (a->neg) {
    if (b->neg) neg = -1;
    else return -1;
  }
  else if (b->neg) return 1;

  if (!a->len) return (!b->neg * -2 + 1) * !!b->len;
  else if (!b->len) return a->neg * -2 + 1;

  a_int = a->len - a->rdx;
  b_int = b->len - b->rdx;
  a_int -= b_int;

  if (a_int) return a_int;

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

  cmp = bc_num_compare(max_num, min_num, b_int + min, digits);

  if (cmp) return cmp * (!a_max * -2 + 1) * neg;

  for (max_num -= diff, i = diff - 1; i < diff; --i) {
    if (max_num[i]) return neg * (!a_max * -2 + 1);
  }

  return 0;
}

static BcStatus bc_num_truncate(BcNum *n, size_t places) {

  BcDigit *ptr;

  assert(places <= n->rdx);

  if (!places) return BC_STATUS_SUCCESS;

  ptr = n->num + places;
  n->len -= places;
  n->rdx -= places;

  memmove(n->num, ptr, n->len * sizeof(BcDigit));
  memset(n->num + n->len, 0, sizeof(BcDigit) * (n->cap - n->len));

  return BC_STATUS_SUCCESS;
}

static BcStatus bc_num_extend(BcNum *n, size_t places) {

  BcStatus status;
  BcDigit *ptr;
  size_t len;

  if (!places) return BC_STATUS_SUCCESS;

  len = n->len + places;

  if (n->cap < len && (status = bc_num_expand(n, len))) return status;

  ptr = n->num + places;

  memmove(ptr, n->num, sizeof(BcDigit) * n->len);
  memset(n->num, 0, sizeof(BcDigit) * places);

  n->len += places;
  n->rdx += places;

  return BC_STATUS_SUCCESS;
}

BcStatus bc_num_inv(BcNum *a, BcNum *b, size_t scale) {

  BcStatus status;
  BcNum one;

  if ((status = bc_num_init(&one, BC_NUM_DEF_SIZE))) return status;

  bc_num_one(&one);
  status = bc_num_div(&one, a, b, scale);
  bc_num_free(&one);

  return status;
}

static BcStatus bc_num_alg_a(BcNum *a, BcNum *b, BcNum *c, size_t scale) {

  BcDigit *ptr;
  BcDigit *ptr_a;
  BcDigit *ptr_b;
  BcDigit *ptr_c;
  size_t i, max, min_rdx, min_int, diff, a_int, b_int;
  BcDigit carry;

  (void) scale;

  if (!a->len) return bc_num_copy(c, b);
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

  a_int = a->len - a->rdx;
  b_int = b->len - b->rdx;

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

  ptr_a = a->num + a->rdx;
  ptr_b = b->num + b->rdx;
  ptr_c = c->num + c->rdx;

  for (carry = 0, i = 0; i < min_rdx + min_int; ++i, ++c->len) {
    ptr_c[i] = ptr_a[i] + ptr_b[i] + carry;
    carry = ptr_c[i] / 10;
    ptr_c[i] %= 10;
  }

  for (; i < max; ++i, ++c->len) {
    ptr_c[i] += ptr[i] + carry;
    carry = ptr_c[i] / 10;
    ptr_c[i] %= 10;
  }

  if (carry) c->num[c->len++] = carry;

  return BC_STATUS_SUCCESS;
}

static BcStatus bc_num_alg_s(BcNum *a, BcNum *b, BcNum *c, size_t sub) {

  BcStatus status;
  int cmp;
  BcNum *minuend;
  BcNum *subtrahend;
  size_t start;
  bool aneg, bneg, neg;

  // Because this function doesn't need to use scale (per the bc spec),
  // I am hijacking it to tell this function whether it is doing an add
  // or a subtract.

  if (!a->len) {
    status = bc_num_copy(c, b);
    c->neg = !b->neg;
    return status;
  }
  else if (!b->len) return bc_num_copy(c, a);

  aneg = a->neg;
  bneg = b->neg;
  a->neg = b->neg = false;

  cmp = bc_num_cmp(a, b, NULL);

  a->neg = aneg;
  b->neg = bneg;

  if (!cmp) {
    bc_num_zero(c);
    return BC_STATUS_SUCCESS;
  }
  else if (cmp > 0) {
    neg = sub && a->neg;
    minuend = a;
    subtrahend = b;
  }
  else {
    neg = sub && !b->neg;
    minuend = b;
    subtrahend = a;
  }

  if ((status = bc_num_copy(c, minuend))) return status;

  c->neg = neg;

  if (c->rdx < subtrahend->rdx) {
    if ((status = bc_num_extend(c, subtrahend->rdx - c->rdx))) return status;
    start = 0;
  }
  else start = c->rdx - subtrahend->rdx;

  bc_num_subArrays(c->num + start, subtrahend->num, subtrahend->len,
                   c->len - subtrahend->len);

  while (c->len > c->rdx && !c->num[c->len - 1]) --c->len;

  return status;
}

static BcStatus bc_num_alg_m(BcNum *a, BcNum *b, BcNum *c, size_t scale) {

  BcStatus status;
  BcDigit carry;
  size_t i, j, len;

  if (!a->len || !b->len) {
    bc_num_zero(c);
    return BC_STATUS_SUCCESS;
  }
  else if (BC_NUM_ONE(a)) {
    status = bc_num_copy(c, b);
    if (a->neg) c->neg = !c->neg;
    return status;
  }
  else if (BC_NUM_ONE(b)) {
    status = bc_num_copy(c, a);
    if (b->neg) c->neg = !c->neg;
    return status;
  }

  scale = BC_MAX(scale, a->rdx);
  scale = BC_MAX(scale, b->rdx);
  c->rdx = a->rdx + b->rdx;

  memset(c->num, 0, sizeof(BcDigit) * c->cap);
  c->len = carry = len = 0;

  for (i = 0; i < b->len; ++i) {

    for (j = 0; j < a->len; ++j) {
      c->num[i + j] += a->num[j] * b->num[i] + carry;
      carry = c->num[i + j] / 10;
      c->num[i + j] %= 10;
    }

    if (carry) {
      c->num[i + j] += carry;
      carry = 0;
      len = BC_MAX(len, i + j + 1);
    }
    else len = BC_MAX(len, i + j);
  }

  c->len = BC_MAX(len, c->rdx);
  c->neg = !a->neg != !b->neg;

  if (scale < c->rdx) status = bc_num_truncate(c, c->rdx - scale);
  else status = BC_STATUS_SUCCESS;

  while (c->len > c->rdx && !c->num[c->len - 1]) --c->len;

  return status;
}

static BcStatus bc_num_alg_d(BcNum *a, BcNum *b, BcNum *c, size_t scale) {

  BcStatus status;
  BcDigit *ptr;
  BcDigit *bptr;
  size_t len, end, i;
  BcDigit q;
  BcNum copy;
  bool zero;

  if (!b->len) return BC_STATUS_MATH_DIVIDE_BY_ZERO;
  else if (!a->len) {
    bc_num_zero(c);
    return BC_STATUS_SUCCESS;
  }
  else if (BC_NUM_ONE(b)) {

    if ((status = bc_num_copy(c, a))) return status;

    if (b->neg) c->neg = !c->neg;

    if (c->rdx < scale) status = bc_num_extend(c, scale - c->rdx);
    else status = bc_num_truncate(c, c->rdx - scale);

    return status;
  }

  if ((status = bc_num_init(&copy, a->len + b->rdx + scale + 1))) return status;
  if ((status = bc_num_copy(&copy, a))) goto err;

  len = b->len;

  if (len > copy.len) {
    if ((status = bc_num_expand(&copy, len + 2))) goto err;
    if ((status = bc_num_extend(&copy, len - copy.len))) goto err;
  }

  if (b->rdx > copy.rdx && (status = bc_num_extend(&copy, b->rdx - copy.rdx)))
    goto err;

  copy.rdx -= b->rdx;

  if (scale > copy.rdx && (status = bc_num_extend(&copy, scale - copy.rdx)))
    goto err;

  if (b->rdx == b->len) {

    bool zero = true;

    for (i = 0; zero && i < len; ++i) zero = !b->num[len - i - 1];

    if (i == len) return BC_STATUS_MATH_DIVIDE_BY_ZERO;

    len -= i - 1;
  }

  if (copy.cap == copy.len) {
    status = bc_num_expand(&copy, copy.len + 1);
    if (status) goto err;
  }

  // We want an extra zero in front to make things simpler.
  copy.num[copy.len++] = 0;
  end = copy.len - len;

  if ((status = bc_num_expand(c, copy.len))) goto err;

  bc_num_zero(c);
  c->rdx = copy.rdx;
  c->len = copy.len;
  bptr = b->num;

  for (i = end - 1; i < end; --i) {

    ptr = copy.num + i;

    for (q = 0; ptr[len] || bc_num_compare(ptr, bptr, len, NULL) >= 0; ++q)
      bc_num_subArrays(ptr, bptr, len, 1);

    c->num[i] = q;
  }

  c->neg = !a->neg != !b->neg;

  while (c->len > c->rdx && !c->num[c->len - 1]) --c->len;

  if (c->rdx > scale) status = bc_num_truncate(c, c->rdx - scale);

  for (i = 0, zero = true; zero && i < c->len; ++i) zero = !c->num[i];
  if (zero) bc_num_zero(c);

err:

  bc_num_free(&copy);

  return status;
}

static BcStatus bc_num_alg_mod(BcNum *a, BcNum *b, BcNum *c, size_t scale) {

  BcStatus status;
  BcNum c1, c2;
  size_t len;

  if (!b->len) return BC_STATUS_MATH_DIVIDE_BY_ZERO;

  if (!a->len) {
    bc_num_zero(c);
    return BC_STATUS_SUCCESS;
  }

  len = a->len + b->len + scale;

  if ((status = bc_num_init(&c1, len))) return status;
  if ((status = bc_num_init(&c2, len))) goto c2_err;
  if ((status = bc_num_div(a, b, &c1, scale))) goto err;

  c->rdx = BC_MAX(scale + b->rdx, a->rdx);

  if ((status = bc_num_mul(&c1, b, &c2, scale))) goto err;

  status = bc_num_sub(a, &c2, c, scale);

err:

  bc_num_free(&c2);

c2_err:

  bc_num_free(&c1);

  return status;
}

static BcStatus bc_num_alg_p(BcNum *a, BcNum *b, BcNum *c, size_t scale) {

  BcStatus status;
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

    if (!b->neg) status = bc_num_copy(c, a);
    else status = bc_num_inv(a, c, scale);

    return status;
  }

  neg = b->neg;
  b->neg = false;

  if (!neg) scale = BC_MIN(a->rdx * pow, BC_MAX(scale, a->rdx));

  if ((status = bc_num_ulong(b, &pow))) return status;
  if ((status = bc_num_init(&copy, a->len))) return status;
  if ((status = bc_num_copy(&copy, a))) goto err;

  for (powrdx = a->rdx; !(pow & 1); pow >>= 1) {
    powrdx <<= 1;
    if ((status = bc_num_mul(&copy, &copy, &copy, powrdx))) goto err;
  }

  if ((status = bc_num_copy(c, &copy))) goto err;

  resrdx = powrdx;

  for (pow >>= 1; pow != 0; pow >>= 1) {

    powrdx <<= 1;

    if ((status = bc_num_mul(&copy, &copy, &copy, powrdx))) goto err;

    if (pow & 1) {
      resrdx += powrdx;
      if ((status = bc_num_mul(c, &copy, c, resrdx))) goto err;
    }
  }

  if (neg && (status = bc_num_inv(a, c, scale))) goto err;

  if (c->rdx > scale && (status = bc_num_truncate(c, c->rdx - scale))) goto err;

  for (zero = true, i = 0; zero && i < c->len; ++i) zero = c->num[i] == 0;
  if (zero) bc_num_zero(c);

err:

  bc_num_free(&copy);

  return status;
}

static BcStatus bc_num_sqrt_newton(BcNum *a, BcNum *b, size_t scale) {

  BcStatus status;
  BcNum num1, num2, two, f, fprime;
  BcNum *x0;
  BcNum *x1;
  BcNum *temp;
  size_t pow, len, digits, resrdx;
  int cmp;

  if (!a->len) {
    bc_num_zero(b);
    return BC_STATUS_SUCCESS;
  }
  else if (a->neg) return BC_STATUS_MATH_NEG_SQRT;
  else if (BC_NUM_ONE(a)) {
    bc_num_one(b);
    return bc_num_extend(b, scale);
  }

  memset(b->num, 0, b->cap * sizeof(BcDigit));
  len = a->len;

  scale = BC_MAX(scale, a->rdx) + 1;

  if ((status = bc_num_init(&num1, len))) return status;
  if ((status = bc_num_init(&num2, num1.len))) goto num2_err;
  if ((status = bc_num_init(&two, BC_NUM_DEF_SIZE))) goto two_err;

  bc_num_one(&two);
  two.num[0] = 2;

  len += scale;

  if ((status = bc_num_init(&f, len))) goto f_err;
  if ((status = bc_num_init(&fprime, len + scale))) goto fprime_err;

  x0 = &num1;
  x1 = &num2;

  bc_num_one(x0);

  pow = a->len - a->rdx;

  if (pow) {

    if (pow & 1) {
      x0->num[0] = 2;
      pow -= 1;
    }
    else {
      x0->num[0] = 6;
      pow -= 2;
    }

    if ((status = bc_num_extend(x0, pow))) goto err;
  }

  cmp = 1;
  x0->rdx = digits = 0;
  resrdx = scale + 1;
  len = (x0->len - x0->rdx) + resrdx;

  while (cmp && digits <= len) {

    if ((status = bc_num_mul(x0, x0, &f, resrdx))) goto err;
    if ((status = bc_num_sub(&f, a, &f, resrdx))) goto err;
    if ((status = bc_num_mul(x0, &two, &fprime, resrdx))) goto err;
    if ((status = bc_num_div(&f, &fprime, &f, resrdx))) goto err;
    if ((status = bc_num_sub(x0, &f, x1, resrdx))) goto err;

    cmp = bc_num_cmp(x1, x0, &digits);

    temp = x0;
    x0 = x1;
    x1 = temp;
  }

  if ((status = bc_num_copy(b, x0))) goto err;

  --scale;

  if (b->rdx > scale) status = bc_num_truncate(b, b->rdx - scale);
  else if (b->rdx < scale) status = bc_num_extend(b, scale - b->rdx);

err:

  bc_num_free(&fprime);

fprime_err:

  bc_num_free(&f);

f_err:

  bc_num_free(&two);

two_err:

  bc_num_free(&num2);

num2_err:

  bc_num_free(&num1);

  return status;
}

static BcStatus bc_num_binary(BcNum *a, BcNum *b, BcNum *c,  size_t scale,
                              BcNumBinaryFunc op, size_t req)
{
  BcStatus status;
  BcNum num2;
  BcNum *ptr_a;
  BcNum *ptr_b;
  bool init;

  assert(a && b && c && op);

  init = false;

  if (c == a) {
    memcpy(&num2, c, sizeof(BcNum));
    ptr_a = &num2;
    init = true;
  }
  else ptr_a = a;

  if (c == b) {

    if (c == a) {
      ptr_b = ptr_a;
    }
    else {
      memcpy(&num2, c, sizeof(BcNum));
      ptr_b = &num2;
      init = true;
    }
  }
  else ptr_b = b;

  if (init) status = bc_num_init(c, req);
  else status = bc_num_expand(c, req);

  if (status) goto err;

  status = op(ptr_a, ptr_b, c, scale);

err:

  if (c == a || c == b) bc_num_free(&num2);

  return status;
}

static BcStatus bc_num_unary(BcNum *a, BcNum *b, size_t scale,
                             BcNumUnaryFunc op, size_t req)
{
  BcStatus status;
  BcNum a2;
  BcNum *ptr_a;

  assert(a && b && op);

  if (b == a) {
    memcpy(&a2, b, sizeof(BcNum));
    ptr_a = &a2;
    status = bc_num_init(b, req);
  }
  else {
    ptr_a = a;
    status = bc_num_expand(b, req);
  }

  if (status) goto err;

  status = op(ptr_a, b, scale);

err:

  if (b == a) bc_num_free(&a2);

  return status;
}

static bool bc_num_strValid(const char *val, size_t base) {

  size_t len, i;
  BcDigit c, b;
  bool small, radix;

  radix = false;
  len = strlen(val);

  if (!len) return true;

  small = base <= 10;
  b = small ? base + '0' : base - 9 + 'A';

  for (i = 0; i < len; ++i) {

    c = val[i];

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

static BcStatus bc_num_parseDecimal(BcNum *n, const char *val) {

  BcStatus status;
  size_t len, i;
  const char *ptr;
  bool zero;

  for (i = 0; val[i] == '0'; ++i);

  val += i;
  len = strlen(val);

  bc_num_zero(n);
  zero = true;

  if (len) {
    for (i = 0; zero && i < len; ++i) zero = val[i] == '0' || val[i] == '.';
    if ((status = bc_num_expand(n, len))) return status;
  }

  if (zero) {
    memset(n->num, 0, sizeof(BcDigit) * n->cap);
    n->neg = false;
    return BC_STATUS_SUCCESS;
  }

  ptr = strchr(val, '.');

  n->rdx = (ptr != 0) * ((val + len) - (ptr + 1));

  for (i = len - 1; i < len; ++n->len, i -= 1 + (val[i - 1] == '.'))
    n->num[i] = val[i] - '0';

  return BC_STATUS_SUCCESS;
}

static BcStatus bc_num_parseBase(BcNum *n, const char *val, BcNum *base) {

  BcStatus status;
  BcNum temp, mult, result;
  size_t i, len, digits;
  BcDigit c;
  bool zero;
  unsigned long v;

  len = strlen(val);
  zero = true;

  for (zero = true, i = 0; zero && i < len; ++i)
    zero = (val[i] == '.' || val[i] == '0');

  if (zero) {
    bc_num_zero(n);
    return BC_STATUS_SUCCESS;
  }

  if ((status = bc_num_init(&temp, BC_NUM_DEF_SIZE))) return status;
  if ((status = bc_num_init(&mult, BC_NUM_DEF_SIZE))) goto mult_err;

  bc_num_zero(n);

  for (i = 0; i < len && (c = val[i]) != '.'; ++i) {

    v = c <= '9' ? c - '0' : c - 'A' + 10;

    if ((status = bc_num_mul(n, base, &mult, 0))) goto int_err;
    if ((status = bc_num_ulong2num(&temp, v))) goto int_err;
    if ((status = bc_num_add(&mult, &temp, n, 0))) goto int_err;
  }

  if (i == len) c = val[i++];
  if (c == '\0') goto int_err;

  assert(c == '.');

  if ((status = bc_num_init(&result, base->len))) goto int_err;

  bc_num_zero(&result);
  bc_num_one(&mult);

  for (digits = 0; i < len && (c = val[i]); ++i, ++digits) {

    v = c <= '9' ? c - '0' : c - 'A' + 10;

    if ((status = bc_num_mul(&result, base, &result, 0))) goto err;
    if ((status = bc_num_ulong2num(&temp, v))) goto err;
    if ((status = bc_num_add(&result, &temp, &result, 0))) goto err;
    if ((status = bc_num_mul(&mult, base, &mult, 0))) goto err;
  }

  if ((status = bc_num_div(&result, &mult, &result, digits))) goto err;
  if ((status = bc_num_add(n, &result, n, digits))) goto err;

  if (n->len) {
    if (n->rdx < digits && n->len) status = bc_num_extend(n, digits - n->rdx);
  }
  else bc_num_zero(n);

err:

  bc_num_free(&result);

int_err:

  bc_num_free(&mult);

mult_err:

  bc_num_free(&temp);

  return status;
}

static BcStatus bc_num_printDigits(unsigned long num, size_t width, bool radix,
                                   size_t *nchars)
{
  size_t exp, pow, div;

  if (*nchars == BC_NUM_PRINT_WIDTH - 1) {
    if (fputc('\\', stdout) == EOF) return BC_STATUS_IO_ERR;
    if (fputc('\n', stdout) == EOF) return BC_STATUS_IO_ERR;
    *nchars = 0;
  }

  if (fputc(radix ? '.' : ' ', stdout) == EOF) return BC_STATUS_IO_ERR;
  ++(*nchars);

  for (exp = 0, pow = 1; exp < width - 1; ++exp, pow *= 10);

  for (; pow; pow /= 10, ++(*nchars)) {

    if (*nchars == BC_NUM_PRINT_WIDTH - 1) {
      if (fputc('\\', stdout) == EOF) return BC_STATUS_IO_ERR;
      if (fputc('\n', stdout) == EOF) return BC_STATUS_IO_ERR;
      *nchars = 0;
    }

    div = num / pow;
    num -= div * pow;

    if (fputc(((char) div) + '0', stdout) == EOF) return BC_STATUS_IO_ERR;
  }

  return BC_STATUS_SUCCESS;
}

static BcStatus bc_num_printHex(unsigned long num, size_t width, bool radix,
                                size_t *nchars)
{
  width += !!radix;
  if (*nchars + width  >= BC_NUM_PRINT_WIDTH) {
    if (fputc('\\', stdout) == EOF) return BC_STATUS_IO_ERR;
    if (fputc('\n', stdout) == EOF) return BC_STATUS_IO_ERR;
    *nchars = 0;
  }

  if (radix && fputc('.', stdout) == EOF) return BC_STATUS_IO_ERR;
  if (fputc(bc_num_hex_digits[num], stdout) == EOF) return BC_STATUS_IO_ERR;

  *nchars = *nchars + width;

  return BC_STATUS_SUCCESS;
}

static BcStatus bc_num_printDecimal(BcNum *n, size_t *nchars) {

  BcStatus status;
  size_t i;

  if (n->neg) {
    if (fputc('-', stdout) == EOF) return BC_STATUS_IO_ERR;
    ++(*nchars);
  }

  status = BC_STATUS_SUCCESS;

  for (i = n->len - 1; !status && i < n->len; --i)
    status = bc_num_printHex(n->num[i], 1, i == n->rdx - 1, nchars);

  return status;
}

static BcStatus bc_num_printBase(BcNum *n, BcNum *base, size_t base_t,
                                 size_t *nchars)
{
  BcStatus status;
  BcVec stack;
  BcNum intp, fracp, digit, frac_len;
  size_t width, i;
  BcNumDigitFunc print;
  unsigned long dig;
  long *ptr;
  bool neg, radix;

  neg = n->neg;
  n->neg = false;

  if (neg) {
    if (fputc('-', stdout) == EOF) return BC_STATUS_IO_ERR;
    ++nchars;
  }

  if (base_t <= BC_NUM_MAX_INPUT_BASE) {
    width = 1;
    print = bc_num_printHex;
  }
  else {
    width = (size_t) floor(log10((double) (base_t - 1)) + 1.0);
    print = bc_num_printDigits;
  }

  if ((status = bc_vec_init(&stack, sizeof(long), NULL))) return status;
  if ((status = bc_num_init(&intp, n->len))) goto int_err;
  if ((status = bc_num_init(&fracp, n->rdx))) goto frac_err;
  if ((status = bc_num_init(&digit, width))) goto digit_err;
  if ((status = bc_num_copy(&intp, n))) goto frac_len_err;
  if ((status = bc_num_truncate(&intp, intp.rdx))) goto frac_len_err;
  if ((status = bc_num_sub(n, &intp, &fracp, 0))) goto frac_len_err;

  while (intp.len) {
    if ((status = bc_num_mod(&intp, base, &digit, 0))) goto frac_len_err;
    if ((status = bc_num_ulong(&digit, &dig))) goto frac_len_err;
    if ((status = bc_vec_push(&stack, &dig))) goto frac_len_err;
    if ((status = bc_num_div(&intp, base, &intp, 0))) goto frac_len_err;
  }

  for (i = 0; i < stack.len; ++i) {
    ptr = bc_vec_item_rev(&stack, i);
    assert(ptr);
    if ((status = print(*ptr, width, false, nchars))) goto frac_len_err;
  }

  if (!n->rdx) goto frac_len_err;

  if ((status = bc_num_init(&frac_len, n->len - n->rdx))) goto frac_len_err;

  bc_num_one(&frac_len);

  for (radix = true; frac_len.len <= n->rdx; radix = false) {
    if ((status = bc_num_mul(&fracp, base, &fracp, n->rdx))) goto err;
    if ((status = bc_num_ulong(&fracp, &dig))) goto err;
    if ((status = bc_num_ulong2num(&intp, dig))) goto err;
    if ((status = bc_num_sub(&fracp, &intp, &fracp, 0))) goto err;
    if ((status = print(dig, width, radix, nchars))) goto err;
    if ((status = bc_num_mul(&frac_len, base, &frac_len, 0))) goto err;
  }

err:

  n->neg = neg;

  bc_num_free(&frac_len);

frac_len_err:

  bc_num_free(&digit);

digit_err:

  bc_num_free(&fracp);

frac_err:

  bc_num_free(&intp);

int_err:

  bc_vec_free(&stack);

  return status;
}

BcStatus bc_num_init(BcNum *n, size_t request) {

  assert(n);

  memset(n, 0, sizeof(BcNum));

  request = request >= BC_NUM_DEF_SIZE ? request : BC_NUM_DEF_SIZE;

  if (!(n->num = malloc(request))) return BC_STATUS_MALLOC_FAIL;

  n->cap = request;

  return BC_STATUS_SUCCESS;
}

BcStatus bc_num_expand(BcNum *n, size_t request) {

  BcDigit *temp;

  assert(n && request);

  if (request <= n->cap) return BC_STATUS_SUCCESS;

  if (!(temp = realloc(n->num, request))) return BC_STATUS_MALLOC_FAIL;

  memset(temp + n->cap, 0, sizeof(char) * (request - n->cap));

  n->num = temp;
  n->cap = request;

  return BC_STATUS_SUCCESS;
}

void bc_num_free(void *num) {

  BcNum *n = (BcNum*) num;

  if (n) {
    if (n->num) free(n->num);
    memset(n, 0, sizeof(BcNum));
  }
}

BcStatus bc_num_copy(BcNum *d, BcNum *s) {

  BcStatus status;

  assert(d && s);

  if (d == s) return BC_STATUS_SUCCESS;

  if ((status = bc_num_expand(d, s->cap))) return status;

  d->len = s->len;
  d->neg = s->neg;
  d->rdx = s->rdx;

  memcpy(d->num, s->num, sizeof(char) * d->len);
  memset(d->num + d->len, 0, sizeof(char) * (d->cap - d->len));

  return BC_STATUS_SUCCESS;
}

BcStatus bc_num_parse(BcNum *n, const char *val, BcNum *base, size_t base_t) {

  BcStatus status;

  assert(n && val && base && base_t >= BC_NUM_MIN_BASE &&
         base_t <= BC_NUM_MAX_INPUT_BASE);

  if (!bc_num_strValid(val, base_t)) return BC_STATUS_MATH_BAD_STRING;

  if (base_t == 10) status = bc_num_parseDecimal(n, val);
  else status = bc_num_parseBase(n, val, base);

  return status;
}

BcStatus bc_num_print(BcNum *n, BcNum *base, size_t base_t,
                      bool newline, size_t *nchars)
{
  BcStatus status;

  assert(n && base && nchars && base_t >= BC_NUM_MIN_BASE &&
         base_t <= BC_BASE_MAX_DEF);

  if (*nchars  >= BC_NUM_PRINT_WIDTH) {
    if (fputc('\\', stdout) == EOF) return BC_STATUS_IO_ERR;
    if (fputc('\n', stdout) == EOF) return BC_STATUS_IO_ERR;
    *nchars = 0;
  }

  if (!n->len) {
    if (fputc('0', stdout) == EOF) return BC_STATUS_IO_ERR;
    ++(*nchars);
    status = BC_STATUS_SUCCESS;
  }
  else if (base_t == 10) status = bc_num_printDecimal(n, nchars);
  else status = bc_num_printBase(n, base, base_t, nchars);

  if (status) return status;

  if (newline) {
    if (fputc('\n', stdout) == EOF) return BC_STATUS_IO_ERR;
    *nchars = 0;
  }

  return status;
}

BcStatus bc_num_ulong(BcNum *n, unsigned long *result) {

  size_t i;
  unsigned long prev, pow;

  assert(n && result);

  if (n->neg) return BC_STATUS_MATH_NEGATIVE;

  *result = 0;
  pow = 1;

  for (i = n->rdx; i < n->len; ++i) {

    prev = *result;
    *result += n->num[i] * pow;
    pow *= 10;

    if (*result < prev) return BC_STATUS_MATH_OVERFLOW;
  }

  return BC_STATUS_SUCCESS;
}

BcStatus bc_num_ulong2num(BcNum *n, unsigned long val) {

  BcStatus status;
  size_t len, i;
  BcDigit *ptr;

  assert(n);

  bc_num_zero(n);

  if (!val) {
    memset(n->num, 0, sizeof(char) * n->cap);
    return BC_STATUS_SUCCESS;
  }

  len = (size_t) ceil(log10(((double) ULONG_MAX) + 1.0f));

  if ((status = bc_num_expand(n, len))) return status;

  for (ptr = n->num, i = 0; val; ++i, ++n->len) {
    ptr[i] = (char) (val % 10);
    val /= 10;
  }

  return BC_STATUS_SUCCESS;
}

BcStatus bc_num_add(BcNum *a, BcNum *b, BcNum *result, size_t scale) {
  (void) scale;
  BcNumBinaryFunc op = (!!a->neg == !!b->neg) ? bc_num_alg_a : bc_num_alg_s;
  return bc_num_binary(a, b, result, false, op, a->len + b->len + 1);
}

BcStatus bc_num_sub(BcNum *a, BcNum *b, BcNum *result, size_t scale) {
  (void) scale;
  BcNumBinaryFunc op = (!!a->neg == !!b->neg) ? bc_num_alg_s : bc_num_alg_a;
  return bc_num_binary(a, b, result, true, op, a->len + b->len + 1);
}

BcStatus bc_num_mul(BcNum *a, BcNum *b, BcNum *result, size_t scale) {
  return bc_num_binary(a, b, result, scale, bc_num_alg_m,
                       a->len + b->len + scale + 1);
}

BcStatus bc_num_div(BcNum *a, BcNum *b, BcNum *result, size_t scale) {
  return bc_num_binary(a, b, result, scale, bc_num_alg_d,
                       a->len + b->len + scale + 1);
}

BcStatus bc_num_mod(BcNum *a, BcNum *b, BcNum *result, size_t scale) {
  return bc_num_binary(a, b, result, scale, bc_num_alg_mod,
                       a->len + b->len + scale + 1);
}

BcStatus bc_num_pow(BcNum *a, BcNum *b, BcNum *result, size_t scale) {
  return bc_num_binary(a, b, result, scale, bc_num_alg_p,
                       (a->len + 1) * (b->len + 1));
}

BcStatus bc_num_sqrt(BcNum *a, BcNum *result, size_t scale) {
  return bc_num_unary(a, result, scale, bc_num_sqrt_newton,
                      a->rdx + (a->len - a->rdx) * 2 + 1);
}

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
