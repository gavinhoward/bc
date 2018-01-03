#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>

/* Copyright 2015-2017, C. Graff  "du" */

struct hold {
	off_t level[100]; 
	int depth; 
	off_t block;
	size_t lastd; 
} hold = { {}, 0, 1024, 0};  // this should be 512 for posix compat

#define HASH 10111
int durecurse(char *, size_t, int *);

void duerror(char *message, int i)
{
        fprintf(stderr, "%s", message);
        if ( i > -1 )
                exit (i);
}
size_t hash(size_t inode)
{
        return (inode + 31) % HASH;
}

size_t *hashtab[HASH];

int populatetab(size_t inode)
{
        size_t *np;

        for ( np = hashtab[hash(inode)] ; np != NULL ; free(np), np = NULL)
                if ( inode == *np )
                         return 0;

        if (!(np = malloc(sizeof(size_t))))
		duerror("malloc() failed", 1);
        *np = inode;
        hashtab[hash(inode)] = np ;
        return 1;
}

void destroytab()
{
        size_t i = 0;
        while (i < HASH)
        {
                free(hashtab[i]);
                hashtab[i] = NULL;
                ++i;
        }
}

int main (int argc, char *argv[]) 
{ 

	int i, opt[7] = { 0, 0, 0, 0, 0, 0, 0}; /* -aHkLsXl */
	struct stat sb;

	while ((i = getopt (argc, argv, "aHkLsxl")) != -1) 
		switch (i) {
			case 'a': 
				opt[0] = 1; 
				break; 
			case 'H': 
				opt[1] = 1; 
				break; // -H follow sumbolic links  .. not done
			case 'k': 
				opt[2] = 1;
				hold.block = 1024;
				break; 
			case 'L': 
				opt[3] = 1; 
				break;
			case 's': 
				opt[4] = 1; 
				break; 
                        case 'x': 
				opt[5] = 1; 
				break; 
			case 'l': 
				opt[6] = 1; 
				break; 
			default: 
				duerror("Usage: du -aHkLsxl\n", 0);
				break;
		} 
        argv += optind;
        argc -= optind;
	
	while(*argv) 
	{
		lstat(*argv, &sb); 
		hold.lastd = sb.st_dev;
		hold.level[1] = 0;

		if (durecurse(*argv, strlen(*argv), opt)) 
		{
			printf("%-5zu\t%s\n", hold.level[1] + 4, *argv); 
			destroytab(); 
		}
		++argv; 
	} 
	return 0;
} 


int durecurse(char *path, size_t len, int *opt)
{
	int c;
	size_t i = 0;
        DIR *dir;
        struct dirent *dentry; 
        char *spath = NULL;
	struct stat sb;
	size_t dlen = 0;
       
	++hold.depth;

        if ( ( dir = opendir(path) ) )
        { 
                dentry = readdir(dir);
                while ( dentry ) 
                {
			dlen = strlen(dentry->d_name);
			if (!(spath = realloc(spath, dlen + len + 2)))
				duerror("realloc() failed", 1);

                        len = sprintf(spath, "%s/%s", path, dentry->d_name);

                        if ( strcmp( ".", dentry->d_name) &&
                            strcmp( "..", dentry->d_name) )
                        { 
				
				if (opt[5] == 1 ) // -x
					lstat(spath, &sb); 
			
				if (opt[5] == 0 || hold.lastd != sb.st_dev )
                                	if ( dentry->d_type == DT_DIR )
						durecurse(spath, len, opt); 
		
				if ( opt[3] == 1 ) // -L 
					stat(spath, &sb); 
				else
					lstat(spath, &sb);
                                
				if ( populatetab(sb.st_ino) ) 
				{ 
					i = ( sb.st_blocks * 512 ) / hold.block; 

					if (opt[0] == 1 && dentry->d_type == DT_REG) // -a 
						printf("%-5zu\t%s\n", i, spath ); 
					c = 1;
					while ( c <= 35 ) 
						hold.level[c++] += i;
                           	}
				
				if ( dentry->d_type == DT_DIR )	
				{ 	
					if ( opt[4] == 0 ) // -s 
					{
                               	         	printf("%-5zu", hold.level[hold.depth]);
						printf("\t%s\n", spath );
					}
                               	        hold.level[hold.depth] = 0; 
					--hold.depth; 
				}
			
				c = hold.depth;
				while ( c <= 35 ) 
                               		hold.level[++c] = 0; 
				
			} 
                        dentry = readdir(dir); 
                } 
                closedir(dir);
		free(spath);

        }
        else
	{ 
		lstat(path, &sb);
		printf("%-5zu\t%s\n", (sb.st_blocks * 512 ) /hold.block, path); 
		return 0;
	} 
	
        return 1; 
}


