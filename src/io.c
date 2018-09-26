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
#include <stdlib.h>
#include <string.h>

#include <io.h>
#include <vm.h>

BcStatus bc_io_getline(char **buf, size_t *n, const char* prompt) {

	char *temp;
	int c;
	size_t size, i;

	if (bcg.tty && (fputs(prompt, stderr) == EOF || fflush(stderr) == EOF))
		return BC_STATUS_IO_ERR;

	for (i = 0, c = 0; c != '\n'; ++i) {

		if (i == *n) {

			size = *n * 2;

			if (size > BC_MAX_LINE || !(temp = realloc(*buf, size + 1)))
				return BC_STATUS_ALLOC_ERR;

			*buf = temp;
			*n = size;
		}

		if ((c = fgetc(stdin)) == EOF) {

			if (errno == EINTR) {

				bcg.sigc = bcg.sig;
				bcg.signe = 0;
				--i;

				if (bcg.tty && (fputs(bc_program_ready_msg, stderr) == EOF ||
				                fputs(prompt, stderr) == EOF || fflush(stderr) == EOF))
				{
					return BC_STATUS_IO_ERR;
				}

				continue;
			}
			else return BC_STATUS_IO_ERR;
		}
		else if ((c < ' ' && !isspace(c)) || c > '~') return BC_STATUS_BIN_FILE;

		(*buf)[i] = (char) c;
	}

	(*buf)[i] = '\0';

	return BC_STATUS_SUCCESS;
}

BcStatus bc_io_fread(const char *path, char **buf) {

	BcStatus s;
	FILE *f;
	size_t size, read;
	long res;

	assert(path && buf);

	if (!(f = fopen(path, "r"))) return BC_STATUS_EXEC_FILE_ERR;

	fseek(f, 0, SEEK_END);

	if ((res = ftell(f)) < 0) {
		s = BC_STATUS_IO_ERR;
		goto malloc_err;
	}

	size = (size_t) res;
	fseek(f, 0, SEEK_SET);

	if (!(*buf = malloc(size + 1))) {
		s = BC_STATUS_ALLOC_ERR;
		goto malloc_err;
	}

	if ((read = fread(*buf, 1, size, f)) != size) {
		s = BC_STATUS_IO_ERR;
		goto read_err;
	}

	(*buf)[size] = '\0';
	fclose(f);

	return BC_STATUS_SUCCESS;

read_err:
	free(*buf);
malloc_err:
	fclose(f);
	return s;
}
