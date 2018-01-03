#include <math.h>
#include "libm.h"
#include <stdlib.h>
double myj1(double x);
double myj0(double x);
double myjn(int y, double x);
double myj1(double x)
{ 
	return myjn(1, x);
} 

double myj0(double x)
{
	return myjn(0, x);
}

double myjn(int y, double x)
{

        int i = 0;
        int n = 20;
        double c = 0.0; 
        double *j;
        double ret = 0; 
	int flip = 0;
	
	/* special cases */
	
	/*   use trigonometric asymptotic aproximation for x >> y */
	//if ( x > ( y + 1 ) * 10000)
	//	return sqrt(2/ (PI * x )) * gcos(x-(2 * y + 1) * (PI/4)); 
	/*   gsqrt(2/ (PI * x )) * cos(x-(2 * n + 1) * (PI/4)) */

        if ( x == 0 && y == 0)
                return 1.0;
	else if ( x == 0 )
		return  0.0;
	
	/* reverse negative arguments */
	if ( y < 0 )
	{
		++flip;
		y = -y;
	} 

        if ( x < 0)
	{ 
		++flip; 
                x = -x; 
	}

	/* scale the number of terms */
	n += (x/10); 
	
        if ( x > 5.0 )
                n *= 10;
	

        if (!(j = malloc(sizeof(j) * (y + 5))))
                return ;

        j[0] = 1.0;
        j[1] = 0.0; 
       


        while ( 1 )
        {
                for (i = y + 4; i > 0; i--)
                {
                        j[i] = j[i-1]; 
			//printf("%lf\n", j[i]);
                } 
		
                j[0] = ((2.0 * (n * (j[1] / x))) - j[2]);
		printf("%lf\n", j[0]);	
                --n;
                if ( n == 0 )
                        break;
                if ( n % 2 )
                        continue;
                c += 2*j[0];
		//printf("%lf\n", c);
        }



        c += j[0];

        ret = j[y] /= c;
	
	/* If a number was reversed and it was even, reverse it again */
	if ( flip == 1 && y % 2 == 1) 
		ret = -ret;

        free(j);


        return ret;

}


