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

#include <bc.h>
#include <num.h>
#include <vector.h>

static const char bc_num_hex_digits[] = "0123456789ABCDEF";

static int bc_num_compareArrays(signed char *array1, signed char *array2,
                                size_t len)
{
  size_t i;
  char c;

  if (array1[len]) return 1;

  for (i = len - 1; i < len; --i) {
    c = array1[i] - array2[i];
    if (c) return c;
  }

  return 0;
}

static BcStatus bc_num_trunc(BcNum *n, size_t places) {

  char *ptr;

  if (places > n->rdx) return BC_STATUS_MATH_INVALID_TRUNCATE;

  if (places == 0) return BC_STATUS_SUCCESS;

  ptr = n->num + places;

  n->len -= places;
  n->rdx -= places;

  memmove(n->num, ptr, n->len * sizeof(char));

  memset(n->num + n->len, 0, sizeof(char) * (n->cap - n->len));

  return BC_STATUS_SUCCESS;
}

static BcStatus bc_num_extend(BcNum *n, size_t places) {

  BcStatus status;
  char *ptr;
  size_t len;

  if (places == 0) return BC_STATUS_SUCCESS;

  len = n->len + places;

  if (n->cap < len) {

    status = bc_num_expand(n, len);

    if (status) return status;
  }

  ptr = n->num + places;

  memmove(ptr, n->num, sizeof(char) * n->len);

  memset(n->num, 0, sizeof(char) * places);

  n->len += places;
  n->rdx += places;

  return BC_STATUS_SUCCESS;
}

static BcStatus bc_num_alg_a(BcNum *a, BcNum *b, BcNum *c, size_t scale) {

  char *ptr;
  char *ptr_a;
  char *ptr_b;
  char *ptr_c;
  size_t i;
  size_t max;
  size_t min;
  size_t diff;
  size_t a_whole;
  size_t b_whole;
  char carry;

  (void) scale;

  c->neg = a->neg;

  memset(c->num, 0, c->cap * sizeof(char));

  c->rdx = BC_MAX(a->rdx, b->rdx);

  min = BC_MIN(a->rdx, b->rdx);

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

  ptr_c = c->num;

  for (i = 0; i < diff; ++i) {
    ptr_c[i] = ptr[i];
    ++c->len;
  }

  ptr_c += diff;

  carry = 0;

  for (i = 0; i < min; ++i) {

    ptr_c[i] = ptr_a[i] + ptr_b[i] + carry;
    ++c->len;

    if (ptr_c[i] >= 10) {
      carry = ptr_c[i] / 10;
      ptr_c[i] %= 10;
    }
    else carry = 0;
  }

  c->rdx = c->len;

  a_whole = a->len - a->rdx;
  b_whole = b->len - b->rdx;

  min = BC_MIN(a_whole, b_whole);

  ptr_a = a->num + a->rdx;
  ptr_b = b->num + b->rdx;
  ptr_c = c->num + c->rdx;

  for (i = 0; i < min; ++i) {

    ptr_c[i] = ptr_a[i] + ptr_b[i] + carry;
    ++c->len;

    if (ptr_c[i] >= 10) {
      carry = ptr_c[i] / 10;
      ptr_c[i] %= 10;
    }
    else carry = 0;
  }

  if (a_whole > b_whole) {
    max = a_whole;
    ptr = ptr_a;
  }
  else {
    max = b_whole;
    ptr = ptr_b;
  }

  for (; i < max; ++i) {

    ptr_c[i] += ptr[i] + carry;
    ++c->len;

    if (ptr_c[i] >= 10) {
      carry = ptr_c[i] / 10;
      ptr_c[i] %= 10;
    }
    else carry = 0;
  }

  if (carry) {
    ++c->len;
    ptr_c[i] = carry;
  }

  return BC_STATUS_SUCCESS;
}

static BcStatus bc_num_alg_s(BcNum *a, BcNum *b, BcNum *c, size_t scale) {

  (void) scale;

  c->rdx = BC_MAX(a->rdx, b->rdx);

  return BC_STATUS_SUCCESS;
}

