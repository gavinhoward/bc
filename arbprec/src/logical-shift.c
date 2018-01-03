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

fxdpnt *arb_rightshift(fxdpnt *a, size_t n, int faux)
{
	/* logical right shift, turns base 10 "990" into "099" 
		else
	   "faux" logical shift turns "990" into "99"
	*/
	size_t i = 0;
	size_t j = a->len - n - 1;
	size_t k = a->len - 1;
	size_t l = 0;

	if (faux == 0)
	{
		for (i = n-1;i < a->len-1; ++i, --j, --k)
			a->number[k] = a->number[j];

		while (n-- > 0)
			a->number[l++] = 0;
	}
	else {
		while (n--&& a->len > 0)
			a->len--;
	}
	return a;
}
