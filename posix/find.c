#include <stdio.h>
#include <dirent.h> 
#include <fnmatch.h>
#include <string.h>
#include <getopt.h>
#include <stdlib.h> 

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h> 

#include <pwd.h>
#include <grp.h>

#include "cutils.h"
/* 
	(Copyright) 2014, , "find", C. Graff 
*/ 

	
struct s {
	char cond[1024][1024]; 		// Conditional arguments
	int truth[1024];	 	// Truth states
	int type[1024]; 		// Option types
	int or; 			// Incrementing OR states
	int nots[1024]; 		// NOT states
	int not; 			// Shifting boolean NOT state
	int n; 				// Total number of options 
} s; 

int find_pattern(char *, size_t, size_t);
int file_type(char *, char *);
int num_links(char *, char *);

int is_user(char *, char []);
int is_group(char *, char *);

void assess_arg(char *, int);

int main(int argc, char *argv[])
{ 
	int c;
	c = 0; 

	s.n = 0;
	while ( c < argc ) 
	{
		
		if ( strcmp(argv[c], "-o" ) == 0 )
			++s.or; 
		else if ( strcmp(argv[c], "!" ) == 0 )
			s.not = 1;
		else if ( strcmp(argv[c], "(" ) == 0 )
                      	; // needs to do something
		else if ( strcmp(argv[c], ")" ) == 0 )
                     	; // needs to do something
                else if ( strcmp(argv[c], "-name" ) == 0 ) 
			assess_arg(argv[++c], 0); 
		else if ( strcmp(argv[c], "-path" ) == 0 ) 
			assess_arg(argv[++c], 1); 
		else if ( strcmp(argv[c], "-nouser" ) == 0 )
			s.type[s.n] = 2;
		else if ( strcmp(argv[c], "-nogroup" ) == 0 )
			s.type[s.n] = 3;
		else if ( strcmp(argv[c], "-xdev" ) == 0 )
			s.type[s.n] = 4;
		else if ( strcmp(argv[c], "-prune" ) == 0 )
			s.type[s.n] = 5;
		else if ( strcmp(argv[c], "-perm" ) == 0 ) 
			s.type[s.n] = 6; 
		else if ( strcmp(argv[c], "-type" ) == 0 ) 
			assess_arg(argv[++c], 7); 
		else if ( strcmp(argv[c], "-links" ) == 0 ) 
                        assess_arg(argv[++c], 8); 
		else if ( strcmp(argv[c], "-user" ) == 0 ) 
                        assess_arg(argv[++c], 9); 
		else if ( strcmp(argv[c], "-group" ) == 0 ) 
                        assess_arg(argv[++c], 10); 
		else if ( strcmp(argv[c], "-size" ) == 0 ) 
                        assess_arg(argv[++c], 11); 
		++c;
	} 
	
	find_pattern(argv[1], strlen(argv[1]), 0);

	return 0;
} 
int find_pattern(char *path, size_t tot, size_t last)
{ 
	DIR *dir;
	struct dirent *d;
	char *spath = cutilmalloc (1);
	int p, cache, good, pass; 
	size_t dlen = 0; 

	if (!(spath))
		return -1;
	
	if ( ( dir = opendir(path) ) ) 
	{
		d = readdir(dir); 
		while (d) 
		{ 
			dlen = strlen(d->d_name); 
			
			last = (tot + dlen + 2); /* +2 = '/' + '\0' */
			spath = cutilrealloc(spath, last); 
			if (!(spath))
				return -1;
			tot = sprintf(spath, "%s/%s", path, d->d_name);

			p = pass = good = 0; 
			cache = s.truth[p]; 
			while ( p < s.n )
			{ 
				// Create an entry / exit point for ()'s 'parenthesis' 

				if ( cache != s.truth[p])
				{ 
					if ( good == 0) // passed, let it through 
						pass = 1;
					else 
						good = 0; 
				} 
				
				cache = s.truth[p];
			
				if ( s.type[p] == 0 ) // -name
				{ 
					if ( s.nots[p] == 0 ) 
						if ( fnmatch(s.cond[p], d->d_name, FNM_PERIOD) != 0 ) 
							good = 1; 
					if ( s.nots[p] == 1 ) 
                                        	if ( fnmatch(s.cond[p], d->d_name, FNM_PERIOD) == 0 )
                                                	good = 1; 
				} 
				if ( s.type[p] == 1 ) // -path
				{
					if ( s.nots[p] == 0 ) 
						if ( strcmp(s.cond[p], spath) != 0 )
							good = 1; 
					if ( s.nots[p] == 1 ) 
						if ( strcmp(s.cond[p], spath) == 0 )
							good = 1; 
				}
				if ( s.type[p] == 7 ) // -type
				{
					if ( s.nots[p] == 0 ) 
						if ( file_type(spath, s.cond[p]) != 0 )
							good = 1; 
					if ( s.nots[p] == 1 ) 
						if ( file_type(spath, s.cond[p]) == 0 )
							good = 1; 
				}
                       		if ( s.type[p] == 8 ) // -links
                                {
                                        if ( s.nots[p] == 0 ) 
                                                if ( num_links(spath, s.cond[p]) != 0 )
                                                        good = 1; 
                                        if ( s.nots[p] == 1 ) 
                                                if ( num_links(spath, s.cond[p]) == 0 )
                                                        good = 1; 
                                }
				if ( s.type[p] == 9 ) // -user
				{
					if ( s.nots[p] == 0 ) 
						if ( is_user(spath, s.cond[p]) != 0 )
							good = 1; 
                                        if ( s.nots[p] == 1 ) 
						if ( is_user(spath, s.cond[p]) == 0 )
                                                        good = 1; 
				}
				if ( s.type[p] == 10 ) // -group
                                {
                                        if ( s.nots[p] == 0 ) 
                                                if ( is_group(spath, s.cond[p]) != 0 )
                                                        good = 1; 
                                        if ( s.nots[p] == 1 ) 
                                                if ( is_group(spath, s.cond[p]) == 0 )
                                                        good = 1; 
                                }
				if ( s.type[p] == 11 ) // -size
				{
					;
				}
				++p;
			} 

			if ( good == 0 || pass == 1) 
				if ( strcmp( ".", d->d_name) && 
				   ( strcmp( "..", d->d_name)) )
			{
				printf("%s\n", spath);
			
			}

			if ( d->d_type == DT_DIR &&
			   ( strcmp( ".", d->d_name)) &&
			   ( strcmp( "..", d->d_name))) 
				find_pattern(spath, tot, last); 
			d = readdir(dir); 
		}
		
	}
	free(spath);
	closedir(dir);
	return 0;
}

