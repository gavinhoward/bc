#include <arbprec/arbprec.h>

int arb_place(fxdpnt *a, fxdpnt *b, size_t *cnt, size_t r)
{
	int temp = 0;
	if ((a->len - a->lp) < (b->len - b->lp))
		if((b->len - b->lp) - (a->len - a->lp) > r)
			return 0;
	if (*cnt < a->len){
		temp = a->number[a->len - *cnt - 1];
		(*cnt)++;
		return temp;
	}
	(*cnt)++;
	return 0;
}

void arb_reverse(ARBT *x, size_t lim)
{
	size_t i = 0, half = lim / 2;
	int swap = 0;
	for (;i < half; i++){
		swap = x[i];
		x[i] = x[lim - i - 1];
		x[lim - i - 1] = swap;
	}
}

fxdpnt *arb_add_inter(fxdpnt *a, fxdpnt *b, fxdpnt *c, int base)
{
	size_t i = 0, j = 0, r = 0;
	int sum = 0, carry = 0;

	c->lp = MAX(a->lp, b->lp);
	c = arb_expand(c, MAX(a->len, b->len) * 2);
	for (; i < a->len || j < b->len;c->len++, ++r){
		sum = arb_place(a, b, &i, r) + arb_place(b, a, &j, r) + carry;
		carry = 0;
		if(sum >= base){
			carry = 1;
			sum -= base;
		}
		c->number[c->len] = sum;
	}
	if (carry){
		c->number[c->len++] = 1;
		c->lp += 1;
	}
	c->rp = c->len - c->lp;
	arb_reverse(c->number, c->len);
	return c;
}


fxdpnt *arb_sub_inter(fxdpnt *a, fxdpnt *b, fxdpnt *c, int base)
{
	size_t width = 0, i = 0, j = 0, r = 0;
	int sum = 0, borrow = 0;
	int mborrow = -1; // mirror borrow must be -1
	int mir = 0;
	size_t z = 0, y = 0; // dummy variables for the mirror
	char *array;

	c->lp = MAX(a->lp, b->lp);
	width = MAX(a->len, b->len);
	c = arb_expand(c, width * 2); // fixme: this is way oversized
	array = arb_malloc((width * 2) * sizeof(ARBT)); // fixme: this is way oversized

	for (; i < a->len || j < b->len;c->len++, ++r){
		mir = arb_place(a, b, &y, r) - arb_place(b, a, &z, r) + mborrow; // mirror
		sum = arb_place(a, b, &i, r) - arb_place(b, a, &j, r) + borrow;

		borrow = 0;
		if(sum < 0){
			borrow = -1;
			sum += base;
		}
		c->number[c->len] = sum;
		// maintain a mirror for subtractions that cross the zero threshold
		y = i;
		z = j;
		mborrow = 0;
		if(mir < 0){
			mborrow = -1;
			mir += base;
		}
		array[c->len] = (base-1) - mir;
	}
	// a left over borrow indicates that the zero threshold was crossed
	if (borrow == -1){
		// swapping pointers would make this faster
		_arb_copy_core(c->number, array, c->len);
		arb_flipsign(c);
	}
	free(array);
	c->rp = c->len - c->lp;
	arb_reverse(c->number, c->len);
	return c;
}

fxdpnt *arb_add(fxdpnt *a, fxdpnt *b, fxdpnt *c, int base)
{
	arb_init(c);
	if (a->sign == '-' && b->sign == '-')
		arb_flipsign(c);
	else if (a->sign == '-')
		return c = arb_sub_inter(b, a, c, base);
	else if (b->sign == '-')
		return c = arb_sub_inter(a, b, c, base);
	return c = arb_add_inter(a, b, c, base);
}

fxdpnt *arb_sub(fxdpnt *a, fxdpnt *b, fxdpnt *c, int base)
{
	arb_init(c);
	if (a->sign == '-' && b->sign == '-')
		arb_flipsign(c);
	else if (a->sign == '-'){
		arb_flipsign(c);
		return c = arb_add_inter(a, b, c, base);
	}
	else if (b->sign == '-' || a->sign == '-')
		return c = arb_add_inter(a, b, c, base);
	return c = arb_sub_inter(a, b, c, base);
}
