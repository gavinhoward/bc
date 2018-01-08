#include <stdio.h>
#include <math.h>
#include <stdlib.h>

double logbase(double x, double base)
{
	// (ln x / ln b) is log for base b of x
	double ret = log10(x) / log10(base);
}
int main(int argc, char *argv[])
{
	// 2 args, number and base
	//printf("answer = %f\b", logbase(strtod(argv[1], 0), strtod(argv[2], 0)));
	double i = 0;
	size_t j = 0;
	printf("/* log table 0 - 50 to simplify access */\n");
	printf("double logtable[] = {");
	while ( i < 50 )
	{
		printf(" %10.13lf,",  log10(i));
		if ( j % 3 == 0 )
			printf("\n");
		++j;
		i += 1;
	}
	printf("};");

	
	return 0;
}