int file_type(char *temp, char *option)
{
	struct stat sb;
	memset(&sb, 0, sizeof(struct stat));
	lstat(temp, &sb); 
	switch (sb.st_mode & S_IFMT) {
                case S_IFBLK:  if ( strcmp(option, "b") == 0 ) return 0; break;
                case S_IFCHR:  if ( strcmp(option, "c") == 0 ) return 0; break;
                case S_IFDIR:  if ( strcmp(option, "d") == 0 ) return 0; break;
		case S_IFLNK:  if ( strcmp(option, "l") == 0 ) return 0; break;
                case S_IFIFO:  if ( strcmp(option, "p") == 0 ) return 0; break;
                case S_IFREG:  if ( strcmp(option, "f") == 0 ) return 0; break;
                case S_IFSOCK: if ( strcmp(option, "s") == 0 ) return 0; break;
                default: break;

        }
	return 1;
}

int num_links(char *temp, char *option)
{

	struct stat sb;
	unsigned int real = atoi(option);

        memset(&sb, 0, sizeof(struct stat));
        lstat(temp, &sb);
	
	if ( real == sb.st_nlink )
		return 0;

	return 1;
}

int is_user(char *temp, char option[])
{ 
        struct stat sb; 
        struct passwd *pwd; 
	
	memset(&sb, 0, sizeof(struct stat)); 
        lstat(temp, &sb); 
        pwd = getpwuid(sb.st_uid);
	if ( pwd != 0 )
		if (strcmp(option, pwd->pw_name) == 0 ) 
			return 0; 

	return 1;
}

int is_group(char *temp, char *option)
{ 
        struct stat sb; 
	struct group *grp;

	memset(&sb, 0, sizeof(struct stat));
        lstat(temp, &sb);
	grp = getgrgid(sb.st_gid);
	if ( grp != 0 )
		if (strcmp(option, grp->gr_name) == 0 )
                	return 0;

        return 1;
}

	
void assess_arg(char *arg, int type)
{
	size_t len = 1024;
	s.type[s.n] = type;
	memset(s.cond[s.n], 0, len);
	strncpy (s.cond[s.n], arg, len);
	s.truth[s.n] = s.or; 
	s.nots[s.n] = s.not;
	s.not = 0;
	++s.n;
}

