#include <time.h>
#include <stdlib.h> 
/*
	Copyright, C. Graff, `sleep'
*/

int main(int argc, char *argv[])
{ 
	struct timespec timea, timeb; 
	double j;
	long k; 

	while ( *++argv && argc ) 
	{
		j = (atof(*argv) * 1000000000);
		while ( **argv != '\0' ) 
			++*argv; 

		switch ( *--(*argv)){
			case 'm': 
				j *= 60; 
				break;
			case 'h': 
				j *= 60*60; 
				break;
			case 'd': 
				j *= 60*60*24; 
				break;
			default:
				break;
		} 
		k = j;
		timea.tv_sec = (k / 1000000000);
		timea.tv_nsec = (k % 1000000000);
		nanosleep(&timea , &timeb); 
	}
	return 0;
} 

