/*
 * *****************************************************************************
 *
 * Copyright (c) 2018-2020 Gavin D. Howard and contributors.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * *****************************************************************************
 *
 * Code for implementing buffered I/O on my own terms.
 *
 */

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <file.h>
#include <vm.h>

static void bc_file_ultoa(unsigned long long val, char buf[BC_FILE_ULL_LENGTH])
{
	char buf2[BC_FILE_ULL_LENGTH];
	size_t i, len;

	memset(buf2, 0, BC_FILE_ULL_LENGTH);

	// The i = 1 is to ensure that there is a null byte at the end.
	for (i = 1; val; ++i) {
		unsigned long long mod = val % 10;
		buf2[i] = ((char) mod) + '0';
		val /= 10;
	}

	len = i;

	for (i = 0; i <= len; ++i) buf[i] = buf2[len - i];
}

static BcStatus bc_file_output(int fd, const char *buf, size_t n) {

	size_t bytes = 0;

	BC_SIG_ASSERT_LOCKED;

	while (bytes < n) {

		ssize_t written = write(fd, buf + bytes, n - bytes);

		if (BC_ERR(written == -1)) {

			if (errno == EPIPE) {
				return BC_STATUS_EOF;
			}
#if BC_ENABLE_SIGNALS
			else if (errno == EINTR) {
				return BC_STATUS_SIGNAL;
			}
#endif // BC_ENABLE_SIGNALS

			return bc_vm_err(BC_ERROR_FATAL_IO_ERR);
		}

		bytes += (size_t) written;
	}

	return BC_STATUS_SUCCESS;
}

BcStatus bc_file_flush(BcFile *restrict f) {

	BcStatus s;

	s = bc_file_output(f->fd, f->v.v, f->v.len);
	bc_vec_npop(&f->v, f->v.len);

	return s;
}

BcStatus bc_file_printf(BcFile *restrict f, const char *fmt, ...) {

	BcStatus s;
	va_list args;

	va_start(args, fmt);
	s = bc_file_vprintf(f, fmt, args);
	va_end(args);

	return s;
}

BcStatus bc_file_vprintf(BcFile *restrict f, const char *fmt, va_list args) {


}

BcStatus bc_file_write(BcFile *restrict f, const char *buf, size_t n) {

	BcStatus s = BC_STATUS_SUCCESS;

	if (n > f->v.cap - f->v.len) {
		s = bc_file_flush(f);
		if (BC_ERR(s)) return s;
		assert(!f->v.len);
	}

	if (BC_UNLIKELY(n > f->v.cap - f->v.len))
		s = bc_file_output(f->fd, buf, n);
	else bc_vec_npush(&f->v, n, buf);

	return s;
}

BcStatus bc_file_puts(BcFile *restrict f, const char *str) {
	return bc_file_write(f, str, strlen(str));
}

BcStatus bc_file_putchar(BcFile *restrict f, uchar c) {

	BcStatus s = BC_STATUS_SUCCESS;

	if (f->v.len == f->v.cap) {
		s = bc_file_flush(f);
		if (BC_ERR(s)) return s;
	}

	assert(f->v.len < f->v.cap);

	bc_vec_push(&f->v, &c);

	return s;
}

void bc_file_init(BcFile *f, int fd, size_t req) {
	f->fd = fd;
	bc_vec_init(&f->v, sizeof(char), NULL);
	bc_vec_expand(&f->v, bc_vm_growSize(req, 1));
}

void bc_file_free(BcFile *f) {
	bc_file_flush(f);
#ifndef NDEBUG
	bc_vec_free(&f->v);
#endif // NDEBUG
}
