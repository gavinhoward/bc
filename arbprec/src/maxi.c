#include <arbprec/arbprec.h>

size_t maxi(size_t a, size_t b, size_t c)
{
	size_t ret = 0;
	ret = MAX(a, b);
	ret = MAX(ret, c);
	return ret;
}

