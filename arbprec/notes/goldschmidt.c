#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	
	if (argc < 4 )
		return 1;
	double N = strtod(argv[1], 0);
	double D = strtod(argv[2], 0);
	double F = strtod(argv[3], 0);
	double LAST = 0;
	size_t i = 0;
	
	while (D >= 1.0000000001)
	{
		N *= F;
		D *= F;
		printf("N == %lf\n", N);
		printf("D == %lf\n", D);
		++i;
		LAST = N;
	}
	
	printf("%zu iterations\n", i);
	printf("factor F == %lf\n", F);
	printf("N == %lf\n", N);
	printf("LAST == %lf\n", LAST);
	printf("D == %lf\n", D);

	return 0;
}
