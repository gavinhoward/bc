#include <stdio.h> 
#include <readline/readline.h>
#include <readline/history.h>

int main(void)
{
        char *userline = malloc(BUFSIZ * sizeof(char));
	
        while ( 1 )
        {
		userline[0] = '\0'; 
                ircline(userline, "[ircline]>> ", 12);
		printf ("Finished line was: \n%s\n", userline);
		add_history(userline);
        }
        return 0;
}


