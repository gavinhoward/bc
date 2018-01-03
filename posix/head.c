#include <stdio.h> 
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h> 

/*
	Copyright 2015-2017, "head.c", Christopher M. Graff
*/

void cathead(int, size_t, int);

int main(int argc, char *argv[])
{ 
	int o;
	size_t lines = 10;

        while ((o = getopt(argc, argv, "n:")) != -1)
                switch (o) {
                        case 'n':
                                lines = strtod(optarg, NULL);
                                break;
                        default:
                                break;
                }

        argv += optind;
        argc -= optind; 

	if (argc == 0)
                cathead(STDIN_FILENO, lines, 0);

        while (*(argv)) 
		cathead(open(*argv++, O_RDONLY), lines, 1);

	return 0; 
} 

void cathead(int source, size_t end, int opened)
{
        size_t i, j; 
	ssize_t ret = 0;
	char buf[4096];
	size_t len = 0;

        if (source == -1)
                return;

        for (i = 0, j=0; (ret = read(source, buf, 4096)) > 0;)
        {
		i = 0;
		len = ret;
                while (i < len)
                {
                        if (buf[i] == '\n')
                                ++j;
			++i; 
                        if (j == end) 
			{
				write(STDOUT_FILENO, buf, i);
				goto end;
			}
                }
		write(STDOUT_FILENO, buf, i);
        }
	if (ret == -1)
		write(2, "head error.\n", 12);
	end:
	if (opened == 1)
        	close(source);
}

