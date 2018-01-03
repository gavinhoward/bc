

#include <iostream>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
using namespace std;

#define TRUE 1
#define FALSE 0

// Math constants we'll use
double const pi=3.1415926535897932384626433;	// pi
double const twopi=2.0*pi;			// pi times 2
double const two_over_pi= 2.0/pi;		// 2/pi
double const halfpi=pi/2.0;			// pi divided by 2
double const threehalfpi=3.0*pi/2.0;  		// pi times 3/2, used in tan routines
double const four_over_pi=4.0/pi;		// 4/pi, used in tan routines
double const qtrpi=pi/4.0;			// pi/4.0, used in tan routines
double const sixthpi=pi/6.0;			// pi/6.0, used in atan routines
double const tansixthpi=tan(sixthpi);		// tan(pi/6), used in atan routines
double const twelfthpi=pi/12.0;			// pi/12.0, used in atan routines
double const tantwelfthpi=tan(twelfthpi);	// tan(pi/12), used in atan routines

// *********************************************************
// ***
// ***   Routines to compute sine and cosine to 3.2 digits
// ***  of accuracy. 
// ***
// *********************************************************
//
//		cos_32s computes cosine (x)
//
//  Accurate to about 3.2 decimal digits over the range [0, pi/2].
//  The input argument is in radians.
//
//  Algorithm:
//		cos(x)= c1 + c2*x**2 + c3*x**4
//   which is the same as:
//		cos(x)= c1 + x**2(c2 + c3*x**2)
//
float cos_32s(float x)
{
const float c1= 0.99940307;
const float c2=-0.49558072;
const float c3= 0.03679168;

float x2;							// The input argument squared

x2=x * x;
return (c1 + x2*(c2 + c3 * x2));
}

//
//  This is the main cosine approximation "driver"
// It reduces the input argument's range to [0, pi/2],
// and then calls the approximator. 
// See the notes for an explanation of the range reduction.
//
float cos_32(float x){
	int quad;						// what quadrant are we in?

	x=fmod(x, twopi);				// Get rid of values > 2* pi
	if(x<0)x=-x;					// cos(-x) = cos(x)
	quad=int(x * two_over_pi);			// Get quadrant # (0 to 3) we're in
	switch (quad){
	case 0: return  cos_32s(x);
	case 1: return -cos_32s(pi-x);
	case 2: return -cos_32s(x-pi);
	case 3: return  cos_32s(twopi-x);
	}
}
//
//   The sine is just cosine shifted a half-pi, so
// we'll adjust the argument and call the cosine approximation.
//
float sin_32(float x){
	return cos_32(halfpi-x);
}

// *********************************************************
// ***
// ***   Routines to compute sine and cosine to 5.2 digits
// ***  of accuracy. 
// ***
// *********************************************************
//
//		cos_52s computes cosine (x)
//
//  Accurate to about 5.2 decimal digits over the range [0, pi/2].
//  The input argument is in radians.
//
//  Algorithm:
//		cos(x)= c1 + c2*x**2 + c3*x**4 + c4*x**6
//   which is the same as:
//		cos(x)= c1 + x**2(c2 + c3*x**2 + c4*x**4)
//		cos(x)= c1 + x**2(c2 + x**2(c3 + c4*x**2))
//
float cos_52s(float x)
{
const float c1= 0.9999932946;
const float c2=-0.4999124376;
const float c3= 0.0414877472;
const float c4=-0.0012712095;

float x2;							// The input argument squared

x2=x * x;
return (c1 + x2*(c2 + x2*(c3 + c4*x2)));
}

