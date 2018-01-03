#include <math.h>
#include <stdint.h>
#include <limits.h>
#include <stddef.h>
#include <float.h>
#include "libm.h"
/*
	Modified taylor / maclaurin series for comparison and testing
	Not performant.
*/
#define PI 3.14159265358979323846

double series_func(double x, int one, int toggler, int exp);
double series_func_driver(double x, int selector);
double mysin(double x);
double mysin(double x)
{ 
	//return series_func(x, 1, 1, 0);/* sin */ 
	return series_func_driver(x, 2);
} 
/*
double asin(double x)
{ 
	return aseries_func(x, 0);
} 

double sinh(double x)
{ 
	return series_func(x, 1, 0, 0);
} 

double cos(double x)
{ 
	return series_func(x, 0, 1, 0);
} 

double acos(double x)
{
	return aseries_func(x, 1);
}

double cosh(double x)
{
	return series_func(x, 0, 0, 0);
} 
*/

double why = 0;
double series_func_driver(double x, int selector)
{ 
        double sum = 0;
	double y[2];
	int n = 0;
	int32_t ix;
	if ( selector == 0 || selector == 2)
	{
		/* argument range reduction */
	        GET_HIGH_WORD(ix, x); 
	        ix &= 2147483647;
	        if (ix <= 1072243195)
	        { 
	                if (ix < 1045430272) 
	                        if ((int)x == 0) 
	                                return x; 
	                return series_func(x, 1, 1, 0);
	        }

	        y[1] = y[0] = x;
	        n = ____rem_pio2(x, y);
	
		fprintf(stderr, "rem_pio n = %19d, y[0] = %19.19lf, y[1] = %19.19lf\n", n, y[0], y[1]);
	}

	if ( selector == 0 ) 
		sum = series_func(x, 0, 1, 0);/* cos */ 
	if ( selector == 1 )
		sum = series_func(x, 0, 0, 0);/* cosh */
	if ( selector == 2 ) /* sin */ 
	{
		switch (n&3) { 
                        case 0: return series_func(y[0], 1, 1, 0);  //  sin
                        case 1: return series_func(y[0], 0, 1, 0);  //  cos
                        case 2: return -series_func(y[0], 1, 1, 0); // -sin
                        default: 
                                return -series_func(y[0], 0, 1, 0) ;// -cos
                }

	}
	if ( selector == 3 )
		sum = series_func(x, 1, 0, 0);/* sinh */
	if ( selector == 4 )
		sum = series_func(x, 0, 0, 1);/* exp >> 1*/ 
        return sum;
} 

double series_func(double x, int one, int toggler, int exp)
{
        size_t i = 0;
        size_t j = 0;
        double product = 1.0;
        double sum = 0.0;
        double last = 0.0;
	int toggle = 1;

        for (i = 0; i < SIZE_MAX; i++)
        {
               	for (j = (2.0*i) + one, product = 1.0; j > 0 ; j--) 
                	product *= x / j;
	
		sum += product * toggle;
		
		if ( toggler != 0 )
			toggle = -toggle;

		if  (last == sum)
			break;

                last = sum; 
        }

	if ( exp )
		sum *=2;
	
        return sum;
}
