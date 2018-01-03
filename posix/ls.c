#include <stdio.h> 
#include <dirent.h>
#include <string.h>
#include <stdlib.h> 
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h> 
#include <pwd.h>
#include <grp.h>



/* 
	Copyright 2015-2017, C. Graff "ls"
*/ 

/* functions */
int compare(const void*, const void*);
void list_dirs(char *);
void ls_error(char *, int);
void octtoperm(int);
void print_plain(size_t);
void print_strings(char *[], size_t, size_t, int);
void prntstats(char *);
void shift_alpha(int, int);
int find_pattern(char *, size_t, size_t); 
char *home;


/* structs */
struct global{
	int plain;
	int alpha;
	int inode;
	int numer;
	int horiz;
	int hiddn;
	int recur;
	char *path;
	char *strings[10025]; 
	char *output[10025];
} global = { 0, 1, 0, 0, 0, 0 , 0, NULL, {}, {} };


int main(int argc, char *argv[])
{ 
	int o;
	/* POSIX ls [-CFRacdilqrtu1][-H | -L  */
	while ((o = getopt (argc, argv, "lUinxhCaR")) != -1)
		switch (o) { 
			case 'l' : 
				global.plain = 1; 
				break;
			case 'U' : 
				global.alpha = 0; 
				break;
			case 'i' : 
				global.inode = 1; 
				break;
			case 'n' : 
				global.numer = 1; 
				break;
			case 'x' : 
				global.horiz = 1; 
				break;
			case 'C' : 
				global.plain = 0; 
				break;
			case 'a' : 
				global.hiddn = 1; 
				break;
			case 'R':
				global.recur = 1;
				break;
			case 'h' : 
				ls_error("Usage:   ls -lUinxhCaR [PATH(S)]\n", 0); 
				break;
			case '?' : 
				return 0; 
			default: 
				break;
		}

	argv += optind;
	argc -= optind;

	/* disable vertical alphabetization with -R (it's broken) */
	if ( global.recur == 1)
		global.horiz = 1;
	
	home = getcwd(home, 100);

	if (argc == 0 ) 
	{
		//global.path = *argv;
		global.path = ".";
		if ( global.recur == 0 )
			list_dirs(".");
		if ( global.recur == 1 )
			find_pattern(".", 1, 0);
	}
	while (argc > 0 && *argv )
	{ 
		global.path = *argv;
		if ( global.recur == 0 )
			list_dirs(*argv);
		if ( global.recur == 1 )
			find_pattern(*argv, strlen(*argv), 0);
		++argv;
	}

	free(home);
	return 0;
} 


void list_dirs(char *argvv) 
{ 
	struct winsize w;
	DIR* a;
	struct dirent* b; 
	size_t max = 1;
	size_t hold = 1; 
	int c, factor, refactor, z, i;


	c = factor = refactor = z = i = 0; 
	int wasin = 0;
	
	/* Discover and count directory entries */
	if ((a = opendir(argvv)))
	{
		while ((b = readdir(a)))
		{ 
			++wasin;
			if (( global.hiddn != 1 && b->d_name[0] == '.' ))
				continue; 
			global.strings[c] = b->d_name;
			hold = strlen(global.strings[c]);
		       	if ( hold > max )
			       	max = hold;
			++c; 
		} 
	}
	else /* it's not a directory, so just lstat it */
	{ 
		prntstats(argvv); 
		return; 
	} 

	if ( max == 1 )
		goto end; 

	/* Obtain terminal information */ 
	ioctl(0, TIOCGWINSZ, &w); 
	factor = w.ws_col / max ;

	refactor = ( w.ws_col - factor ) / max ; 

	/* Alphabetize discovered entries */
	if ( global.alpha == 1 ) 
		qsort(global.strings, c, sizeof (char*), compare);

	if ( global.plain == 1 ) 
		print_plain(c); 
	
	if ( global.plain == 0 )
	{
		if ( global.horiz == 1 )
		{
			print_strings(global.strings, c - refactor -1, refactor, max);
			printf("\n"); 
		}
		else {
			shift_alpha(c, refactor); 
			print_strings(global.output, c, refactor, max); 
		} 
	} 
	end:
	closedir(a);
}


void shift_alpha(int c, int refactor)
{
	/* Format columnar lists to alphabetize vertically */
	int cnt, sft, sftc, vary = 0;
	cnt = sft = sftc = vary = 0;
	while ( cnt < c )
	{ 
		global.output[sft] = global.strings[cnt];
		sft = (sft + refactor);
		++sftc;
		if (sftc == ( c / refactor ) + 1)
		{
			++vary;
			sft = vary;
			sftc = 0;
		}
		++cnt;
	} 
}