static BcStatus bc_num_alg_m(BcNum *a, BcNum *b, BcNum *c, size_t scale) {

  BcStatus status;
  char carry;
  size_t i;
  size_t j;
  size_t len;

  if (BC_NUM_ZERO(a) || BC_NUM_ZERO(b)) {
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

  memset(c->num, 0, sizeof(char) * c->cap);
  c->len = 0;

  carry = 0;
  len = 0;

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

  if (scale < c->rdx) status = bc_num_trunc(c, c->rdx - scale);
  else status = BC_STATUS_SUCCESS;

  // Remove leading zeros.
  while (c->len > c->rdx && !c->num[c->len - 1]) --c->len;

  return status;
}

static BcStatus bc_num_alg_d(BcNum *a, BcNum *b, BcNum *c, size_t scale) {

  BcStatus status;
  signed char *ptr;
  signed char *bptr;
  size_t len;
  size_t end;
  size_t i;
  BcNum copy;

  if (BC_NUM_ZERO(b)) return BC_STATUS_MATH_DIVIDE_BY_ZERO;
  else if (BC_NUM_ZERO(a)) {
    bc_num_zero(c);
    return BC_STATUS_SUCCESS;
  }
  else if (BC_NUM_ONE(b)) {
    status = bc_num_copy(c, a);
    if (b->neg) c->neg = !c->neg;
    status = bc_num_extend(c, scale);
    return status;
  }

  status = bc_num_init(&copy, a->len + b->rdx + scale + 1);

  if (status) return status;

  status = bc_num_copy(&copy, a);

  if (status) goto err;

  if (b->rdx > copy.rdx) {
    status = bc_num_extend(&copy, b->rdx - copy.rdx);
    if (status) goto err;
  }

  copy.rdx -= b->rdx;

  if (scale > copy.rdx) {
    status = bc_num_extend(&copy, scale - copy.rdx);
    if (status) goto err;
  }

  len = b->len;

  if (b->len > copy.len) {
    status = bc_num_extend(&copy, b->len + 2);
    if (status) goto err;
  }

  // We want an extra zero in front to make things simpler.
  copy.num[copy.len] = 0;
  ++copy.len;

  end = copy.len - len;

  status = bc_num_expand(c, copy.len);

  if (status) goto err;

  bc_num_zero(c);
  c->rdx = copy.rdx;
  c->len = copy.len;

  bptr = (signed char*) b->num;

  for (i = end - 1; i < end; --i) {

    size_t j;
    size_t k;
    int quotient;

    ptr = (signed char*) (copy.num + i);

    quotient = 0;

    while (bc_num_compareArrays(ptr, bptr, len) >= 0) {

      for (j = 0; j < len; ++j) {

        ptr[j] -= bptr[j];

        k = j;

        while (ptr[k] < 0) {

          ptr[k] += 10;
          ++k;

          if (k > len) return BC_STATUS_MATH_OVERFLOW;

          ptr[k] -= 1;
        }
      }

      ++quotient;
    }

    c->num[i] = quotient;
  }

  c->neg = !a->neg != !b->neg;

  // Remove leading zeros.
  while (c->len > c->rdx && !c->num[c->len - 1]) --c->len;

  if (c->rdx > scale) status = bc_num_trunc(c, c->rdx - scale);

err:

  bc_num_free(&copy);

  return status;
}

static BcStatus bc_num_alg_rem(BcNum *a, BcNum *b, BcNum *c) {

  BcStatus status;
  BcNum quotient;
  BcNum product;

  status = bc_num_init(&quotient, a->len + b->len);

  if (status) return status;

  status = bc_num_div(a, b, &quotient, 0);

  if (status) goto div_err;

  status = bc_num_trunc(&quotient, quotient.rdx);

  if (status) goto div_err;

  status = bc_num_init(&product, quotient.len + b->len);

  if (status) goto div_err;

  status = bc_num_mul(b, &quotient, &product, b->rdx);

  if (status) goto err;

  status = bc_num_sub(a, &product, c, BC_MAX(a->rdx, b->rdx));

err:

  bc_num_free(&product);

div_err:

  bc_num_free(&quotient);

  return status;
}

static BcStatus bc_num_alg_mod(BcNum *a, BcNum *b, BcNum *c, size_t scale) {

  BcStatus status;
  BcNum c1;
  BcNum c2;
  size_t len;

  if (scale == 0) return bc_num_alg_rem(a, b, c);

  len = a->len + b->len + scale;

  status = bc_num_init(&c1, len);

  if (status) return status;

  status = bc_num_init(&c2, len);

  if (status) goto c2_err;

  status = bc_num_div(a, b, &c1, scale);

  if (status) goto err;

  c->rdx = BC_MAX(scale + b->rdx, a->rdx);

  status = bc_num_mul(&c1, b, &c2, scale);

  if (status) goto err;

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
  BcNum one;
  long pow;
  unsigned long upow;
  size_t i;
  size_t powrdx;
  size_t resrdx;
  bool neg;
  bool zero;

  if (b->rdx) return BC_STATUS_MATH_NON_INTEGER;

  status = bc_num_long(b, &pow);

  if (status) return status;

  if (pow == 0) {
    bc_num_one(c);
    return BC_STATUS_SUCCESS;
  }
  else if (BC_NUM_ZERO(a)) {
    bc_num_zero(c);
    return BC_STATUS_SUCCESS;
  }
  else if (pow == 1) {
    return bc_num_copy(c, a);
  }
  else if (pow == -1) {

    status = bc_num_init(&one, BC_NUM_DEF_SIZE);

    if (status) return status;

    bc_num_one(&one);

    return bc_num_div(&one, a, c, scale);
  }
  else if (pow < 0) {
    neg = true;
    upow = -pow;
  }
  else {
    neg = false;
    upow = pow;
    scale = BC_MIN(a->rdx * upow, BC_MAX(scale, a->rdx));
  }

  status = bc_num_init(&copy, a->len);

  if (status) return status;

  status = bc_num_copy(&copy, a);

  if (status) goto err;

  powrdx = a->rdx;

  while (!(upow & 1)) {

    powrdx <<= 1;

    status = bc_num_mul(&copy, &copy, &copy, powrdx);

    if (status) goto err;

    upow >>= 1;
  }

  status = bc_num_copy(c, &copy);

  if (status) goto err;

  resrdx = powrdx;
  upow >>= 1;

  while (upow != 0) {

    powrdx <<= 1;

    status = bc_num_mul(&copy, &copy, &copy, powrdx);

    if (status) goto err;

    if (upow & 1) {
      resrdx += powrdx;
      bc_num_mul(c, &copy, c, resrdx);
    }

    upow >>= 1;
  }

  if (neg) {

    status = bc_num_init(&one, BC_NUM_DEF_SIZE);

    if (status) goto err;

    bc_num_one(&one);

    status = bc_num_div(&one, c, c, scale);

    if (status) {
      bc_num_free(&one);
      goto err;
    }
  }

  if (c->rdx > scale) {
    status = bc_num_trunc(c, c->rdx - scale);
    if (status) goto err;
  }

  for (zero = true, i = 0; zero && i < c->len; ++i) zero = c->num[i] == 0;

  if (zero) bc_num_zero(c);

err:

  bc_num_free(&copy);

  return status;
}

static BcStatus bc_num_sqrt_newton(BcNum *a, BcNum *b, size_t scale) {

  BcStatus status;
  BcNum num1;
  BcNum num2;
  BcNum two;
  BcNum *x0;
  BcNum *x1;
  BcNum *temp;
  size_t pow;
  BcNum f;
  BcNum fprime;
  size_t len;
  size_t i;

  scale = BC_MAX(scale, a->rdx) + 1;

  len = a->len;

  status = bc_num_init(&num1, len);

  if (status) return status;

  status = bc_num_init(&num2, num1.len);

  if (status) goto num2_err;

  status = bc_num_init(&two, BC_NUM_DEF_SIZE);

  if (status) goto two_err;

  bc_num_one(&two);
  two.num[0] = 2;

  len += scale;

  status = bc_num_init(&f, len);

  if (status) goto f_err;

  status = bc_num_init(&fprime, len + scale);

  if (status) goto fprime_err;

  x0 = &num1;
  x1 = &num2;

  bc_num_one(x0);

  pow = a->len - a->rdx;

  if (pow & 1) {
    x0->num[0] = 2;
    pow -= 1;
  }
  else {
    x0->num[0] = 6;
    pow -= 2;
  }

  status = bc_num_extend(x0, pow);

  if (status) goto err;

  x0->rdx = 0;

  len = (x0->len + scale) * 2;

  for (i = 1; i < len; i *= 2) {

    status = bc_num_mul(x0, x0, &f, scale);

    if (status) goto err;

    status = bc_num_sub(&f, a, &f, scale);

    if (status) goto err;

    status = bc_num_mul(x0, &two, &fprime, scale);

    if (status) goto err;

    status = bc_num_div(&f, &fprime, &f, scale);

    if (status) goto err;

    status = bc_num_sub(x0, &f, x1, scale);

    if (status) goto err;

    temp = x0;
    x0 = x1;
    x1 = temp;
  }

  status = bc_num_copy(b, x1);

  if (status) goto err;

  if (b->rdx > scale) status = bc_num_trunc(b, b->rdx - scale);

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

  if (!a || !b || !c || !op) return BC_STATUS_INVALID_PARAM;

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

  if (status) return status;

  status = op(ptr_a, ptr_b, c, scale);

  if (c == a || c == b) bc_num_free(&num2);

  return status;
}

static BcStatus bc_num_unary(BcNum *a, BcNum *b, size_t scale,
                               BcNumUnaryFunc op, size_t req)
{
  BcStatus status;
  BcNum a2;
  BcNum *ptr_a;

  if (!a || !b || !op) return BC_STATUS_INVALID_PARAM;

  if (b == a) {

    memcpy(&a2, b, sizeof(BcNum));
    ptr_a = &a2;

    status = bc_num_init(b, req);
  }
  else {
    ptr_a = a;
    status = bc_num_expand(b, req);
  }

  if (status) return status;

  status = op(ptr_a, b, scale);

  if (b == a) bc_num_free(&a2);

  return status;
}

static bool bc_num_strValid(const char *val, size_t base) {

  size_t len;
  size_t i;
  char c;
  char b;
  bool radix;

  radix = false;

  len = strlen(val);

  if (!len) return true;

  if (base <= 10) {

    b = base + '0';

    for (i = 0; i < len; ++i) {

      c = val[i];

      if (c == '.') {

        if (radix) return false;

        radix = true;
        continue;
      }

      if (c < '0' || c >= b) return false;
    }
  }
  else {

    b = base - 9 + 'A';

    for (i = 0; i < len; ++i) {

      c = val[i];

      if (c == '.') {

        if (radix) return false;

        radix = true;
        continue;
      }

      if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= b))) return false;
    }
  }

  return true;
}

