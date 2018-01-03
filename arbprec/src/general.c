#include <arbprec/arbprec.h>

void arb_free(fxdpnt *flt)
{
	if (flt->number)
		free(flt->number);
	free(flt);
}

void arb_init(fxdpnt *flt)
{
	flt->sign = '+';
	flt->len = flt->lp = 0;
}

void arb_flipsign(fxdpnt *flt)
{
	if (flt->sign == '+')
		flt->sign = '-';
	else if (flt->sign == '-')
		flt->sign = '+';
}

void arb_setsign(fxdpnt *a, fxdpnt *b, fxdpnt *c)
{
	arb_init(c);
	if (a->sign == '-')
		arb_flipsign(c);
	if (b->sign == '-')
		arb_flipsign(c);
}

void arb_error(char *message)
{
	fprintf(stderr, "%s\n", message);
	exit(1);
}

void *arb_malloc(size_t len)
{
	void *ret;
	if(!(ret = malloc(len)))
		arb_error("arb_malloc (malloc) failed\n");
	return ret;
}

void *arb_calloc(size_t nmemb, size_t len)
{
	void *ret; 
	if(!(ret = calloc(nmemb, len)))
		arb_error("arb_calloc (calloc) failed\n");
	return ret;
}

fxdpnt *arb_alloc(size_t len)
{
	fxdpnt *ret = arb_malloc(sizeof(fxdpnt));
	ret->number = arb_calloc(1, sizeof(ARBT) * len);
	ret->sign = '+';
	ret->lp = 0; //FIXME: this should likely be "len"
	ret->rp = 0;
	ret->allocated = len;
	ret->len = len;
	ret->chunk = 4;
	return ret;
}

void *arb_realloc(void *ptr, size_t len)
{
	void *ret;
	if(!(ret = realloc(ptr, len)))
		arb_error("realloc failed\n"); // FIXME: return an error, and inform the user
	return ret;
}

fxdpnt *arb_expand(fxdpnt *flt, size_t request)
{
	size_t hold = 0;
	if (flt == NULL){
		flt = arb_alloc(request);
		flt->allocated = request;
	} else if (request >= flt->allocated){ 
		hold = flt->len;
		flt->allocated = (request + flt->chunk);
		flt->number = arb_realloc(flt->number, flt->allocated * sizeof(ARBT));
		memset(flt->number + hold, 0, (flt->allocated - hold) * sizeof(ARBT));
	}
	return flt;
}

