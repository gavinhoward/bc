#include <readline/readline.h>

size_t szstrcatn(char *dest, char *src, size_t limit, size_t offlen)
{
	/*
		Type of function that concatenates two strings
		and returns the number of bytes processed. "offlen"
		must contain the value of any offset to dest. 
	*/
	size_t destlen = strlen(dest);
	size_t i = 0;

	if ( offlen >= limit )
		return 0;

	for (i=0;destlen < (limit - offlen) - 1 && src[i] != 0;++i,++destlen) 
		dest[destlen] = src[i];
	
	dest[destlen] = 0;

	return destlen;
}

void greadprint(char *l, size_t len, char *prompt, size_t plen)
{
        static size_t deep = 0;
	size_t i;
	size_t j;
	size_t y;
	size_t z; 
	char s[READLINE_LIMIT];
	size_t a = 0;
	size_t limit = READLINE_LIMIT - 1; 
        
	s[0] = 0;
	
	/* calculate row pos and shove lines up */ 
        for (i = plen, j = len; j > (hglb.w - i) + 1; i = 0)
        {
		if ( deep == 0 ) /* if user is scanning don't shove lines */
			a += szstrcatn(s + a, T_CURSUP1ROW, limit, a);
		else if ( deep > 0 )
			--deep;
		j -= ( hglb.w - i);
        }
   
	/* clear character attributes */
	a += szstrcatn(s + a, T_CLRCHARS, limit, a);

        /* clear the line */
	a += szstrcatn(s + a, "\r", limit, a);

	/* Dump any attribute manipulations before the prompt */
	write(1, s, a);
	a = 0;
	s[a] = 0;

        /* write the prompt out to stderr as a convention */
	write(2, prompt, plen);

        /* write the user's line out */
	l[len] = 0;
	a += szstrcatn(s + a, l, limit, a);

        /* clear to the end of the line ( ' ' .. + '\b' .. works too) */
	a += szstrcatn(s + a, T_CLRCUR2END, limit, a);

        /* walk the cursor backward to the user's position */
        for (i=0, y = len + plen; i < hglb.laro ;++i,--y)
        { 
		a += szstrcatn(s + a, T_CURSBK1COL, limit, a);
		if ( y % (hglb.w) == 0 )
		{ 
			a += szstrcatn(s + a, T_CURSUP1ROW, limit, a);
			for (z=0; z < hglb.w;++z)
				a += szstrcatn(s + a, T_CURS4D1COL, limit, a); 
			++deep;
		}
        }

	write(1, s, a);
}