static BcStatus bc_num_parseDecimal(BcNum *n, const char *val) {

  BcStatus status;
  size_t len;
  size_t i;
  const char *ptr;
  size_t radix;
  size_t end;
  char *num;

  for (i = 0; val[i] == '0'; ++i);

  val += i;

  len = strlen(val);

  bc_num_zero(n);

  if (len) {

    bool zero;

    zero = true;

    for (i = 0; zero && i < len; ++i) {
      if (val[i] != '0' && val[i] != '.') zero = false;
    }

    if (zero) {
      memset(n->num, 0, sizeof(char) * n->cap);
      n->neg = false;
      return BC_STATUS_SUCCESS;
    }

    status = bc_num_expand(n, len);

    if (status) return status;

  }
  else {
    memset(n->num, 0, sizeof(char) * n->cap);
    n->neg = false;
    return BC_STATUS_SUCCESS;
  }

  ptr = strchr(val, '.');

  if (ptr) {
    radix = ptr - val;
    ++ptr;
    n->rdx = (val + len) - ptr;
  }
  else {
    radix = len;
    n->rdx = 0;
  }

  end = n->rdx - 1;

  for (i = 0; i < n->rdx; ++i) {
    n->num[i] = BC_NUM_FROM_CHAR(ptr[end - i]);
    n->len += 1;
  }

  if (i >= len) return BC_STATUS_SUCCESS;

  num = n->num + n->len;
  end = radix - 1;

  for (i = 0; i < radix; ++i) {
    num[i] = BC_NUM_FROM_CHAR(val[end - i]);
    ++n->len;
  }

  return BC_STATUS_SUCCESS;
}

