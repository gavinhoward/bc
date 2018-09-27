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
 * Common code for the lexers.
 *
 */

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>

#include <status.h>
#include <lex.h>
#include <vm.h>

BcStatus bc_lex_string(BcLex *l, char end) {

	BcStatus s;
	const char *buf;
	size_t len, nls = 0, i = l->idx;
	char c;

	l->t.t = BC_LEX_STRING;

	for (; (c = l->buffer[i]) != end && c != '\0'; ++i) nls += (c == '\n');

	if (c == '\0') {
		l->idx = i;
		return BC_STATUS_LEX_NO_STRING_END;
	}

	len = i - l->idx;
	if (len > (unsigned long) BC_MAX_STRING) return BC_STATUS_EXEC_STRING_LEN;

	buf = l->buffer + l->idx;
	if ((s = bc_vec_string(&l->t.v, len, buf))) return s;

	l->idx = i + 1;
	l->line += nls;

	return BC_STATUS_SUCCESS;
}

void bc_lex_lineComment(BcLex *l) {
	l->t.t = BC_LEX_WHITESPACE;
	while (l->idx < l->len && l->buffer[l->idx++] != '\n');
	--l->idx;
}

BcStatus bc_lex_number(BcLex *l, char start) {

	BcStatus s;
	const char *buf = l->buffer + l->idx;
	size_t len, hits, bslashes = 0, i = 0, j;
	char c = buf[i];
	bool last_pt, pt = start == '.';

	last_pt = pt;
	l->t.t = BC_LEX_NUMBER;

	while (c && ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') ||
	             (c == '.' && !pt) || (c == '\\' && buf[i + 1] == '\n')))
	{
		if (c != '\\') {
			last_pt = c == '.';
			pt = pt || last_pt;
		}
		else {
			++i;
			bslashes += 1;
		}

		c = buf[++i];
	}

	len = i + 1 * !last_pt - bslashes * 2;
	if (len > BC_MAX_NUM) return BC_STATUS_EXEC_NUM_LEN;

	bc_vec_npop(&l->t.v, l->t.v.len);
	if ((s = bc_vec_expand(&l->t.v, len + 1))) return s;
	if ((s = bc_vec_push(&l->t.v, 1, &start))) return s;

	buf -= 1;
	hits = 0;

	for (j = 1; j < len + hits * 2; ++j) {

		c = buf[j];

		// If we have hit a backslash, skip it. We don't have
		// to check for a newline because it's guaranteed.
		if (hits < bslashes && c == '\\') {
			++hits;
			++j;
			continue;
		}

		if ((s = bc_vec_push(&l->t.v, 1, &c))) return s;
	}

	if ((s = bc_vec_pushByte(&l->t.v, '\0'))) return s;
	l->idx += i;

	return BC_STATUS_SUCCESS;
}

BcStatus bc_lex_name(BcLex *l) {

	BcStatus s;
	size_t i;
	char c;
	const char *buf = l->buffer + l->idx - 1;

	for (i = 0; i < sizeof(bc_lex_kws) / sizeof(bc_lex_kws[0]); ++i) {

		unsigned long len = (unsigned long) bc_lex_kws[i].len;
		if (!strncmp(buf, bc_lex_kws[i].name, len)) {

			l->t.t = BC_LEX_KEY_AUTO + (BcLexToken) i;

			if (!bc_lex_kws[i].posix &&
			    (s = bc_vm_posix_error(BC_STATUS_POSIX_BAD_KEYWORD, l->file,
			                           l->line, bc_lex_kws[i].name)))
			{
				return s;
			}

			// We minus 1 because the index has already been incremented.
			l->idx += len - 1;

			return BC_STATUS_SUCCESS;
		}
	}

	l->t.t = BC_LEX_NAME;

	i = 0;
	c = buf[i];

	while ((c >= 'a' && c<= 'z') || (c >= '0' && c <= '9') || c == '_')
			c = buf[++i];

	if (i > 1 && (s = bc_vm_posix_error(BC_STATUS_POSIX_NAME_LEN,
	                                    l->file, l->line, buf)))
	{
		return s;
	}

	if (i > BC_MAX_STRING) return BC_STATUS_EXEC_NAME_LEN;
	if ((s = bc_vec_string(&l->t.v, i, buf))) return s;

	// Increment the index. We minus 1 because it has already been incremented.
	l->idx += i - 1;

	return BC_STATUS_SUCCESS;
}

BcStatus bc_lex_init(BcLex *l, BcLexNext next) {
	assert(l);
	l->next = next;
	return bc_vec_init(&l->t.v, sizeof(uint8_t), NULL);
}

void bc_lex_free(BcLex *l) {
	assert(l);
	bc_vec_free(&l->t.v);
}

void bc_lex_file(BcLex *l, const char *file) {
	assert(l && file);
	l->line = 1;
	l->newline = false;
	l->file = file;
}

BcStatus bc_lex_text(BcLex *l, const char *text) {
	assert(l && text);
	l->buffer = text;
	l->idx = 0;
	l->len = strlen(text);
	l->t.t = BC_LEX_INVALID;
	return l->next(l);
}
