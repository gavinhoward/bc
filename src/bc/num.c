/*
 * Copyright 2018 Contributors
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.

 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.

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
 *******************************************************************************
 *
 * A special license exemption is granted to the Toybox project and to the
 * Android Open Source Project to use this source under the following BSD
 * 0-Clause License:
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
 *******************************************************************************
 *
 * Code for the number type.
 *
 */

#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include <bc/bc.h>
#include <bc/num.h>

static BcStatus bc_num_construct(BcNum* num, size_t request);

static BcStatus bc_num_expand(BcNum* num, size_t request);

static void bc_num_destruct(BcNum* num);

static BcStatus bc_num_unary(BcNum* a, BcNum* b, size_t scale,
                               BcUnaryFunc op, size_t req);

static BcStatus bc_num_binary(BcNum* a, BcNum* b, BcNum* c, size_t scale,
                              BcBinaryFunc op, size_t req);

static BcStatus bc_num_alg_a(BcNum* a, BcNum* b, BcNum* c, size_t scale);
static BcStatus bc_num_alg_s(BcNum* a, BcNum* b, BcNum* c, size_t scale);
static BcStatus bc_num_alg_m(BcNum* a, BcNum* b, BcNum* c, size_t scale);
static BcStatus bc_num_alg_d(BcNum* a, BcNum* b, BcNum* c, size_t scale);
static BcStatus bc_num_alg_mod(BcNum* a, BcNum* b, BcNum* c, size_t scale);
static BcStatus bc_num_alg_p(BcNum* a, BcNum* b, BcNum* c, size_t scale);

static BcStatus bc_num_sqrt_newton(BcNum* a, BcNum* b, size_t scale);

static bool bc_num_strValid(const char* val, size_t base);

static BcStatus bc_num_parseDecimal(BcNum *n, const char* val,
                                     size_t scale);
static BcStatus bc_num_parseLowBase(BcNum* n, const char* val,
                                     size_t base, size_t scale);
static BcStatus bc_num_parseHighBase(BcNum* n, const char* val,
                                      size_t base, size_t scale);

static BcStatus bc_num_printDecimal(BcNum* n, FILE* f);
static BcStatus bc_num_printLowBase(BcNum* n, size_t base, FILE* f);
static BcStatus bc_num_printHighBase(BcNum* n, size_t base, FILE* f);
static BcStatus bc_num_printHighestBase(BcNum* n, size_t base, FILE* f);

BcStatus bc_num_parse(BcNum* num, const char* val,
                       size_t base, size_t scale)
{
  BcStatus status;

  if (!num || !val) return BC_STATUS_INVALID_PARAM;

  if (base < BC_NUM_MIN_BASE || base > BC_NUM_MAX_INPUT_BASE)
    return BC_STATUS_EXEC_INVALID_IBASE;

  if (!bc_num_strValid(val, base)) return BC_STATUS_MATH_INVALID_STRING;

  if (base == 10) {
    status = bc_num_parseDecimal(num, val, scale);
  }
  else if (base < 10) {
    status = bc_num_parseLowBase(num, val, base, scale);
  }
  else {
    status = bc_num_parseHighBase(num, val, base, scale);
  }

  return status;
}

BcStatus bc_num_copy(BcNum* dest, BcNum* src) {

  BcStatus status;
  BcNum* d;
  BcNum* s;

  d = (BcNum*) dest;
  s = (BcNum*) src;

  if (!d || !s || d == s) return BC_STATUS_INVALID_PARAM;

  status = bc_num_expand(d, s->len + s->unused);

  if (status) return status;

  d->len = s->len;
  d->neg = s->neg;
  d->radix = s->radix;

  memcpy(d->num, s->num, sizeof(char) * d->len);

  return BC_STATUS_SUCCESS;
}

BcStatus bc_num_print(BcNum* num, size_t base) {
  return bc_num_fprint(num, base, stdout);
}