static BcStatus bc_num_parseBase(BcNum *n, const char *val, BcNum *base) {

  BcStatus status;
  BcNum temp;
  BcNum mult;
  BcNum result;
  size_t i;
  size_t len;
  size_t digits;
  char c;
  bool zero;

  len = strlen(val);

  zero = true;

  for (i = 0; zero && i < len; ++i) {
    c = val[i];
    zero = (c == '.' || c == '0');
  }

  if (zero) {
    bc_num_zero(n);
    return BC_STATUS_SUCCESS;
  }

  status = bc_num_init(&temp, BC_NUM_DEF_SIZE);

  if (status) return status;

  status = bc_num_init(&mult, BC_NUM_DEF_SIZE);

  if (status) goto mult_err;

  bc_num_zero(n);

  for (i = 0; i < len && (c = val[i]) != '.'; ++i) {

    status = bc_num_mul(n, base, &mult, 0);

    if (status) goto int_err;

    status = bc_num_long2num(&temp, (long) c);

    if (status) goto int_err;

    status = bc_num_add(&mult, &temp, n, 0);

    if (status) goto int_err;
  }

  if (c == '\0') goto int_err;

  assert(c == '.');

  status = bc_num_init(&result, base->len);

  if (status) goto int_err;

  ++i;
  bc_num_zero(&result);
  bc_num_one(&mult);

  for (digits = 0; i < len; ++i, ++digits) {

    c = val[i];

    status = bc_num_mul(&result, base, &result, 0);

    if (status) goto err;

    status = bc_num_long2num(&temp, (long) c);

    if (status) goto err;

    status = bc_num_add(&result, &temp, &result, 0);

    if (status) goto err;

    status = bc_num_mul(&mult, base, &mult, 0);

    if (status) goto err;
  }

  status = bc_num_div(&result, &mult, &result, digits);

  if (status) goto err;

  status = bc_num_add(n, &result, n, 0);

err:

  bc_num_free(&result);

int_err:

  bc_num_free(&mult);

mult_err:

  bc_num_free(&temp);

  return status;
}