//
//  This is the main cosine approximation "driver"
// It reduces the input argument's range to [0, pi/2],
// and then calls the approximator. 
// See the notes for an explanation of the range reduction.
//
float cos_52(float x){
	int quad;						// what quadrant are we in?

	x=fmod(x, twopi);				// Get rid of values > 2* pi
	if(x<0)x=-x;					// cos(-x) = cos(x)
	quad=int(x * two_over_pi);			// Get quadrant # (0 to 3) we're in
	switch (quad){
	case 0: return  cos_52s(x);
	case 1: return -cos_52s(pi-x);
	case 2: return -cos_52s(x-pi);
	case 3: return  cos_52s(twopi-x);
	}
}
//
//   The sine is just cosine shifted a half-pi, so
// we'll adjust the argument and call the cosine approximation.
//
float sin_52(float x){
	return cos_52(halfpi-x);
}

// *********************************************************
// ***
// ***   Routines to compute sine and cosine to 7.3 digits
// ***  of accuracy. 
// ***
// *********************************************************
//
//		cos_73s computes cosine (x)
//
//  Accurate to about 7.3 decimal digits over the range [0, pi/2].
//  The input argument is in radians.
//
//  Algorithm:
//		cos(x)= c1 + c2*x**2 + c3*x**4 + c4*x**6 + c5*x**8
//   which is the same as:
//		cos(x)= c1 + x**2(c2 + c3*x**2 + c4*x**4 + c5*x**6)
//		cos(x)= c1 + x**2(c2 + x**2(c3 + c4*x**2 + c5*x**4))
//		cos(x)= c1 + x**2(c2 + x**2(c3 + x**2(c4 + c5*x**2)))
//
double cos_73s(double x)
{
const double c1= 0.999999953464;
const double c2=-0.499999053455;
const double c3= 0.0416635846769;
const double c4=-0.0013853704264;
const double c5= 0.00002315393167;  	
																

double x2;							// The input argument squared

x2=x * x;
return (c1 + x2*(c2 + x2*(c3 + x2*(c4 + c5*x2))));
}

//
//  This is the main cosine approximation "driver"
// It reduces the input argument's range to [0, pi/2],
// and then calls the approximator. 
// See the notes for an explanation of the range reduction.
//
double cos_73(double x){
	int quad;						// what quadrant are we in?

	x=fmod(x, twopi);				// Get rid of values > 2* pi
	if(x<0)x=-x;					// cos(-x) = cos(x)
	quad=int(x * two_over_pi);			// Get quadrant # (0 to 3) we're in
	switch (quad){
	case 0: return  cos_73s(x);
	case 1: return -cos_73s(pi-x);
	case 2: return -cos_73s(x-pi);
	case 3: return  cos_73s(twopi-x);
	}
}
//
//   The sine is just cosine shifted a half-pi, so
// we'll adjust the argument and call the cosine approximation.
//
double sin_73(double x){
	return cos_73(halfpi-x);
}

// *********************************************************
// ***
// ***   Routines to compute sine and cosine to 12.1 digits
// ***  of accuracy. 
// ***
// *********************************************************
//
//		cos_121s computes cosine (x)
//
//  Accurate to about 12.1 decimal digits over the range [0, pi/2].
//  The input argument is in radians.
//
//  Algorithm:
//		cos(x)= c1 + c2*x**2 + c3*x**4 + c4*x**6 + c5*x**8 + c6*x**10 + c7*x**12
//   which is the same as:
//		cos(x)= c1 + x**2(c2 + c3*x**2 + c4*x**4 + c5*x**6 + c6*x**8 + c7*x**10)
//		cos(x)= c1 + x**2(c2 + x**2(c3 + c4*x**2 + c5*x**4 + c6*x**6 + c7*x**8 ))
//		cos(x)= c1 + x**2(c2 + x**2(c3 + x**2(c4 + c5*x**2 + c6*x**4 + c7*x**6 )))
//		cos(x)= c1 + x**2(c2 + x**2(c3 + x**2(c4 + x**2(c5 + c6*x**2 + c7*x**4 ))))
//		cos(x)= c1 + x**2(c2 + x**2(c3 + x**2(c4 + x**2(c5 + x**2(c6 + c7*x**2 )))))
//
double cos_121s(double x)
{
const double c1= 0.99999999999925182;
const double c2=-0.49999999997024012;
const double c3= 0.041666666473384543;
const double c4=-0.001388888418000423;
const double c5= 0.0000248010406484558;
const double c6=-0.0000002752469638432;
const double c7= 0.0000000019907856854;

double x2;							// The input argument squared

x2=x * x;
return (c1 + x2*(c2 + x2*(c3 + x2*(c4 + x2*(c5 + x2*(c6 + c7*x2))))));
}

