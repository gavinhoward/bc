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

void bc_lex_lineComment(BcLex *l) {
	l->t.t = BC_LEX_WHITESPACE;
	while (l->idx < l->len && l->buffer[l->idx++] != '\n');
	--l->idx;
}

void bc_lex_whitespace(BcLex *l) {
	char c;
	l->t.t = BC_LEX_WHITESPACE;
	for (; (c = l->buffer[l->idx]) != '\n' && isspace(c); ++l->idx);
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
	if ((s = bc_vec_push(&l->t.v, &start))) return s;

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

		if ((s = bc_vec_push(&l->t.v, &c))) return s;
	}

	if ((s = bc_vec_pushByte(&l->t.v, '\0'))) return s;
	l->idx += i;

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
	l->t.t = l->t.last = BC_LEX_INVALID;
	return bc_lex_next(l);
}

BcStatus bc_lex_next(BcLex *l) {

	BcStatus s;

	assert(l);

	if ((l->t.last = l->t.t) == BC_LEX_EOF) return BC_STATUS_LEX_EOF;

	if (l->idx == l->len) {
		l->newline = true;
		l->t.t = BC_LEX_EOF;
		return BC_STATUS_SUCCESS;
	}

	l->line += l->newline;
	l->newline = false;

	// Loop until failure or we don't have whitespace. This
	// is so the parser doesn't get inundated with whitespace.
	while (!(s = l->next(l)) && l->t.t == BC_LEX_WHITESPACE);

	return s;
}
