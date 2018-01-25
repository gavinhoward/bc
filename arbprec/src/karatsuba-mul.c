/**
 * karatsuba-mul.c - Karatsuba Multiplication
 *
 * Author: Bao Hexing <HexingB@qq.com>
 * Created:  9 January 2018
 *
 * Copyright © 2018, Bao Hexing. All Rights Reserved.
 */

#include <arbprec/arbprec.h>

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
static fxdpnt *arb_karatsuba_mul_core(fxdpnt *x, fxdpnt *y, fxdpnt *z, int base)
{
  if (y->len == 1) {
    arb_expand(z, x->len + 1);
    arb_mul_core(x->number, x->len, y->number, y->len, z->number, base);
    z->lp = z->len = x->len + 1;
    return z;
  } else if (x->len == 1) {
    arb_expand(z, y->len + 1);
    arb_mul_core(y->number, y->len, x->number, x->len, z->number, base);
    z->lp = z->len = y->len + 1;
    return z;
  }

  size_t m = MIN(x->len, y->len) / 2;

  fxdpnt *x1 = arb_expand(NULL, x->len - m); x1 = arb_expand(x1, x->len - m);
  fxdpnt *y1 = arb_expand(NULL, y->len - m); y1 = arb_expand(y1, y->len - m);
  fxdpnt *x0 = arb_expand(NULL, m); x0 = arb_expand(x0, m);
  fxdpnt *y0 = arb_expand(NULL, m); y0 = arb_expand(y0, m);

  memcpy(x1->number, x->number, (x->len - m) * sizeof (ARBT));
  memcpy(y1->number, y->number, (y->len - m) * sizeof (ARBT));
  memcpy(x0->number, x->number + x->len - m, m * sizeof (ARBT));
  memcpy(y0->number, y->number + y->len - m, m * sizeof (ARBT));

  x1->lp = x1->len = x->len - m; x1->rp = 0;
  y1->lp = y1->len = y->len - m; y1->rp = 0;
  x0->lp = x0->len = m; x0->rp = 0;
  y0->lp = y0->len = m; y0->rp = 0;

  fxdpnt *z1 = arb_expand(NULL, 1);
  fxdpnt *z2 = arb_expand(NULL, 1);
  fxdpnt *z3 = arb_expand(NULL, 1);
  fxdpnt *z4 = arb_expand(NULL, 1);
  fxdpnt *z5 = arb_expand(NULL, 1);

  z1 = arb_karatsuba_mul_core(x1, y1, z1, base);
  z2 = arb_add(x1, x0, z2, base);
  z3 = arb_add(y1, y0, z3, base);
  z4 = arb_karatsuba_mul_core(x0, y0, z4, base);
  z5 = arb_karatsuba_mul_core(z2, z3, z5, base);

  fxdpnt *z6 = arb_expand(NULL, 1);
  fxdpnt *z7 = arb_expand(NULL, 1);

  z6 = arb_sub(z5, z1, z6, base);
  z7 = arb_sub(z6, z4, z7, base);
  z7 = arb_expand(z7, z7->len + m);
  z7->lp += m;
  z7->len += m;

  z1 = arb_expand(z1, z1->len + 2 * m);
  z1->lp += 2 * m;
  z1->len += 2 * m;

  fxdpnt *z8 = arb_expand(NULL, 1);
  z8 = arb_add(z1, z7, z8, base);
  z = arb_add(z8, z4, z, base);

  arb_free(x0);
  arb_free(y0);
  arb_free(x1);
  arb_free(y1);
  arb_free(z1);
  arb_free(z2);
  arb_free(z3);
  arb_free(z4);
  arb_free(z5);
  arb_free(z6);
  arb_free(z7);
  arb_free(z8);

  return z;
}

fxdpnt *arb_karatsuba_mul(fxdpnt *x, fxdpnt *y, fxdpnt *z, int base)
{
  arb_setsign(x, y, z);
  z = arb_karatsuba_mul_core(x, y, z, base);
  z->rp = x->rp + y->rp;
  z->lp = z->len - z->rp;
  return z;
}

