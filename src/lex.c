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
 * The lexer.
 *
 */

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>

#include <status.h>
#include <lex.h>
#include <vm.h>

BcStatus bc_lex_string(BcLex *l) {

	BcStatus s;
	const char *buf;
	size_t len, nls = 0, i = l->idx;
	char c;

	l->t.t = BC_LEX_STRING;

	for (; (c = l->buffer[i]) != '"' && c != '\0'; ++i) nls += (c == '\n');

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

BcStatus bc_lex_comment(BcLex *l) {

	size_t i, nls = 0;
	const char *buf = l->buffer;
	bool end = false;

	l->t.t = BC_LEX_WHITESPACE;

	for (i = ++l->idx; !end; i += !end) {

		char c;

		for (; (c = buf[i]) != '*' && c != '\0'; ++i) nls += (c == '\n');

		if (c == '\0' || buf[i + 1] == '\0') {
			l->idx = i;
			return BC_STATUS_LEX_NO_COMMENT_END;
		}

		end = buf[i + 1] == '/';
	}

	l->idx = i + 2;
	l->line += nls;

	return BC_STATUS_SUCCESS;
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
	if ((s = bc_vec_expand(&l->t.v, len + 1)))
		return s;
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

BcStatus bc_lex_token(BcLex *l) {

	BcStatus s = BC_STATUS_SUCCESS;
	char c, c2;

	// This is the workhorse of the lexer.
	switch ((c = l->buffer[l->idx++])) {

		case '\0':
		case '\n':
		{
			l->newline = true;
			l->t.t = !c ? BC_LEX_EOF : BC_LEX_NLINE;
			break;
		}

		case '\t':
		case '\v':
		case '\f':
		case '\r':
		case ' ':
		{
			l->t.t = BC_LEX_WHITESPACE;
			for (; (c = l->buffer[l->idx]) != '\n' && isspace(c); ++l->idx);
			break;
		}

		case '!':
		{
			c2 = l->buffer[l->idx];

			if (c2 == '=') {
				++l->idx;
				l->t.t = BC_LEX_OP_REL_NE;
			}
			else {

				if ((s = bc_vm_posix_error(BC_STATUS_POSIX_BOOL_OPS,
				                           l->file, l->line, "!")))
				{
					return s;
				}

				l->t.t = BC_LEX_OP_BOOL_NOT;
			}

			break;
		}

		case '"':
		{
			s = bc_lex_string(l);
			break;
		}

		case '#':
		{
			if ((s = bc_vm_posix_error(BC_STATUS_POSIX_SCRIPT_COMMENT,
			                           l->file, l->line, NULL)))
			{
				return s;
			}

			l->t.t = BC_LEX_WHITESPACE;
			while (l->idx < l->len && l->buffer[l->idx++] != '\n');
			--l->idx;

			break;
		}

		case '%':
		{
			if ((c2 = l->buffer[l->idx]) == '=') {
				++l->idx;
				l->t.t = BC_LEX_OP_ASSIGN_MODULUS;
			}
			else l->t.t = BC_LEX_OP_MODULUS;
			break;
		}

		case '&':
		{
			if ((c2 = l->buffer[l->idx]) == '&') {

				if ((s = bc_vm_posix_error(BC_STATUS_POSIX_BOOL_OPS,
				                           l->file, l->line, "&&")))
				{
					return s;
				}

				++l->idx;
				l->t.t = BC_LEX_OP_BOOL_AND;
			}
			else {
				l->t.t = BC_LEX_INVALID;
				s = BC_STATUS_LEX_BAD_CHARACTER;
			}

			break;
		}

		case '(':
		case ')':
		{
			l->t.t = (BcLexToken) (c - '(' + BC_LEX_LPAREN);
			break;
		}

		case '*':
		{
			if ((c2 = l->buffer[l->idx]) == '=') {
				++l->idx;
				l->t.t = BC_LEX_OP_ASSIGN_MULTIPLY;
			}
			else l->t.t = BC_LEX_OP_MULTIPLY;
			break;
		}

		case '+':
		{
			if ((c2 = l->buffer[l->idx]) == '=') {
				++l->idx;
				l->t.t = BC_LEX_OP_ASSIGN_PLUS;
			}
			else if (c2 == '+') {
				++l->idx;
				l->t.t = BC_LEX_OP_INC;
			}
			else l->t.t = BC_LEX_OP_PLUS;
			break;
		}

		case ',':
		{
			l->t.t = BC_LEX_COMMA;
			break;
		}

		case '-':
		{
			if ((c2 = l->buffer[l->idx]) == '=') {
				++l->idx;
				l->t.t = BC_LEX_OP_ASSIGN_MINUS;
			}
			else if (c2 == '-') {
				++l->idx;
				l->t.t = BC_LEX_OP_DEC;
			}
			else l->t.t = BC_LEX_OP_MINUS;
			break;
		}

		case '.':
		{
			c2 = l->buffer[l->idx];
			if (isdigit(c2)) s = bc_lex_number(l, c);
			else {
				s = bc_vm_posix_error(BC_STATUS_POSIX_DOT_LAST,
				                      l->file, l->line, NULL);
				l->t.t = BC_LEX_KEY_LAST;
			}
			break;
		}

		case '/':
		{
			if ((c2 = l->buffer[l->idx]) == '=') {
				++l->idx;
				l->t.t = BC_LEX_OP_ASSIGN_DIVIDE;
			}
			else if (c2 == '*') s = bc_lex_comment(l);
			else l->t.t = BC_LEX_OP_DIVIDE;
			break;
		}

		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
		{
			s = bc_lex_number(l, c);
			break;
		}

		case ';':
		{
			l->t.t = BC_LEX_SCOLON;
			break;
		}

		case '<':
		{
			if ((c2 = l->buffer[l->idx]) == '=') {
				++l->idx;
				l->t.t = BC_LEX_OP_REL_LE;
			}
			else l->t.t = BC_LEX_OP_REL_LT;
			break;
		}

		case '=':
		{
			if ((c2 = l->buffer[l->idx]) == '=') {
				++l->idx;
				l->t.t = BC_LEX_OP_REL_EQ;
			}
			else l->t.t = BC_LEX_OP_ASSIGN;
			break;
		}

		case '>':
		{
			if ((c2 = l->buffer[l->idx]) == '=') {
				++l->idx;
				l->t.t = BC_LEX_OP_REL_GE;
			}
			else l->t.t = BC_LEX_OP_REL_GT;
			break;
		}

		case '[':
		case ']':
		{
			l->t.t = (BcLexToken) (c - '[' + BC_LEX_LBRACKET);
			break;
		}

		case '\\':
		{
			if (l->buffer[l->idx] == '\n') {
				l->t.t = BC_LEX_WHITESPACE;
				++l->idx;
			}
			else s = BC_STATUS_LEX_BAD_CHARACTER;
			break;
		}

		case '^':
		{
			if (l->buffer[l->idx] == '=') {
				l->t.t = BC_LEX_OP_ASSIGN_POWER;
				++l->idx;
			}
			else l->t.t = BC_LEX_OP_POWER;
			break;
		}

		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
		case 'g':
		case 'h':
		case 'i':
		case 'j':
		case 'k':
		case 'l':
		case 'm':
		case 'n':
		case 'o':
		case 'p':
		case 'q':
		case 'r':
		case 's':
		case 't':
		case 'u':
		case 'v':
		case 'w':
		case 'x':
		case 'y':
		case 'z':
		{
			s = bc_lex_name(l);
			break;
		}

		case '{':
		case '}':
		{
			l->t.t = (BcLexToken) (c - '{' + BC_LEX_LBRACE);
			break;
		}

		case '|':
		{
			if ((c2 = l->buffer[l->idx]) == '|') {

				if ((s = bc_vm_posix_error(BC_STATUS_POSIX_BOOL_OPS,
				                           l->file, l->line, "||")))
				{
					return s;
				}

				++l->idx;
				l->t.t = BC_LEX_OP_BOOL_OR;
			}
			else {
				l->t.t = BC_LEX_INVALID;
				s = BC_STATUS_LEX_BAD_CHARACTER;
			}

			break;
		}

		default:
		{
			l->t.t = BC_LEX_INVALID;
			s = BC_STATUS_LEX_BAD_CHARACTER;
			break;
		}
	}

	return s;
}

BcStatus bc_lex_init(BcLex *l) {
	assert(l);
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

BcStatus bc_lex_next(BcLex *l) {

	BcStatus status;

	assert(l);

	if (l->t.t == BC_LEX_EOF) return BC_STATUS_LEX_EOF;

	if (l->idx == l->len) {
		l->newline = true;
		l->t.t = BC_LEX_EOF;
		return BC_STATUS_SUCCESS;
	}

	if (l->newline) {
		++l->line;
		l->newline = false;
	}

	// Loop until failure or we don't have whitespace. This
	// is so the parser doesn't get inundated with whitespace.
	do {
		status = bc_lex_token(l);
	} while (!status && l->t.t == BC_LEX_WHITESPACE);

	return status;
}

BcStatus bc_lex_text(BcLex *l, const char *text) {
	assert(l && text);
	l->buffer = text;
	l->idx = 0;
	l->len = strlen(text);
	l->t.t = BC_LEX_INVALID;
	return bc_lex_next(l);
}
