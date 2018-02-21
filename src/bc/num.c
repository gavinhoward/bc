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

static void bc_num_zero(BcNum* n);

static BcStatus bc_num_unary(BcNum* a, BcNum* b, size_t scale,
                             BcUnaryFunc op, size_t req);

static BcStatus bc_num_binary(BcNum* a, BcNum* b, BcNum* c, size_t scale,
                              BcBinaryFunc op, size_t req);

static BcStatus bc_num_alg_a(BcNum* a, BcNum* b, BcNum* c, size_t scale);
static BcStatus bc_num_alg_s(BcNum* a, BcNum* b, BcNum* c, size_t scale);
static BcStatus bc_num_alg_m(BcNum* a, BcNum* b, BcNum* c, size_t scale);
static BcStatus bc_num_alg_d(BcNum* a, BcNum* b, BcNum* c, size_t scale);
static BcStatus bc_num_alg_mod(BcNum* a, BcNum* b, BcNum* c, size_t scale);
static BcStatus bc_num_alg_rem(BcNum* a, BcNum* b, BcNum* c);
static BcStatus bc_num_alg_p(BcNum* a, BcNum* b, BcNum* c, size_t scale);

static BcStatus bc_num_sqrt_newton(BcNum* a, BcNum* b, size_t scale);

static bool bc_num_strValid(const char* val, size_t base);

static BcStatus bc_num_parseDecimal(BcNum *n, const char* val, size_t scale);
static BcStatus bc_num_parseLowBase(BcNum* n, const char* val,
                                     size_t base, size_t scale);
static BcStatus bc_num_parseHighBase(BcNum* n, const char* val,
                                      size_t base, size_t scale);

static BcStatus bc_num_printDecimal(BcNum* n, FILE* f);
static BcStatus bc_num_printLowBase(BcNum* n, size_t base, FILE* f);
static BcStatus bc_num_printHighBase(BcNum* n, size_t base, FILE* f);
static BcStatus bc_num_printHighestBase(BcNum* n, size_t base, FILE* f);

static BcStatus bc_num_trunc(BcNum* n, size_t places);

BcStatus bc_num_init(BcNum* n, size_t request) {

  if (!n || !request) return BC_STATUS_INVALID_PARAM;

  memset(n, 0, sizeof(BcNum));

  n->num = malloc(request);

  if (!n->num) return BC_STATUS_MALLOC_FAIL;

  n->cap = request;

  return BC_STATUS_SUCCESS;
}

BcStatus bc_num_expand(BcNum* n, size_t request) {

  if (!n || !request) return BC_STATUS_INVALID_PARAM;

  if (request > n->cap) {

    char* temp;

    temp = realloc(n->num, request);

    if (!temp) return BC_STATUS_MALLOC_FAIL;

    memset(temp + n->cap, 0, sizeof(char) * (request - n->cap));

    n->num = temp;
    n->cap = request;
  }

  return BC_STATUS_SUCCESS;
}

void bc_num_free(BcNum* n) {

  if (!n) return;

  if (n->num) free(n->num);

  memset(n, 0, sizeof(BcNum));
}

BcStatus bc_num_copy(BcNum* d, BcNum* s) {

  BcStatus status;

  if (!d || !s || d == s) return BC_STATUS_INVALID_PARAM;

  status = bc_num_expand(d, s->cap);

  if (status) return status;

  d->len = s->len;
  d->neg = s->neg;
  d->rdx = s->rdx;

  memcpy(d->num, s->num, sizeof(char) * d->len);

  return BC_STATUS_SUCCESS;
}

BcStatus bc_num_parse(BcNum* n, const char* val, size_t base, size_t scale) {

  BcStatus status;

  if (!n || !val) return BC_STATUS_INVALID_PARAM;

  if (base < BC_NUM_MIN_BASE || base > BC_NUM_MAX_INPUT_BASE)
    return BC_STATUS_EXEC_INVALID_IBASE;

  if (!bc_num_strValid(val, base)) return BC_STATUS_MATH_INVALID_STRING;

  if (base == 10) status = bc_num_parseDecimal(n, val, scale);
  else if (base < 10) status = bc_num_parseLowBase(n, val, base, scale);
  else status = bc_num_parseHighBase(n, val, base, scale);

  return status;
}

BcStatus bc_num_print(BcNum* n, size_t base) {
  return bc_num_fprint(n, base, stdout);
}

