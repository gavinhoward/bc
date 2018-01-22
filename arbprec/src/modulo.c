#include <arbprec/arbprec.h>

fxdpnt *arb_mod(fxdpnt *a, fxdpnt *b, fxdpnt *c, int base, size_t scale)
{
	size_t newscale = MAX(a->len, b->len + scale);
	fxdpnt *tmp = arb_expand(NULL, newscale);
	fxdpnt *tmp2 = arb_expand(NULL, newscale);
	tmp = arb_alg_d(a, b, tmp, base, scale);
	tmp2 = arb_mul(tmp, b, tmp2, base, newscale);
	c = arb_sub(a, tmp2, c, base);
	free(tmp);
	free(tmp2);
	return c;
}
