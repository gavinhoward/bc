#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <regex.h> 
#include <ctype.h>
#include <limits.h>
//#include "../lib/string.h"
//#include "../lib/unistd.h"



/* Copyright 2015, C. Graff  "grep" */


struct opt {
        int o[12]; // EFcefilnqsvx + 1
} opt = {{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  }}; 

void grep_exit(char *, int);
int strindex(char [], char []); 
void grep(char **, int, char **, int, int);

int main(int argc, char **argv)
{ 
	if ( argc < 2 ) 
		return 1;
	size_t len = PATH_MAX;
        char *pats[100]; 
	int o;
        int i = 0; 
	int cflags = 0;
	FILE *fp;
	char line[PATH_MAX + 1];

	while ((o = getopt (argc, argv, "EFce:f:ilnqsvxh")) != -1)
                switch (o) { 
                        case 'E': /* Extended regex */ 
				cflags |= REG_EXTENDED; 
				break; 
                        case 'F': /* No regex */ 
				opt.o[2] = 1;
				break;
                        case 'c': /* Only count lines */ 
				opt.o[3] = 1; 
				break;
                        case 'e': /* Pattern list */ 
				pats[i] = malloc(len);	
				if (!(pats[i]))
                                        return 0;
                                memset(pats[i], 0, len);
                                strcpy(pats[i], optarg);
                                ++i;
                                opt.o[4] = 1; 
				break; 
                        case 'f': /* Pattern file */ 
				fp = fopen(optarg, "r"); 
				if ( !(fp) )
					break;
                		while ((fgets(line, BUFSIZ, fp)) != NULL)
				{
					line[strlen(line)-1] = '\0';
				 	pats[i] = malloc(len);
					if (!(pats[i]))
						return 0;
                                	memset(pats[i], 0, len);
                                	strcpy(pats[i], line);
					++i; 
			  	}
				fclose(fp);
				opt.o[5] = 1; 
				break; 
                        case 'i': /* Case insensitive */ 
				cflags |= REG_ICASE; 
				opt.o[6] = 1; 
				break; 
                        case 'l': /* File names only */ 
				opt.o[7] = 1; 
				break; 
                        case 'n': /* Number lines */ 
				opt.o[8] = 1 ; 
				break;
                        case 'q': /* Exit 0 if match */ 
				opt.o[9] = 1; 
				break;
                        case 's': /* No error messages */ 
				opt.o[10] = 1; 
				break; 
                        case 'v': /* Non-matching lines */ 
				opt.o[11] = 1; 
				break; 
                        case 'x': /* Rntire fixed string */
				opt.o[12] = 1; 
				break; 
			case 'h': 
				grep_exit("Usage grep -EFcefilnqsvxh\n", 0); 
				break;
                        case '?': 
				grep_exit("Invalid option\n", 1); 
				break;
			default: 
				break;
                }
        argv += optind;
        argc -= optind; 
		
	if ( i == 0)
	{ 
		pats[0] = malloc(len);
		if (!(pats[0]))
                        return 0;
                memset(pats[0], 0, len);
                strcpy(pats[0], argv[0]);
		++argv;
		--argc; 
		i = 1; 
	}

	grep(pats, i, argv, argc , cflags);
	return 0;
}

void grep(char **pats, int pnum, char **argvv, int argcc, int cflags)
{
	FILE *fp;
        char line[PATH_MAX + 1];
        regex_t re;
        regmatch_t rm[20]; 
	int c, truexit, p, good, linum, total;
	c = truexit = 0;
	
	while ( c < argcc || argcc == 0)
	{ 
		p = good = linum = total = 0; 

		if ( argcc == 0 ) 
			fp = stdin; 
		else 
			fp = fopen(argvv[c], "r");
	

        	while ((fgets(line, 1000, fp)) != NULL)
        	{
			line[strlen(line)-1] = '\0';
			p = good = 0; 
			++linum;

			while ( p < pnum )
			{ 
				if ( opt.o[12] == 1) // -x
				{ 
					if ( opt.o[6] == 0 )
						if (strcmp(line, pats[p]) == 0 )
						{	 
                                                	truexit = 1;
                                                	good = 1; 
                                        	} 
					if ( opt.o[6] == 1 )
						if (strcasecmp(line, pats[p]) == 0 )
                                        	{
                                                	truexit = 1;
                                                	good = 1;
                                        	} 
					++p;
                                        continue; 
				}
				if ( opt.o[2] == 1) // -F
				{ 
					if (strindex(line, pats[p]) >= 0)
					{ 
                                                truexit = 1;
                                                good = 1;
                                        }
					++p; 
				}
				if ( opt.o[2] == 0)
				{
					if ((regcomp(&re, pats[p], cflags)) != 0)
                				grep_exit("Regex Failed!\n", 0);
                			if (((regexec(&re, line, 2, rm, 0)) == 0)) 
					{
						truexit = 1;
                        			good = 1; 
					}
					++p;
		      			regfree(&re);
				}
			} 

			if ( opt.o[11] == 1 ) // -v
			{
                       	  	if ( good == 0 )
                                 	good = 1;
                         	else if ( good == 1 )
                                 	good = 0;
			}

			if ( opt.o[9] == 1 ) // -q
			{
				if ( good == 1 ) 
					exit (0);
			}
        
			if ( good == 1 )
			{ 
				if ( opt.o[3] == 1 ) // -c
				{
					++total;
					continue;
				}
				
				if ( argcc > 1 )
					printf("%s:", argvv[c]); 

				if ( opt.o[7] == 1 ) // -l 
				{
					printf("\n");
					break;
				}
				
				if ( opt.o[8] == 1 ) // -n 
					printf("%d:", linum); 

				printf("%s\n", line); 
			}
      	  	} 
		if ( opt.o[3] == 1 ) // -c 
		{
			if ( argcc > 1 ) 
				printf("%s:", argvv[c]);
			printf("%d\n", total); 
		} 
		++c;
		if ( argcc == 0 ) break;
	} 
	if ( truexit == 1 )
		exit (0);
	else 
        	exit (1);
} 

int strindex(char s[], char t[]) // -F
{
	int i, j, k;
	for (i = 0; s[i] != '\0'; i++) 
	{ 
		if ( opt.o[6] == 1 ) /* case insensitive */
			for (j=i, k=0 ; t[k]!='\0' && tolower(s[j])==tolower(t[k]); j++, k++)
                        	;
		if ( opt.o[6] == 0 ) /* case sensitive */
			for (j=i, k=0 ; t[k]!='\0' && s[j]==t[k]; j++, k++)
				; 
		if (k > 0 && t[k] == '\0')
			return i;
	}
	return -1;
} 

void grep_exit(char *message, int i)
{ 
	if ( opt.o[10] != 1 ) // -s
		fprintf (stderr, "%s", message);
	exit (i);
}

