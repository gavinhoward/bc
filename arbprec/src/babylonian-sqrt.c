#include <arbprec/arbprec.h>

fxdpnt *arb_babylonian_sqrt(fxdpnt *x, fxdpnt *c, int base, int scale)
{
	size_t i = 0;
	arb_init(c);
	arb_copy(c, x); 
	fxdpnt *two = arb_str2fxdpnt("2.0");
	fxdpnt *sum = arb_expand(NULL, c->len + x->len);
	fxdpnt *quo = arb_expand(NULL, c->len + x->len); 
	fxdpnt *last = NULL;
	last = arb_expand(last, c->len); 
	arb_copy(last, c);

	for(;i < 10000;++i)
	{
		quo = arb_alg_d(x, c, quo, base, scale);
		sum = arb_add(quo, c, sum, base);
		c = arb_alg_d(sum, two, c, base, scale);
		if (arb_compare(c, last, base) == 0)
			break;
		arb_copy(last, c);
		//arb_print(c);
	}
	if (i > 9998)
		fprintf(stderr, "sqrt had a problem\n");
	
	arb_free(sum);
	arb_free(quo);
	arb_free(two);
	arb_free(last);
	return c;
}
