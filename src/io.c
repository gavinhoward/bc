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
#include <bc.h>

BcStatus bc_io_getline(char **buf, size_t *n) {

  char *temp;
  int c;
  size_t size, i;

  for (i = 0, c = 0; c != '\n'; ++i) {

    if (i == *n) {

      size = *n * 2;

      if (size > (1 << 20) || !(temp = realloc(*buf, size + 1)))
        return BC_STATUS_MALLOC_FAIL;

      *buf = temp;
      *n = size;
    }

    if ((c = fgetc(stdin)) == EOF) {

      if (errno == EINTR) {
        bcg.sig_int_catches = bcg.sig_int;
        --i;
        continue;
      }
      else return BC_STATUS_IO_ERR;
    }
    else if (!c || (iscntrl(c) && !isspace(c)) || c > SCHAR_MAX)
      return BC_STATUS_BINARY_FILE;

    (*buf)[i] = c;
  }

  (*buf)[i] = '\0';

  return BC_STATUS_SUCCESS;
}

BcStatus bc_io_fread(const char *path, char **buf) {

  BcStatus st;
  FILE *f;
  size_t size, read;

  assert(path && buf);

  if (!(f = fopen(path, "r"))) return BC_STATUS_EXEC_FILE_ERR;

  fseek(f, 0, SEEK_END);
  size = ftell(f);
  fseek(f, 0, SEEK_SET);

  if (!(*buf = malloc(size + 1))) {
    st = BC_STATUS_MALLOC_FAIL;
    goto malloc_err;
  }

  if ((read = fread(*buf, 1, size, f)) != size) {
    st = BC_STATUS_IO_ERR;
    goto read_err;
  }

  (*buf)[size] = '\0';
  fclose(f);

  return BC_STATUS_SUCCESS;

read_err:
  free(*buf);
malloc_err:
  fclose(f);
  return st;
}
