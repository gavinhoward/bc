#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h> 
#include <limits.h>

/* Copyright 2015, C. Graff  "which" */
void which(char *, int);

int main (int argc, char *argv[])
{ 
	int o, all; 
	all = 0;
	while ((o = getopt (argc, argv, "ah")) != -1)
                switch (o) {
                        case 'a':
				all = 1;
				break;
			case 'h':
				printf("Usage: which [OPTION] [FILE(S)]\n");
				break;
			default:
				break;
                }

	argv += optind;
        argc -= optind;

	while ( *argv ) 
		which(*argv++, all); 

	return 0;
}

void which(char *argv, int all)
{
        char word[(PATH_MAX + 1) * 2];
        char *token;

        token = strtok(getenv("PATH"), ":");

        while ( token != NULL )
        {
                sprintf(word, "%s/%s",token, argv);
                if ( access(word, X_OK) == 0 )
                {
                        printf("%s\n", word);
                        if ( all != 1 )
                                break;
                }
                token = strtok(NULL, ":");
        }
}

