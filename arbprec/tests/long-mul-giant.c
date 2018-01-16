#include <arbprec/arbprec.h>
#define MAXIMA 1000000
int main(int argc, char *argv[])
{

	char *string1 = malloc(MAXIMA);
	char *string2 = malloc(MAXIMA);
 
	size_t i = 0;

	while ( i < MAXIMA )
	{
		string1[i] = (i % 10) + '0';
		++i;
	}
	i = 0;
	while ( i < MAXIMA )
	{
		string2[i] = (i % 9) + '0';
		++i;
	}
	fprintf(stderr, "past initialization\n");
	write(1, string1, MAXIMA);
	write(1, string2, MAXIMA);
	fprintf(stderr, "past confirmation\n");
	int base = 10;
	fxdpnt *a, *b, *c;

	a = arb_str2fxdpnt(string1);
	b = arb_str2fxdpnt(string2);
	c = arb_expand(NULL, 1);
	c = arb_mul(a, b, c, base, 10);
	arb_print(c);
	arb_free(a);
	arb_free(b);
	arb_free(c);
	return 0;
}

