#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <float.h>

double trigfunc(int p, double x);
double mysin(double);
double driver(double);
double sin20(double);
int main(int argc, char *argv[])
{
        double x;

        if ( argc > 1 )
                x = strtod(argv[1], 0);
        else
                x = 19; 
	printf("sin() =          %19.55lf\n", sin(x)); 
        printf("cos() =          %19.55f\n", cos(x)); 
        return 0;
}