BcStatus bc_num_fprint(BcNum* n, size_t base, FILE* f) {

  BcStatus status;

  if (!n || !f) return BC_STATUS_INVALID_PARAM;

  if (base < BC_NUM_MIN_BASE || base > BC_NUM_MAX_OUTPUT_BASE)
    return BC_STATUS_EXEC_INVALID_OBASE;

  if (base == 10) status = bc_num_printDecimal(n, f);
  else if (base < 10) status = bc_num_printLowBase(n, base, f);
  else if (base <= 16) status = bc_num_printHighBase(n, base, f);
  else status = bc_num_printHighestBase(n, base, f);

  return status;
}

BcStatus bc_num_long(BcNum* n, long* result) {

  size_t i;
  unsigned long temp;
  unsigned long prev;
  unsigned long pow;

  if (!n || !result) return BC_STATUS_INVALID_PARAM;

  if (n->rdx != n->len) return BC_STATUS_MATH_NON_INTEGER;

  temp = 0;
  pow = 1;

  for (i = 0; i < n->len; ++i) {

    prev = temp;

    temp += n->num[i] * pow;

    pow *= 10;

    if (temp < prev) return BC_STATUS_MATH_OVERFLOW;
  }

  *result = temp;

  return BC_STATUS_SUCCESS;
}

BcStatus bc_num_ulong(BcNum* n, unsigned long* result) {

  size_t i;
  unsigned long prev;
  unsigned long pow;

  if (!n || !result) return BC_STATUS_INVALID_PARAM;

  if (n->rdx != n->len) return BC_STATUS_MATH_NON_INTEGER;

  if (n->neg) return BC_STATUS_MATH_NEGATIVE;

  *result = 0;
  pow = 1;

  for (i = 0; i < n->len; ++i) {

    prev = *result;

    *result += n->num[i] * pow;

    pow *= 10;

    if (*result < prev) return BC_STATUS_MATH_OVERFLOW;
  }

  return BC_STATUS_SUCCESS;
}

