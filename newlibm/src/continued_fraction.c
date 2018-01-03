#include <math.h>
#include <stdint.h>
#include <limits.h>
#include <stddef.h>
#include <float.h>
#include "libm.h"

/* continued fraction expansions for comparative study. not performant */

double trigfunc(int p, double x)
{
	int ss = x;
	double yy[2];
	yy[0] = x;
        yy[1] = x;
	int n = 0;
        n = ____rem_pio2(x, yy);

	x = yy[0];
	size_t i = 30;
	double r = 0;
	double s = 0;
	double y = 0;
	double Z = (x-1)/(x+1);
	double ZZ= 0;
	double sum1 = 0;
	double sum2 = 0;

	switch (n&3) {
                        case 0: p = 1;  //  sin
                        case 1: p = 2;  //  cos
                        case 2: p = 1; // -sin
                        default:
                                p = 2;// -cos
                }


	if ( p == 5)
		Z=x; 

	ZZ = Z * Z;

	if (p <= 3)
		r = - x * x;    /* trig */
	else 
		r = x * x;      /* hyperbolic */
	
	s = 4 * i + 2;

	if ( p < 4 || p > 5 )	/* ! log */
		for (; i > 0; i--)
			s = 4 * i - 2 + r/s;


	for (; (p == 4 || p == 5) && i > 0; i--) /* log */
		s = (2*i -1) - i*i*(ZZ)/s;

	switch (p % 6)

	{
		case 0 : 
			y = (s + x)/(s - x);		/* exp */
			break;
		case 1 : 
			y = 2 * x * s/(s * s - r);	/* sin, sinh */
			break;
		case 2 : 
			y = (s * s + r)/(s * s - r);	/* cos, cosh */
			break;
		case 3 : 
			y = 2 * x * s/(s * s + r);	/* tan, tanh */
			break; 
		case 4 :
			y = 2*Z/s;			/* log */
			break; 
		case 5 : 
			y = (2*Z/s) / 2;		/* atanh */
			break;
				
			/*
				These all have identities relating to log
				so can likely be expressed using this series
				function

				asinh(x) = ln( x + [sqrt](x2 + 1) ) 
				acosh(x) = ln( x [sqrt](x2 - 1) ) 
				atanh(x) = 1/2 ln( (1+x)/(1-x) ) 
	
				Conversely, these functions all bear a
				relationship to exp()
	
				sinh(x) = ( e(x) - e(-x) )/2 
				cosh(x) = ( e(x) + e(-x) )/2 
				tanh(x) = sinh(x)/cosh(x) = ( e(x) - e(-x) )/( e(x) + e(-x) ) 
		     
			*/


		case 6 :
			break;
	}
	return y;
}

