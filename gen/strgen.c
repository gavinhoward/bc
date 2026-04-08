/*
 * *****************************************************************************
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2018-2026 Gavin D. Howard and contributors.
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
 * Generates a const array from a bc script.
 *
 */

#include <assert.h>
#include <stdbool.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>

#include <fcntl.h>
#include <sys/stat.h>

#ifndef _WIN32
#include <unistd.h>
#endif // _WIN32

// For some reason, Windows can't have this header.
#ifndef _WIN32
#include <libgen.h>
#endif // _WIN32

// This pulls in cross-platform stuff.
#include <status.h>

// clang-format off

// The usage help.
static const char* const bc_gen_usage =
	"usage: %s input output exclude name [label [define [remove_tabs]]]\n";

static const char* const bc_gen_ex_start = "{{ A H N HN }}";
static const char* const bc_gen_ex_end = "{{ end }}";

// This is exactly what it looks like. It just slaps a simple license header on
// the generated C source file.
static const char* const bc_gen_header =
	"// Copyright (c) 2018-2026 Gavin D. Howard and contributors.\n"
	"// Licensed under the 2-clause BSD license.\n"
	"// *** AUTOMATICALLY GENERATED FROM %s. DO NOT MODIFY. ***\n\n";
// clang-format on

// These are just format strings used to generate the C source.
static const char* const bc_gen_label = "const char *%s = \"%s\";\n\n";
static const char* const bc_gen_label_extern = "extern const char *%s;\n\n";
static const char* const bc_gen_ifdef = "#if %s\n";
static const char* const bc_gen_endif = "#endif // %s\n";
static const char* const bc_gen_name = "const char %s[] = {\n";
static const char* const bc_gen_name_extern = "extern const char %s[];\n\n";

// Error codes. We can't use 0 because these are used as exit statuses, and 0
// as an exit status is not an error.
#define IO_ERR (1)
#define INVALID_INPUT_FILE (2)
#define INVALID_PARAMS (3)

// This is the max width to print characters to the screen. This is to ensure
// that lines don't go much over 80 characters.
#define MAX_WIDTH (72)

#define BC_GEN_READ_STREAM_CHUNK ((size_t) 8192)
#define BC_GEN_READ_STREAM_MAX ((size_t) (1024U * 1024U * 1024U))

/**
 * Open a file. This function is to smooth over differences between POSIX and
 * Windows.
 * @param f         A pointer to the FILE pointer that will be initialized.
 * @param filename  The name of the file.
 * @param mode      The mode to open the file in.
 */
static void
open_file(FILE** f, const char* filename, const char* mode)
{
#ifndef _WIN32

	*f = fopen(filename, mode);

#else // _WIN32

	// We want the file pointer to be NULL on failure, but fopen_s() is not
	// guaranteed to set it.
	*f = NULL;
	fopen_s(f, filename, mode);

#endif // _WIN32
}

/**
 * A portability file open function. This is copied from src/read.c. Make sure
 * to update that if this changes.
 * @param path  The path to the file to open.
 * @param mode  The mode to open in.
 */

#ifndef _WIN32

/**
 * Sets close-on-exec if O_CLOEXEC is unavailable at open() time.
 * @param fd  The fd to mark.
 */
static void
bc_read_cloexec(int fd)
{
#if !defined(O_CLOEXEC) && defined(F_GETFD) && defined(F_SETFD) && defined(FD_CLOEXEC)
	int flags;

	if (fd < 0) return;

	flags = fcntl(fd, F_GETFD);
	if (flags >= 0) (void) fcntl(fd, F_SETFD, flags | FD_CLOEXEC);
#else // !defined(O_CLOEXEC) && defined(F_GETFD) && defined(F_SETFD) && defined(FD_CLOEXEC)
	(void) fd;
#endif // !defined(O_CLOEXEC) && defined(F_GETFD) && defined(F_SETFD) && defined(FD_CLOEXEC)
}

#endif // !_WIN32

static int
bc_read_open(const char* path, int mode)
{
	int fd;

#ifndef _WIN32
	int flags = mode;

#ifdef O_CLOEXEC
	flags |= O_CLOEXEC;
#endif // O_CLOEXEC

#ifdef O_NOCTTY
	flags |= O_NOCTTY;
#endif // O_NOCTTY

	fd = open(path, flags);
	bc_read_cloexec(fd);
#else // _WIN32
	fd = -1;
	open(&fd, path, mode);
#endif

	return fd;
}

/**
 * Adds @a a and @a b and exits on overflow.
 * @param a  The first operand.
 * @param b  The second operand.
 * @return   The sum of @a a and @a b.
 */
