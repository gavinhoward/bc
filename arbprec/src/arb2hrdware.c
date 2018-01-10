#include <arbprec/arbprec.h>

ARBT arb2hrdware(ARBT *a, size_t len, int base)
{
	/* TODO: calculate in the negative range like modern atoi */
	ARBT ret = 0;
	size_t i = 0;
        for (; i < len; ++i) {
                ret = (base * ret) + (a[i]);
        }
	return ret;
}

