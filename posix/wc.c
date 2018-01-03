#include <stdio.h> 
#include <fcntl.h> 
#include <unistd.h> 
#include <stdlib.h>
#include <unistd.h>
#include <locale.h>

/* Copyright 2015, C. Graff  "wc" */ 

struct hd {
	int nl;
	int nw;
	int nc;
	int nb;
} hd = { 0, 0, 0, 0 };

void countsyms(int, int *); 
void wc_print(int *);
void wc_total(int *);


int main(int argc, char *argv[])
{ 

	int opt[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; 
	
	++argv;
        --argc;

        while ( *argv && **argv == '-'  )
        {
                while (*(*argv + 1 ) != '\0')
                {
                        if ( *(*argv  + 1) == 'l' ) 
				*(opt) = *(opt+1) = 1;
                        else if ( *(*argv + 1) == 'w' ) 
				*(opt) = *(opt+2) = 1;
                        else if ( *(*argv + 1) == 'c' ) 
				*(opt) = *(opt+3) = 1;
                        else if ( *(*argv + 1) == 'm' ) 
			{
				*(opt) = *(opt+4) = *(opt+9) = 1; 
			}
                        else
                                return 0;
                        ++*argv;
                }
                ++argv;
                --argc;
        } 

	if ( argc == 0 ) 
	{
		countsyms(STDIN_FILENO, opt);
		wc_print(opt);
		printf("\n");
	}
        while  (*(argv)) 
	{
		countsyms(open(*argv,O_RDONLY), opt); 
		wc_print(opt);
		printf("%s\n", *argv);
		++argv; 
	}
	
	if ( argc == 2 )
		wc_total(opt);
	
	return 0; 
}

void countsyms(int source, int *opt)
{ 
	if ( source == -1 )
                return;

	*(opt+5) = *(opt+6) = *(opt+7) = *(opt+8) = 0;

	size_t i, j, k; 
        i = j = k = 0; 
	char buf[BUFSIZ];

	while ((i = read(source, buf, BUFSIZ)) > 0) 
	{ 
		for (j = 0; j < i ; ) 
		{ 
			if ( *(buf+j) >= 0) 
			{
				++hd.nb;
				++*(opt+8); 
			}
	
			switch(*(buf+j)){ 
				case '\n' : 
					k = 0; 
					++*(opt+5); // newlines
					++hd.nl; // global newlines
					break;
				case '\r' : 
					k = 0;
					break;
				case '\f' : 
					k = 0;
					break;
				case ' '  : 
					k = 0;
					break;
				case '\t' : 
					k = 0;
					break;
				case '\v' : 
					k = 0;
					break;
				default : 
					if (k == 0)
					{ 
						++*(opt+6); // words
						++hd.nw; // global words
						k = 1; // state
					}
					break; 
			} 
			++j; 
		} 
		*(opt+7) += i;
		hd.nc += i;
	} 
	close(source); 
} 

void wc_print(int *opt)
{ 
	if ( *(opt) == 0 || *(opt+1) == 1 ) 
		printf("%6d  ", *(opt+5));
	if ( *(opt) == 0 || *(opt+2) == 1 ) 
		printf("%6d  ", *(opt+6));
	if ( *(opt) == 0 || *(opt+3) == 1 ) 
		printf("%6d  ", *(opt+7));
	if ( *(opt) == 1 && *(opt+9) == 1 )  
		printf("%6d  ", *(opt+8)); 
}

void wc_total(int *opt)
{
	if ( *(opt) == 0 || *(opt+1) == 1 )
                printf("%6d  ", hd.nl );
        if ( *(opt) == 0 || *(opt+2) == 1 )
                printf("%6d  ", hd.nw );
        if ( *(opt) == 0 || *(opt+3) == 1 )
                printf("%6d  ", hd.nc );
        if ( *(opt) == 1 && *(opt+9) == 1 )
                printf("%6d  ", hd.nb );
	printf("total\n");
}