static size_t
bc_gen_growSize(size_t a, size_t b)
{
	size_t res = a + b;

	if (BC_ERR(res < a))
	{
		fprintf(stderr, "Could not malloc\n");
		exit(INVALID_INPUT_FILE);
	}

	return res;
}

/**
 * Reads a file and returns the file as a string. This has been copied from
 * src/read.c. Make sure to change that if this changes.
 * @param path  The path to the file.
 * @return      The contents of the file as a string.
 */
static char*
bc_read_file(const char* path)
{
	int e = IO_ERR;
	size_t size, to_read;
	struct stat pstat;
	int fd = -1;
	char* buf;
	char* buf2;
	bool regular;

	// This has been copied from src/read.c. Make sure to change that if this
	// changes.

	assert(path != NULL);

#if BC_DEBUG
	// Need this to quiet MSan.
	// NOLINTNEXTLINE
	memset(&pstat, 0, sizeof(struct stat));
#endif // BC_DEBUG

	fd = bc_read_open(path, O_RDONLY);

	// If we can't read a file, we just barf.
	if (BC_ERR(fd < 0))
	{
		fprintf(stderr, "Could not open file: %s\n", path);
		exit(INVALID_INPUT_FILE);
	}

	// The reason we call fstat is to eliminate TOCTOU race conditions. This
	// way, we have an open file, so it's not going anywhere.
	if (BC_ERR(fstat(fd, &pstat) == -1))
	{
		fprintf(stderr, "Could not stat file: %s\n", path);
		if (fd >= 0) close(fd);
		exit(INVALID_INPUT_FILE);
	}

	// Make sure it's not a directory.
	if (BC_ERR(S_ISDIR(pstat.st_mode)))
	{
		fprintf(stderr, "Path is directory: %s\n", path);
		if (fd >= 0) close(fd);
		exit(INVALID_INPUT_FILE);
	}

	regular = true;
#if defined(S_ISREG)
	regular = S_ISREG(pstat.st_mode);
#endif // defined(S_ISREG)

	if (regular)
	{
		// Reject sizes that cannot fit in size_t plus the trailing nul.
		if (BC_ERR(pstat.st_size < 0 ||
		           (uintmax_t) pstat.st_size > (uintmax_t) (SIZE_MAX - 1)))
		{
			fprintf(stderr, "Invalid file size: %s\n", path);
			if (fd >= 0) close(fd);
			exit(INVALID_INPUT_FILE);
		}

		// Get the size of the file and allocate that much.
		size = (size_t) pstat.st_size;
		buf = (char*) malloc(bc_gen_growSize(size, 1));
		if (buf == NULL)
		{
			fprintf(stderr, "Could not malloc\n");
			if (fd >= 0) close(fd);
			exit(INVALID_INPUT_FILE);
		}

		buf2 = buf;
		to_read = size;

		while (to_read)
		{
			size_t chunk = to_read > (size_t) INT_MAX ? (size_t) INT_MAX : to_read;

			ssize_t r = read(fd, buf2, chunk);
			if (BC_ERR(r < 0))
			{
				if (errno == EINTR) continue;
				if (fd >= 0) close(fd);
				free(buf);
				exit(e);
			}
			if (BC_ERR(r == 0))
			{
				if (fd >= 0) close(fd);
				free(buf);
				exit(e);
			}
			to_read -= (size_t) r;
			buf2 += (size_t) r;
		}
	}
	else
	{
		size_t cap, req;
		char stream_buf[BC_GEN_READ_STREAM_CHUNK];

		cap = BC_GEN_READ_STREAM_CHUNK;
		size = 0;

		buf = (char*) malloc(cap);
		if (buf == NULL)
		{
			fprintf(stderr, "Could not malloc\n");
			if (fd >= 0) close(fd);
			exit(INVALID_INPUT_FILE);
		}

		for (;;)
		{
			ssize_t r = read(fd, stream_buf, sizeof(stream_buf));

			if (r == 0) break;
			if (BC_ERR(r < 0))
			{
				if (errno == EINTR) continue;
				if (fd >= 0) close(fd);
				free(buf);
				exit(e);
			}

			if (BC_ERR((size_t) r > BC_GEN_READ_STREAM_MAX - size))
			{
				if (fd >= 0) close(fd);
				free(buf);
				exit(INVALID_INPUT_FILE);
			}

			if (BC_ERR(size > SIZE_MAX - (size_t) r))
			{
				if (fd >= 0) close(fd);
				free(buf);
				exit(INVALID_INPUT_FILE);
			}

			req = size + (size_t) r;

			if (req > cap)
			{
				size_t ncap = cap;

				while (ncap < req)
				{
					if (ncap > SIZE_MAX / 2) ncap = req;
					else ncap += ncap;
				}

				buf2 = (char*) realloc(buf, ncap);

				if (buf2 == NULL)
				{
					if (fd >= 0) close(fd);
					free(buf);
					exit(INVALID_INPUT_FILE);
				}

				buf = buf2;
				cap = ncap;
			}

			memcpy(buf + size, stream_buf, (size_t) r);
			size = req;
		}

		if (size == cap)
		{
			cap = bc_gen_growSize(cap, 1);
			buf2 = (char*) realloc(buf, cap);

			if (buf2 == NULL)
			{
				if (fd >= 0) close(fd);
				free(buf);
				exit(INVALID_INPUT_FILE);
			}

			buf = buf2;
		}
	}

	// Got to have a nul byte.
	buf[size] = '\0';

	if (fd >= 0) close(fd);
	fd = -1;

	return buf;
}

