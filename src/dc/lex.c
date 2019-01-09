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

#if DC_ENABLED

#include <ctype.h>

#include <status.h>
#include <lex.h>
#include <dc.h>
#include <vm.h>

BcStatus dc_lex_register(BcLex *l) {

	BcStatus s = BC_STATUS_SUCCESS;

	if (DC_X && isspace(l->buf[l->i - 1])) {
		bc_lex_whitespace(l);
		++l->i;
		s = bc_lex_name(l);
	}
	else {
		bc_vec_npop(&l->str, l->str.len);
		bc_vec_pushByte(&l->str, l->buf[l->i - 1]);
		bc_vec_pushByte(&l->str, '\0');
		l->t = BC_LEX_NAME;
	}

	return s;
}

BcStatus dc_lex_string(BcLex *l) {

	size_t depth = 1, nls = 0, i = l->i;
	char c;

	l->t = BC_LEX_STR;
	bc_vec_npop(&l->str, l->str.len);

	for (; (c = l->buf[i]) && depth; ++i) {

		if (i == l->i || l->buf[i - 1] != '\\') {
			depth += (c == '[');
			depth -= (c == ']');
		}

		nls += (c == '\n');

		if (depth) bc_vec_push(&l->str, &c);
	}

	if (c == '\0' && depth) {
		l->i = i;
		return bc_lex_err(l, BC_ERROR_PARSE_STRING);
	}

	bc_vec_pushByte(&l->str, '\0');
	if (i - l->i > BC_MAX_STRING)
		return bc_lex_verr(l, BC_ERROR_EXEC_STRING_LEN, BC_MAX_STRING);

	l->i = i;
	l->line += nls;

	return BC_STATUS_SUCCESS;
}

BcStatus dc_lex_token(BcLex *l) {

	BcStatus s = BC_STATUS_SUCCESS;
	char c = l->buf[l->i++], c2;
	size_t i;

	for (i = 0; i < dc_lex_regs_len; ++i) {
		if (l->last == dc_lex_regs[i]) return dc_lex_register(l);
	}

	if (c >= '$' && c <= '~' &&
	    (l->t = dc_lex_tokens[(c - '$')]) != BC_LEX_INVALID)
	{
		return s;
	}

	// This is the workhorse of the lexer.
	switch (c) {

		case '\0':
		{
			l->t = BC_LEX_EOF;
			break;
		}

		case '\n':
		{
			l->t = BC_LEX_NLINE;
			break;
		}

		case '\t':
		case '\v':
		case '\f':
		case '\r':
		case ' ':
		{
			bc_lex_whitespace(l);
			break;
		}

		case '!':
		{
			c2 = l->buf[l->i];

			if (c2 == '=') l->t = BC_LEX_OP_REL_NE;
			else if (c2 == '<') l->t = BC_LEX_OP_REL_LE;
			else if (c2 == '>') l->t = BC_LEX_OP_REL_GE;
			else return bc_lex_invalidChar(l, c);

			++l->i;
			break;
		}

		case '#':
		{
			bc_lex_lineComment(l);
			break;
		}

		case '.':
		{
			c2 = l->buf[l->i];
			if (BC_LEX_NUM_CHAR(c2, 'F', true)) s = bc_lex_number(l, c);
			else s = bc_lex_invalidChar(l, c);
			break;
		}

		case '/':
		{
			if (l->buf[l->i] == '*') s = bc_lex_comment(l);
			else l->t = BC_LEX_OP_DIVIDE;

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
			s = bc_lex_invalidChar(l, c);
			break;
		}
	}

	return s;
}
#endif // DC_ENABLED
