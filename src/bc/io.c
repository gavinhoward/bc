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

#include <stdio.h>
#include <stdlib.h>

#include <bc.h>
#include <io.h>

long bc_io_frag(char *buf, long len, int term, BcIoGetc bc_getc, void *ctx) {

  long i;
  int c;

  if (!buf || len < 0 || !bc_getc) return -1;

  for (c = (~term) | 1, i = 0; i < len; i++) {

    if (c == (int) '\0' || c == term || (c = bc_getc(ctx)) == EOF) {
      buf[i] = '\0';
      break;
    }

    buf[i] = (char) c;
  }

  return i;
}

static int bc_io_xfgetc(void *ctx) {
  return fgetc((FILE *) ctx);
}

long bc_io_fgets(char * buf, int n, FILE* fp) {

  long len;

  if (!buf) return -1;

  if (n == 1) {
    buf[0] = '\0';
    return 0;
  }

  if (n < 1 || !fp) return -1;

  len = bc_io_frag(buf, n - 1, (int) '\n', bc_io_xfgetc, fp);

  if (len >= 0) buf[len] = '\0';

  return len;
}

BcStatus bc_io_fgetline(char** p, size_t *n, FILE* fp) {

  size_t mlen, slen, dlen, len;
  char *s;
  char *t;

  if (!p || !n || !fp) return BC_STATUS_INVALID_ARG;

  if (!p) {

    char blk[64];

    for (slen = 0; ; slen += 64) {

      len = (size_t) bc_io_frag(blk, 64, (int) '\n', bc_io_xfgetc, fp);

      if (len != 64 || blk[len - 1] == '\n' || blk[len - 1] == '\0')
        return slen + len;
    }
  }

  mlen = 8;
  slen = 0;

  s = *p;

  for (;;) {

    mlen += mlen;

    if ((t = realloc(s, mlen)) == NULL) break;

    s = t;
    dlen = mlen - slen - 1;

    len = (size_t) bc_io_frag(s + slen, dlen, (int) '\n', bc_io_xfgetc, fp);

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

BcStatus bc_io_fread(const char *path, char** buf) {

  BcStatus status;
  FILE* f;
  size_t size;
  size_t read;

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