BcStatus bc_num_long2num(BcNum* n, long val) {

  BcStatus status;
  size_t len;
  size_t i;
  char* ptr;
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

BcStatus bc_num_ulong2num(BcNum* n, unsigned long val) {

  BcStatus status;
  size_t len;
  size_t i;
  char* ptr;

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

BcStatus bc_num_truncate(BcNum* n) {
  if (!n) return BC_STATUS_INVALID_PARAM;
  return bc_num_trunc(n, n->rdx);
}

BcStatus bc_num_add(BcNum* a, BcNum* b, BcNum* result, size_t scale) {

  BcBinaryFunc op;

  if ((a->neg && b->neg) || (!a->neg && !b->neg)) op = bc_num_alg_a;
  else op = bc_num_alg_s;

  return bc_num_binary(a, b, result, scale, op, a->len + b->len + 1);
}

BcStatus bc_num_sub(BcNum* a, BcNum* b, BcNum* result, size_t scale) {

  BcBinaryFunc op;

  if (a->neg && b->neg) op = bc_num_alg_s;
  else if (a->neg || b->neg) op = bc_num_alg_a;
  else op = bc_num_alg_s;

  return bc_num_binary(a, a, result, scale, op, a->len + b->len + 1);
}

BcStatus bc_num_mul(BcNum* a, BcNum* b, BcNum* result, size_t scale) {
  return bc_num_binary(a, b, result, scale, bc_num_alg_m, a->len + b->len);
}

BcStatus bc_num_div(BcNum* a, BcNum* b, BcNum* result, size_t scale) {
  return bc_num_binary(a, b, result, scale, bc_num_alg_d, a->len + b->len);
}

BcStatus bc_num_mod(BcNum* a, BcNum* b, BcNum* result, size_t scale) {
  return bc_num_binary(a, b, result, scale, bc_num_alg_mod, a->len + b->len);
}

BcStatus bc_num_pow(BcNum* a, BcNum* b, BcNum* result, size_t scale) {
  return bc_num_binary(a, b, result, scale, bc_num_alg_p, a->len * b->len);
}

BcStatus bc_num_sqrt(BcNum* a, BcNum* result, size_t scale) {
  return bc_num_unary(a, result, scale, bc_num_sqrt_newton,
                      a->rdx + (a->len - a->rdx) * 2);
}

int bc_num_compare(BcNum* a, BcNum* b) {

  BcNum* a2;
  BcNum* b2;
  size_t i;
  size_t min;
  char* max_num;
  char* min_num;
  bool a_max;
  bool neg;
  size_t a_int;
  size_t b_int;
  char* ptr_a;
  char* ptr_b;
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

static void bc_num_zero(BcNum* n) {

  if (!n) return;

  n->neg = false;
  n->len = 0;
  n->rdx = 0;
}

static BcStatus bc_num_unary(BcNum* a, BcNum* b, size_t scale,
                               BcUnaryFunc op, size_t req)
{
  BcStatus status;
  BcNum a2;
  BcNum* ptr_a;

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

static BcStatus bc_num_binary(BcNum* a, BcNum* b, BcNum* c,  size_t scale,
                              BcBinaryFunc op, size_t req)
{
  BcStatus status;
  BcNum a2;
  BcNum b2;
  BcNum* ptr_a;
  BcNum* ptr_b;
  bool init;

  if (!a || !b || !c || !op) return BC_STATUS_INVALID_PARAM;

  init = false;

  if (c == a) {
    memcpy(&a2, c, sizeof(BcNum));
    ptr_a = &a2;
    init = true;
  }
  else ptr_a = a;

  if (c == b) {

    if (c == a) {
      ptr_b = ptr_a;
    }
    else {
      memcpy(&b2, c, sizeof(BcNum));
      ptr_b = &b2;
      init = true;
    }
  }
  else ptr_b = b;

  if (init) status = bc_num_init(c, req);
  else status = bc_num_expand(c, req);

  if (status) return status;

  status = op(ptr_a, ptr_b, c, scale);

  if (c == a) bc_num_free(&a2);
  else if (c == b) bc_num_free(&b2);

  return status;
}

static BcStatus bc_num_alg_a(BcNum* a, BcNum* b, BcNum* c, size_t scale) {

  char* ptr;
  char* ptr_a;
  char* ptr_b;
  char* ptr_c;
  size_t i;
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

    carry = 0;

    while (ptr_c[i] >= 10) {
      carry += 1;
      ptr_c[i] -= 10;
    }
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

    carry = 0;

    while (ptr_c[i] >= 10) {
      carry += 1;
      ptr_c[i] -= 10;
    }
  }

  if (a_whole > b_whole) {

    for (; i < a_whole; ++i) {

      ptr_c[i] = ptr_a[i] + carry;
      ++c->len;

      carry = 0;
    }
  }
  else {

    for (; i < b_whole; ++i) {

      ptr_c[i] = ptr_b[i] + carry;
      ++c->len;

      carry = 0;
    }
  }

  return BC_STATUS_SUCCESS;
}

static BcStatus bc_num_alg_s(BcNum* a, BcNum* b, BcNum* c, size_t scale) {

  (void) scale;

  c->rdx = BC_MAX(a->rdx, b->rdx);

  return BC_STATUS_SUCCESS;
}

static BcStatus bc_num_alg_m(BcNum* a, BcNum* b, BcNum* c, size_t scale) {

  scale = BC_MAX(scale, a->rdx);
  scale = BC_MAX(scale, b->rdx);
  c->rdx = BC_MIN(a->rdx + b->rdx, scale);

  return BC_STATUS_SUCCESS;
}

static BcStatus bc_num_alg_d(BcNum* a, BcNum* b, BcNum* c, size_t scale) {
  (void) a;
  (void) b;
  (void) c;
  (void) scale;
  return BC_STATUS_SUCCESS;
}

static BcStatus bc_num_alg_mod(BcNum* a, BcNum* b, BcNum* c, size_t scale) {

  if (scale == 0) return bc_num_alg_rem(a, b, c);

  // TODO: Compute a / b.

  c->rdx = BC_MAX(scale + b->rdx, a->rdx);

  // TODO: Compute a - (a / b) * b.

  return BC_STATUS_SUCCESS;
}

static BcStatus bc_num_alg_rem(BcNum* a, BcNum* b, BcNum* c) {
  (void) a;
  (void) b;
  (void) c;
  return BC_STATUS_SUCCESS;
}

static BcStatus bc_num_alg_p(BcNum* a, BcNum* b, BcNum* c, size_t scale) {
  (void) a;
  (void) b;
  (void) c;
  (void) scale;
  return BC_STATUS_SUCCESS;
}

static BcStatus bc_num_sqrt_newton(BcNum* a, BcNum* b, size_t scale) {
  (void) a;
  (void) b;
  (void) scale;
  return BC_STATUS_SUCCESS;
}

static bool bc_num_strValid(const char* val, size_t base) {

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

static BcStatus bc_num_parseDecimal(BcNum* n, const char* val, size_t scale) {

  BcStatus status;
  size_t len;
  char c;
  size_t i;
  const char* ptr;
  size_t radix;
  size_t inv_pow;
  char* num;

  len = strlen(val);

  bc_num_zero(n);

  if (len) {

    status = bc_num_expand(n, len);

    if (status) return status;

    memset(n->num, 0, sizeof(char) * len);
  }
  else {

    n->num = malloc(BC_NUM_DEF_SIZE);

    if (!n->num) return BC_STATUS_MALLOC_FAIL;

    memset(n->num, 0, sizeof(char) * BC_NUM_DEF_SIZE);

    return BC_STATUS_SUCCESS;
  }

  ptr = strchr(val, '.');

  if (ptr) radix = ptr - val;
  else radix = len;

  for (i = 0; val[i] == '0'; ++i);

  ptr = val + i;
  radix -= i;
  len -= i;

  scale = len - radix;

  for (i = 0; i < radix; ++i) {
    n->num[i] = BC_NUM_FROM_CHAR(ptr[i]);
    n->len += 1;
  }

  ptr += radix + 1;

  n->rdx = radix;

  if (i >= len) return BC_STATUS_SUCCESS;

  inv_pow = 0;

  while (inv_pow < scale && ptr[inv_pow] == '0') ++inv_pow;

  if (!radix && (inv_pow >= scale || ptr[inv_pow] == '\0')) {

    if (!n->len) n->neg = false;

    n->rdx = n->len;

    return BC_STATUS_SUCCESS;
  }

  num = n->num + n->len;

  for (i = 0; i < inv_pow; ++i) {
    num[i] = 0;
    ++n->len;
  }

  c = ptr[i];

  while (i < scale && c != '\0') {

    num[i] = BC_NUM_FROM_CHAR(c);

    ++n->len;

    ++i;
    c = ptr[i];
  }

  return BC_STATUS_SUCCESS;
}

static BcStatus bc_num_parseLowBase(BcNum* n, const char* val,
                                     size_t base, size_t scale)
{
  (void) n;
  (void) val;
  (void) base;
  (void) scale;
  return BC_STATUS_SUCCESS;
}
static BcStatus bc_num_parseHighBase(BcNum* n, const char* val,
                                      size_t base, size_t scale)
{
  (void) n;
  (void) val;
  (void) base;
  (void) scale;
  return BC_STATUS_SUCCESS;
}

static BcStatus bc_num_printDecimal(BcNum* n, FILE* f) {

  size_t i;
  size_t chars;

  chars = 0;

  if (n->len) {

    if (n->neg) {
      fputc('-', f);
      ++chars;
    }

    for (i = 0; i < n->rdx; ++i) {

      fputc(BC_NUM_TO_CHAR(n->num[i]), f);
      ++chars;

      if (chars == BC_NUM_PRINT_WIDTH) {
        fputc('\\', f);
        fputc('\n', f);
        chars = 0;
      }
    }

    if (i < n->len) {

      fputc('.', f);
      ++chars;

      if (chars == BC_NUM_PRINT_WIDTH) {
        fputc('\\', f);
        fputc('\n', f);
        chars = 0;
      }

      for (; i < n->len; ++i) {

        fputc(BC_NUM_TO_CHAR(n->num[i]), f);
        ++chars;

        if (chars == BC_NUM_PRINT_WIDTH) {
          fputc('\\', f);
          fputc('\n', f);
          chars = 0;
        }
      }
    }
  }
  else {
    fputc('0', f);
    ++chars;
  }

  if (chars) fputc('\n', f);

  return BC_STATUS_SUCCESS;
}

static BcStatus bc_num_printLowBase(BcNum* n, size_t base, FILE* f) {

  size_t size;
  char* buf;

  size = BC_MAX(n->rdx, n->len - n->rdx) * ((10 * 2) / base);

  buf = malloc(size);

  if (!buf) return BC_STATUS_MALLOC_FAIL;

  fputc('\n', f);

  free(buf);

  return BC_STATUS_SUCCESS;
}

static BcStatus bc_num_printHighBase(BcNum* n, size_t base, FILE* f) {
  (void) n;
  (void) base;
  (void) f;
  return BC_STATUS_SUCCESS;
}

static BcStatus bc_num_printHighestBase(BcNum* n, size_t base, FILE* f) {
  (void) n;
  (void) base;
  (void) f;
  return BC_STATUS_SUCCESS;
}

static BcStatus bc_num_trunc(BcNum* n, size_t places) {

  char* ptr;

  if (places > n->rdx) return BC_STATUS_MATH_INVALID_TRUNCATE;

  ptr = n->num + places;

  n->len -= places;
  n->rdx -= places;

  memmove(n->num, ptr, n->len * sizeof(char));

  memset(n->num + n->len, 0, sizeof(char) * (n->cap - n->len));

  return BC_STATUS_SUCCESS;
}