//
//  This is the main cosine approximation "driver"
// It reduces the input argument's range to [0, pi/2],
// and then calls the approximator. 
// See the notes for an explanation of the range reduction.
//
double cos_121(double x){
	int quad;						// what quadrant are we in?

	x=fmod(x, twopi);				// Get rid of values > 2* pi
	if(x<0)x=-x;					// cos(-x) = cos(x)
	quad=int(x * two_over_pi);			// Get quadrant # (0 to 3) we're in
	switch (quad){
	case 0: return  cos_121s(x);
	case 1: return -cos_121s(pi-x);
	case 2: return -cos_121s(x-pi);
	case 3: return  cos_121s(twopi-x);
	}
}
//
//   The sine is just cosine shifted a half-pi, so
// we'll adjust the argument and call the cosine approximation.
//
double sin_121(double x){
	return cos_121(halfpi-x);
}

// *********************************************************
// ***
// ***   Routines to compute tangent to 3.2 digits
// ***  of accuracy. 
// ***
// *********************************************************
//
//		tan_32s computes tan(pi*x/4)
//
//  Accurate to about 3.2 decimal digits over the range [0, pi/4].
//  The input argument is in radians. Note that the function
//  computes tan(pi*x/4), NOT tan(x); it's up to the range
//  reduction algorithm that calls this to scale things properly.
//
//  Algorithm:
//		tan(x)= x*c1/(c2 + x**2)
//
float tan_32s(float x)
{
const float c1=-3.6112171;
const float c2=-4.6133253;

float x2;							// The input argument squared

x2=x * x;
return (x*c1/(c2 + x2));
}

//
//  This is the main tangent approximation "driver"
// It reduces the input argument's range to [0, pi/4],
// and then calls the approximator. 
// See the notes for an explanation of the range reduction.
// Enter with positive angles only.
//
// WARNING: We do not test for the tangent approaching infinity,
// which it will at x=pi/2 and x=3*pi/2. If this is a problem
// in your application, take appropriate action.
//
float tan_32(float x){
	int octant;						// what octant are we in?

	x=fmod(x, twopi);				// Get rid of values >2 *pi
	octant=int(x * four_over_pi);			// Get octant # (0 to 7)
	switch (octant){
	case 0: return      tan_32s(x              *four_over_pi);
	case 1: return  1.0/tan_32s((halfpi-x)     *four_over_pi);
	case 2: return -1.0/tan_32s((x-halfpi)     *four_over_pi);
	case 3: return -    tan_32s((pi-x)         *four_over_pi);
	case 4: return      tan_32s((x-pi)         *four_over_pi);
	case 5: return  1.0/tan_32s((threehalfpi-x)*four_over_pi);
	case 6: return -1.0/tan_32s((x-threehalfpi)*four_over_pi);
	case 7: return -    tan_32s((twopi-x)      *four_over_pi);
	}
}

// *********************************************************
// ***
// ***   Routines to compute tangent to 5.6 digits
// ***  of accuracy. 
// ***
// *********************************************************
//
//		tan_56s computes tan(pi*x/4)
//
//  Accurate to about 5.6 decimal digits over the range [0, pi/4].
//  The input argument is in radians. Note that the function
//  computes tan(pi*x/4), NOT tan(x); it's up to the range
//  reduction algorithm that calls this to scale things properly.
//
//  Algorithm:
//		tan(x)= x(c1 + c2*x**2)/(c3 + x**2)
//
float tan_56s(float x)
{
const float c1=-3.16783027;
const float c2= 0.134516124;
const float c3=-4.033321984;

float x2;							// The input argument squared

x2=x * x;
return (x*(c1 + c2 * x2)/(c3 + x2));
}

