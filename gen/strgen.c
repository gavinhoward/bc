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
	"// Licensed under the 2-clause BSD license.\n"
	"// *** AUTOMATICALLY GENERATED FROM %s. DO NOT MODIFY. ***\n";

static const char* const bc_gen_include = "#include <%s>\n\n";
static const char* const bc_gen_label = "const char *%s = \"%s\";\n\n";
static const char* const bc_gen_ifdef = "#if %s\n";
static const char* const bc_gen_endif = "#endif // %s\n";
static const char* const bc_gen_name = "const char %s[] = {\n";

#define IO_ERR (1)
#define INVALID_INPUT_FILE (2)
#define INVALID_PARAMS (3)

#define MAX_WIDTH (74)

int main(int argc, char *argv[]) {

	FILE *in, *out;
	char *label, *define, *name, *include;
	int c, count, slashes, err = IO_ERR;
	bool has_label, has_define, remove_tabs;

	if (argc < 5) {
		printf("usage: %s input output name header [label [define [remove_tabs]]]\n", argv[0]);
		return INVALID_PARAMS;
	}

	name = argv[3];
	include = argv[4];

	has_label = (argc > 5 && strcmp("", argv[5]) != 0);
	label = has_label ? argv[5] : "";

	has_define = (argc > 6 && strcmp("", argv[6]) != 0);
	define = has_define ? argv[6] : "";

	remove_tabs = (argc > 7);

	in = fopen(argv[1], "r");
	if (!in) return INVALID_INPUT_FILE;

	out = fopen(argv[2], "w");
	if (!out) goto out_err;

	if (fprintf(out, bc_gen_header, argv[1]) < 0) goto err;
	if (has_define && fprintf(out, bc_gen_ifdef, define) < 0) goto err;
	if (fprintf(out, bc_gen_include, include) < 0) goto err;
	if (has_label && fprintf(out, bc_gen_label, label, argv[1]) < 0) goto err;
	if (fprintf(out, bc_gen_name, name) < 0) goto err;

	c = count = slashes = 0;

	while (slashes < 2 && (c = fgetc(in)) >= 0) {
		slashes += (slashes == 1 && c == '/' && fgetc(in) == '\n');
		slashes += (!slashes && c == '/' && fgetc(in) == '*');
	}

	if (c < 0) {
		err = INVALID_INPUT_FILE;
		goto err;
	}

	while ((c = fgetc(in)) == '\n');

	while (c >= 0) {

		int val;

		if (!remove_tabs || c != '\t') {

			if (!count && fputc('\t', out) == EOF) goto err;

			val = fprintf(out, "%d,", c);
			if (val < 0) goto err;

			count += val;

			if (count > MAX_WIDTH) {
				count = 0;
				if (fputc('\n', out) == EOF) goto err;
			}
		}

		c = fgetc(in);
	}

	if (!count && (fputc(' ', out) == EOF || fputc(' ', out) == EOF)) goto err;
	if (fprintf(out, "0\n};\n") < 0) goto err;

	err = (has_define && fprintf(out, bc_gen_endif, define) < 0);

err:
	fclose(out);
out_err:
	fclose(in);
	return err;
}
