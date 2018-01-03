#include <stdio.h>
#include "mpc.h"

int main(void)
{
	mpc_t a;
	mpc_init2(a, 100);
	mpc_strtoc(a, "123", NULL, a);
	mpc_sin(a, 100, 110);
	return 0;
}