//
//  This is the main tangent approximation "driver"
// It reduces the input argument's range to [0, pi/4],
// and then calls the approximator. 
// See the notes for an explanation of the range reduction.
// Enter with positive angles only.
//
// WARNING: We do not test for the tangent approaching infinity,
// which it will at x=pi/2 and x=3*pi/2. If this is a problem
// in your application, take appropriate action.
//
float tan_56(float x){
	int octant;						// what octant are we in?

	x=fmod(x, twopi);				// Get rid of values >2 *pi
	octant=int(x * four_over_pi);			// Get octant # (0 to 7)
	switch (octant){
	case 0: return      tan_56s(x              *four_over_pi);
	case 1: return  1.0/tan_56s((halfpi-x)     *four_over_pi);
	case 2: return -1.0/tan_56s((x-halfpi)     *four_over_pi);
	case 3: return -    tan_56s((pi-x)         *four_over_pi);
	case 4: return      tan_56s((x-pi)         *four_over_pi);
	case 5: return  1.0/tan_56s((threehalfpi-x)*four_over_pi);
	case 6: return -1.0/tan_56s((x-threehalfpi)*four_over_pi);
	case 7: return -    tan_56s((twopi-x)      *four_over_pi);
	}
}


// *********************************************************
// ***
// ***   Routines to compute tangent to 8.2 digits
// ***  of accuracy. 
// ***
// *********************************************************
//
//		tan_82s computes tan(pi*x/4)
//
//  Accurate to about 8.2 decimal digits over the range [0, pi/4].
//  The input argument is in radians. Note that the function
//  computes tan(pi*x/4), NOT tan(x); it's up to the range
//  reduction algorithm that calls this to scale things properly.
//
//  Algorithm:
//		tan(x)= x(c1 + c2*x**2)/(c3 + c4*x**2 + x**4)
//
double tan_82s(double x)
{
const double c1= 211.849369664121;
const double c2=- 12.5288887278448 ;
const double c3= 269.7350131214121;
const double c4=- 71.4145309347748;

double x2;							// The input argument squared

x2=x * x;
return (x*(c1 + c2 * x2)/(c3 + x2*(c4 + x2)));
}

//
//  This is the main tangent approximation "driver"
// It reduces the input argument's range to [0, pi/4],
// and then calls the approximator. 
// See the notes for an explanation of the range reduction.
// Enter with positive angles only.
//
// WARNING: We do not test for the tangent approaching infinity,
// which it will at x=pi/2 and x=3*pi/2. If this is a problem
// in your application, take appropriate action.
//
double tan_82(double x){
	int octant;						// what octant are we in?

	x=fmod(x, twopi);				// Get rid of values >2 *pi
	octant=int(x * four_over_pi);			// Get octant # (0 to 7)
	switch (octant){
	case 0: return      tan_82s(x              *four_over_pi);
	case 1: return  1.0/tan_82s((halfpi-x)     *four_over_pi);
	case 2: return -1.0/tan_82s((x-halfpi)     *four_over_pi);
	case 3: return -    tan_82s((pi-x)         *four_over_pi);
	case 4: return      tan_82s((x-pi)         *four_over_pi);
	case 5: return  1.0/tan_82s((threehalfpi-x)*four_over_pi);
	case 6: return -1.0/tan_82s((x-threehalfpi)*four_over_pi);
	case 7: return -    tan_82s((twopi-x)      *four_over_pi);
	}
}

