#ifndef BC_IO_H
#define BC_IO_H

#include <stdio.h>
#include <stdlib.h>

typedef int (*BcIoGetc)(void*);

long bc_io_frag(char* buf, long len, int term, BcIoGetc bcgetc, void* ctx);

long bc_io_fgets(char * buf, int n, FILE* fp);

#define bc_io_gets(buf, n) bc_io_fgets((buf), (n), stdin)

size_t bc_io_fgetline(char** p, size_t* n, FILE* fp);

#define bc_io_getline(p, n) bc_io_fgetline((p), (n), stdin)

#endif // BC_IO_H
