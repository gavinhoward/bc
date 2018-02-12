/*
 * Copyright 2018 Contributors
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.

 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.

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
 *******************************************************************************
 *
 * A special license exemption is granted to the Toybox project and to the
 * Android Open Source Project to use this source under the following BSD
 * 0-Clause License:
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
 *******************************************************************************
 *
 * Code to handle I/O.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include <bc/bc.h>
#include <bc/io.h>

long bc_io_frag(char *buf, long len, int term, BcIoGetc bcgetc, void* ctx) {

  long i;
  int c;

  if (!buf || len < 0 || !bcgetc) return -1;

  for (c = (~term) | 1, i = 0; i < len; i++) {

    if (c == (int) '\0' || c == term || (c = bcgetc(ctx)) == EOF) {
      buf[i] = '\0';
      break;
    }

    buf[i] = (char) c;
  }

  return i;
}

static int bc_io_xfgetc(void* ctx) {
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

size_t bc_io_fgetline(char** p, size_t* n, FILE* fp) {

  size_t mlen, slen, dlen, len;
  char* s;
  char* t;

  if (!fp) return BC_INVALID_IDX;

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

      return slen;
    }
  }

  return BC_INVALID_IDX;
}
