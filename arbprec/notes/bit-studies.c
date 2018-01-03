#include<stdio.h>
#include<math.h>
#include<stdlib.h>

union {
	double d;
	size_t ui;
} sani;

int main(int argc, char *argv[])
{ 
	


	if ( argc < 3 )
	{
		printf("2 args\n");
		return 1;
	}
	unsigned int N = strtol(argv[1], 0, 10);
	unsigned int D = strtol(argv[2], 0, 10);
	unsigned int redu = strtol(argv[3], 0, 10);
	
	

	size_t reductions = 0; 

	N = N >> redu; 
	D = D >> redu; 

        printf("\n\n%10d\n", N);
	printf("\n\n%10d\n", D); 
	N = N >> redu; 
	D = D >> redu;
	++reductions; 
	printf("reductions = %zu\n", reductions);
        printf("\n\n%10d\n", N);
	printf("\n\n%10d\n", D);

	sani.d = 100.;
	sani.d = 10.;
	sani.d = .00000000000001;
	printf("%zu\n", sani.ui);
	return 0;
}
