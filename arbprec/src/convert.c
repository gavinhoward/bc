#include <arbprec/arbprec.h>

void convert_radix(ARBT *array, size_t len, int value, int ibase, int obase)
{
	int carry = value;
	int tmp = 0;
	size_t i = 0;
	
	for (i = len; i > 0; i--)
	{
		tmp = (array[i-1] * ibase) + carry;
		array[i-1] = tmp % obase;
		carry = tmp / obase;
	}
}

ARBT *distconvs(ARBT *array, size_t len, int ibase, int obase)
{
	ARBT *p;
	size_t i = 0;
	p = arb_calloc(len, sizeof(ARBT));
	for (; i < len; i++)
		convert_radix(p, len, array[i], ibase, obase);
	return p;
}

fxdpnt *convert(fxdpnt *a, fxdpnt *b, int ibase, int obase)
{
	arb_copy(b, a);
	b->number = distconvs(a->number, a->len, ibase, obase);
	return b;
}