/**
 * Outputs a label, which is a string literal that the code can use as a name
 * for the file that is being turned into a string. This is important for the
 * math libraries because the parse and lex code expects a filename. The label
 * becomes the filename for the purposes of lexing and parsing.
 *
 * The label is generated from bc_gen_label (above). It has the form:
 *
 * const char *<label_name> = <label>;
 *
 * This function is also needed to smooth out differences between POSIX and
 * Windows, specifically, the fact that Windows uses backslashes for filenames
 * and that backslashes have to be escaped in a string literal.
 *
 * @param out    The file to output to.
 * @param label  The label name.
 * @param name   The actual label text, which is a filename.
 * @return       Positive if no error, negative on error, just like *printf().
 */
static int
output_label(FILE* out, const char* label, const char* name)
{
#ifndef _WIN32

	return fprintf(out, bc_gen_label, label, name);

#else // _WIN32

	size_t i, count = 0, len = strlen(name);
	char* buf;
	int ret;

	// This loop counts how many backslashes there are in the label.
	for (i = 0; i < len; ++i)
	{
		count += (name[i] == '\\');
	}

	buf = (char*) malloc(len + 1 + count);
	if (buf == NULL) return -1;

	count = 0;

	// This loop is the meat of the Windows version. What it does is copy the
	// label byte-for-byte, unless it encounters a backslash, in which case, it
	// copies the backslash twice to have it escaped properly in the string
	// literal.
	for (i = 0; i < len; ++i)
	{
		buf[i + count] = name[i];

		if (name[i] == '\\')
		{
			count += 1;
			buf[i + count] = name[i];
		}
	}

	buf[i + count] = '\0';

	ret = fprintf(out, bc_gen_label, label, buf);

	free(buf);

	return ret;

#endif // _WIN32
}

/**
 * This program generates C strings (well, actually, C char arrays) from text
 * files. It generates 1 C source file. The resulting file has this structure:
 *
 * <Copyright Header>
 *
 * [<Label Extern>]
 *
 * <Char Array Extern>
 *
 * [<Preprocessor Guard Begin>]
 * [<Label Definition>]
 *
 * <Char Array Definition>
 * [<Preprocessor Guard End>]
 *
 * Anything surrounded by square brackets may not be in the final generated
 * source file.
 *
 * The required command-line parameters are:
 *
 * input    Input filename.
 * output   Output filename.
 * exclude  Whether to exclude extra math-only stuff.
 * name     The name of the char array.
 *
 * The optional parameters are:
 *
 * label        If given, a label for the char array. See the comment for the
 *              output_label() function. It is meant as a "filename" for the
 *              text when processed by bc and dc. If label is given, then the
 *              <Label Extern> and <Label Definition> will exist in the
 *              generated source file.
 * define       If given, a preprocessor macro that should be used as a guard
 *              for the char array and its label. If define is given, then
 *              <Preprocessor Guard Begin> will exist in the form
 *              "#if <define>" as part of the generated source file, and
 *              <Preprocessor Guard End> will exist in the form
 *              "endif // <define>".
 * remove_tabs  If this parameter exists, it must be an integer. If it is
 *              non-zero, then tabs are removed from the input file text before
 *              outputting to the output char array.
 *
 * All text files that are transformed have license comments. This program finds
 * the end of that comment and strips it out as well.
 */