static BcStatus bc_num_printDecimal(BcNum *n, FILE *f) {

  size_t i;
  size_t chars;

  chars = 0;

  if (n->neg) {
    if (fputc('-', f) == EOF) return BC_STATUS_IO_ERR;
    ++chars;
  }

  for (i = n->len - 1; i >= n->rdx && i < n->len; --i) {

    if (fputc(BC_NUM_TO_CHAR(n->num[i]), f) == EOF) return BC_STATUS_IO_ERR;
    ++chars;

    if (chars == BC_NUM_PRINT_WIDTH) {
      if (fputc('\\', f) == EOF) return BC_STATUS_IO_ERR;
      if (fputc('\n', f) == EOF) return BC_STATUS_IO_ERR;
      chars = 0;
    }
  }

  if (n->rdx) {

    if (fputc('.', f) == EOF) return BC_STATUS_IO_ERR;
    ++chars;

    if (chars == BC_NUM_PRINT_WIDTH) {
      if (fputc('\\', f) == EOF) return BC_STATUS_IO_ERR;
      if (fputc('\n', f) == EOF) return BC_STATUS_IO_ERR;
      chars = 0;
    }

    for (; i < n->len; --i) {

      if (fputc(BC_NUM_TO_CHAR(n->num[i]), f) == EOF) return BC_STATUS_IO_ERR;
      ++chars;

      if (chars == BC_NUM_PRINT_WIDTH) {
        if (fputc('\\', f) == EOF) return BC_STATUS_IO_ERR;
        if (fputc('\n', f) == EOF) return BC_STATUS_IO_ERR;
        chars = 0;
      }
    }
  }

  if (chars) {
    if (fputc('\n', f) == EOF) return BC_STATUS_IO_ERR;
  }

  return BC_STATUS_SUCCESS;
}

static BcStatus bc_num_printDigits(unsigned long num, size_t width,
                                   size_t *nchars, FILE *f)
{
  if (*nchars + width + 1 >= BC_NUM_PRINT_WIDTH) {
    if (fputc('\\', f) == EOF) return BC_STATUS_IO_ERR;
    if (fputc('\n', f) == EOF) return BC_STATUS_IO_ERR;
    *nchars = 0;
  }
  else {
    if (fputc(' ', f) == EOF) return BC_STATUS_IO_ERR;
    ++(*nchars);
  }

  if (fprintf(f, "%0*lu", (unsigned int) width, num) < 0)
    return BC_STATUS_IO_ERR;

  *nchars = *nchars + width;

  return BC_STATUS_SUCCESS;
}

static BcStatus bc_num_printHex(unsigned long num, size_t width,
                                size_t *nchars, FILE *f)
{
  if (*nchars + width >= BC_NUM_PRINT_WIDTH) {
    if (fputc('\\', f) == EOF) return BC_STATUS_IO_ERR;
    if (fputc('\n', f) == EOF) return BC_STATUS_IO_ERR;
    *nchars = 0;
  }

  if (fputc(bc_num_hex_digits[num], f) == EOF) return BC_STATUS_IO_ERR;

  *nchars = *nchars + width;

  return BC_STATUS_SUCCESS;
}

