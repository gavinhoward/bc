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

BcStatus bc_read_line(BcVec *vec, const char *prompt) {

	int i;
	signed char c = 0;

	if (bcg.ttyin && !bcg.posix) {
		bc_vm_puts(prompt, stdout);
		bc_vm_fflush(stdout);
	}

	assert(vec && vec->size == sizeof(char));

	bc_vec_npop(vec, vec->len);

	while (c != '\n') {

		i = fgetc(stdin);

		if (i == EOF) {

#if BC_ENABLE_SIGNALS
			if (errno == EINTR) {

				bcg.sigc = bcg.sig;
				bcg.signe = 0;

				if (bcg.ttyin) {
					bc_vm_puts(bc_program_ready_msg, stderr);
					bc_vm_fflush(stderr);
					if (!bcg.posix) bc_vm_puts(prompt, stdout);
					bc_vm_fflush(stdout);
				}

				continue;
			}
#endif // BC_ENABLE_SIGNALS

			return BC_STATUS_IO_ERR;
		}

		c = (signed char) i;
		if (i > UCHAR_MAX || BC_READ_BIN_CHAR(c)) return BC_STATUS_BIN_FILE;
		bc_vec_push(vec, &c);
	}

	bc_vec_pushByte(vec, '\0');

	return BC_STATUS_SUCCESS;
}

BcStatus bc_read_input(BcVm *vm, const char *prompt) {
#if BC_ENABLE_HISTORY
	if (bcg.ttyin) return bc_history_line(&vm->hist, &vm->line, prompt);
	else return bc_read_line(&vm->line, prompt);
#else
	return bc_read_line(&vm->line, prompt);
#endif
}

void bc_read_file(const char *path, char **buf) {

	BcStatus s = BC_STATUS_IO_ERR;
	FILE *f;
	size_t size, read;
	long res;
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
	*buf = bc_vm_malloc(size + 1);

	read = fread(*buf, 1, size, f);
	if (read != size) goto read_err;

	(*buf)[size] = '\0';
	s = BC_STATUS_BIN_FILE;

	for (read = 0; read < size; ++read) {
		if (BC_READ_BIN_CHAR((*buf)[read])) goto read_err;
	}

	fclose(f);

	return;

read_err:
	free(*buf);
malloc_err:
	fclose(f);
	bc_vm_exit(s);
}
