/*
 * *****************************************************************************
 *
 * Copyright 2018 Gavin D. Howard
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * *****************************************************************************
 *
 * Code to handle special I/O for bc.
 *
 */

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <read.h>
#include <program.h>
#include <vm.h>

BcStatus bc_read_line(BcVec *vec, const char* prompt) {

	int i;
	signed char c = 0;

	if (bcg.ttyin && !bcg.posix) {
		bc_vm_puts(prompt, stderr);
		bc_vm_fflush(stderr);
	}

	assert(vec && vec->size == sizeof(char));

	bc_vec_npop(vec, vec->len);

	while (c != '\n') {

		i = fgetc(stdin);

		if (i == EOF) {

			if (errno != EINTR) return BC_STATUS_IO_ERR;

#if BC_ENABLE_SIGNALS
			bcg.sigc = bcg.sig;
			bcg.signe = 0;

			if (bcg.ttyin) {
				bc_vm_puts(bc_program_ready_msg, stderr);
				if (!bcg.posix) bc_vm_puts(prompt, stderr);
				bc_vm_fflush(stderr);
			}
#endif // BC_ENABLE_SIGNALS

			continue;
		}

		if (i > UCHAR_MAX) return BC_STATUS_LEX_BAD_CHAR;

		c = (char) i;
		if (BC_LEX_BIN_CHAR(c)) return BC_STATUS_BIN_FILE;
		bc_vec_push(vec, &c);
	}

	bc_vec_pushByte(vec, '\0');

	return BC_STATUS_SUCCESS;
}

char* bc_read_file(const char *path) {

	BcStatus s = BC_STATUS_IO_ERR;
	FILE *f;
	size_t size, read;
	long res;
	char *buf;
	struct stat pstat;

	assert(path);

	f = fopen(path, "r");
	if (!f) bc_vm_exit(BC_STATUS_EXEC_FILE_ERR);
	if (fstat(fileno(f), &pstat) == -1) goto malloc_err;

	if (S_ISDIR(pstat.st_mode)) {
		s = BC_STATUS_PATH_IS_DIR;
		goto malloc_err;
	}

	if (fseek(f, 0, SEEK_END) == -1) goto malloc_err;
	res = ftell(f);
	if (res < 0) goto malloc_err;
	if (fseek(f, 0, SEEK_SET) == -1) goto malloc_err;

	size = (size_t) res;
	buf = bc_vm_malloc(size + 1);

	read = fread(buf, 1, size, f);
	if (read != size) goto read_err;

	(buf)[size] = '\0';
	fclose(f);

	return buf;

read_err:
	free(buf);
malloc_err:
	fclose(f);
	bc_vm_exit(s);
	return NULL;
}
