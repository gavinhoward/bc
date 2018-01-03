#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <float.h>

double trigfunc(int p, double x);
double mysin(double);
double driver(double);
double sin20(double);
double strange = 6.410290407010279602425714995528976754871E-26L;
double strange1 = 6.41029040E-26L;
int main(int argc, char *argv[])
{
	printf("%19.19LF\n", strange);
	printf("%19.19ll\n", strange1);
	return 0;
}
