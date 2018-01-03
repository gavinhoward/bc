#include <stdio.h>

#include <string.h>
#include <ctype.h>
#define MAXTOKEN 100
enum { NAME, PARENS, BRACKETS };
void dcl(void);
void dirdcl(void);
int gettoken(void);
int tokentype;
char token[MAXTOKEN];
char name[MAXTOKEN];
char datatype[MAXTOKEN];
char out[1000];


/* dcl: parse a declarator */
void dcl(void)
{
	int ns;
	for (ns = 0; gettoken() == '*'; ) /* count *'s */
		ns++;
	dirdcl();
	while (ns-- > 0)
		strcat(out, " pointer to");
}
/* dirdcl: parse a direct declarator */
void dirdcl(void)
{
	int type;
	if (tokentype == '(') {
		/* ( dcl ) */
		dcl();
		if (tokentype != ')')
			printf("error: missing )\n");
	} else if (tokentype == NAME) /* variable name */
		strcpy(name, token);
	else
		printf("error: expected name or (dcl)\n");
	while ((type=gettoken()) == PARENS || type == BRACKETS)
		if (type == PARENS)
			strcat(out, " function returning");
		else {
			strcat(out, " array");
			strcat(out, token);
			strcat(out, " of");
		}
} 

int main() /* convert declaration to words */
{
	while (gettoken() != EOF) {
		/* 1st token on line */
		strcpy(datatype, token); /* is the datatype */
		out[0] = '\0';
		dcl();
		/* parse rest of line */
		if (tokentype != '\n')
			printf("syntax error\n");
		printf("%s: %s %s\n", name, out, datatype);
	}
	return 0;
}

int gettoken(void) /* return next token */
{
	char *p = token;
	int c = 0;
	while ((c = getc(stdin)) == ' ' || c == '\t')
		;
	if (c == '(') {if ((c = getc(stdin)) == ')') {
		strcpy(token, "()");
		return tokentype = PARENS;
	} else {
		ungetc(c, stdin);
		return tokentype = '(';
	}
	} else if (c == '[') {
		for (*p++ = c; (*p++ = getc(stdin)) != ']'; )
		;
		*p = '\0';
		return tokentype = BRACKETS;
	} else if (isalpha(c)) {
		for (*p++ = c; isalnum(c = getc(stdin)); )
			*p++ = c;
		*p = '\0';
		ungetc(c, stdin);
		return tokentype = NAME;
	} else
	return tokentype = c;
}


