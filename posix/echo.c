#include <stdio.h> 

/* Copyright 2015, C. Graff  "echo" */

void prtoctesc(char **, int []); 

int main(int argc, char *argv[])
{ 
	(void)argc;

	int opt[5] = { 0, 0, 1, 0, 0}; // (bool)-e, (bool)-n, (bool), increment, cache 
	++argv;
	while ( *argv )
	{ 
		if ( **argv == '-'  ) 
		{ 
			while ( *(*argv + 1 ) != '\0' && opt[2] == 1)
			{ 
				if ( *(*argv  + 1) == 'e' ) 
					opt[0] = 1; 
				else if ( *(*argv + 1) == 'n' ) 
					opt[1] = 1; 
				else 
					opt[0] = opt[1] = opt[2] = 0; 
				++*argv;
			} 
		} 
		else
			 break;

		if ( opt[2] == 1 ) 
			++argv; 

		if ( opt[2] == 0 )
		{
			while ( **argv)
                            --*argv; 
			++*argv;
			break; 
		}
	} 
          
	while ( *argv ) 
	{ 
		while ( **argv != '\0' )
		{ 
			if ( **argv  == '\\' && opt[0] == 1)
                        {
                                switch (*(*argv +1) ){
                             		case 'a' :
                                                printf("\a");
                                      		++*argv;
                                                break;
                                        case 'b' :
                                                printf("\b");
                                       		++*argv;
                                                break;
                                        case 'c' :
                                                return 0;
                                        	++*argv;
                                                break;
                                        case 'f' :
                                                printf("\f");
                                         	++*argv;
                                                break;
                                        case 'n' :
                                                printf("\n");
                                          	++*argv;
                                                break;
                                        case 'r' :
                                                printf("\r");
                                           	++*argv;
                                                break;
                                        case 't' :
                                                printf("\t");
                                            	++*argv;
                                                break;
                                        case 'v' :
                                                printf("\v");
                                             	++*argv;
                                                break;
                                        case '\\' :
                                                printf("\\");
                                              	++*argv;
                                                break; 
					case '0' : 
						prtoctesc(argv, opt);
                                                break;
					default:
						break;
				}
			}
			else 
				printf("%c", **argv);
			++*argv; 
		} 
		++argv;
		if ( *argv )
			printf(" "); 
	} 
	if ( opt[1] != 1 )
		printf("\n");
	return 0;
} 

void prtoctesc(char **argvv, int opt[])
{
	opt[4] = opt[3] = 0;
	while ( (*(*argvv +2)) >= '0' && (*(*argvv +2)) <= '7' && opt[3] < 3 )
	{
		opt[4] = (opt[4] << 3) | (*(*argvv +2) - '0');
		++*argvv;
		++opt[3];
	}
	++*argvv;
	printf("%c", opt[4]);
}

