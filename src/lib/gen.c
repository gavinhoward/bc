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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libgen.h>

static const char* const bc_gen_usage = "usage: gen bc_script output\n";
static const char* const bc_gen_header =
  "/*\n * *** AUTOMATICALLY GENERATED from %s. ***\n"
  " * *** DO NOT MODIFY ***\n */\n\n";

#define INVALID_PARAMS (1)
#define MALLOC_FAIL (2)
#define INVALID_INPUT_FILE (3)
#define INVALID_OUTPUT_FILE (4)
#define IO_ERR (5)

int main(int argc, char* argv[]) {

  FILE* in;
  FILE* out;
  int c;
  int count;
  char* buf;
  char* base;

  if (argc != 3) {
    printf(bc_gen_usage);
    return INVALID_PARAMS;
  }

  buf = malloc(strlen(argv[1]) + 1);

  if (!buf) return MALLOC_FAIL;

  strcpy(buf, argv[1]);

  base = basename(buf);

  in = fopen(argv[1], "r");

  if (!in) return INVALID_INPUT_FILE;

  out = fopen(argv[2], "w");

  if (!out) return INVALID_OUTPUT_FILE;

  count = 0;

  if (fprintf(out, bc_gen_header, base) < 0) return IO_ERR;

  if (fprintf(out, "const char* const bc_lib_name = \"%s\";\n\n", base) < 0)
    return IO_ERR;

  if (fprintf(out, "const unsigned char bc_lib[] = {\n") < 0) return IO_ERR;

  while ((c = fgetc(in)) >= 0) {

    int val;

    if (!count) {
      if (fprintf(out, "  ") < 0) return IO_ERR;
    }

    val = fprintf(out, "%d,", c);

    if (val < 0) return IO_ERR;

    count += val;

    if (count > 72) {
      if (fputc('\n', out) == EOF) return IO_ERR;
      count = 0;
    }
  }

  if (!count) {
    if (fputc('\n', out) == EOF) return IO_ERR;
  }

  if (fprintf(out, "0\n};\n") < 0) return IO_ERR;

  free(buf);
  fclose(in);
  fclose(out);

  return 0;
}
