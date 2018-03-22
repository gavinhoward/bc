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
 * Code to handle I/O.
 *
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <status.h>
#include <io.h>

long bc_io_frag(char *buf, long len, int term, FILE *fp) {

  long i;
  int c;

  assert(buf && len >= 0);

  for (c = (~term) | 1, i = 0; i < len; i++) {

    if (c == (int) '\0' || c == term || (c = fgetc(fp)) == EOF) {
      buf[i] = '\0';
      break;
    }

    buf[i] = (char) c;
  }

  return i;
}

BcStatus bc_io_fgetline(char** p, size_t *n, FILE *fp) {

  size_t mlen, slen, dlen, len;
  char *s;
  char *t;

  assert(p && n && fp);

  if (!p) {

    char blk[64];

    for (slen = 0; ; slen += 64) {

      len = (size_t) bc_io_frag(blk, 64, (int) '\n', fp);

      if (len != 64 || blk[len - 1] == '\n' || blk[len - 1] == '\0') {
        *n = slen + len;
        return BC_STATUS_SUCCESS;
      }
    }
  }

  mlen = 8;
  slen = 0;

  s = *p;

  for (;;) {

    mlen += mlen;

    if (!(t = realloc(s, mlen))) break;

    s = t;
    dlen = mlen - slen - 1;

    len = (size_t) bc_io_frag(s + slen, dlen, (int) '\n', fp);

    slen += len;

    if (len < dlen || t[slen - 1] == '\n' || t[slen - 1] == '\0') {

      s[slen] = '\0';
      *p = s;
      *n = slen;

      return BC_STATUS_SUCCESS;
    }
  }

  return BC_STATUS_IO_ERR;
}

BcStatus bc_io_fread(const char *path, char **buf) {

  BcStatus status;
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
    status = BC_STATUS_MALLOC_FAIL;
    goto malloc_err;
  }

  read = fread(*buf, 1, size, f);

  if (read != size) {
    status = BC_STATUS_IO_ERR;
    goto read_err;
  }

  (*buf)[size] = '\0';

  fclose(f);

  return BC_STATUS_SUCCESS;

read_err:

  free(*buf);

malloc_err:

  fclose(f);

  return status;
}
