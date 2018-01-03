#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <fnmatch.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <limits.h>
#include <libgen.h>
#include <fcntl.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "cutils.h"

/* 
	Copyright 2015-2017, "fetch.c", Christopher M. Graff
	Usage: fetch http://www.csit.parkland.edu/~cgraff1/graflibc/README.html
*/

void parseurl(char *);
void fetch(char *, char *, char *);
void writeout(int, int);

int main (int argc, char *argv[])
{ 
	if ( argc == 1 )
		cutilerror("Usage: ./fetch http://www.csit.parkland.edu/~cgraff1/graflibc/README.html\n", -1); 
	++argv;
	while (*argv) 
		parseurl(*argv++); 

	return 0;
}

void parseurl(char *argv)
{
	char *host;
	char *type;
	char *page;

	type = host = page = argv;
	
	if ((host = strstr(argv, "://")))
	{
		*host = '\0';
		host += 3;
	}
	if ((page = strstr(host, "/")))
		*page++ = '\0'; 

	printf("Attempting an [%s] protocol on [%s] to retrieve [%s]\n ", type, host, page);
	fetch(type, host, page);
	return;
}

void fetch(char *type, char *host, char *page) 
{ 
	struct addrinfo hints, *res;    
	int sck;			
	int output;			
	char message[4096];		
	size_t len = 0; 
	memset(&hints, 0, sizeof(hints)); 
	if ( strcmp(type, "http") == 0)
	{
		
		len = snprintf(message, 4096, "GET /%s HTTP/1.0\r\nHost: %s\r\n\r\n", page, host);
		if (len == 4096)
			cutilerror("User argument length is not sane", 0);
	}
	else
		cutilerror("Protocol not supported", 0); 
	
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM; 
	
	if ((getaddrinfo(host, type, &hints, &res)) != 0)
		cutilerror("getaddrinfo() failed", 1); 
	
	if ((sck = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1)
		cutilerror("socket() failed", 1); 
	
	if ((connect(sck, res->ai_addr, res->ai_addrlen)) == -1)
		cutilerror("connect() failed", 1); 
	
	write(sck, message, strlen(message)); 
	
	if ((output = open(basename(page), O_CREAT|O_RDWR, S_IRUSR|S_IWUSR)) == -1 )
		cutilerror("open() failed", 1); 
	
	writeout(sck, output); 
	
	freeaddrinfo(res);
	close(sck);
}

void writeout(int sck, int output)
{ 
	size_t i;
	size_t n; 
	int lever; 
    	char buf[4096];
	char *luf;
	
	i = n = lever = 0;

	while ((n = read(sck, buf, 4096)) > 0)
	{
		i = 0;
		if (lever == 0)
		{ 
			if ((luf = strstr(buf, "\n\r")))
			{
				i = (luf - buf); 
				i += 3;
			}
			lever = 1;
		}
		write(output, buf + i, n - i);
	}
}


