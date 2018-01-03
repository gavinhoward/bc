#include <readline/readline.h>

size_t greadgetch(char *l, char *prompt, size_t plen)
{
        static size_t len = 0;
	static size_t ret = 0;
        int c;
	char *line = NULL;
	
	char pat[4096] = { 0 };
	size_t z = 0;
	size_t y = 0;
	size_t point = 0; 
	c = readchar(); 
        
        switch (c) { 
	case K_ESCAPE:
		c = readchar();
		switch (c) {
		case '[':
                        c = readchar();
			switch (c) { 
			case 'A': /* arrow up */
				if ( hglb.c > 0 )
				{
					--hglb.c; 
					memcpy(l, hist[hglb.c].line, hist[hglb.c].len);
					len = hist[hglb.c].len;
					hglb.laro = 0;
				}
				break;
			case 'B': /* arrow down */
				if ( hglb.c < hglb.t)
				{
					++hglb.c;
					if ( hglb.c < hglb.t)
					{
						memcpy(l, hist[hglb.c].line, hist[hglb.c].len);
						len = hist[hglb.c].len;
					}else
						len = 0;
					hglb.laro = 0;
				}
				break;
			case 'C': /* right arrow */ 
				if ( hglb.laro > 0 )
					--hglb.laro;
                                break;
			case 'D': /* left arrow */ 
				if ( hglb.laro < len )
					++hglb.laro; 
				break; 
			case 'H': /* Home */
				hglb.laro = len;
                                break;
			case '5': /* page up ( not used ) */ 
				c = readchar(); 
				break;
			case '6': /* page down ( not used ) */ 
				c = readchar();
				break;
			}
		}
		return len;
	case '\t':
		l[len] = 0;
		pat[0] = 0;
		greadprint(l, len, prompt, plen); 
		if (l[0] == '/' && l[1] != '/' )
		{
			memcpy(pat, l , 1);
			pat[1] = 0; 
			if ((line = find_pattern(pat, l +1, strlen(l+1))))
	                        sprintf(l + 1, "%s", line);
			goto end;
		}
		for (point=0,y=0,z=len ;z > 0;--z ,++y)
		{
			if (l[z] == '/' && point == 0)
				point = y;
			if (l[z] == ' ' )
				break;
		}
		memcpy(pat, l + (len -y) + 1, y + 1); 
		pat[(y-point)] = 0; 
		
		if ((line = find_pattern(pat, l + (len-point) + 1, strlen(l + (len -point) + 1)))) 
			sprintf(l + (len -y) + 1, "%s", line); 
		end:
	        len = strlen(l);
	        free(line);
		l[len] = '\0';
		return len;
		break;
        case '\n':
		write(1, "\n", 1);
		l[len] = '\0';
		ret = len;
		len = 0;
		hglb.r = 0;
		hglb.laro = 0;
                return ret;
	case K_BACKSPACE:
		if ( len == 0 )
			return len;
		if ( hglb.laro ) 
		{
			z = (len - hglb.laro);
                        memmove(l + z - 1, l + z, hglb.laro);
		}
		l[--len] = '\0';
		return len; 
        default:
		/* this needs to check that memmove() can't 
		   go out of bounds
		*/
		if ( hglb.laro )
		{
			z = (len - hglb.laro);
			memmove(l + z + 1, l + z, hglb.laro);
			l[z] = c; 
		}
		else { 
			l[len] = c;
		} 
		if ( len < READLINE_LIMIT -1)
			l[++len] = '\0'; 
	
                break;
        }

	l[len] = '\0';
	return len;
}
