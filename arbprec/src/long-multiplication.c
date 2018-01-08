#include <arbprec/arbprec.h>

size_t arb_mul_core(ARBT *a, size_t alen, ARBT *b, size_t blen, ARBT *c, int base)
{
	ARBT prod = 0, carry = 0;
	size_t i = 0;
	size_t j = 0;
        size_t k = 0;
	size_t last = 0;
	size_t ret = blen;
	c[k] = 0;
	c[alen + blen -1] = 0;
	for (i = alen; i > 0 ; i--){
		last = k;
		for (j = blen, k = i + j, carry = 0; j > 0 ; j--, k--){
			prod = (a[i-1]) * (b[j-1]) + (c[k-1]) + carry;
			carry = prod / base;
			c[k-1] = (prod % base);
		}
		if (k != last) {
			++ret;	
			c[k-1] = 0;
		}
		c[k-1] += carry;
	}
	return ret;
}

fxdpnt *arb_mul(fxdpnt *a, fxdpnt *b, fxdpnt *c, int base)
{
	arb_setsign(a, b, c);
	c = arb_expand(c, a->len + b->len);
	arb_mul_core(a->number, a->len, b->number, b->len, c->number, base);
	c->lp = a->lp + b->lp;
	c->rp = a->rp + b->rp;
	c->rp = MAX(a->rp, b->rp);
	c->rp = MIN(a->rp + b->rp, maxi(20, a->rp, b->rp));
	c->len = c->rp + c->lp;
	return c;
}

