/* origin: FreeBSD /usr/src/lib/msun/src/math_private.h */
/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

#ifndef _LIBM_H
#define _LIBM_H

#include <stdint.h>
#include <stdio.h>


double scalbn(double, int);
double floor(double);

union dshape {
	double value;
	uint64_t bits;
};
/*
	The following code is called type-punning. It would be nice to have 
	a series of notes here describing how it works.
	If the values are printed, they are in octets which makes reading
	them fairly difficult. A conversion to human readable format should
	be devised.
*/

/* Get two 32 bit ints from a double.*/
#define EXTRACT_WORDS(hi,lo,d)					\
{								\
	union dshape ____u;					\
	____u.value = (d);					\
	(hi) = ____u.bits >> 32;				\
	(lo) = (uint32_t)____u.bits;				\
}

/* Get the more significant 32 bit int from a double.*/
#define GET_HIGH_WORD(i,d)					\
{								\
	union dshape ____u;					\
	____u.value = (d);					\
	(i) = ____u.bits >> 32;					\
}

/* Get the less significant 32 bit int from a double.*/
#define GET_LOW_WORD(i,d)					\
{								\
	union dshape ____u;					\
	____u.value = (d);					\
	(i) = (uint32_t)____u.bits;				\
}

/* Set a double from two 32 bit ints.*/
#define INSERT_WORDS(d,hi,lo)					\
{								\
	union dshape ____u;					\
	____u.bits = ((uint64_t)(hi) << 32) | (uint32_t)(lo);	\
	(d) = ____u.value;					\
	fprintf(stderr, "INSERT_WORDS %lf\n", d); \
}

/* Set the more significant 32 bits of a double from an int.*/
#define SET_HIGH_WORD(d,hi)					\
{								\
	union dshape ____u;					\
	____u.value = (d);					\
	____u.bits &= 0xffffffff;				\
	____u.bits |= (uint64_t)(hi) << 32;			\
	(d) = ____u.value;					\
	fprintf(stderr, "SET_HIGH_WORD %d\n", d); \
}

/* Set the less significant 32 bits of a double from an int.*/
#define SET_LOW_WORD(d,lo)					\
{								\
	union dshape ____u;					\
	____u.value = (d);					\
	____u.bits &= 0xffffffff00000000ull;			\
	____u.bits |= (uint32_t)(lo);				\
	(d) = ____u.value;					\
	fprintf(stderr, "SET_LOW_WORD %lf\n", d); \
}

/* fdlibm kernel functions */
int ____rem_pio2_large(double *, double *, int, int, int);
int ____rem_pio2(double, double *);
double ____sin(double, double, int);
double ____cos(double, double);
double ____tan(double, double, int);

#define STRICT_ASSIGN(type, lval, rval){			\
	volatile type ____v = (rval);				\
	(lval) = ____v;						\
	fprintf(stderr, "STRICT_ASSIGN  lval %19.19lf   rval %19.19lf\n", lval, rval); \
}
//#define STRICT_ASSIGN(type, lval, rval) ((lval) = (type)(rval))

#endif
