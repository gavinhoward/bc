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

#include <limits.h>
#include <stdbool.h>

#include <status.h>
#include <lex.h>
#include <parse.h>

#if BC_ENABLED

// ** Exclude start. **
// ** Busybox exclude start. **
int bc_main(int argc, char **argv);

extern const char bc_help[];
// ** Busybox exclude end. **
// ** Exclude end. **

typedef struct BcLexKeyword {
	uchar data;
	const char name[9];
} BcLexKeyword;

#define BC_LEX_CHAR_MSB(bit) ((bit) << (CHAR_BIT - 1))

#define BC_LEX_KW_POSIX(kw) ((kw).data & (BC_LEX_CHAR_MSB(1)))
#define BC_LEX_KW_LEN(kw) ((kw).data & ~(BC_LEX_CHAR_MSB(1)))

#define BC_LEX_KW_ENTRY(a, b, c) \
	{ .data = (b) & ~(BC_LEX_CHAR_MSB(1)) | BC_LEX_CHAR_MSB(c),.name = a }

extern const BcLexKeyword bc_lex_kws[20];

BcStatus bc_lex_token(BcLex *l);

#define BC_PARSE_OP(p, l) \
	(((p) & ~(BC_LEX_CHAR_MSB(1))) | (BC_LEX_CHAR_MSB(l)))

#define BC_PARSE_OP_LEFT(op) ((op) & BC_LEX_CHAR_MSB(1))
#define BC_PARSE_OP_PREC(op) ((op) & ~BC_LEX_CHAR_MSB(1))

#define BC_PARSE_EXPR_ENTRY(e1, e2, e3, e4, e5, e6, e7, e8)  \
	(((e1) << 7) | ((e2) << 6) | ((e3) << 5) | ((e4) << 4) | \
	 ((e5) << 3) | ((e6) << 2) | ((e7) << 1) | ((e8) << 0))

#define BC_PARSE_EXPR(i) \
	(bc_parse_exprs[(((i) & ~(0x07)) >> 3)] & (1 << (7 - ((i) & 0x07))))

#define BC_PARSE_TOP_OP(p) (*((BcLexType*) bc_vec_top(&(p)->ops)))
#define BC_PARSE_LEAF(p, rparen) \
	(((p) >= BC_INST_NUM && (p) <= BC_INST_SQRT) || (rparen) || \
	(p) == BC_INST_INC_POST || (p) == BC_INST_DEC_POST)

// We can calculate the conversion between tokens and exprs by subtracting the
// position of the first operator in the lex enum and adding the position of
// the first in the expr enum. Note: This only works for binary operators.
#define BC_PARSE_TOKEN_INST(t) ((uchar) ((t) - BC_LEX_NEG + BC_INST_NEG))

// ** Exclude start. **
// ** Busybox exclude start. **
void bc_parse_init(BcParse *p, struct BcProgram *prog, size_t func);
BcStatus bc_parse_expr(BcParse *p, uint8_t flags);
// ** Busybox exclude end. **
// ** Exclude end. **

BcStatus bc_parse_parse(BcParse *p);

BcStatus bc_parse_expr_error(BcParse *p, uint8_t flags, BcParseNext next);
BcStatus bc_parse_expr_status(BcParse *p, uint8_t flags, BcParseNext next);
BcStatus bc_parse_else(BcParse *p);
BcStatus bc_parse_stmt(BcParse *p);

#if BC_ENABLE_SIGNALS
extern const char bc_sig_msg[];
#endif // BC_ENABLE_SIGNALS

extern const char* const bc_parse_const1;
extern const uint8_t bc_parse_exprs[];
extern const uchar bc_parse_ops[];
extern const BcParseNext bc_parse_next_expr;
extern const BcParseNext bc_parse_next_param;
extern const BcParseNext bc_parse_next_print;
extern const BcParseNext bc_parse_next_rel;
extern const BcParseNext bc_parse_next_elem;
extern const BcParseNext bc_parse_next_for;
extern const BcParseNext bc_parse_next_read;

#endif // BC_ENABLED

#endif // BC_BC_H
