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

// ** Exclude start. **
BcStatus bc_lex_identifier(BcLex *l);
BcStatus bc_lex_string(BcLex *l);
void bc_lex_assign(BcLex *l, BcLexType with, BcLexType without);
BcStatus bc_lex_comment(BcLex *l);
// ** Exclude end. **

BcStatus bc_lex_token(BcLex *l);

#define BC_PARSE_LEAF(p, rparen) \
	(((p) >= BC_INST_NUM && (p) <= BC_INST_SQRT) || (rparen) || \
	(p) == BC_INST_INC_POST || (p) == BC_INST_DEC_POST)

// We can calculate the conversion between tokens and exprs by subtracting the
// position of the first operator in the lex enum and adding the position of the
// first in the expr enum. Note: This only works for binary operators.
#define BC_PARSE_TOKEN_INST(t) ((char) ((t) - BC_LEX_NEG + BC_INST_NEG))

// ** Exclude start. **
BcStatus bc_parse_init(BcParse *p, struct BcProgram *prog, size_t func);
BcStatus bc_parse_expression(BcParse *p, uint8_t flags);
BcStatus bc_parse_operator(BcParse *p, BcVec *ops, BcLexType type,
                           size_t start, size_t *nexprs, bool next);
BcStatus bc_parse_rightParen(BcParse *p, BcVec *ops, size_t *nexs);
BcStatus bc_parse_params(BcParse *p, uint8_t flags);
BcStatus bc_parse_call(BcParse *p, char *name, uint8_t flags);
BcStatus bc_parse_name(BcParse *p, BcInst *type, uint8_t flags);
BcStatus bc_parse_read(BcParse *p);
BcStatus bc_parse_builtin(BcParse *p, BcLexType type,
                          uint8_t flags, BcInst *prev);
BcStatus bc_parse_scale(BcParse *p, BcInst *type, uint8_t flags);
BcStatus bc_parse_incdec(BcParse *p, BcInst *prev, bool *paren_expr,
                         size_t *nexprs, uint8_t flags);
BcStatus bc_parse_minus(BcParse *p, BcVec *ops, BcInst *prev,
                        size_t start, bool rparen, size_t *nexprs);
BcStatus bc_parse_string(BcParse *p, char inst);
BcStatus bc_parse_print(BcParse *p);
BcStatus bc_parse_return(BcParse *p);
BcStatus bc_parse_endBody(BcParse *p, bool brace);
BcStatus bc_parse_startBody(BcParse *p, uint8_t flags);
void bc_parse_noElse(BcParse *p);
BcStatus bc_parse_if(BcParse *p);
BcStatus bc_parse_else(BcParse *p);
BcStatus bc_parse_while(BcParse *p);
BcStatus bc_parse_for(BcParse *p);
BcStatus bc_parse_loopExit(BcParse *p, BcLexType type);
BcStatus bc_parse_func(BcParse *p);
BcStatus bc_parse_auto(BcParse *p);
BcStatus bc_parse_body(BcParse *p, bool brace);
BcStatus bc_parse_stmt(BcParse *p);
// ** Exclude end. **

BcStatus bc_parse_parse(BcParse *p);

BcStatus bc_parse_else(BcParse *p);
BcStatus bc_parse_stmt(BcParse *p);

BcStatus bc_parse_expr(BcParse *p, uint8_t flags, BcParseNext next);

#endif // BC_ENABLED

#endif // BC_BC_H