// *********************************************************
// ***
// ***   Routines to compute tangent to 14 digits
// ***  of accuracy. 
// ***
// *********************************************************
//
//		tan_14s computes tan(pi*x/4)
//
//  Accurate to about 14 decimal digits over the range [0, pi/4].
//  The input argument is in radians. Note that the function
//  computes tan(pi*x/4), NOT tan(x); it's up to the range
//  reduction algorithm that calls this to scale things properly.
//
//  Algorithm:
//		tan(x)= x(c1 + c2*x**2 + c3*x**4)/(c4 + c5*x**2 + c6*x**4 + x**6)
//
double tan_14s(double x)
{
const double c1=-34287.4662577359568109624;
const double c2=  2566.7175462315050423295;
const double c3=-   26.5366371951731325438;
const double c4=-43656.1579281292375769579;
const double c5= 12244.4839556747426927793;
const double c6=-  336.611376245464339493;

double x2;							// The input argument squared

x2=x * x;
return (x*(c1 + x2*(c2 + x2*c3))/(c4 + x2*(c5 + x2*(c6 + x2))));
}

//
//  This is the main tangent approximation "driver"
// It reduces the input argument's range to [0, pi/4],
// and then calls the approximator. 
// See the notes for an explanation of the range reduction.
// Enter with positive angles only.
//
// WARNING: We do not test for the tangent approaching infinity,
// which it will at x=pi/2 and x=3*pi/2. If this is a problem
// in your application, take appropriate action.
//
double tan_14(double x){
	int octant;						// what octant are we in?

	x=fmod(x, twopi);				// Get rid of values >2 *pi
	octant=int(x * four_over_pi);			// Get octant # (0 to 7)
	switch (octant){
	case 0: return      tan_14s(x              *four_over_pi);
	case 1: return  1.0/tan_14s((halfpi-x)     *four_over_pi);
	case 2: return -1.0/tan_14s((x-halfpi)     *four_over_pi);
	case 3: return -    tan_14s((pi-x)         *four_over_pi);
	case 4: return      tan_14s((x-pi)         *four_over_pi);
	case 5: return  1.0/tan_14s((threehalfpi-x)*four_over_pi);
	case 6: return -1.0/tan_14s((x-threehalfpi)*four_over_pi);
	case 7: return -    tan_14s((twopi-x)      *four_over_pi);
	}
}


// *********************************************************
// ***
// ***   Routines to compute arctangent to 6.6 digits
// ***  of accuracy. 
// ***
// *********************************************************
//
//		atan_66s computes atan(x)
//
//  Accurate to about 6.6 decimal digits over the range [0, pi/12].
//
//  Algorithm:
//		atan(x)= x(c1 + c2*x**2)/(c3 + x**2)
//
double atan_66s(double x)
{
const double c1=1.6867629106;
const double c2=0.4378497304;
const double c3=1.6867633134;


double x2;							// The input argument squared

x2=x * x;
return (x*(c1 + x2*c2)/(c3 + x2));
}

//
//  This is the main arctangent approximation "driver"
// It reduces the input argument's range to [0, pi/12],
// and then calls the approximator. 
//
//
double atan_66(double x){
double y;							// return from atan__s function
int complement= FALSE;				// true if arg was >1 
int region= FALSE;					// true depending on region arg is in
int sign= FALSE;					// true if arg was < 0

if (x <0 ){
	x=-x;
	sign=TRUE;						// arctan(-x)=-arctan(x)
}
if (x > 1.0){
	x=1.0/x;						// keep arg between 0 and 1
	complement=TRUE;
}
if (x > tantwelfthpi){
	x = (x-tansixthpi)/(1+tansixthpi*x);	// reduce arg to under tan(pi/12)
	region=TRUE;
}

y=atan_66s(x);						// run the approximation
if (region) y+=sixthpi;				// correct for region we're in
if (complement)y=halfpi-y;			// correct for 1/x if we did that
if (sign)y=-y;						// correct for negative arg
return (y);

}

