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

BcStatus bc_lex_invalidChar(BcLex *l, char c) {
	l->t = BC_LEX_INVALID;
	return bc_vm_error(BC_ERROR_PARSE_CHAR, l->line, c);
}

void bc_lex_lineComment(BcLex *l) {
	l->t = BC_LEX_WHITESPACE;
	while (l->i < l->len && l->buf[l->i] != '\n') ++l->i;
}

BcStatus bc_lex_comment(BcLex *l) {

	size_t i, nlines = 0;
	const char *buf = l->buf;
	bool end = false;
	char c;

	l->t = BC_LEX_WHITESPACE;

	for (i = ++l->i; !end; i += !end) {

		for (c = buf[i]; c != '*' && c != 0; c = buf[++i]) nlines += c == '\n';

		if (c == 0 || buf[i + 1] == '\0') {
			l->i = i;
			return bc_vm_error(BC_ERROR_PARSE_COMMENT, l->line);
		}

		end = buf[i + 1] == '/';
	}

	l->i = i + 2;
	l->line += nlines;

	return BC_STATUS_SUCCESS;
}

void bc_lex_whitespace(BcLex *l) {
	char c;
	l->t = BC_LEX_WHITESPACE;
	for (c = l->buf[l->i]; c != '\n' && isspace(c); c = l->buf[++l->i]);
}

BcStatus bc_lex_number(BcLex *l, char start) {

	const char *buf = l->buf + l->i;
	size_t len, hits = 0, bslashes = 0, i = 0, j;
	char last_valid, c = buf[i];
	bool last_pt, pt = start == '.';

	last_pt = pt;
	l->t = BC_LEX_NUMBER;
	last_valid = BC_IS_BC ? 'Z' : 'F';

	while (c != 0 && (isdigit(c) || (c >= 'A' && c <= last_valid) ||
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

	if (len > BC_MAX_NUM)
		return bc_vm_error(BC_ERROR_EXEC_NUM_LEN, l->line, BC_MAX_NUM);

	bc_vec_npop(&l->str, l->str.len);
	bc_vec_expand(&l->str, len + 1);
	bc_vec_push(&l->str, &start);

	for (buf -= 1, j = 1; j < len + hits * 2; ++j) {

		c = buf[j];

		// If we have hit a backslash, skip it. We don't have
		// to check for a newline because it's guaranteed.
		if (hits < bslashes && c == '\\') {
			++hits;
			++j;
			continue;
		}

		bc_vec_push(&l->str, &c);
	}

	bc_vec_pushByte(&l->str, '\0');
	l->i += i;

	return BC_STATUS_SUCCESS;
}

BcStatus bc_lex_name(BcLex *l) {

	size_t i = 0;
	const char *buf = l->buf + l->i - 1;
	char c = buf[i];

	l->t = BC_LEX_NAME;

	while ((c >= 'a' && c <= 'z') || isdigit(c) || c == '_') c = buf[++i];

	if (i > BC_MAX_NAME)
		return bc_vm_error(BC_ERROR_EXEC_NAME_LEN, l->line, BC_MAX_NAME);

	bc_vec_string(&l->str, i, buf);

	// Increment the index. We minus 1 because it has already been incremented.
	l->i += i - 1;

	return BC_STATUS_SUCCESS;
}

void bc_lex_init(BcLex *l, BcLexNext next) {
	assert(l);
	l->next = next;
	bc_vec_init(&l->str, sizeof(char), NULL);
}

void bc_lex_free(BcLex *l) {
	assert(l);
	bc_vec_free(&l->str);
}

void bc_lex_file(BcLex *l, const char *file) {
	assert(l && file);
	l->line = 1;
	vm->file = file;
}

BcStatus bc_lex_next(BcLex *l) {

	BcStatus s;

	assert(l);

	l->last = l->t;
	l->line += l->last == BC_LEX_NLINE;

	if (l->last == BC_LEX_EOF) return bc_vm_error(BC_ERROR_PARSE_EOF, l->line);

	l->t = BC_LEX_EOF;

	if (l->i == l->len) return BC_STATUS_SUCCESS;

	// Loop until failure or we don't have whitespace. This
	// is so the parser doesn't get inundated with whitespace.
	do {
		s = l->next(l);
	} while (!s && l->t == BC_LEX_WHITESPACE);

	return s;
}

BcStatus bc_lex_text(BcLex *l, const char *text) {
	assert(l && text);
	l->buf = text;
	l->i = 0;
	l->len = strlen(text);
	l->t = l->last = BC_LEX_INVALID;
	return bc_lex_next(l);
}
