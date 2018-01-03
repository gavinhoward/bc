#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h> 
#include <unistd.h> 
#include "cutils.h"


/* 
	Copyright 2015-2017, "tail.c", Christopher M. Graff
*/ 

void cattail(int, unsigned int *);

int main(int argc, char *argv[])
{ 
	int o;
	unsigned int opt[4] = { 0, 10, 0, 0};
	while ((o = getopt (argc, argv, "c:fn:")) != -1)
                switch (o) { 
			case 'f': 
				*(opt+3) = 1; 
                                break;
			case 'c': 
				*(opt) = 2; 
				if ((*optarg) == '-' )
                                {
                                        *(opt) = 2;
                                        ++optarg;
                                } 
				else if ((*optarg) == '+' )
                                {
                                        *(opt) = 3;
                                        ++optarg;
                                } 
                                *(opt+2) = atoi(optarg); 
                                break;
			case 'n': 
				*(opt) = 0; 
				if ((*optarg) == '-' )
				{ 
					*(opt) = 0;
					++optarg;
				} 
				else if ((*optarg) == '+' )
                                { 
					*(opt) = 1; 
                                        ++optarg;
                                } 
				*(opt+1) = atoi(optarg); 
				break; 
                        default: 
				break;
                }

        argv += optind;
        argc -= optind;
	
	if ( argc == 0 ) 
		cattail(STDIN_FILENO, opt);
        while  (*(argv))
		cattail(open(*argv++,O_RDONLY), opt);
	return 0; 
}

void cattail(int source, unsigned int *opt)
{ 
	size_t i, j, n, z, seekto;
	char buf[BUFSIZ];
	int *loci = cutilmalloc(sizeof(int));
	int compensate = 0;

	if ((source == -1))
		return;

	seekto = z = i = j = n = 0; 

	while (*(opt) != 3 && (i = read(source, buf, BUFSIZ)) > 0)
	{ 
		for (j = 0; j < i ;) 
		{
			if ( buf[j] == '\n' ) 
			{
				loci = cutilrealloc(loci, sizeof(int) * (n + 3));
				*(loci+n++) = z; 
			}
			++j;
			++z;
		} 
	}

	if ( buf[j - 1] != '\n' )
		compensate = 1;

        if ( *(opt) == 0 ) 
	{
		if ( n > *(opt+1) - 1) 
			seekto = loci[n - *(opt+1) ] + compensate;
		else
			seekto = loci[n] ; 
	}
	else if ( *(opt) == 1 )
	{ 
		if ( *(opt+1) == 1 )
			seekto = 0;
		else 
			seekto = loci[*(opt+1) - 2] +1; 
	} 
	else if ( *(opt) == 2 ) 
		seekto = z - *(opt+2); 
	else if ( *(opt) == 3 ) 
		seekto = *(opt+2) -1;

	lseek(source, seekto, SEEK_SET);
	i = 0;

	while ( 1 )
	{ 
		while ((i = read(source, buf, BUFSIZ)) > 0) 
               		write(STDOUT_FILENO, buf, i); 
	
		if ( *(opt+3) != 1 ) // not -f
			break;

		usleep(100);
	}
	free(loci);
	close(source);
}

