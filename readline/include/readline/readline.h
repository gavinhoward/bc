#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h> 
#include <unistd.h> 
#include <sys/types.h>
#include <dirent.h>
#include <terminal/ansi.h>
#include <libgen.h>

/*
	2017 (C) Copyright, readline.h, CM Graff
*/
#define READLINE_LIMIT 4096

/* function prototypes */ 
size_t ircline(char *, char *, size_t);
void ircprint(char *, size_t, char *, size_t); 
void determinewin(void); 
int readchar(void); 
size_t greadgetch(char *, char *, size_t);
void greadprint(char *, size_t, char *, size_t);

char *readline(char *);
extern void add_history(const char *);
char * find_pattern(char *path, char *pat, size_t patlen);

/* structures */
struct hglb {		/* globals	*/
	size_t t;	/* Total	*/
	size_t c;	/* Current	*/
	int r;		/* Runstate	*/
	size_t laro;	/* left arrow(s)*/ 
	size_t w;
	size_t h;
}hglb;

struct hist {			/* history lines	*/
	char line[READLINE_LIMIT];	/* lines	*/ 
	size_t len;		/* line length		*/
}*hist;

struct winsize win;

