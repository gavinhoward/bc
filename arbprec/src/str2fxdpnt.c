#include <assert.h>

#include <arbprec/arbprec.h>

int arb_base(int a)
{
	    static int glph[110] = { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		         0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		         0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		         0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		         0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  0,  0,
		         0,  0,  0,  0,  0, 10, 11, 12, 13, 14, 15, 16,
		        17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28,
		        29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
		        41, 42, 43, 44, 45,  0,  0,  0,  0,  0,  0,  0,
		         0,  0};

		if (a < 110)
			    return glph[a];
		return 0;
}

fxdpnt *arb_str2fxdpnt(const char *str)
{
	return arb_parse_str(NULL, str);
}

fxdpnt *arb_parse_str(fxdpnt *flt, const char *str)
{
	assert(str);

	if (!flt) {
		flt = arb_expand(NULL, 1);
	}

	size_t i = 0;
	int flt_set = 0, sign_set = 0;

	flt->len = flt->lp = flt->rp = 0;

	for (i = 0; str[i] != '\0'; ++i){
		if (str[i] == '.'){
			flt_set = 1;
			flt->lp = i - sign_set;
		}
		else if (str[i] == '+'){
			sign_set = 1;
			flt->sign = '+';
		}
		else if (str[i] == '-'){
			sign_set = 1;
			flt->sign = '-';
		}
		else{
			flt = arb_expand(flt, flt->len + 1);
			flt->number[flt->len++] = arb_base(str[i]);
		}
	}

	if (flt_set == 0)
		flt->lp = flt->len;

	flt->rp = flt->len - flt->lp;

	return flt;
}

