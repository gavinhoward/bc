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
 * The lexer for bc.
 *
 */

#include <assert.h>
#include <ctype.h>

#include <lex.h>
#include <vm.h>

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
			s = bc_lex_string(l, '"');
			break;
		}

		case '#':
		{
			if ((s = bc_vm_posix_error(BC_STATUS_POSIX_SCRIPT_COMMENT,
			                           l->file, l->line, NULL)))
			{
				return s;
			}

			bc_lex_lineComment(l);

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

BcStatus bc_lex_next(BcLex *l) {

	BcStatus s;

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
		s = bc_lex_token(l);
	} while (!s && l->t.t == BC_LEX_WHITESPACE);

	return s;
}
