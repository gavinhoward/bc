#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

/* stat() */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h> 
#include <limits.h> 

/* waitpid() */
#include <sys/wait.h>

/* greadline() */
#include <sys/ioctl.h>
#include <termios.h>
#include <stdint.h>
#include <readline/readline.h>

/*
	(Copyright) 2014-2017, "shell.c", CM Graff
*/

#define GSHPROMPT	"gsh>> "

struct glb
{ 
	size_t count;	/* num of commands */
	size_t len;	/* input array len */
	int inascript;	/* script mode 	*/
	int testparse;	/* -t mode 	*/
	int cmode;	/* -c mode 	*/
}glb = { 0, 0, 0, 0, 0};


struct commands
{
	/* commands */
	char *argv[1024]; 
	size_t argc;
	/* io */
	char *outfp;
	char *infp;
	int outflags;
	int in;
	int out;
	/* pipes */
	int piped;
	/* processes */
	pid_t pids;
	int bg;
	int err;
	/* logic */
	int boole;	/* controlled by the preceding command */

	int ifa;	/* This command is a master -1 off -- 0 false -- 1 true */
	int thena;	/* This command is a true slave to last master -1 off -- 0 - N master */
	int elsea;	/* This command is a false slave to last master -1 off -- 0 - N master */
	int fia;	/* shift last master status out */
	
} *cmds;

/* functions */
int cd(char *);
int destroy(int, char *, char *);
int execute();
int exitsh(char *);
static void initialize(size_t);
int parse(char *);
void verbosity(void);

int main(int argc, char **argv)
{ 
	int o = 0; 
	int ret = 0; 
	int let = 0;
	int fp = 0; 
	char *l;

	while ((o = getopt (argc, argv, "ct")) != -1)
                switch (o) {
                        case 'c': 
				glb.cmode = 1;
				break;
			case 't':
                                glb.testparse = 1;
                                break;
			default:
				break;
		}
	argv += optind;
        argc -= optind;

	if (!(cmds = malloc(sizeof(struct commands) * 1)))
		return 1;

	if ( glb.cmode  != 1 )
	{
		if(!(l = malloc(sizeof(char) * PATH_MAX)))
			return 1;
		memset(l, 0, PATH_MAX);
	}

	if ( glb.cmode  == 1 )
		l = argv[0]; 

    	while ( 1 )
    	{ 
		glb.count = 0;
		/* read a script into the primary input array */
		if ( argv[0] != NULL && glb.cmode  == 0 )
		{ 
			glb.inascript = 1;
  			if ( (fp = open ( argv[0], O_RDONLY )) == -1 ) 
				destroy(0, "File not found\n", l); 
			glb.len = lseek(fp, 0, SEEK_END); 
			lseek(fp, 0, SEEK_SET); 
			if ( glb.len  > PATH_MAX ) 
			{
				if(!(l = realloc(l, sizeof(char) * glb.len + 1)))
					return 1;
				memset(l, 0, glb.len + 1);
			}
			read(fp, l, glb.len);
			close(fp);
		} 
		/* get user input */
		else if (!argc) 
		{ 
			l = readline(GSHPROMPT);
			add_history(l);
			++glb.count;
		}

		/* interpret the primary input array */
		ret = parse(l);

		if ( glb.testparse == 1 )
			continue;
				
		if (ret == 0)
			let = execute();
		
		if (let >= 1 || glb.cmode || glb.inascript)
			destroy(let, "", l);
	} 
	
	return 0;
}

int cd(char *argv)
{
	if ((chdir(argv)) > -1 )
		return 0;
	return 1;
}

int destroy(int exitno, char *message, char *l)
{ 
	write(2, message, strlen(message));
	if ( glb.cmode == 0)
		free(l);
	free(cmds);
	//free(hist);
	exit(exitno); 
}

int exitsh(char *string)
{
        int ret = 0;
        if ( string != NULL )
                if ( (ret = atoi(string)) > -1 )
                        return ret;
        return 1;
}


