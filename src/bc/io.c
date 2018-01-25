#include <stdio.h>
#include <stdlib.h>

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

	if (!fp) return (size_t) -1;

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

	return (size_t) -1;
}
