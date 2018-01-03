#include <readline/readline.h>

size_t ircline(char *l, char *prompt, size_t promptlen)
{ 
	size_t len = 0;
	hglb.r = 1; 
	
	while ( hglb.r )
	{ 
		determinewin();
		ircprint(l, len, prompt, promptlen);
		len = greadgetch(l, prompt, promptlen);
	}

	return len;
}
