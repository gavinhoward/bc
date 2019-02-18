#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <string.h>

#define USAGE "realpath FILE\n"

int main(int argc, char *argv[]) {
    char resolved[PATH_MAX+1];
    char *p;

    if(argc < 2) {
        fprintf(stderr,USAGE);
        fflush(stderr);
        return 1;
    }

    p = realpath(argv[1],resolved);

    if(p == NULL) {
        fprintf(stderr,"Error: %s\n",strerror(errno));
        fflush(stderr);
        return 1;
    }

    fprintf(stdout,"%s\n",resolved);

    return 0;
}
