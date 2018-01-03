#include "libm.h"

float cos3(float);
float cos5(float);
double cos20(double x);
double driver(double x);
double const pi=3.1415926535897932384626433;		/* pi */
double const twopi= 6.283185307179586476925286600;	/* 2.0*pi */
double const halfpi= 1.5707963267948966192313216500;	/* pi/2.0 */

double sin20(double x){
	//return driver(halfpi - x);
	
	double f[2];
	double y[2];
        y[0] = x;
        y[1] = x;
        ____rem_pio2(x, y);
	____rem_pio2(pi, f);
	return cos20(f[0] - y[0]);
}
double why2 = 0;
double driver(double x)
{
	int n;
	double y[2];
	int32_t ix;
        

        GET_HIGH_WORD(ix, x);
        ix &= 2147483647;
        if (ix <= 1072243195)
        {
                if (ix < 1045430272)
                        if ((int)x == 0)
                                return x;
		printf("inner mode\n");
                return cos20(x);
        }
	
        y[0] = x;
        y[1] = x;
        n = ____rem_pio2(x, y);
	cos20(y[0]);
} 

float cos3(float x)
{
	const float c1= 0.99940307;
	const float c2= -0.49558072;
	const float c3= 0.03679168;
	float x2;
	x2=x * x;
	return (c1 + x2*(c2 + c3 * x2));
}

float cos5(float x)
{
	float c1= 0.9999932946;
	float c2= -0.4999124376;
	float c3= 0.0414877472;
	float c4= -0.0012712095;
	float x2;
	x2=x * x;
	return (c1 + x2*(c2 + x2*(c3 + c4*x2)));
}

double cos20(double x)
{
	double c1 = 0.9999999999999999999936329;
	double c2 = -0.49999999999999999948362843;
	double c3 = 0.04166666666666665975670054;
	double c4 = -0.00138888888888885302082298;
	double c5 = 0.000024801587301492746422297;
	double c6 = -0.00000027557319209666748555;
	double c7 = 0.0000000020876755667423458605;
	double c8 = -0.0000000000114706701991777771;
	double c9 = 0.0000000000000477687298095717;
	double c10= -0.00000000000000015119893746887;
	double x2;
        
        x2=x * x;
	
	return c1 + x2 *(c2 + x2 *(c3 + x2 *(c4 + x2 *(c5 + x2 *(c6 + x2 *(c7 + x2 *(c8 + x2 *(c9 + x2 *c10))))))));
}
