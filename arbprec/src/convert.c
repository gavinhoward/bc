#include <arbprec/arbprec.h>
#include <stdint.h>
#include <math.h>

fxdpnt *convert(fxdpnt *a, fxdpnt *b, int ibase, int obase)
{
	arb_copy(b, a);
	ARBT *sv;
	long long carry = 0;
	long long prod = 0;
	size_t i = 0;
	size_t j = 0;
	size_t k = 0;
	
	if (ibase > obase)
		/* ln(x) / ln(base) == ln_base(x) */
		k = (size_t) ((2*a->len) / log10(obase)) ;
	else 
		k = a->len;
	sv = arb_calloc(k, sizeof(ARBT));
	ARBT *real = arb_calloc(k, sizeof(ARBT));
	memcpy(real + (k - a->len), a->number, a->len * sizeof(ARBT));
	memset(real, 0, (k - a->len) * sizeof(ARBT));

	for (i = 0; i < k; ++i) { 
		carry = real[i];
		prod = 0;
		for (j = k; j > 0; j--) {
			prod = (sv[j-1] * ibase) + carry;
			sv[j-1] = prod % obase;
			carry = prod / obase;
		}
	}
	if (carry)
		printf("we have a left over carry\n");
	b->number = sv;
	b->len = k;
	b->lp = k;
	return b;
}

fxdpnt *conv_frac(fxdpnt *a, fxdpnt *b, int ibase, int obase)
{
	arb_copy(b, a);
	ARBT *p;
	ARBT *o;
	size_t i = 0;
	size_t k = 0;
	size_t len = 0;
	size_t z = a->len;
	size_t size = 0;
	
	fxdpnt *obh = hrdware2arb(obase);
	
	k = a->len;
	p = arb_calloc(k * 2, sizeof(ARBT));
	o = arb_calloc(k * 2, sizeof(ARBT));
	
	ARBT *frac = arb_calloc(z * 2, sizeof(ARBT));
	memcpy(frac, a->number, a->len * sizeof(ARBT));
	for (i = 0; i < k; ++i) {
		memset(o, 0, z * sizeof(ARBT)); 
		len = arb_mul_core(frac, z, obh->number, obh->len, o, ibase);
		size = len - z;
		p[i] = arb2hrdware(o, size , 10); /* note: absorbs leading zeros */
		_arb_copy_core(frac, o + size , z);
        }

	b->number = p;
	b->len = z;
	b->lp = 0; 
	arb_free(obh);
	return b;
}


fxdpnt *convall(fxdpnt *a, fxdpnt *b, int ibase, int obase)
{
	arb_copy(b, a);
	ARBT *sv;
	long long carry = 0;
	long long prod = 0;
	size_t i = 0;
	size_t j = 0;
	size_t k = 0;
	size_t z = 0;
	size_t len = 0;
	size_t size = 0;
	
	ARBT *p;
	ARBT *o;

	/* real */
	if (ibase > obase)
		k = (size_t) ((2*a->lp) / log10(obase)) ;
	else 
		k = a->lp;
	sv = arb_calloc(k, sizeof(ARBT));
	ARBT *real = arb_calloc(k, sizeof(ARBT));
	memcpy(real + (k - a->lp), a->number, a->lp * sizeof(ARBT));
	memset(real, 0, (k - a->lp) * sizeof(ARBT));

	for (i = 0; i < k; ++i) { 
		carry = real[i];
		prod = 0;
		for (j = k; j > 0; j--) {
			prod = (sv[j-1] * ibase) + carry;
			sv[j-1] = prod % obase;
			carry = prod / obase;
		}
	}
	b = arb_expand(b, k);
	_arb_copy_core(b->number, sv, k);
	b->lp = k;
	//b->number = sv;
	
	/* fractional */
	k = a->rp;
	z = a->rp;
	fxdpnt *obh = hrdware2arb(obase);
	p = arb_calloc(k * 2, sizeof(ARBT));
	o = arb_calloc(k * 2, sizeof(ARBT));
	
	ARBT *frac = arb_calloc(z * 2, sizeof(ARBT));
	memcpy(frac, a->number + a->lp, a->rp * sizeof(ARBT));
	for (i = 0; i < k; ++i) {
		memset(o, 0, z * sizeof(ARBT)); 
		len = arb_mul_core(frac, z, obh->number, obh->len, o, ibase);
		size = len - z;
		p[i] = arb2hrdware(o, size , 10); /* note: absorbs leading zeros */
		_arb_copy_core(frac, o + size , z);
        }
	b = arb_expand(b, b->lp + k);
	_arb_copy_core(b->number + b->lp, p, k);
	b->rp = k;
	//b->number = p;
	
	/* finish off */
	b->len = b->lp + b->rp;
	return b;
}
