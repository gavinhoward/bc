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
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <io.h>
#include <bc.h>

BcStatus bc_io_getline(char **buf, size_t *n, FILE *f) {

  char *buf2, *temp;
  size_t size, len, new_size;
  bool done;

  buf2 = *buf;
  size = *n;

  do {

    if (!fgets(buf2, size, f)) {
      if (errno == EINTR) bcg.sig_int_catches = bcg.sig_int;
      else return BC_STATUS_IO_ERR;
    }

    len = strlen(buf2);
    done = len != size || buf2[len - 1] == '\n';

    if (!done) {

      new_size = *n * 2;
      temp = realloc(*buf, new_size + 1);

      if (!temp) return BC_STATUS_IO_ERR;

      *buf = temp;
      buf2 = temp + *n * sizeof(char);
      size = new_size - *n;
      *n = new_size;
    }

  } while (!done);

  return BC_STATUS_SUCCESS;
}

BcStatus bc_io_fread(const char *path, char **buf) {

  BcStatus st;
  FILE *f;
  size_t size, read;

  assert(path && buf);

  f = fopen(path, "r");

  if (!f) return BC_STATUS_EXEC_FILE_ERR;

  fseek(f, 0, SEEK_END);
  size = ftell(f);

  fseek(f, 0, SEEK_SET);

  *buf = malloc(size + 1);

  if (!*buf) {
    st = BC_STATUS_MALLOC_FAIL;
    goto malloc_err;
  }

  read = fread(*buf, 1, size, f);

  if (read != size) {
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
