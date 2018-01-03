#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>

/* A simple test written for musl-otter-cross */

int main(void)
{
	/* Check that there is at least a minimal amount of POSIX compat */
	size_t len = SIZE_MAX;
	ssize_t ret = SSIZE_MAX;
	size_t i = 0;
	char *string = "This is not part of the test";
	off_t j = SIZE_MAX;

	printf("%s\n", string);
	i = strlen(string);
	if ( i > 0 )
		ret = write(1, string, i);

	exit(0);
}

