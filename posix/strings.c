#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <ctype.h>

/*
	Copyright 2016-2017, 'strings.c', C. Graff

	"strings - find printable strings in files"

	Conformance:
		Aligned with POSIX 2013
		All POSIX comments are quoted, the author's are not

	Extensions:
		TODO:
			-f filename
*/

int strings(char *, size_t, char);

int main(int argc, char *argv[])
{ 
	int o = 0;
	size_t number = 3;
	char format = '\0';
	char *help = " [-a] [-t format] [-n number] [file...]\n";
	int ret = EXIT_SUCCESS;

	while ((o = getopt (argc, argv, "at:n:h")) != -1)
		switch (o) {
			case 'a': /* "Scan files in their entirety." */
				break;
			case 't': /* "Write each string preceded by its byte 
				      offset from the start of the file." */
				if ( optarg && *optarg )
					format = *optarg;
				break;
			case 'n':/* "Specify the minimum string length." */ 
				number = strtoul(optarg, 0, 10) - 1;
		 		break;
			case 'h':
				if ( *argv )
					write(2, *argv, strlen(*argv));
				write(2, help, strlen(help));
				exit(ret);
			default:
				break;
		}

	argv += optind;
	argc -= optind;

	if ( argc == 0 )
		ret = strings(NULL, number, format); 

	while ( *(argv) )
		ret = strings(*argv++, number, format);

	return ret; 
}
int strings(char *file, size_t number, char format)
{
	int fd = STDIN_FILENO;
	int inastring;
	char hold[1024];
	char *buf;
	char *buffer;
	ssize_t ret;
	size_t i;
	size_t j;
	size_t offset;
	size_t len;

	if (!(buffer = malloc(number)))
		return 1;

	if (!(buf = malloc(4096)))
		return 1;

	if (file && (fd = open(file, O_RDONLY)) == -1 )
		return 1;

	ret = i = j = offset = inastring = 0;

	while ((ret = read(fd, buf, 4096)) > 0) 
	{
		/* Add some error checking here  or switch to stdio.h functions */
		for (j = 0;(ssize_t)j < ret ;j++, offset++) /* ret is a ssize_t so this cast is safe */
		{
			buffer[i] = buf[j];
		
			if (!isprint(buffer[i]) && buffer[i] != '\t') {
				if (inastring)
					write(1, "\n", 1);
				inastring = 0;
				i = 0;

				continue;
			}
			if (i < number) {
				i++;
				continue;
			}
			if (!inastring) { 
				inastring = 1;
				len = 0; 
				switch(format){
					case 'd': /* "The offset shall be written in decimal." */
						len = snprintf(hold, 1024, "%7d %s", (int)(offset - number), buffer);
						break;
					case 'o': /* "The offset shall be written in octal." */
						len = snprintf(hold, 1024, "%7o %s", (unsigned int)(offset - number), buffer);
						break;
					case 'x': /* "The offset shall be written in hexadecimal." */
						len = snprintf(hold, 1024, "%7x %s", (unsigned int)(offset - number), buffer); 
						break;
					default:
						
						break;
				} 
				write(1, hold, len);
				continue;
			} 
			write(1, buffer + i, 1); 
		}
	}

	if (fd != STDIN_FILENO) 
		close(fd);

	free(buffer);
	free(buf);

	return 0; /* Change this to something useful */
}
