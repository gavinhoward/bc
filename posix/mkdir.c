#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h> 
#include "cutils.h"

/* Copyright 2015, C. Graff  "mkdir" */ 

void mkdir_verbose(char *, mode_t, int);
void drmake(char *, mode_t, int, int);

int main (int argc, char *argv[]) 
{ 

	mode_t mode = 0755; 
	int i, j, k;
	i = j = k = 0;

	while ((i = getopt (argc, argv, "hvpm:")) != -1) 
		switch (i) {
			case 'm': 
				mode = strtol(optarg, NULL, 8);
				break;
			case 'p': 
				k = 1; 
				break;
			case 'v': 
				j = 1;
				break;
			case 'h': 
				cutilerror("Usage mkdir -pvm:\n", 0); 
				break; 
			default: 
				break;
		}

        argv += optind;
        argc -= optind;

	while ( *(argv) )
		drmake(*argv++, mode, j, k);

	return 0;

}
void drmake(char *array, mode_t mode, int verbose, int parents)
{
	size_t len, index = 0; 
        len = index = strlen(array);
        if ( parents == 0 )
        { 
                mkdir_verbose(array, mode, verbose); 
              	return;
        }

        while (--index - 1) 
                if ( array[index] == '/' )
                        array[index] = '\0'; 

        for ( ;--len - 1 ; index++)
        {
                if ( array[index] == '\0' )
                { 
                        mkdir_verbose(array, mode, verbose);
			while ( array[index] == '\0' ) 
                       		 array[index++] = '/'; 
                } 
        }
}

void mkdir_verbose(char *string, mode_t mode, int verbose)
{
	mkdir(string, mode);
        if ( verbose == 1 )
                printf("Making directory: %s  mode: %o\n", string, mode);
}

