#include <arbprec/arbprec.h>
#include <stdint.h>
/* log of base table 0 - 50 to simplify access */
double logtable[] = {       0.0000000000000,
 0.0000000000000, 0.3010299956640, 0.4771212547197,
 0.6020599913280, 0.6989700043360, 0.7781512503836,
 0.8450980400143, 0.9030899869919, 0.9542425094393,
 1.0000000000000, 1.0413926851582, 1.0791812460476,
 1.1139433523068, 1.1461280356782, 1.1760912590557,
 1.2041199826559, 1.2304489213783, 1.2552725051033,
 1.2787536009528, 1.3010299956640, 1.3222192947339,
 1.3424226808222, 1.3617278360176, 1.3802112417116,
 1.3979400086720, 1.4149733479708, 1.4313637641590,
 1.4471580313422, 1.4623979978990, 1.4771212547197,
 1.4913616938343, 1.5051499783199, 1.5185139398779,
 1.5314789170423, 1.5440680443503, 1.5563025007673,
 1.5682017240670, 1.5797835966168, 1.5910646070265,
 1.6020599913280, 1.6127838567197, 1.6232492903979,
 1.6334684555796, 1.6434526764862, 1.6532125137753,
 1.6627578316816, 1.6720978579357, 1.6812412373756,
 1.6901960800285};


fxdpnt *convert(fxdpnt *a, fxdpnt *b, int ibase, int obase)
{
	arb_copy(b, a);
	ARBT *p;
	ARBT carry = 0;
	ARBT prod = 0;
	size_t i = 0;
	size_t j = 0;
	size_t k = 0;
	// FIXME: handle bases greater than 50, perhaps by linking in libm and
	// calculating ln() on the fly
	if (ibase > obase && obase < 50)
		k = (size_t) (a->len / logtable[obase]);
	else 
		k = a->len;
	p = arb_calloc(k, sizeof(ARBT));
	ARBT *array = arb_calloc(k, sizeof(ARBT));
	memcpy(array + (k - a->len), a->number, a->len * sizeof(ARBT));
	memset(array, 0, (k - a->len) * sizeof(ARBT));

	for (; i < k; ++i) { 
		carry = array[i];
		prod = 0;
		for (j = k; j > 0; j--) {
			prod = (p[j-1] * ibase) + carry;
			p[j-1] = prod % obase;
			carry = prod / obase;
		}
	}
	b->number = p;
	b->len = k;
	b->lp = k;
	return b;
}

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

ARBT arb2hrdware(ARBT *a, size_t len, int base)
{
	ARBT ret = 0;
	size_t i = 0;
        for (; i < len; ++i) {
                ret = (base * ret) + (a[i]);
        }
	return ret;
}

fxdpnt *conv_frac(fxdpnt *a, fxdpnt *b, int ibase, int obase)
{
	//arb_mul_core(array, k, obh, obh->len, p, ibase)
	arb_copy(b, a);
	ARBT *p;
	ARBT *o;
	size_t i = 0;
	size_t j = 0;
	size_t k = 0;
	char obasehold[1000];
	size_t len = 0;
	size_t z = a->len;
	size_t size = 0;
	size_t off = 0;

	sprintf(obasehold, "%d", obase);
	printf("%s\n", obasehold);
	fxdpnt *obh = arb_str2fxdpnt(obasehold);

	//if (ibase > obase)
	//	k = (size_t) (a->len / logtable[obase]);
	//else 
		k = a->len;
	p = arb_calloc(k, sizeof(ARBT));
	o = arb_calloc(k, sizeof(ARBT));
	
	ARBT *array = arb_calloc(k, sizeof(ARBT));
	memcpy(array, a->number, a->len * sizeof(ARBT)); 

	if (ibase>=obase)
	{
		for (; i < k; ++i) {
			p[i] = arb_short_mul(array, a->len, obase, ibase);
			printf("p[i] == %d\n", p[i]);
		}
	}
	else {
		for (; i < k; ++i) {
			memset(o, 0, z * sizeof(ARBT)); 
			len = arb_mul_core(array, z, obh->number, obh->len, o, ibase);
			_print_core(stdout, array, z, a->len, 0); 
			_print_core(stdout, o, len, a->len, 0);
			size = len - z;
			p[i] = arb2hrdware(o, size , 10);
			printf("p[i] = %d\n", p[i]);
			_arb_copy_core(array, o + size , z);
                }
	}
	b->number = p;
	b->len = z;
	b->lp = 0; 
	
	return b;
}