// *********************************************************
// ***
// ***   Routines to compute arctangent to 13.7 digits
// ***  of accuracy. 
// ***
// *********************************************************
//
//		atan_137s computes atan(x)
//
//  Accurate to about 13.7 decimal digits over the range [0, pi/12].
//
//  Algorithm:
//		atan(x)= x(c1 + c2*x**2 + c3*x**4)/(c4 + c5*x**2 + c6*x**4 + x**6)
//
double atan_137s(double x)
{
const double c1= 48.70107004404898384;
const double c2= 49.5326263772254345;
const double c3=  9.40604244231624;
const double c4= 48.70107004404996166;
const double c5= 65.7663163908956299;
const double c6= 21.587934067020262;

double x2;							// The input argument squared

x2=x * x;
return (x*(c1 + x2*(c2 + x2*c3))/(c4 + x2*(c5 + x2*(c6 + x2))));
}

//
//  This is the main arctangent approximation "driver"
// It reduces the input argument's range to [0, pi/12],
// and then calls the approximator. 
//
//
double atan_137(double x){
double y;							// return from atan__s function
int complement= FALSE;				// true if arg was >1 
int region= FALSE;					// true depending on region arg is in
int sign= FALSE;					// true if arg was < 0

if (x <0 ){
	x=-x;
	sign=TRUE;						// arctan(-x)=-arctan(x)
}
if (x > 1.0){
	x=1.0/x;						// keep arg between 0 and 1
	complement=TRUE;
}
if (x > tantwelfthpi){
	x = (x-tansixthpi)/(1+tansixthpi*x);	// reduce arg to under tan(pi/12)
	region=TRUE;
}

y=atan_137s(x);						// run the approximation
if (region) y+=sixthpi;				// correct for region we're in
if (complement)y=halfpi-y;			// correct for 1/x if we did that
if (sign)y=-y;						// correct for negative arg
return (y);

}

//
//   Run the approximation over it's range. Write the results compared
// to those from the C library to "sincos.txt".
//

int main()
{

	FILE *results;						// File that will contain results
	int i;
	double b;

	if( (results = fopen( "sincos.txt", "w" )) == NULL )
		printf( "ERROR: The result file could not be opened\n" );

// Print a header line identifying the meaning of each column of data
	fprintf(results,"x, Cosine, Sine, Tangent, aTan");
	fprintf(results,",cos_32  Error, sin_32 Error");
	fprintf(results,",cos_52  Error, sin_52 Error");
	fprintf(results,",cos_73  Error, sin_73 Error");
	fprintf(results,",cos_121 Error, sin_121 Error");
	fprintf(results,",tan_32  pc Error");
	fprintf(results,",tan_56  pc Error");
	fprintf(results,",tan_82  pc Error");
	fprintf(results,",tan_14  pc Error");
	fprintf(results,",atan_66 Error");
	fprintf(results,",atan_137 Error");
	fprintf(results,"\n");

// Run the approximations over the entire range
 
	for(i=0;i<361;i+=1){	
		b=(double)(i)*2.0*pi/360.0;			// Convert degrees to radians
 
		fprintf(results,"%d,%1.14f,%1.14f, %1.14f",i, cos(b), sin(b), tan(b), atan(tan(b)));
		fprintf(results,",%1.14f,%1.14f",cos(b)- cos_32(b), sin(b)-sin_32(b));
		fprintf(results,",%1.14f,%1.14f",cos(b)- cos_52(b), sin(b)-sin_52(b));
		fprintf(results,",%1.14f,%1.14f",cos(b)- cos_73(b), sin(b)-sin_73(b));
		fprintf(results,",%1.14f,%1.14f",cos(b)- cos_121(b),sin(b)-sin_121(b));
		fprintf(results,",%1.14f",100.0*(tan(b)-tan_32(b))/tan_32(b));
		fprintf(results,",%1.14f",100.0*(tan(b)-tan_56(b))/tan_56(b));
		fprintf(results,",%1.14f",100.0*(tan(b)-tan_82(b))/tan_82(b));
		fprintf(results,",%1.14f",100.0*(tan(b)-tan_14(b))/tan_14(b));
		fprintf(results,",%1.14f",atan(tan(b))-atan_66(tan(b)));
		fprintf(results,",%1.14f",atan(tan(b))-atan_137(tan(b)));
		fprintf(results,"\n");
	}    
  
 fclose(results);

 return 0;
}