static BcStatus bc_num_printBase(BcNum *n, BcNum *base, size_t base_t, FILE* f) {

  BcStatus status;
  BcVec stack;
  BcNum intp;
  BcNum fracp;
  BcNum digit;
  BcNum frac_len;
  size_t nchars;
  size_t width;
  BcNumDigitFunc print;
  size_t i;

  nchars = 0;

  if (n->neg) {
    if (fputc('-', f) == EOF) return BC_STATUS_IO_ERR;
    ++nchars;
  }

  if (base_t <= 16) {
    width = 1;
    print = bc_num_printHex;
  }
  else {
    width = (size_t) floor(log10((double) (base_t - 1)) + 1.0);
    print = bc_num_printDigits;
  }

  status = bc_vec_init(&stack, sizeof(unsigned long), NULL);

  if (status) return status;

  status = bc_num_init(&intp, n->len);

  if (status) goto int_err;

  status = bc_num_init(&fracp, n->rdx);

  if (status) goto frac_err;

  status = bc_num_init(&digit, width);

  if (status) goto digit_err;

  status = bc_num_copy(&intp, n);

  if (status) goto frac_len_err;

  status = bc_num_truncate(&intp);

  if (status) goto frac_len_err;

  status = bc_num_sub(n, &intp, &fracp, 0);

  if (status) goto frac_len_err;

  while (!BC_NUM_ZERO(&intp)) {

    unsigned long dig;

    status = bc_num_mod(&intp, base, &digit, 0);

    if (status) goto frac_len_err;

    status = bc_num_ulong(&digit, &dig);

    if (status) goto frac_len_err;

    status = bc_vec_push(&stack, &dig);

    if (status) goto frac_len_err;

    status = bc_num_div(&intp, base, &intp, 0);

    if (status) goto frac_len_err;
  }

  for (i = 0; i < stack.len; ++i) {

    unsigned long *ptr;

    ptr = bc_vec_item_rev(&stack, i);

    status = print(*ptr, width, &nchars, f);

    if (status) goto frac_len_err;
  }

  if (!n->rdx) goto frac_len_err;

  if (fputc('.', f) == EOF) {
    status = BC_STATUS_IO_ERR;
    goto frac_len_err;
  }

  status = bc_num_init(&frac_len, n->len - n->rdx);

  if (status) goto frac_len_err;

  bc_num_one(&frac_len);

  while (frac_len.len <= n->len) {

    unsigned long fdigit;

    status = bc_num_mul(&fracp, base, &fracp, n->rdx);

    if (status) goto err;

    status = bc_num_ulong(&fracp, &fdigit);

    if (status) goto err;

    status = bc_num_ulong2num(&intp, fdigit);

    if (status) goto err;

    status = bc_num_sub(&fracp, &intp, &fracp, 0);

    if (status) goto err;

    status = print(fdigit, width, &nchars, f);

    if (status) goto err;

    status = bc_num_mul(&frac_len, base, &frac_len, 0);

    if (status) goto err;
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

  return status;
}

BcStatus bc_num_init(BcNum *n, size_t request) {

  if (!n) return BC_STATUS_INVALID_PARAM;

  memset(n, 0, sizeof(BcNum));

  request = request >= BC_NUM_DEF_SIZE ? request : BC_NUM_DEF_SIZE;

  n->num = malloc(request);

  if (!n->num) return BC_STATUS_MALLOC_FAIL;

  n->cap = request;

  return BC_STATUS_SUCCESS;
}

BcStatus bc_num_expand(BcNum *n, size_t request) {

  if (!n || !request) return BC_STATUS_INVALID_PARAM;

  if (request > n->cap) {

    char *temp;

    temp = realloc(n->num, request);

    if (!temp) return BC_STATUS_MALLOC_FAIL;

    memset(temp + n->cap, 0, sizeof(char) * (request - n->cap));

    n->num = temp;
    n->cap = request;
  }

  return BC_STATUS_SUCCESS;
}

void bc_num_free(void *num) {

  BcNum *n;

  if (!num) return;

  n = (BcNum*) num;

  if (n->num) free(n->num);

  memset(n, 0, sizeof(BcNum));
}

BcStatus bc_num_copy(void *dest, void *src) {

  BcStatus status;

  BcNum *d;
  BcNum *s;

  if (!dest || !src) return BC_STATUS_INVALID_PARAM;

  if (dest == src) return BC_STATUS_SUCCESS;

  d = (BcNum*) dest;
  s = (BcNum*) src;

  status = bc_num_expand(d, s->cap);

  if (status) return status;

  d->len = s->len;
  d->neg = s->neg;
  d->rdx = s->rdx;

  memcpy(d->num, s->num, sizeof(char) * d->len);
  memset(d->num + d->len, 0, sizeof(char) * (d->cap - d->len));

  return BC_STATUS_SUCCESS;
}

BcStatus bc_num_parse(BcNum *n, const char *val, BcNum *base, size_t base_t) {

  BcStatus status;

  if (!n || !val) return BC_STATUS_INVALID_PARAM;

  if (base_t < BC_NUM_MIN_BASE || base_t > BC_NUM_MAX_INPUT_BASE)
    return BC_STATUS_EXEC_INVALID_IBASE;

  if (!bc_num_strValid(val, base_t)) return BC_STATUS_MATH_INVALID_STRING;

  if (base_t == 10) status = bc_num_parseDecimal(n, val);
  else status = bc_num_parseBase(n, val, base);

  return status;
}

BcStatus bc_num_print(BcNum *n, BcNum *base, size_t base_t) {
  return bc_num_fprint(n, base, base_t, stdout);
}

BcStatus bc_num_fprint(BcNum *n, BcNum *base, size_t base_t, FILE *f) {

  BcStatus status;

  if (!n || !f) return BC_STATUS_INVALID_PARAM;

  if (base_t < BC_NUM_MIN_BASE || base_t > BC_NUM_MAX_OUTPUT_BASE)
    return BC_STATUS_EXEC_INVALID_OBASE;

  if (BC_NUM_ZERO(n)) {
    if (fputc('0', f) == EOF) return BC_STATUS_IO_ERR;
    if (fputc('\n', f) == EOF) return BC_STATUS_IO_ERR;
    status = BC_STATUS_SUCCESS;
  }
  else if (base_t == 10) status = bc_num_printDecimal(n, f);
  else status = bc_num_printBase(n, base, base_t, f);

  return status;
}

BcStatus bc_num_long(BcNum *n, long *result) {

  size_t i;
  unsigned long temp;
  unsigned long prev;
  unsigned long pow;

  if (!n || !result) return BC_STATUS_INVALID_PARAM;

  temp = 0;
  pow = 1;

  for (i = n->rdx; i < n->len; ++i) {

    prev = temp;

    temp += n->num[i] * pow;

    pow *= 10;

    if (temp < prev) return BC_STATUS_MATH_OVERFLOW;
  }

  if (n->neg) temp = -temp;

  *result = temp;

  return BC_STATUS_SUCCESS;
}

BcStatus bc_num_ulong(BcNum *n, unsigned long *result) {

  size_t i;
  unsigned long prev;
  unsigned long pow;

  if (!n || !result) return BC_STATUS_INVALID_PARAM;

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

BcStatus bc_num_long2num(BcNum *n, long val) {

  BcStatus status;
  size_t len;
  size_t i;
  char *ptr;
  char carry;

  if (!n) return BC_STATUS_INVALID_PARAM;

  bc_num_zero(n);

  if (!val) {
    memset(n->num, 0, sizeof(char) * n->cap);
    return BC_STATUS_SUCCESS;
  }

  carry = 0;

  if (val < 0) {

    if (val == LONG_MIN) {
      carry = 1;
      val += 1;
    }

    val = -val;
    n->neg = true;
  }

  len = (size_t) ceil(log10(((double) ULONG_MAX) + 1.0f));

  status = bc_num_expand(n, len);

  if (status) return status;

  ptr = n->num;

  for (i = 0; val; ++i) {
    ptr[i] = (char) (val % 10);
    val /= 10;
  }

  if (carry) ptr[i - 1] += carry;

  return BC_STATUS_SUCCESS;
}

BcStatus bc_num_ulong2num(BcNum *n, unsigned long val) {

  BcStatus status;
  size_t len;
  size_t i;
  char *ptr;

  if (!n) return BC_STATUS_INVALID_PARAM;

  bc_num_zero(n);

  if (!val) {
    memset(n->num, 0, sizeof(char) * n->cap);
    return BC_STATUS_SUCCESS;
  }

  len = (size_t) ceil(log10(((double) ULONG_MAX) + 1.0f));

  status = bc_num_expand(n, len);

  if (status) return status;

  ptr = n->num;

  for (i = 0; val; ++i) {
    ptr[i] = (char) (val % 10);
    val /= 10;
  }

  return BC_STATUS_SUCCESS;
}

BcStatus bc_num_truncate(BcNum *n) {
  if (!n) return BC_STATUS_INVALID_PARAM;
  return bc_num_trunc(n, n->rdx);
}

BcStatus bc_num_add(BcNum *a, BcNum *b, BcNum *result, size_t scale) {

  BcNumBinaryFunc op;

  if ((a->neg && b->neg) || (!a->neg && !b->neg)) op = bc_num_alg_a;
  else op = bc_num_alg_s;

  return bc_num_binary(a, b, result, scale, op, a->len + b->len + 1);
}

BcStatus bc_num_sub(BcNum *a, BcNum *b, BcNum *result, size_t scale) {

  BcNumBinaryFunc op;

  if (a->neg && b->neg) op = bc_num_alg_s;
  else if (a->neg || b->neg) op = bc_num_alg_a;
  else op = bc_num_alg_s;

  return bc_num_binary(a, a, result, scale, op, a->len + b->len + 1);
}

BcStatus bc_num_mul(BcNum *a, BcNum *b, BcNum *result, size_t scale) {
  return bc_num_binary(a, b, result, scale, bc_num_alg_m,
                       a->len + b->len + scale);
}

BcStatus bc_num_div(BcNum *a, BcNum *b, BcNum *result, size_t scale) {
  return bc_num_binary(a, b, result, scale, bc_num_alg_d,
                       a->len + b->len + scale);
}

BcStatus bc_num_mod(BcNum *a, BcNum *b, BcNum *result, size_t scale) {
  return bc_num_binary(a, b, result, scale, bc_num_alg_mod,
                       a->len + b->len + scale);
}

BcStatus bc_num_pow(BcNum *a, BcNum *b, BcNum *result, size_t scale) {
  return bc_num_binary(a, b, result, scale, bc_num_alg_p,
                       (a->len + 1) * (b->len + 1));
}

BcStatus bc_num_sqrt(BcNum *a, BcNum *result, size_t scale) {
  return bc_num_unary(a, result, scale, bc_num_sqrt_newton,
                      a->rdx + (a->len - a->rdx) * 2);
}

int bc_num_compare(BcNum *a, BcNum *b) {

  BcNum *a2;
  BcNum *b2;
  size_t i;
  size_t min;
  char *max_num;
  char *min_num;
  bool a_max;
  bool neg;
  size_t a_int;
  size_t b_int;
  char *ptr_a;
  char *ptr_b;
  size_t diff;

  a2 = (BcNum*) a;
  b2 = (BcNum*) b;

  if (!a2) {

    if (b2== NULL) return 0;
    else return b2->neg ? 1 : -1;
  }
  else if (!b2) return a2->neg ? -1 : 1;

  neg = false;

  if (a2->neg) {

    if (b2->neg) neg = true;
    else return -1;
  }
  else if (b2->neg) return 1;

  a_int = a2->len - a2->rdx;
  b_int = b2->len - b2->rdx;

  if (a_int > b_int) return 1;
  else if (b_int > a_int) return -1;

  ptr_a = a2->num + a2->rdx;
  ptr_b = b2->num + b2->rdx;

  for (i = a_int - 1; i < a_int; --i) {

    char c;

    c = ptr_a[i] - ptr_b[i];

    if (c) return neg ? -c : c;
  }

  a_max = a2->rdx > b2->rdx;

  if (a_max) {

    min = b2->rdx;

    diff = a2->rdx - b2->rdx;

    max_num = a2->num + diff;
    min_num = b2->num;

    for (i = min - 1; i < min; --i) {

      char c;

      c = max_num[i] - min_num[i];

      if (c) return neg ? -c : c;
    }

    max_num -= diff;

    for (i = diff - 1; i < diff; --i) {
      if (max_num[i]) return neg ? -1 : 1;
    }
  }
  else {

    min = a2->rdx;

    diff = b2->rdx - a2->rdx;

    max_num = b2->num + diff;
    min_num = a2->num;

    for (i = min - 1; i < min; --i) {

      char c;

      c = max_num[i] - min_num[i];

      if (c) return neg ? c : -c;
    }

    max_num -= diff;

    for (i = diff - 1; i < diff; --i) {
      if (max_num[i]) return neg ? 1 : -1;
    }
  }

  return 0;
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
