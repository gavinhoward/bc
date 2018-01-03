#include <stdio.h> 
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <libgen.h> 
#include <sys/stat.h>
#include <sys/types.h> 
#include <limits.h>
#include <sys/time.h> 
#include <sys/types.h>
#include <utime.h>

/* Copyright 2015, C. Graff  "cp" */
struct glb {
	char src[PATH_MAX + 1];
	char dst[PATH_MAX + 1];
	size_t len; 
	int op[10];
} glb = { {}, {}, 0, { 0,0,0,0,0,0,0,0,0}};

int recursivecp(char *, size_t);
void deftype(char *);
void copyfile(char *, char *, int);
void copy(char *, char *);

char *pointtoloc(char *source);
void copy_error(char *, char *, int); 

int main(int argc, char *argv[])
{ 
	
	int i, c = 0; 
	char temp[( PATH_MAX + 1 ) * 2]; 
        struct stat fst, fst2;
       
	mode_t mode = 0755;
	
	while ((i = getopt (argc, argv, "fHLPparRh")) != -1)
                switch (i) { 
			case 'f':
				glb.op[0] = 1;
                                break;
                        case 'H': 
				glb.op[1] = 1; 
				break;
                        case 'L': 
				glb.op[3] = 1; 
				break;
                        case 'P': 
				glb.op[3] = 0; 
				break;
                        case 'p': 
				glb.op[5] = 1; 
				break;
                        case 'a': 
				glb.op[5] = glb.op[7] = 1; 
				glb.op[3] = 0;
                                break;
                        case 'r': 
				glb.op[7] = 1; 
				break;
                        case 'R': 
				glb.op[7] = 1; 
				break;
                        case 'h': 
				copy_error("Usage : ./cp -fHLPparRh\n", "", 0);
                                break;
			default:
				break; 
                }

        argv += optind;
        argc -= optind; 

	if ( argc < 2 ) 
		copy_error("Requires 2 or more arguments", "",  0); 
	
	if (argc == 2 ) /* handle exactly 2 args */
		glb.op[9] = 1; 

	for ( c = 0; c < argc - 1 ; ++c)
	{ 
		if (lstat(argv[c], &fst) != 0)
		{ 
			copy_error("File not found : ", argv[c],  -1);
			continue;
		}
		
		lstat(argv[argc -1], &fst2);

		if ( S_ISLNK(fst.st_mode) )  /* -L -H */
			if ( glb.op[3] != 1 && glb.op[1] != 1 ) 
			{ 
				copy_error("Not copying link : ", argv[c],  -1);
				continue; 
			}
		
		if ( S_ISDIR(fst.st_mode) )
		{
			if ( glb.op[7] == 1 ) /* -rR */
				mkdir(argv[argc -1], mode);
			else { 
				copy_error("Not copying directory : ", argv[c], -1);
                       		continue;
			}
		} 
	
		if ( S_ISDIR(fst.st_mode) ) /* this never happens without -r */
		{
			strcpy(glb.dst, argv[argc-1]); 
			glb.len = strlen(argv[c]);
			/* src name becomes child dest dir if argc > 2 ... */
			strcpy(glb.src, basename(argv[c])); 
			if (argc != 2 )
			{
				/* otherwise concatenate it to dest and make the dir */
				sprintf(temp, "%s/%s/",argv[argc-1], glb.src);
				mkdir(temp, mode); 
			}

			recursivecp(argv[c], strlen(argv[c]));
		}
		else { 
			if ( S_ISDIR(fst2.st_mode) )
				copyfile(argv[c], argv[argc-1], 0);
			else 
				copy(argv[c], argv[argc-1]);
		}
	} 
	return 0; 

}