int
main(int argc, char* argv[])
{
	char* in;
	FILE* out;
	const char* label;
	const char* define;
	char* name;
	unsigned int count, slashes, err = IO_ERR;
	bool has_label, has_define, remove_tabs, exclude_extra_math;
	size_t i;

	if (argc < 5)
	{
		printf(bc_gen_usage, argv[0]);
		return INVALID_PARAMS;
	}

	exclude_extra_math = (strtoul(argv[3], NULL, 10) != 0);

	name = argv[4];

	has_label = (argc > 5 && strcmp("", argv[5]) != 0);
	label = has_label ? argv[5] : "";

	has_define = (argc > 6 && strcmp("", argv[6]) != 0);
	define = has_define ? argv[6] : "";

	remove_tabs = (argc > 7 && atoi(argv[7]) != 0);

	in = bc_read_file(argv[1]);
	if (in == NULL) return INVALID_INPUT_FILE;

	open_file(&out, argv[2], "w");
	if (out == NULL) goto out_err;

	if (fprintf(out, bc_gen_header, argv[1]) < 0) goto err;
	if (has_label && fprintf(out, bc_gen_label_extern, label) < 0) goto err;
	if (fprintf(out, bc_gen_name_extern, name) < 0) goto err;
	if (has_define && fprintf(out, bc_gen_ifdef, define) < 0) goto err;
	if (has_label && output_label(out, label, argv[1]) < 0) goto err;
	if (fprintf(out, bc_gen_name, name) < 0) goto err;

	i = count = slashes = 0;

	// This is where the end of the license comment is found.
	while (slashes < 2 && in[i] > 0)
	{
		if (slashes == 1 && in[i] == '*' && in[i + 1] == '/' &&
		    (in[i + 2] == '\n' || in[i + 2] == '\r'))
		{
			slashes += 1;
			i += 2;
		}
		else if (!slashes && in[i] == '/' && in[i + 1] == '*')
		{
			slashes += 1;
			i += 1;
		}

		i += 1;
	}

	// The file is invalid if the end of the license comment could not be found.
	if (in[i] == 0)
	{
		fprintf(stderr, "Could not find end of license comment\n");
		err = INVALID_INPUT_FILE;
		goto err;
	}

	i += 1;

	// Do not put extra newlines at the beginning of the char array.
	while (in[i] == '\n' || in[i] == '\r')
	{
		i += 1;
	}

	// This loop is what generates the actual char array. It counts how many
	// chars it has printed per line in order to insert newlines at appropriate
	// places. It also skips tabs if they should be removed.
	while (in[i] != 0)
	{
		int val;

		if (in[i] == '\r')
		{
			i += 1;
			continue;
		}

		if (!remove_tabs || in[i] != '\t')
		{
			// Check for excluding something for extra math.
			if (in[i] == '{')
			{
				// If we found the start...
				if (!strncmp(in + i, bc_gen_ex_start, strlen(bc_gen_ex_start)))
				{
					if (exclude_extra_math)
					{
						// Get past the braces.
						i += 2;

						// Find the end of the end.
						while (in[i] != '{' && strncmp(in + i, bc_gen_ex_end,
						                               strlen(bc_gen_ex_end)))
						{
							i += 1;
						}

						i += strlen(bc_gen_ex_end);

						// Skip the last newline.
						if (in[i] == '\r') i += 1;
						i += 1;
						continue;
					}
					else
					{
						i += strlen(bc_gen_ex_start);

						// Skip the last newline.
						if (in[i] == '\r') i += 1;
						i += 1;
						continue;
					}
				}
				else if (!exclude_extra_math &&
				         !strncmp(in + i, bc_gen_ex_end, strlen(bc_gen_ex_end)))
				{
					i += strlen(bc_gen_ex_end);

					// Skip the last newline.
					if (in[i] == '\r') i += 1;
					i += 1;
					continue;
				}
			}

			// Print a tab if we are at the beginning of a line.
			if (!count && fputc('\t', out) == EOF) goto err;

			// Print the character.
			val = fprintf(out, "%d,", in[i]);
			if (val < 0) goto err;

			// Adjust the count.
			count += (unsigned int) val;
			if (count > MAX_WIDTH)
			{
				count = 0;
				if (fputc('\n', out) == EOF) goto err;
			}
		}

		i += 1;
	}

	// Make sure the end looks nice and insert the NUL byte at the end.
	if (!count && (fputc(' ', out) == EOF || fputc(' ', out) == EOF)) goto err;
	if (fprintf(out, "0\n};\n") < 0) goto err;

	err = (has_define && fprintf(out, bc_gen_endif, define) < 0);

err:
	fclose(out);
out_err:
	free(in);
	return (int) err;
}
