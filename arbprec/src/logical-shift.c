#include <arbprec/arbprec.h>

fxdpnt *arb_leftshift(fxdpnt *a, size_t n, int faux)
{
	(void) faux;
	size_t i = 0;
	size_t j = n;
	size_t k = 0;
	size_t l = a->len -1;

	for (i = n-1;i < a->len-1; ++i, ++j, ++k)
		a->number[k] = a->number[j];

	while (l > a->len -1 - n)
		a->number[l--] = 0;

	return a;
}

void rightshift_core(ARBT *a, size_t len, size_t n)
{ 
	memmove(a + n, a, (len - n) * sizeof(ARBT));
        while (n-- > 0)
                 a[n] = 0;
}

fxdpnt *arb_rightshift(fxdpnt *a, size_t n, int faux)
{
	/* logical right shift, turns base 10 "990" into "099" */
	if (faux == 0) {
		rightshift_core(a->number, a->len, n);
	}
	else {
		while (n--&& a->len > 0)
			a->len--;
	}
	return a;
}
