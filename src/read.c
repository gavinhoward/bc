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
#include <string.h>

#include <signal.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <read.h>
#include <history.h>
#include <program.h>
#include <vm.h>

static bool bc_read_binary(const char *buf, size_t size) {

	size_t i;

	for (i = 0; i < size; ++i) {
		if (BC_ERR(BC_READ_BIN_CHAR(buf[i]))) return true;
	}

	return false;
}

BcStatus bc_read_chars(BcVec *vec, const char *prompt) {

	int i;
	signed char c = 0;

	assert(vec && vec->size == sizeof(char));

	bc_vec_npop(vec, vec->len);

	if ((BC_TTYIN || BC_I) && !BC_S) {
		bc_vm_puts(prompt, stderr);
		bc_vm_fflush(stderr);
	}

	while (BC_NO_SIGNAL && c != '\n') {

		i = fgetc(stdin);

		if (BC_UNLIKELY(i == EOF)) {

#if BC_ENABLE_SIGNALS
			if (errno == EINTR) {

				if (BC_SIGTERM) return BC_STATUS_SIGNAL;

				vm->sig = 0;

				if (BC_TTYIN || BC_I) {
					bc_vm_puts(bc_program_ready_msg, stderr);
					if (!BC_S) bc_vm_puts(prompt, stderr);
					bc_vm_fflush(stderr);
				}
				else return BC_STATUS_SIGNAL;

				continue;
			}
#endif // BC_ENABLE_SIGNALS

			bc_vec_pushByte(vec, '\0');
			return BC_STATUS_EOF;
		}

		c = (signed char) i;
		bc_vec_push(vec, &c);
	}

	bc_vec_pushByte(vec, '\0');

	return BC_SIGNAL ? BC_STATUS_SIGNAL : BC_STATUS_SUCCESS;
}

BcStatus bc_read_line(BcVec *vec, const char *prompt) {

	BcStatus s;

	// We are about to output to stderr, so flush stdout to
	// make sure that we don't get the outputs mixed up.
	bc_vm_fflush(stdout);

#if BC_ENABLE_HISTORY
	s = bc_history_line(&vm->history, vec, prompt);
#else // BC_ENABLE_HISTORY
	s = bc_read_chars(vec, prompt);
#endif // BC_ENABLE_HISTORY

	if (BC_ERR(s && s != BC_STATUS_EOF)) return s;
	if (BC_ERR(bc_read_binary(vec->v, vec->len - 1)))
		return bc_vm_verr(BC_ERROR_FATAL_BIN_FILE, bc_program_stdin_name);

	return s;
}

BcStatus bc_read_file(const char *path, char **buf) {

	BcError e = BC_ERROR_FATAL_IO_ERR;
	FILE *f;
	size_t size, read;
	long res;
	struct stat pstat;

	assert(path);

	f = fopen(path, "r");
	if (BC_ERR(!f)) return bc_vm_verr(BC_ERROR_FATAL_FILE_ERR, path);
	if (BC_ERR(fstat(fileno(f), &pstat) == -1)) goto malloc_err;

	if (BC_ERR(S_ISDIR(pstat.st_mode))) {
		e = BC_ERROR_FATAL_PATH_DIR;
		goto malloc_err;
	}

	if (BC_ERR(fseek(f, 0, SEEK_END) == -1)) goto malloc_err;
	res = ftell(f);
	if (BC_ERR(res < 0)) goto malloc_err;
	if (BC_ERR(fseek(f, 0, SEEK_SET) == -1)) goto malloc_err;

	size = (size_t) res;
	*buf = bc_vm_malloc(size + 1);

	read = fread(*buf, 1, size, f);
	if (BC_ERR(read != size)) goto read_err;

	(*buf)[size] = '\0';

	if (BC_ERR(bc_read_binary(*buf, size))) {
		e = BC_ERROR_FATAL_BIN_FILE;
		goto read_err;
	}

	fclose(f);

	return BC_STATUS_SUCCESS;

read_err:
	free(*buf);
malloc_err:
	fclose(f);
	return bc_vm_verr(e, path);
}
