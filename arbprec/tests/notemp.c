#include <arbprec/arbprec.h>

int main(int argc, char *argv[])
{
	if (argc < 4 )
		arb_error("Needs 3 args, such as:  123 / 123 base scale");

	int base = strtol(argv[3], 0, 10);
	int scale = strtol(argv[4], 0, 10);

	fxdpnt *a, *b, *c;
	a = arb_str2fxdpnt(argv[1]);
	b = arb_str2fxdpnt(argv[2]);

	c = arb_add(a, b, a, base);
	arb_print(c);
	c = arb_sub(a, b, a, base);
	arb_print(c);
//	c = arb_mul(a, b, a, base, scale);
//	arb_print(c);
//	c = arb_alg_d(a, b, a, base, scale);
//	arb_print(c);

	c = arb_add(a, b, b, base);
	arb_print(c);
	c = arb_sub(a, b, b, base);
	arb_print(c);
//	c = arb_mul(a, b, b, base, scale);
//	arb_print(c);
//	c = arb_alg_d(a, b, b, base, scale);
//	arb_print(c);

	c = arb_add(c, c, c, base);
	arb_print(c);
	c = arb_sub(c, c, c, base);
	arb_print(c);
//	c = arb_mul(c, c, c, base, scale);
//	arb_print(c);
//	c = arb_alg_d(c, c, c, base, scale);
//	arb_print(c);

	arb_free(a);
	arb_free(b);

	return 0;
}

