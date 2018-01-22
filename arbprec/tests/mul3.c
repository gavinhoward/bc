#include <arbprec/arbprec.h>

int main(int argc, char *argv[])
{
	if (argc < 7 )
		arb_error("Needs 6 args, such as: 3 3 3 3 base scale");
 
	
	int base = strtol(argv[5], 0, 10);
	int scale = strtol(argv[6], 0, 10);

	fxdpnt *a, *b, *c, *d, *e, *e_;

	a = arb_str2fxdpnt(argv[1]);
	b = arb_str2fxdpnt(argv[2]);
	c = arb_str2fxdpnt(argv[3]);
	d = arb_str2fxdpnt(argv[4]);
	e = arb_expand(NULL, 1);
	e_ = arb_expand(NULL, 1);
	e  = arb_mul(a, b , e , base, scale);
	e_ = arb_mul(c, e , e_, base, scale);
	e  = arb_mul(d, e_, e , base, scale);
	arb_print(e);
	arb_free(a);
	arb_free(b);
	arb_free(c);
	arb_free(d);
	arb_free(e);
	return 0;
}