void print_strings(char *s[], size_t c, size_t refactor, int max)
{
	size_t i, z;

	for (i = 0, z = 0; i <= c + refactor; i++)
	{ 
		if ((s[i])) 
			printf("%-*s ", max , s[i]);
		if ( ++z % refactor == 0 )
			printf("\n"); 
	} 
}

void print_plain(size_t c)
{ 
	size_t i = 0; 
	
	if ( global.recur == 0 && chdir(global.path) != 0 )
		ls_error("failure to chdir()\v", 1);
	for (; i < c ; ++i) 
		prntstats(global.strings[i]);
}

void ls_error(char *message, int i)
{ 
	if ( i > 0 )
		perror("Error: ");
	fprintf(stderr, "%s", message);
	exit (i);
}

void octtoperm(int octal)
{ 
	char s[16];
	size_t i = 0;

	switch (octal & S_IFMT)
	{
		case S_IFBLK: 
			s[i++] = 'b';
			break;
		case S_IFCHR: 
			s[i++] = 'c';
			break;
		case S_IFDIR: 
			s[i++] = 'd';
			break;
		case S_IFIFO: 
			s[i++] = 'p';
			break;
		case S_IFLNK: 
			s[i++] = 'l';
			break;
		case S_IFREG: 
			s[i++] = '-';
			break; 
		case S_IFSOCK: 
			s[i++] = 'S';
			break; 
		default:
			s[i++] = '?';
			break; 

	} 

	if (octal & S_IRUSR)
		s[i++] = 'r';
	else
		s[i++] = '-';
	if (octal & S_IWUSR)
		s[i++] = 'w';
	else
		s[i++] = '-';
	if (octal & S_IXUSR)
		s[i++] = 'x';
	else
		s[i++] = '-';
	if (octal & S_IRGRP)
		s[i++] = 'r';
	else
		s[i++] = '-';
	if (octal & S_IWGRP)
		s[i++] = 'w';
	else
		s[i++] = '-';
	if (octal & S_IXGRP)
		s[i++] = 'x';
	else
		s[i++] = '-';
	if (octal & S_IROTH)
		s[i++] = 'r';
	else
		s[i++] = '-';
	if (octal & S_IWOTH)
		s[i++] = 'w';
	else
		s[i++] = '-';

	if ( octal & S_ISVTX && octal & S_IXOTH)
		s[i++] = 't';
	else if (octal & S_ISVTX) 
		s[i++] = 'T';
	else if (octal & S_IXOTH)
		s[i++] = 'x';
	else	
		s[i++] = '-';

	write(1, s, i);
}

int compare (const void * a, const void * b )
{
	return strcmp(*(char **)a, *(char **)b);
}

void prntstats(char *file)
{
	struct stat sb; 
	struct group *grp;
	struct passwd *pwd;

	//memset(&sb, 0, sizeof(struct stat)); 
	if ( lstat(file, &sb) != 0 )
		return;
	octtoperm(sb.st_mode);
	printf(" "); 
	if ( global.inode == 1)
	      printf("%-8ld ", sb.st_ino);
	printf("%-3ld ", sb.st_nlink);
	if ( global.numer == 1)
		printf("%ld %ld ", (long int)sb.st_uid, (long int)sb.st_gid);
	else {
		if ((pwd = getpwuid(sb.st_uid)) != NULL )
			printf("%s ", pwd->pw_name);
		if ((grp = getgrgid(sb.st_gid)) != NULL )
			printf("%s ", grp->gr_name);
	} 
	printf("%8lld ", (long long int)sb.st_size); 
	printf("%.16s   ", strchr(ctime(&sb.st_mtime), ' '));
	printf("%s\n", file); 
} 

int find_pattern(char *path, size_t tot, size_t last)
{ 
	DIR *dir;
	struct dirent *d;
	char *spath = malloc (1); 
	size_t dlen = 0;

	
	if ( chdir(path) == 0)
	{
		printf("%s:\n", path);
		list_dirs(".");
		chdir(home);
		printf("\n");
	}else
		return -1;
				
	if (!(spath))
		return -1;
	
	if ( ( dir = opendir(path) ) ) 
	{
		d = readdir(dir);

		while (d) 
		{
			dlen = strlen(d->d_name);
			last = (tot + dlen + 2); 
			spath = realloc(spath, last);
			if (!(spath))
				return -1;

			tot = sprintf(spath, "%s/%s", path, d->d_name); 

			if ( d->d_type == DT_DIR &&
			   ( strcmp( ".", d->d_name)) &&
			   ( strcmp( "..", d->d_name))) 
			{ 
				
				find_pattern(spath, tot, last); 
			} 
			d = readdir(dir); 
		} 
	}
	free(spath);
	closedir(dir);
	return 0;
} 
