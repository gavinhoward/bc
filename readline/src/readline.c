#include <readline/readline.h>

char *readline(char *prompt)
{ 
	size_t promptlen = strlen(prompt);
	size_t len = 0;
	char *l;
	if (!(l = malloc(READLINE_LIMIT)))
		return NULL;
	
	hglb.r = 1; 
	
	while ( hglb.r )
	{
		determinewin(); 
		greadprint(l, len, prompt, promptlen); 
       		len = greadgetch(l, prompt, promptlen); 
	}

	return l;
}
