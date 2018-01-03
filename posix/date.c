#include <time.h>
#include <unistd.h>
#include <locale.h>
#include <stdlib.h>
#include <time.h>

/* 
	2017 Copyright, `date', CM Graff
*/

size_t date(char *, char *, size_t);
void datewrite(int, char *, size_t, int);

int main(int argc, char *argv[])
{
	int o;
        size_t n;
	char buf[256];
	char *format;
	/* POSIX specifies this format as date's default */
	char *fallback = "%a %b %e %H:%M:%S %Z %Y"; 

	format = fallback; 
	
	if (!setlocale(LC_CTYPE, ""))
		datewrite(2, "Unable to set locale", 20, 1);

	while ((o = getopt (argc, argv, "u")) != -1)
                switch (o) {
                        case 'u': /* POSIX specifies that -u should set TZ to UTC0 */
                                if((setenv("TZ", "UTC0", 1)) == -1)
					datewrite(2, "Unable to set setenv\n", 21, 1);
    				tzset();
                                break;
			default:
				datewrite(2, "Invalid\n", 8, 1);
				break;
			}

	argv += optind;
        argc -= optind; 

	if ( *argv )
	{
		if ( **argv == '+' )
		{
			++*argv;
			format = *argv;
		}
		else
			datewrite(2, "Date format error?\n", 19, 1);
	} 

	if ((n = date(buf, format, 100)) == 0)
		datewrite(2, "Unable to obtain date\n", 22, 1);

	datewrite(1, buf, n, 0);

	return 0;
}

void datewrite(int desc, char *message, size_t len, int exitcode)
{
	write(desc, message, len);
	exit(exitcode);
}

size_t date(char *buf, char *format, size_t max)
{
        size_t n = 0;
        time_t t;
        struct tm *tm;
        if ((t = time(0)) == -1)
                return 0;
        if (!(tm = localtime(&t)))
                return 0;
        if ((n = strftime(buf, max, format, tm)) == 0)
                return 0;
	buf[n++] = '\n';
	buf[n++] = '\0';
        return n;
}

