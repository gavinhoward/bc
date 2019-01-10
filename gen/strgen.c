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
 * Generates a const array from a bc script.
 *
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>

#include <libgen.h>

static const char* const bc_gen_header =
	"// Copyright (c) 2018 Gavin D. Howard.\n"
	"// Licensed under the 0-clause BSD license.\n"
	"// *** AUTOMATICALLY GENERATED FROM %s. DO NOT MODIFY. ***\n";

static const char* const bc_gen_include = "#include <%s>\n\n";
static const char* const bc_gen_label = "const char *%s = \"%s\";\n\n";
static const char* const bc_gen_ifdef = "#if %s\n";
static const char* const bc_gen_endif = "#endif // %s\n";
static const char* const bc_gen_name = "const char %s[] = {\n";

#define INVALID_PARAMS (1)
#define INVALID_INPUT_FILE (2)
#define INVALID_OUTPUT_FILE (3)
#define IO_ERR (4)

#define MAX_WIDTH (74)

int main(int argc, char *argv[]) {

	FILE *in, *out;
	char *label, *define, *name, *include;
	int c, count, err, slashes;
	bool has_label, has_define, remove_tabs;

	err = 0;

	if (argc < 5) {
		printf("usage: %s input output name header [label [define [remove_tabs]]]\n", argv[0]);
		return INVALID_PARAMS;
	}

	name = argv[3];
	include = argv[4];

	printf("Header: %s\n", include);

	has_label = argc > 5 && strcmp("", argv[5]);
	label = has_label ? argv[5] : "";

	has_define = argc > 6 && strcmp("", argv[6]);
	define = has_define ? argv[6] : "";

	remove_tabs = argc > 7;

	in = fopen(argv[1], "r");

	if (!in) return INVALID_INPUT_FILE;

	out = fopen(argv[2], "w");

	if (!out) {
		err = INVALID_OUTPUT_FILE;
		goto out_err;
	}

	if (fprintf(out, bc_gen_header, argv[1]) < 0) {
		err = IO_ERR;
		goto error;
	}

	if (has_define && fprintf(out, bc_gen_ifdef, define) < 0) {
		err = IO_ERR;
		goto error;
	}

	if (fprintf(out, bc_gen_include, include) < 0) {
		err = IO_ERR;
		goto error;
	}

	if (has_label && fprintf(out, bc_gen_label, label, argv[1]) < 0) {
		err = IO_ERR;
		goto error;
	}

	if (fprintf(out, bc_gen_name, name) < 0) {
		err = IO_ERR;
		goto error;
	}

	c = count = slashes = 0;

	while (slashes < 2 && (c = fgetc(in)) >= 0) {
		if (slashes == 1 && c == '/' && fgetc(in) == '\n') ++slashes;
		if (!slashes && c == '/' && fgetc(in) == '*') ++slashes;
	}

	if (c < 0) {
		err = INVALID_INPUT_FILE;
		goto error;
	}

	c = fgetc(in);

	if (c == '\n') c = fgetc(in);

	while (c >= 0) {

		int val;

		if (!remove_tabs || c != '\t') {

			if (!count) {
				if (fputc('\t', out) == EOF) {
					err = IO_ERR;
					goto error;
				}
			}

			val = fprintf(out, "%d,", c);

			if (val < 0) {
				err = IO_ERR;
				goto error;
			}

			count += val;

			if (count > MAX_WIDTH) {

				count = 0;

				if (fputc('\n', out) == EOF) {
					err = IO_ERR;
					goto error;
				}
			}
		}

		c = fgetc(in);
	}

	if (!count) {
		if (fputc(' ', out) == EOF || fputc(' ', out) == EOF) {
			err = IO_ERR;
			goto error;
		}
	}

	if (fprintf(out, "0\n};\n") < 0) {
		err = IO_ERR;
		goto error;
	}

	if (has_define && fprintf(out, bc_gen_endif, define) < 0) err = IO_ERR;

error:
	fclose(out);
out_err:
	fclose(in);
	return err;
}
