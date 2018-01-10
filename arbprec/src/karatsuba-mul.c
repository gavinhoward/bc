/**
 * karatsuba-mul.c - Karatsuba Multiplication
 *
 * Author: Bao Hexing <HexingB@qq.com>
 * Created:  9 January 2018
 *
 * Copyright © 2018, Bao Hexing. All Rights Reserved.
 */

#include <arbprec/arbprec.h>

static fxdpnt *arb_karatsuba_mul_inter(fxdpnt *x, fxdpnt *y, fxdpnt *z, int base);

static fxdpnt *arb_tmp_fxdpnt(ARBT *digits, size_t len, fxdpnt *number)
{
  number->number = digits;
  number->len = len;
  number->sign = '+';
  number->lp = len;
  number->rp = 0;
  number->len = len;
  number->allocated = 0;
  number->chunk = 4;
  return number;
}

/* use x[len], y, to calculate z[len + 1], or z[len] */
static size_t arb_karatsuba_mul_single(ARBT *x, size_t len, int y, fxdpnt *z, int base)
{
  long prod;
  long quot = 0;
  long remainder = 0;
  size_t i;

  z = arb_expand(z, len + 1);
  for (i = 0; i < len; ++i) {
    prod = x[len - i - 1] * y + quot;
    quot = prod / base;
    remainder = (ARBT)(prod - quot * base);
    z->number[len - i] = (ARBT)remainder;
  }
  z->number[0] = (ARBT)quot;
  z->lp = z->len = len + 1;
  z->rp = 0;

  return z->len;
}

/* use x[len], y[len], to calculate z[2 * len], or z[2 * len - 1] */
/*
  x = x1 * 10 ^ m + x0
  y = y1 * 10 ^ m + y0
  x * y = (x1 * 10 ^ m + x0) * (y1 * 10 ^ m + y0)
  = x1 * y1 * 10 ^ 2m + x1 * y0 * 10 ^ m + x0 * y1 * 10 ^ m + x0 * y0
  = x1 * y1 * 10 ^ 2m + [(x1 + x0) * (y1 + y0) - x1 * y1 - x0 * y0] * 10 ^ m + x0 * y0
  = z1 * 10 ^ 2m + (z2 * z3 - z1 - z4) * 10 ^ m + z4
  let:
  z1 = x1 * y1
  z2 = x1 + x0
  z3 = y1 + y0
  z4 = x0 * y0
  z5 = z2 * z3
*/
static fxdpnt *arb_karatsuba_mul_core(ARBT *x, size_t x_len, ARBT *y, size_t y_len, fxdpnt *z, int base)
{
  if (y_len == 1) {
    arb_karatsuba_mul_single(x, x_len, y[0], z, base);
    return z;
  } else if (x_len == 1) {
    arb_karatsuba_mul_single(y, y_len, x[0], z, base);
    return z;
  }

  size_t m = MIN(x_len, y_len) / 2;
  fxdpnt x1, y1, x0, y0, z1, z2, z3, z4, z5;
  arb_tmp_fxdpnt(x, x_len - m, &x1);
  arb_tmp_fxdpnt(y, y_len - m, &y1);
  arb_tmp_fxdpnt(x + x_len - m, m, &x0);
  arb_tmp_fxdpnt(y + y_len - m, m, &y0);
  arb_tmp_fxdpnt(NULL, 0, &z1);
  arb_tmp_fxdpnt(NULL, 0, &z2);
  arb_tmp_fxdpnt(NULL, 0, &z3);
  arb_tmp_fxdpnt(NULL, 0, &z4);
  arb_tmp_fxdpnt(NULL, 0, &z5);

  arb_karatsuba_mul_inter(&x1, &y1, &z1, base);
  arb_add(&x1, &x0, &z2, base);
  arb_add(&y1, &y0, &z3, base);
  arb_karatsuba_mul_inter(&x0, &y0, &z4, base);
  arb_karatsuba_mul_inter(&z2, &z3, &z5, base);

  fxdpnt z6, z7;
  arb_tmp_fxdpnt(NULL, 0, &z6);
  arb_tmp_fxdpnt(NULL, 0, &z7);

  arb_sub(&z5, &z1, &z6, base);
  arb_sub(&z6, &z4, &z7, base);
  arb_expand(&z7, z7.len + m);
  z7.lp += m;
  z7.len += m;

  arb_expand(&z1, z1.len + 2 * m);
  z1.lp += 2 * m;
  z1.len += 2 * m;

  fxdpnt z8;
  arb_tmp_fxdpnt(NULL, 0, &z8);
  arb_add(&z1, &z7, &z8, base);
  arb_add(&z8, &z4, z, base);

  free(z1.number);
  free(z2.number);
  free(z3.number);
  free(z4.number);
  free(z5.number);
  free(z6.number);
  free(z7.number);
  free(z8.number);

  return z;
}

static fxdpnt *arb_karatsuba_mul_inter(fxdpnt *x, fxdpnt *y, fxdpnt *z, int base)
{
  return arb_karatsuba_mul_core(x->number, x->len, y->number, y->len, z, base);
}

fxdpnt *arb_karatsuba_mul(fxdpnt *x, fxdpnt *y, fxdpnt *z, int base)
{
  arb_setsign(x, y, z);
  z = arb_karatsuba_mul_core(x->number, x->len, y->number, y->len, z, base);
  z->rp = x->rp + y->rp;
  z->lp = z->len - z->rp;
  return z;
}
