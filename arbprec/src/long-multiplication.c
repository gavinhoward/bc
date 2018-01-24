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

fxdpnt *arb_mul(fxdpnt *a, fxdpnt *b, fxdpnt *c, int base, size_t scale)
{
	arb_setsign(a, b, c);
	c = arb_expand(c, a->len + b->len);
	//arb_mul_core(a->number, a->len, b->number, b->len, c->number, base);
	fxdpnt *a2 = arb_expand(NULL, a->len);
	fxdpnt *b2 = arb_expand(NULL, b->len);
	arb_copy(a2, a);
	arb_copy(b2, b);
	arb_expand(a2, MAX(scale, a2->len));
	arb_expand(b2, MAX(scale, b2->len));
	arb_mul_core(a2->number, a2->len, b2->number, b2->len, c->number, base);
	c->lp = a2->lp + b2->lp;
	c->rp = a2->rp + b2->rp;
	c->rp = MAX(a2->rp, b2->rp);
	c->rp = MIN(a2->rp + b2->rp, maxi(scale, a2->rp, b2->rp));
	c->len = c->rp + c->lp;
	c = remove_leading_zeros(c);
	arb_free(a2);
	arb_free(b2);
	return c;
}