BcStatus bc_num_fprint(BcNum* num, size_t base, FILE* f) {

  BcStatus status;

  if (!num || !f) return BC_STATUS_INVALID_PARAM;

  if (base < BC_NUM_MIN_BASE || base > BC_NUM_MAX_OUTPUT_BASE)
    return BC_STATUS_EXEC_INVALID_OBASE;

  if (base == 10) {
    status = bc_num_printDecimal(num, f);
  }
  else if (base < 10) {
    status = bc_num_printLowBase(num, base, f);
  }
  else if (base <= 16) {
    status = bc_num_printHighBase(num, base, f);
  }
  else {
    status = bc_num_printHighestBase(num, base, f);
  }

  return status;
}

BcStatus bc_num_add(BcNum* a, BcNum* b, BcNum* result, size_t scale) {
  return bc_num_binary(a, b, result, scale, bc_num_alg_a, a->len + b->len);
}

BcStatus bc_num_sub(BcNum* a, BcNum* b, BcNum* result, size_t scale) {

  BcNum* a_num;
  BcNum* b_num;

  a_num = (BcNum*) a;
  b_num = (BcNum*) b;

  return bc_num_binary(a_num, b_num, result, scale, bc_num_alg_s,
                       a_num->len + b_num->len);
}

BcStatus bc_num_mul(BcNum* a, BcNum* b, BcNum* result, size_t scale) {

  BcNum* a_num;
  BcNum* b_num;

  a_num = (BcNum*) a;
  b_num = (BcNum*) b;

  return bc_num_binary(a_num, b_num, result, scale, bc_num_alg_m,
                       a_num->len + b_num->len);
}

BcStatus bc_num_div(BcNum* a, BcNum* b, BcNum* result, size_t scale) {

  BcNum* a_num;
  BcNum* b_num;

  a_num = (BcNum*) a;
  b_num = (BcNum*) b;

  return bc_num_binary(a_num, b_num, result, scale, bc_num_alg_d,
                       a_num->len + b_num->len);
}

BcStatus bc_num_mod(BcNum* a, BcNum* b, BcNum* result, size_t scale) {

  BcNum* a_num;
  BcNum* b_num;

  a_num = (BcNum*) a;
  b_num = (BcNum*) b;

  return bc_num_binary(a_num, b_num, result, scale, bc_num_alg_mod,
                       a_num->len + b_num->len);
}

BcStatus bc_num_pow(BcNum* a, BcNum* b, BcNum* result, size_t scale) {

  BcNum* a_num;
  BcNum* b_num;

  a_num = (BcNum*) a;
  b_num = (BcNum*) b;

  return bc_num_binary(a_num, b_num, result, scale, bc_num_alg_p,
                       a_num->len + b_num->len);
}

BcStatus bc_num_sqrt(BcNum* a, BcNum* result, size_t scale) {

  BcNum* a_num;

  a_num = (BcNum*) a;

  return bc_num_unary(a_num, result, scale, bc_num_sqrt_newton, a_num->len * 2);
}

bool bc_num_isInteger(BcNum* num) {

  BcNum* n;

  n = (BcNum*) num;

  if (!n) return false;

  return n->radix == n->len;
}

int bc_num_compare(BcNum* a, BcNum* b) {

  BcNum* a2;
  BcNum* b2;
  size_t i;
  size_t max;
  size_t min;
  char* max_num;
  char* min_num;
  bool a_max;

  a2 = (BcNum*) a;
  b2 = (BcNum*) b;

  if (!a2) {

    if (b2== NULL) {
      return 0;
    }
    else {
      return b2->neg ? 1 : -1;
    }
  }
  else if (!b2) {
    return a2->neg ? -1 : 1;
  }

  if (a2->radix > b2->radix) {
    return 1;
  }
  else if (b2->radix > a2->radix) {
    return -1;
  }

  for (i = 0; i < a2->radix; ++i) {

    char c;

    c = a2->num[i] - b2->num[i];

    if (c) return c;
  }

  a_max = a2->len > b2->len;

  if (a_max) {

    max = a2->len - a2->radix;
    min = b2->len - b2->radix;

    max_num = a2->num + a2->radix;
    min_num = b2->num + b2->radix;
  }
  else {

    max = b2->len - b2->radix;
    min = a2->len - a2->radix;

    max_num = b2->num + b2->radix;
    min_num = a2->num + a2->radix;
  }

  for (i = 0; i < min; ++i) {

    char c;

    c = max_num[i] - min_num[i];

    if (c) return a_max ? c : -c;
  }

  for (; i < max; ++i) {
    if (max_num[i]) return a_max ? 1 : -1;
  }

  return 0;
}

