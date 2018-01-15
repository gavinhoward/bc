#include <arbprec/arbprec.h>

int main(int argc, char *argv[])
{
	if (argc < 5 )
		arb_error("Needs 3 args, such as: 123 123 scale base");
 
	
	int base = strtol(argv[3], 0, 10);
	int scale = strtol(argv[4], 0, 10);

	fxdpnt *a, *b, *c;

	a = arb_str2fxdpnt(argv[1]);
	b = arb_str2fxdpnt(argv[2]);
	c = arb_expand(NULL, 1);
	c = arb_mul(a, b, c, base, scale);
	arb_print(c);
	arb_free(a);
	arb_free(b);
	arb_free(c);
	return 0;
}

