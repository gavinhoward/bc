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
 * The lexer for dc.
 *
 */

#include <ctype.h>

#include <status.h>
#include <lex.h>
#include <dc.h>
#include <vm.h>

BcStatus dc_lex_string(BcLex *l) {

	BcStatus s;
	size_t depth = 1, nls = 0, idx, i = l->idx;
	char c;

	l->t.t = BC_LEX_STRING;

	bc_vec_npop(&l->t.v, l->t.v.len);

	for (idx = i; (c = l->buffer[i]) && depth; ++i) {

		depth += (c == '[');
		depth -= (c == ']');
		nls += (c == '\n');

		if (c == '#') {

			bc_lex_lineComment(l);

			// We increment by 1 to skip the newline.
			if (l->buffer[l->idx]) {
				i = l->idx + 1;
				++nls;
			}
		}
		else {
			if (depth && (s = bc_vec_push(&l->t.v, &c))) return s;
			if (c == '\\') {
				depth -= (l->buffer[i + 1] == '[');
				depth += (l->buffer[i + 1] == ']');
			}
		}
	}

	if (c == '\0') {
		l->idx = i;
		return BC_STATUS_LEX_NO_STRING_END;
	}

	if ((s = bc_vec_pushByte(&l->t.v, '\0'))) return s;
	if (i - idx > BC_MAX_STRING) return BC_STATUS_EXEC_STRING_LEN;

	l->idx = i + 1;
	l->line += nls;

	return BC_STATUS_SUCCESS;
}

BcStatus dc_lex_token(BcLex *l) {

	BcStatus s = BC_STATUS_SUCCESS;
	char c = l->buffer[l->idx++], c2;

	if (c >= '%' && (l->t.t = dc_lex_tokens[(c - '%')]) != BC_LEX_INVALID)
		return s;

	// This is the workhorse of the lexer.
	switch (c) {

		case '\0':
		{
			l->t.t = BC_LEX_EOF;
			break;
		}

		case '\n':
		case '\t':
		case '\v':
		case '\f':
		case '\r':
		case ' ':
		{
			l->newline = c == '\n';
			bc_lex_whitespace(l);
			break;
		}

		case '!':
		{
			c2 = l->buffer[l->idx];

			if (c2 == '=') l->t.t = BC_LEX_OP_REL_NE;
			else if (c2 == '<') l->t.t = BC_LEX_OP_REL_GE;
			else if (c2 == '>') l->t.t = BC_LEX_OP_REL_LE;
			else return BC_STATUS_LEX_BAD_CHAR;

			++l->idx;
			break;
		}

		case '#':
		{
			bc_lex_lineComment(l);
			break;
		}

		case '.':
		{
			if (isdigit(l->buffer[l->idx])) s = bc_lex_number(l, c);
			else s = BC_STATUS_LEX_BAD_CHAR;
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

		case '[':
		{
			s = dc_lex_string(l);
			break;
		}

		default:
		{
			l->t.t = BC_LEX_INVALID;
			s = BC_STATUS_LEX_BAD_CHAR;
			break;
		}
	}

	return s;
}
