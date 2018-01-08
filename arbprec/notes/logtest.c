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
	printf("answer = %f\b", logbase(strtod(argv[1], 0), strtod(argv[2], 0)));
	
	return 0;
}
