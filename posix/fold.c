#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <locale.h>
/* 
	Copyright 2015, C. Graff  "fold"
*/

void catfold(FILE *, int *);

int main(int argc, char *argv[])
{ 
	if (!setlocale(LC_CTYPE, ""))
		fprintf(stderr, "unable to set locale\n");

	int o, opt[3] = { 80, 0, 0 }; /* width, bytes, spaces */ 

	while ((o = getopt(argc, argv, "bsw:")) != EOF) 
		switch(o) {
			case 'b': 
				opt[1] = 1;
				break;
			case 's': 
				opt[2] = 1;
				break;
			case 'w':
				//opt[0] = atoi(optarg);
				opt[0] = strtod(optarg, NULL);
				break; 
			default:
				return 1;
		} 
	
	argv += optind;
	argc -= optind;

	if ( argc == 0 )
		catfold(stdin, opt);

	while ( *argv) 
		catfold(fopen( *(argv++) , "r"), opt); 
	
	return 0; 
}

void catfold(FILE *fp, int *opt)
{ 

 	size_t i, j;
	wchar_t wc, buf[BUFSIZ]; 
	int column, hold, w, pass, splitable; 
	
	if (!(fp))
		return;

	i = column = hold = 0;

	splitable = -1;

	while ((wc = getc(fp)) >= 0) 
	{
		buf[i++] = wc; 
		pass = 0;
		if (wc == '\n') 
		{ 
			column = 0;
			pass = 1;
			splitable =-1;
		}
		else {
			if (opt[1])
				w = 1;
			else if (wc == '\t') 
			{ 
				w = 8 - (column % 8); 
				if (w > opt[0] - column)
					w = 8;
			}
			else if (wc == '\r') 
				column = w = 0;
			else if (wc == '\b') 
			{
				if (column)
				       column--;
				w = 0;
			}
			else
				w = 1; 

			if (column && w > opt[0] - column)
			{
				printf("\n");
				column -= hold;
				splitable = -1;
			}
			column += w; 
			if (opt[2] && (wc == '\t' || wc == ' ')) 
				splitable = 1;
		}
		if (splitable || pass == 1)
		{ 
			for (j = 0; j < i; j++)
				printf("%c", (buf[j]));
			i = 0;
			hold = column; 
			splitable = -1; 
		} 
	}
	fclose(fp); 
} 

