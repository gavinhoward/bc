#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h> 

/* 
	Copyright 2014-2017, `cat' "cat.c", Christopher M. Graff
*/ 

int concatenate(int, int);

int main(int argc, char *argv[])
{
	if (argc == 1) 
		if (concatenate(0, 0) == -1)
			return 1;
        while  (*++argv) 
		if (concatenate(open(*argv, O_RDONLY), 1) == -1)
			return 1;
	return 0; 
}

int concatenate(int source, int opened)
{
        ssize_t n = 0;
        char buf[4096];

        if (source == -1)
                return -1;

        while ((n = read(source, buf, 4096)) > 0)
                write(1, buf, n);
	
	if (n == -1)
		return -1;

	if (opened)
        	close(source);

	return 1;
}