int execute()
{ 
	int fildes[2]; 
	mode_t mode;
	size_t k; 

	k = 0;
	mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH; 


	for ( k = 0; k < glb.count ; k++ )
	{ 

		/* Boolean logic */
		/*_______________*/

		/* share bools across pipes */
		if (k && cmds[k].piped == 1 && cmds[k - 1].boole > -1 ) 
			cmds[k].boole = cmds[k - 1].boole; 

		/* ||  */
		if ( k && cmds[k - 1].err == 0 && cmds[k - 1].boole == 1 )
			continue;
		
		/* && */
		if ( k && cmds[k - 1].err != 0 && cmds[k - 1].boole == 0 )
			continue; 
	
		/* io */
		/*----*/

		/* < */
		if ( cmds[k].infp != NULL ) 
		{
			if( (cmds[k].in = open(cmds[k].infp, O_RDONLY) ) == -1 )
                	{
                        	//fprintf(stderr,"'%s' File not found.\n", cmds[k].infp);
				write(2, "'", 1);
				write(2, cmds[k].infp, strlen(cmds[k].infp));
				write(2, " ' File not found.\n", 19); 
                        	return 0;
                	} 
		}

		/* >, >> */
		if ( cmds[k].outfp != NULL ) 
		{ 
			if((cmds[k].out = open(cmds[k].outfp, cmds[k].outflags, mode)) == -1)
                	{
				write(2, " Permission denied?\n", 20);
                        	return 0;
                	} 
		} 

		/* | */
		if ( cmds[k].piped == 1 && k < glb.count - 1)
		{ 
                        if ( pipe(fildes) == -1 )
                                return 0; 
			cmds[k + 1].in = fildes[0];
                        cmds[k].out = fildes[1];
                }
		
		/* builtins */
		/*----------*/
		if ( cmds[k].argv[0] )
                {
			if (strcmp (cmds[k].argv[0], "cd") == 0 )
			{ 
				cmds[k].err = cd(cmds[k].argv[1]);
				continue; 
			} 
			else if (strcmp (cmds[k].argv[0], "exit") == 0 )
  	              	{ 
				return exitsh(cmds[k].argv[1]); 
                	} 
		}
	

		/* Normal commands */ 
		/*-----------------*/
		cmds[k].pids = fork();

		if (cmds[k].pids < 0)
        		return 0; 
		/* parent */
		else if (cmds[k].pids  > 0)
		{ 
			; 
		}
		/* child */
		else 
		{ 
			if ( cmds[k].in != -1 )
				dup2(cmds[k].in, STDIN_FILENO); 
			if ( cmds[k].out != -1  ) 
				dup2(cmds[k].out, STDOUT_FILENO); 
			/* check that this is not a NULL pointer, yes lame */
			if ( cmds[k].argv[0] )
			{
				execvp(cmds[k].argv[0], cmds[k].argv);
               			write(2, "gsh: ", 6);
				write(2, cmds[k].argv[0], strlen(cmds[k].argv[0]));
			}
			else
				write(2, "gsh: ", 6);
			write(2, " not found\n", 11);
                        
                        _exit(1); 
		} 
		
		
		if ( cmds[k].bg != 1 )
		{ 
			waitpid(cmds[k].pids, &cmds[k].err, 0); 
			
                	if( cmds[k].out != -1 )
                        	close(cmds[k].out); 
                	if( cmds[k].in != -1 )
                        	close(cmds[k].in); 
		}
		else
		{ 
			fprintf(stderr,"[%d]\n", cmds[k].pids); 
		} 
	} 
	glb.count = 0;
	return 0;
}

void verbosity(void)
{
        size_t i = 0;
        size_t j = 0;

	fprintf(stderr, "Total command vectors: %zu \n",glb.count);
        
        for (i = 0; i < glb.count ;i++)
        {
		fprintf(stderr, "\n");
		
                for ( j = 0; j <= cmds[i].argc ; ++j) 
                        fprintf(stderr, "%s\t\targv           %zu\n", cmds[i].argv[j], j); 
                if ( cmds[i].infp != NULL )
                        fprintf(stderr, "%s\t\tin  <   vector %zu\n", cmds[i].infp, i);
                if ( cmds[i].outfp != NULL ) 
                        fprintf(stderr, "%s\t\tout >   vector %zu\n", cmds[i].outfp, i); 
        }
	if ( glb.inascript)
		exit(1);
}

static void initialize(size_t i)
{
        if (!(cmds = realloc(cmds, sizeof(struct commands) * (i + 1))))
	{
		perror(" realloc()" );
		exit(1);
	}

	cmds[i].argv[0] = NULL;
	cmds[i].argc = 0; 

	cmds[i].outflags = O_APPEND|O_RDWR|O_CREAT;
        cmds[i].outfp = NULL;
        cmds[i].infp = NULL;
        cmds[i].in = -1;
        cmds[i].out = -1;
        
        cmds[i].bg = 0;
        cmds[i].piped = 0;
        cmds[i].err = 0;
        cmds[i].boole = -1;

	cmds[i].ifa = -1;
	cmds[i].thena = -1;
	cmds[i].elsea = -1;
	cmds[i].fia = -1;
}