static BcStatus bc_num_construct(BcNum* num, size_t request) {

  if (!num || !request) return BC_STATUS_INVALID_PARAM;

  memset(num, 0, sizeof(BcNum));

  num->num = malloc(request);

  if (!num->num) {
    return BC_STATUS_MALLOC_FAIL;
  }

  num->unused = request;

  return BC_STATUS_SUCCESS;
}

static BcStatus bc_num_expand(BcNum* num, size_t request) {

  if (!num || !request) return BC_STATUS_INVALID_PARAM;

  if (request > num->len + num->unused) {

    size_t extra;
    char* temp;

    extra = request - (num->len + num->unused);

    temp = realloc(num->num, request);

    if (!temp) {
      return BC_STATUS_MALLOC_FAIL;
    }

    num->num = temp;

    num->unused += extra;
  }

  return BC_STATUS_SUCCESS;
}

static void bc_num_destruct(BcNum* num) {

  if (!num) return;

  if (num->num) free(num->num);

  memset(num, 0, sizeof(BcNum));
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

    status = bc_num_construct(b, req);
  }
  else {
    ptr_a = a;
    status = bc_num_expand(b, req);
  }

  if (status) return status;

  status = op(ptr_a, b, scale);

  if (b == a) {
    bc_num_destruct(&a2);
  }

  return status;
}

static BcStatus bc_num_binary(BcNum* a, BcNum* b, BcNum* c,
                               size_t scale, BcBinaryFunc op, size_t req)
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
  else {
    ptr_a = a;
  }

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
  else {
    ptr_b = b;
  }

  if (init) {
    status = bc_num_construct(c, req);
  }
  else {
    status = bc_num_expand(c, req);
  }

  if (status) return status;

  status = op(ptr_a, ptr_b, c, scale);

  if (c == a) {
    bc_num_destruct(&a2);
  }
  else if (c == b) {
    bc_num_destruct(&b2);
  }

  return status;
}

static BcStatus bc_num_alg_a(BcNum* a, BcNum* b, BcNum* c, size_t scale) {

}

static BcStatus bc_num_alg_s(BcNum* a, BcNum* b, BcNum* c, size_t scale) {

}

static BcStatus bc_num_alg_m(BcNum* a, BcNum* b, BcNum* c, size_t scale) {

}

static BcStatus bc_num_alg_d(BcNum* a, BcNum* b, BcNum* c, size_t scale) {

}

static BcStatus bc_num_alg_mod(BcNum* a, BcNum* b, BcNum* c, size_t scale) {

}

static BcStatus bc_num_alg_p(BcNum* a, BcNum* b, BcNum* c, size_t scale) {

}

