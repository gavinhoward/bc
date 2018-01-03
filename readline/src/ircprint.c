#include <readline/readline.h>

void ircprint(char *l, size_t len, char *prompt, size_t plen)
{ 
	size_t i; 
	size_t real = 0; 
	size_t off = 0;
	size_t half;

	if ( len )
	{
		hglb.w = (hglb.w - 1 - plen);

		half = (hglb.w / 2);
	
		if ( len > (hglb.w) )
			off = len - hglb.w;
	}
	/* clear character attributes */
	write(1, T_CLRCHARS, T_CLRCHARS_SZ);
	
	/* clear the line */
	write(1, "\r", 1);
	write(1, T_CLRCUR2END, T_CLRCUR2END_SZ);

	/* write the prompt out */
	write(1, prompt, plen);

	if ( len )
	{
		if (hglb.laro >= half) 
			real = (hglb.laro - half); 

		/* write the user's line out */
		if (len < hglb.w)
	       	 	write(1, l, len);
		else if ( len - hglb.laro <= half ) 
			write(1, l, hglb.w);
		else
			write(1, l + off - real, hglb.w);

		if (hglb.laro)
		{
			/* split */
			if (len - hglb.laro > half )
			{
				for (i=0; i < hglb.laro && i < half; ++i) 
			       		write(1, T_CURSBK1COL, T_CURSBK1COL_SZ);
			}
			/* don't split */
			else if ( len < hglb.w )
			{
				for (i = 0; i < hglb.laro; ++i)
					write(1, T_CURSBK1COL, T_CURSBK1COL_SZ);
			}
			else
			{
				for (i = len - hglb.laro ; i < hglb.w; ++i) 
	      				write(1, T_CURSBK1COL, T_CURSBK1COL_SZ);
			} 
		}
	}
} 
