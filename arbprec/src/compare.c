#include <arbprec/arbprec.h>

int arb_compare(fxdpnt *a, fxdpnt *b, int base)
{
	size_t i = 0;
	int sign = a->sign;
	if (a->sign == '-' && b->sign == '+')
		return -1;
	if (a->sign == '+' && b->sign == '-')
		return 1;
	fxdpnt *c = NULL;
	c = arb_expand(NULL, a->len + b->len); //FIXME initialize at runtime
	c = arb_sub(a, b, c, base);
	if (c->sign != sign){
		arb_free(c);
		return -1;
	}
	for (;i< c->len;++i){
		if (c->number[i] != 0){
			arb_free(c);
			return 1;
		}
	}
	arb_free(c);
	return 0;
}

