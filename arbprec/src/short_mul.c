#include <arbprec/arbprec.h>

ARBT arb_short_mul(ARBT *a, size_t i, int b, int base)
{ 
        ARBT carry = 0;
        for (; i > 0 ; i--) {
                a[i-1] *= b;
                a[i-1] += carry;
                carry = a[i-1] / base;
                a[i-1] %= base;
        }
	return carry;
}