static BcStatus bc_num_sqrt_newton(BcNum* a, BcNum* b, size_t scale) {

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

  c = val[0];

  i = c == '-' || c == '+' ? 1 : 0;

  if (base <= 10) {

    b = base + '0';

    for (; i < len; ++i) {

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

    for (; i < len; ++i) {

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

static BcStatus bc_num_parseDecimal(BcNum* n, const char* val,
                                     size_t scale) {

  BcStatus status;
  size_t len;
  char c;
  size_t i;
  const char* ptr;
  size_t radix;
  size_t inv_pow;
  char* num;

  len = strlen(val);

  n->len = 0;
  n->neg = false;
  n->radix = 0;

  if (len) {

    status = bc_num_expand(n, len);

    if (status) return status;
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

  c = val[0];

  n->neg = c == '-';

  i = c == '-' || c == '+' ? 1 : 0;

  while (val[i] == '0') ++i;

  ptr = val + i;
  radix -= i;
  len -= i;

  for (i = 0; i < radix; ++i) {
    n->num[i] = BC_NUM_FROM_CHAR(ptr[i]);
    n->len += 1;
    n->unused -= 1;
  }

  ptr += radix + 1;

  n->radix = radix;

  if (i >= len) return BC_STATUS_SUCCESS;

  inv_pow = 0;

  while (inv_pow < scale && ptr[inv_pow] == '0') ++inv_pow;

  if (inv_pow >= scale || ptr[inv_pow] == '\0') {

    if (!n->len) n->neg = false;

    n->radix = n->len;

    return BC_STATUS_SUCCESS;
  }

  num = n->num + n->len;

  for (i = 0; i < inv_pow; ++i) {
    num[i] = 0;
    ++n->len;
    --n->unused;
  }

  c = ptr[i];

  while (i < scale && c != '\0') {

    num[i] = BC_NUM_FROM_CHAR(c);

    ++n->len;
    --n->unused;

    ++i;
    c = ptr[i];
  }

  return BC_STATUS_SUCCESS;
}

static BcStatus bc_num_parseLowBase(BcNum* n, const char* val,
                                     size_t base, size_t scale)
{
  const char* ptr;
  char* nptr;
  size_t len;
  size_t radix;
  size_t digits;
  char c;
  char carry;
  size_t pow;

  len = strlen(val);

  ptr = strchr(val, '.');

  if (ptr) {
    --ptr;
  }
  else {
    ptr = val + len - 1;
  }

  radix = ptr - val;

  nptr = n->num + radix;
  ++radix;

  c = 0;
  carry = 0;
  digits = 0;
  pow = 1;

  while (ptr >= val) {

    c += BC_NUM_FROM_CHAR(*ptr) * pow;
    --ptr;
    pow *= base;

    if (pow > 10) {

      --nptr;

      while (c >= 10) {
        *nptr += 1;
        c -= 10;
      }

      *(nptr + 1) += c;
      ++digits;

      carry = 1;
      c = 0;
      pow = 1;
    }
  }

  if (c) {
    *nptr += c;
    --nptr;
    ++digits;
  }

  if (nptr >= n->num) {
    memmove(n->num, nptr + 1, digits * sizeof(char));
  }

  // TODO: After radix.

  return BC_STATUS_SUCCESS;
}
static BcStatus bc_num_parseHighBase(BcNum* n, const char* val,
                                      size_t base, size_t scale)
{

}

static BcStatus bc_num_printDecimal(BcNum* n, FILE* f) {

  size_t i;

  if (n->len) {

    if (n->neg) fputc('-', f);

    for (i = 0; i < n->radix; ++i) fputc(BC_NUM_TO_CHAR(n->num[i]), f);

    if (i < n->len) {

      fputc('.', f);

      for (; i < n->len; ++i) fputc(BC_NUM_TO_CHAR(n->num[i]), f);
    }
  }
  else {
    fputc('0', f);
  }

  fputc('\n', f);

  return BC_STATUS_SUCCESS;
}

static BcStatus bc_num_printLowBase(BcNum* n, size_t base, FILE* f) {

  size_t size;
  char* buf;
  size_t i;

  size = BC_MAX(n->radix, n->len - n->radix) * ((10 * 2) / base);

  buf = malloc(size);

  if (!buf) return BC_STATUS_MALLOC_FAIL;

  i = 0;

  fputc('\n', f);

  free(buf);

  return BC_STATUS_SUCCESS;
}

static BcStatus bc_num_printHighBase(BcNum* n, size_t base, FILE* f) {

}

static BcStatus bc_num_printHighestBase(BcNum* n, size_t base, FILE* f) {

}