int recursivecp(char *path, size_t len)
{
	/*
		recurse file tree and
		send paths --> deftype() --> copyfile() --> copy()
	*/

        DIR *dir;
        struct dirent *dentry; 
        char *spath = malloc(1);
	size_t dlen = 0;
	
	if (!(spath))
		return -1;

        if ( ( dir = opendir(path) ) )
        {
                dentry = readdir(dir); 
                while ( dentry )
                { 
			dlen = strlen(dentry->d_name);
			spath = realloc(spath, len + dlen + 2);
			if (!(spath))
				return -1;
			
                        sprintf(spath, "%s/%s", path, dentry->d_name); 

                        if ( strcmp( ".", dentry->d_name) &&
                            strcmp( "..", dentry->d_name))
                        { 
				deftype(spath);
				if ( dentry->d_type == DT_DIR )
                                      	recursivecp(spath, len);
                        }
                        dentry = readdir(dir);
                }
		free(spath);
                closedir(dir);
        } 
	return 0;
}


void deftype(char *spath)
{ 
	/*
		make dirs + set their permissions, 
		precompute strings to send --> copyfile() --> copy()
	*/

	struct stat fst; 
	char temp[(PATH_MAX + 1) * 4]; 
	char *temp2 = pointtoloc(spath); 

	lstat(spath, &fst); 

	if (!(S_ISREG(fst.st_mode) || 
		S_ISDIR(fst.st_mode) || 
		S_ISLNK(fst.st_mode))) 
		return; 

	if ( glb.op[9] == 1 ) /* argc == 2 */
		sprintf(temp, "%s/%s", glb.dst, temp2);
	else
		sprintf(temp, "%s/%s/%s", glb.dst, glb.src, temp2); 

	if ( S_ISDIR(fst.st_mode) ) 
	{
		mkdir(temp, 0755); 
		chown(temp,fst.st_uid, fst.st_gid);
		chmod(temp, fst.st_mode); 
		return;
	} 

	copyfile(spath, dirname(temp), 1); 

}

void copyfile(char *source, char *dst, int i)
{
	/* make links, precompute strings to send --> copy() */
	char buf[PATH_MAX + 1]; 
	char temp[(PATH_MAX +1) *5]; 

	if ( ((glb.op[1] != 1 || i== 1) && glb.op[3] != 1) ) /* -H -L */
	{
		memset(buf, 0, PATH_MAX); 
		if ( readlink(source, buf, PATH_MAX) > 0 ) 
		{ 
			memset(temp, 0, PATH_MAX + 1); 
			getcwd(temp, PATH_MAX); 
			if ( chdir(dst) != 0 )
        	       	 	copy_error("Not found", dst, 1);
   	             	symlink(buf , basename(source));
			chdir(temp);
			return;
		} 
	} 
	sprintf(temp, "%s/%s",dst, basename(source)); 
	copy(source, temp); 
} 


void copy(char *source, char *dst)
{ 
	/* copy a file and set its permissions */
	struct utimbuf times; 
	char buf[PATH_MAX];
	struct stat fst; 
	int n = 0, fd1 = -5, fd2 = -5; 

	lstat(source,&fst); 

	if ( (fd1 = open(source ,O_RDONLY)) == -1 ) 
		return;

	if ( (fd2 = open(dst, O_CREAT|O_RDWR|O_TRUNC)) == -1 )
	{
		if ( glb.op[0] == 1 ) /* -f */
		{
			unlink(dst);
			if ( (fd2 = open(dst, O_CREAT|O_RDWR|O_TRUNC)) == -1 )
				return;
		}
		else
			return; 
	}

	while ((n = read(fd1, buf, PATH_MAX)) > 0) 
		write(fd2, buf, n);

        chown(dst, fst.st_uid, fst.st_gid);
        chmod(dst, fst.st_mode); 

	if ( glb.op[5] == 1 ) /* -p */
	{
		times.actime = fst.st_atime;
		times.modtime = fst.st_mtime;
		utime(dst, &times);
	} 

	close(fd1);
	close(fd2);
}


char *pointtoloc(char *source)
{
	/* Determine concatenation point between src arg and current path */
	size_t i = 0;
	for ( ; i < glb.len ; ++i ) 
		++source; 
	return source; 
}


void copy_error(char *message, char *file, int i)
{ 
        fprintf(stderr, "%s %s\n", message, file); 
        if ( i > 0 )
                perror("Error: ");
	if ( i > -1 )
        	exit (i); 
}

