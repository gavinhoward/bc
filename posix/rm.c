#include <stdio.h> 
#include <dirent.h>
#include <string.h> 
#include <unistd.h> 
#include <limits.h>
#include <stdlib.h>


/* Copyright 2015, C. Graff  "rm" */



int query(char *, char *, int *); 
int remove_pattern(char *, size_t, int *);

int main(int argc, char *argv[])
{ 
	int recur = 0; 
	int opt[2] = { 0, 0 };

        ++argv;
        --argc;
        while ( *argv && **argv == '-' )
        { 
                while ( *(*argv + 1 ) != '\0')
                {
                        if ( *(*argv  + 1) == 'r' )
				recur = 1;
                        else if ( *(*argv + 1) == 'R' )
				recur = 1;
			else if ( *(*argv + 1) == 'f' ) 
				opt[1] = 1; 
			else if ( *(*argv + 1) == 'i' )
                              	opt[0] = 1;
                        else if ( *(*argv + 1) == 'h' )
			{
				fprintf(stderr, "rm -Rrfih\n");
				return 0;
			}
                        else
				return 0; 

                        ++*argv;
                }
                ++argv;
                --argc;
        }

	while (*argv && recur == 1)
        { 
		remove_pattern(*argv, strlen(*argv), opt);
		if (query("Remove :", *argv, opt))
			remove(*argv);
		argv++;
        }

        if ( recur == 0 )
                while ( *argv )
                        remove(*argv++);

        return 0;
}


int remove_pattern(char *path, size_t len, int *opt)
{
        DIR *dir;
        struct dirent *dentry;
	char *spath = malloc(1);
	size_t dlen = 0; 

        if ( ( dir = opendir(path) ) )
        {
                dentry = readdir(dir);
                while ( dentry )
                {
			dlen = strlen(dentry->d_name);
			spath = realloc(spath, len + dlen + 2);
			if (!(spath))
				return -1;
			len = sprintf(spath, "%s/%s", path, dentry->d_name);

			if ( strcmp( ".", dentry->d_name) &&
                            strcmp( "..", dentry->d_name))
			{
				if ( dentry->d_type == DT_DIR )
                        	{ 
					if (query("Recurse into ", spath, opt))
                               	 		remove_pattern(spath, len, opt);
                        	} 
				if (query("Remove ", spath, opt))
					remove(spath);
			}
                        dentry = readdir(dir); 
                }
		free(spath);
		closedir(dir); 
        }
	else 
		return 0;

	return 1; 
}


int query(char *message, char *file, int *opt)
{
	char buf[10];
	if ( opt[1] == 1 ) /* -f  */
		return 1;

	if ( access(file, W_OK) != 0 || opt[0] == 1 ) /* -i */
	{ 
		if ( access(file, W_OK) != 0 ) 
			printf("%s write protected file :%s  ", message, file);
		else 
			printf("%s :%s  ", message, file);
		fgets(buf, 10, stdin);
		if ( buf[0] == 'y' )
			return 1;
		else 
			return 0;
	} 

	return 1;
}