int parse(char *l)
{

	/* grammar */
	int atoken = 0;
	int awhite = 1;
	int aredir = 0;
	int aquote = 0; 
	/* total commands */
	size_t c = 0;
	
	/* remain at the start of a command sequence */
	char *last;
	
	/* intialialize a data structure member and the pointer to input */
	initialize(c);
        last = l;

	/* discover tokens and commands */
        while ( *l )
        {
		if (*l == '"')
                { 
			*l = '\0';
			last = ( l + 1);
			/* routing switch for quotes */
			if ( aquote == 0 ) 
				aquote = 1; 
			else
				aquote = 0;
		}
		if (*l == ';')
                {
                        *l = '\0';
                        initialize(++c);
                        last = ( l + 1 ); 
			atoken = 1;
			awhite = 0;
			aredir = 0;
                }
		else if (*l == '\n' && *(l + 1) != '\n')
                { 
                        *l = '\0'; 
                       	initialize(++c);
                        last = ( l + 1); 
			atoken = 1;
                        awhite = 0;
			aredir = 0;
                }
		else if (*l == '\n')
		{
			*l = '\0';
			atoken = 1;
			awhite = 0;
			aredir = 0;
		}
                else if ( *l == '|' && *(l + 1) == '|' )
                {
                        cmds[c].boole = 1;
                        *l = '\0';
                        ++l;
                        *l = '\0';
                        initialize(++c);
                        last = ( l + 1 ); 
			atoken = 1;
			awhite = 0;
			aredir = 0;
                }
                else if (*l == '|')
                {
                        cmds[c].piped = 1;
                        *l = '\0';
                        initialize(++c);
                        last = (l + 1); 
			atoken = 1;
                        awhite = 0;
			aredir = 0;
                }
                else if (*l == '&' && *(l + 1)  == '&')
                {
                        cmds[c].boole = 0;
                        *l = '\0';
                        ++l;
                        *l = '\0';
                        initialize(++c);
                        last = (l + 1); 
			atoken = 1;
			awhite = 0;
			aredir = 0;
                }
                else if (*l == '&')
                {
                        cmds[c].bg = 1;
                        *l = '\0';
                        initialize(++c);
                        last = ( l + 1 ); 
			atoken = 1;
			awhite = 0;
			aredir = 0;
                }
                else if (*l == '>' && *(l + 1) == '>')
                {
                        *l = '\0';
                        ++l;
                        *l = '\0';
                        cmds[c].outflags = O_APPEND|O_RDWR|O_CREAT;
			while ( *( l + 1 ) == ' ') /* strip whitespace */
                                *l++ = '\0';
                        cmds[c].outfp = ( l + 1 );
			atoken = 1;
			awhite = 0;
			aredir = 1;
                }
                else if (*l == '>')
                {
                        *l = '\0';
                        cmds[c].outflags = O_TRUNC|O_RDWR|O_CREAT;
			while ( *( l + 1 ) == ' ') /* strip whitespace */
				*l++ = '\0';
                        cmds[c].outfp = ( l + 1 );
			atoken = 1;
			awhite = 0;
			aredir = 1;
                }
                else if (*l == '<')
                {
                        *l = '\0';
			while ( *( l + 1 ) == ' ') /* strip whitespace */
                                *l++ = '\0'; 
                        cmds[c].infp = ( l + 1 ); 
			atoken = 1;
			awhite = 0;
			aredir = 1;
                }
		else if ( *l != ' ' && *l != '\t' )
		{
			if (aredir == 0 )
			{
				cmds[c].argv[cmds[c].argc] = last;
				cmds[c].argv[cmds[c].argc + 1] = NULL;/* speed up + hack */
			}
			awhite = 0;
			atoken = 0; 
		}
		else if ( *l == ' ' || *l == '\t' )
                { 
			if ( atoken == 0 && awhite == 0 && aredir == 0 && aquote == 0)
				cmds[c].argc++;
			
			if (aquote == 0)
			{
				awhite = 1;
                        	*l = '\0';
				last = ( l + 1 );
			}
			//last = ( l + 1 );
                }
                ++l;
        }
	glb.count += c;

	/* An exception could be added here to check for NULL ultimate args */
	
	if ( glb.cmode )
		++glb.count;

	if ( glb.testparse )
		verbosity();
	
        return 0; 
}


