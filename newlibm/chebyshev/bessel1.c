/*****************************************************************
*  Program to demonstrate the Bessel function asymptotic series  *
* -------------------------------------------------------------- *
*       Reference: BASIC Scientific Subroutines, Vol. II         *
*       By F.R. Ruckdeschel, BYTE/McGRAWW-HILL, 1981 [1].        *
*                                                                *
*                           C++ Version by J.-P. Moreau, Paris.  *
*                                  (www.jpmoreau.fr)             *
* -------------------------------------------------------------- *
* SAMPLE RUN:                                                    *
*                                                                *
*  X            J0(X)            J1(X)        N            E     *
* -------------------------------------------------------------- *
*  1           0.733562         0.402234      1       0.1121521  *
*  2           0.221488         0.578634      1       0.0070095  *
*  3          -0.259956         0.340699      2       0.0007853  *
*  4          -0.396826        -0.065886      2       0.0001398  *
*  5          -0.177486        -0.327696      3       0.0000155  *
*  6           0.150635        -0.276760      3       0.0000036  *
*  7           0.300051        -0.004696      4       0.0000004  *
*  8           0.171638         0.234650      5       0.0000000  *
*  9          -0.090332         0.245324      5       0.0000000  *
* 10          -0.245930         0.043476      6       0.0000000  *
* 11          -0.171187        -0.176788      5       0.0000000  *
* 12           0.047689        -0.223450      5       0.0000000  *
* 13           0.206925        -0.070319      4       0.0000000  *
* 14           0.171072         0.133376      4       0.0000000  *
* 15          -0.014225         0.205105      4       0.0000000  *
* -------------------------------------------------------------- *
*                                                                *
*****************************************************************/
#include <stdio.h>
#include <math.h>

#define PI 4*atan(1)

double e, e3, J0, J1, x;
double m1, m2, n1, n2;
int    i, n;


/*********************************************************
* Bessel function asymptotic series subroutine. This     *
* program calculates the zeroth and first order Bessel   *
* functions using an asymptotic series expansion.        *
* The required input are X and a convergence factor e.   *
* Returned are the two Bessel functions, J0(X) and J1(X) *
* ------------------------------------------------------ *
* Reference; Algorithms for RPN calculators, by Ball,    *
* L.A. Wiley and sons.                                   *
*********************************************************/
void Calcul_Bessel()  {
//Labels: e100, e200
  double a,a1,a2,b,c,e1,e2,e4,x1;
  int    m;
  //Calculate P and Q polynomials; P0(x)=m1, P1(x)=m2,
  //Q0(x)=n1, Q1(x)=n2  
  a = 1; a1 = 1; a2 = 1; b = 1; c = 1;
  e1 = 1e6;
  m = -1;
  x1 = 1.0 / (8.0 * x);
  x1 = x1 * x1;
  m1 = 1.0; m2 = 1.0;
  n1 = -1.0 / (8.0 * x);
  n2 = -3.0 * n1;
  n = 0;
e100:  m = m + 2; a = a * m * m;
  m = m + 2; a = a * m * m;
  c = c * x1;
  a1 = a1 * a2; a2 = a2 + 1.0; a1 = a1 * a2;
  a2 = a2 + 1.0; e2 = a * c / a1;
  e4 = 1.0 + (m + 2) / m + (m + 2) * (m + 2) / (a2 * 8 * x) + (m + 2) * (m + 4) / (a2 * 8 * x);
  e4 = e4 * e2;
  //Test for divergence
  if (fabs(e4) > e1) goto e200;
  e1 = fabs(e2);
  m1 = m1 - e2;
  m2 = m2 + e2 * (m + 2) / m;
  n1 = n1 + e2 * (m + 2) * (m + 2) / (a2 * 8 * x);
  n2 = n2 - e2 * (m + 2) * (m + 4) / (a2 * 8 * x);
  n++;
  //Test for convergence criterion
  if (e1 < e3) goto e200;
  goto e100;
e200: a = PI;
  e = e2;
  b = sqrt(2.0 / (a * x));
  J0 = b * (m1 * cos(x - a / 4) - n1 * sin(x - a / 4.0));
  J1 = b * (m2 * cos(x - 3 * a / 4.0) - n2 * sin(x - 3 * a / 4.0));
}


void main()  {
  e3=1e-8;
  printf("\n   X            J0(X)            J1(X)        N            E      \n");
  printf("------------------------------------------------------------------\n");
  for (i = 1; i < 16; i++) {
    x = (double) i;
    Calcul_Bessel();
    printf("  %2d          %9.6f        %9.6f      %d       %9.7f\n",i,J0,J1,n,e);
  }
  printf("------------------------------------------------------------------\n");
}

