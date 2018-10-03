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
 * Definitions for bc.
 *
 */

#ifndef BC_BC_H
#define BC_BC_H

#include <stdbool.h>

#include <status.h>
#include <lex.h>
#include <parse.h>

#ifdef BC_ENABLED

// ** Exclude start. **
BcStatus bc_main(int argc, char *argv[]);

extern const char bc_help[];
// ** Exclude end. **

typedef struct BcLexKeyword {

	const char name[9];
	const char len;
	const bool posix;

} BcLexKeyword;

#define BC_LEX_KW_ENTRY(a, b, c) { .name = a, .len = (b), .posix = (c) }

extern const BcLexKeyword bc_lex_kws[20];

BcStatus bc_lex_token(BcLex *l);

#define BC_PARSE_LEAF(p, rparen) \
	(((p) >= BC_INST_NUM && (p) <= BC_INST_SQRT) || (rparen) || \
	(p) == BC_INST_INC_POST || (p) == BC_INST_DEC_POST)

// We can calculate the conversion between tokens and exprs by subtracting the
// position of the first operator in the lex enum and adding the position of the
// first in the expr enum. Note: This only works for binary operators.
#define BC_PARSE_TOKEN_TO_INST(t) ((char) ((t) - BC_LEX_NEG + BC_INST_NEG))

// ** Exclude start. **
BcStatus bc_parse_init(BcParse *p, struct BcProgram *prog);
// ** Exclude end. **

BcStatus bc_parse_expr(BcParse *p, BcVec *code, uint8_t flags, BcParseNext next);

#endif // BC_ENABLED

#endif // BC_BC_H
