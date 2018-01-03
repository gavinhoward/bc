#include <arbprec/arbprec.h>

double newton_iteration(double x, double eps)
{
        double guess = eps;
        double new_guess = 0;
        size_t i = 0;
        while (i < 100)
        {
                // method 1
                new_guess = guess * (2 - x * guess);
                // method 2
                //new_guess = guess + guess * ( 1 - x * guess); 
                if (guess == new_guess)
                        break;
                guess = new_guess;
                printf("%19.19lf\n", new_guess);
                ++i;

        }
        printf("iterations = %zu\n", i);
        return guess;
}

fxdpnt *arb_newtonian_div(fxdpnt *a, fxdpnt *b, fxdpnt *c, int base, int scale)
{
	(void)scale;
	fxdpnt *guess = arb_expand(NULL, a->len);
	fxdpnt *newguess = arb_expand(NULL, 1); 
	fxdpnt *hold = arb_expand(NULL, 1);
	fxdpnt *hold2 = arb_expand(NULL, 1);
	fxdpnt *two = arb_str2fxdpnt("2.00000");
	fxdpnt *reciprocal;

	guess->lp = 0;
	guess->rp = a->len;
	memset(guess->number, 0, a->len);
	guess->number[a->len -1] = 5;
	guess->len = a->len;

	arb_print(guess);
	arb_print(two);
	size_t i = 0;
	while (i < 10)
	{ 
		hold = arb_mul(b, guess, hold, base); // x * guess
		hold2 = arb_sub(two, hold, hold2, base); // 2 - x * guess
		newguess = arb_mul(guess, hold2, newguess, base); // guess * (2-x*guess)
		arb_copy(guess, newguess);
		arb_print(guess);
		++i;
	}
	reciprocal = guess;
	c = arb_mul(reciprocal, a, c, base);
	return c;
}
