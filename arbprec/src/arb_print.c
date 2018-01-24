#include <arbprec/arbprec.h>

int arb_highbase(int a)
{
	// Handle high bases
	static int glph[36] = { '0', '1', '2', '3', '4', '5', '6', '7', '8',
				'9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
				'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q',
				'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z' };
	if (a < 36) 
		return glph[a]; 
	else // just use the ascii values for bases that are very high
		return a;
}

void _print_core(FILE *fp, ARBT *number, size_t len, size_t radix, size_t sign)
{
	size_t i = 0;
	size_t k = sign;

	for (i=0; i < len ; ++i){

		if (k % 68 == 0 && k != 0)
			fprintf(fp, "\\\n");

		if (radix == i)
		{
			fprintf(fp, "."); 
			++k;
			if (k % 68 == 0 && k != 0) 
				fprintf(fp, "\\\n"); 
		}
		fprintf(fp, "%c", arb_highbase((number[i])));
		++k;
	}	
	fprintf(fp, "\n");
	fflush(fp);
}

void arb_print(fxdpnt *flt)
{
	size_t sign = 0;
	if (flt->sign == '-')
	{
		putchar(flt->sign);
		sign = 1;
	}
	_print_core(stdout, flt->number, flt->len, flt->lp, sign);
}
