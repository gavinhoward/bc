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

#if BC_ENABLED

#include <limits.h>
#include <stdbool.h>

#include <status.h>
#include <lex.h>
#include <parse.h>

int bc_main(int argc, char **argv);

extern const char bc_help[];
extern const char bc_lib[];
extern const char* bc_lib_name;
extern const char bc_lib2[];
extern const char* bc_lib2_name;

typedef struct BcLexKeyword {
	uchar data;
	const char name[9];
} BcLexKeyword;

#define BC_LEX_CHAR_MSB(bit) ((bit) << (CHAR_BIT - 1))

#define BC_LEX_KW_POSIX(kw) ((kw)->data & (BC_LEX_CHAR_MSB(1)))
#define BC_LEX_KW_LEN(kw) ((size_t) ((kw)->data & ~(BC_LEX_CHAR_MSB(1))))

#define BC_LEX_KW_ENTRY(a, b, c) \
	{ .data = ((b) & ~(BC_LEX_CHAR_MSB(1))) | BC_LEX_CHAR_MSB(c),.name = a }

#define bc_lex_posixErr(l, e) (bc_vm_posixError((e), (l)->line))
#define bc_lex_vposixErr(l, e, ...) \
	(bc_vm_posixError((e), (l)->line, __VA_ARGS__))

extern const BcLexKeyword bc_lex_kws[];
extern const size_t bc_lex_kws_len;

BcStatus bc_lex_token(BcLex *l);

#define BC_PARSE_TOP_FLAG_PTR(p) ((uint16_t*) bc_vec_top(&(p)->flags))
#define BC_PARSE_TOP_FLAG(p) (*(BC_PARSE_TOP_FLAG_PTR(p)))

#define BC_PARSE_FLAG_BRACE (1<<0)
#define BC_PARSE_BRACE(p) (BC_PARSE_TOP_FLAG(p) & BC_PARSE_FLAG_BRACE)

#define BC_PARSE_FLAG_FUNC_INNER (1<<1)
#define BC_PARSE_FUNC_INNER(p) (BC_PARSE_TOP_FLAG(p) & BC_PARSE_FLAG_FUNC_INNER)

#define BC_PARSE_FLAG_FUNC (1<<2)
#define BC_PARSE_FUNC(p) (BC_PARSE_TOP_FLAG(p) & BC_PARSE_FLAG_FUNC)

#define BC_PARSE_FLAG_BODY (1<<3)
#define BC_PARSE_BODY(p) (BC_PARSE_TOP_FLAG(p) & BC_PARSE_FLAG_BODY)

#define BC_PARSE_FLAG_LOOP (1<<4)
#define BC_PARSE_LOOP(p) (BC_PARSE_TOP_FLAG(p) & BC_PARSE_FLAG_LOOP)

#define BC_PARSE_FLAG_LOOP_INNER (1<<5)
#define BC_PARSE_LOOP_INNER(p) (BC_PARSE_TOP_FLAG(p) & BC_PARSE_FLAG_LOOP_INNER)

#define BC_PARSE_FLAG_IF (1<<6)
#define BC_PARSE_IF(p) (BC_PARSE_TOP_FLAG(p) & BC_PARSE_FLAG_IF)

#define BC_PARSE_FLAG_ELSE (1<<7)
#define BC_PARSE_ELSE(p) (BC_PARSE_TOP_FLAG(p) & BC_PARSE_FLAG_ELSE)

#define BC_PARSE_FLAG_IF_END (1<<8)
#define BC_PARSE_IF_END(p) (BC_PARSE_TOP_FLAG(p) & BC_PARSE_FLAG_IF_END)

#define BC_PARSE_NO_EXEC(p) ((p)->flags.len != 1 || BC_PARSE_TOP_FLAG(p) != 0)

#define BC_PARSE_DELIMITER(t) \
	((t) == BC_LEX_SCOLON || (t) == BC_LEX_NLINE || (t) == BC_LEX_EOF)

#define BC_PARSE_BLOCK_STMT(f) \
	((f) & (BC_PARSE_FLAG_ELSE | BC_PARSE_FLAG_LOOP_INNER))

#define BC_PARSE_OP(p, l) (((p) & ~(BC_LEX_CHAR_MSB(1))) | (BC_LEX_CHAR_MSB(l)))

#define BC_PARSE_OP_DATA(t) bc_parse_ops[((t) - BC_LEX_OP_INC)]
#define BC_PARSE_OP_LEFT(op) (BC_PARSE_OP_DATA(op) & BC_LEX_CHAR_MSB(1))
#define BC_PARSE_OP_PREC(op) (BC_PARSE_OP_DATA(op) & ~(BC_LEX_CHAR_MSB(1)))

#define BC_PARSE_EXPR_ENTRY(e1, e2, e3, e4, e5, e6, e7, e8)  \
	(((e1) << 7) | ((e2) << 6) | ((e3) << 5) | ((e4) << 4) | \
	 ((e5) << 3) | ((e6) << 2) | ((e7) << 1) | ((e8) << 0))

#define BC_PARSE_EXPR(i) \
	(bc_parse_exprs[(((i) & (uchar) ~(0x07)) >> 3)] & (1 << (7 - ((i) & 0x07))))

#define BC_PARSE_TOP_OP(p) (*((BcLexType*) bc_vec_top(&(p)->ops)))
#define BC_PARSE_LEAF(prev, bin_last, rparen) \
	(!(bin_last) && ((rparen) || bc_parse_inst_isLeaf(prev)))
#define BC_PARSE_INST_VAR(t) \
	((t) >= BC_INST_VAR && (t) <= BC_INST_SCALE && (t) != BC_INST_ARRAY)

#define BC_PARSE_PREV_PREFIX(p) \
	((p) >= BC_INST_INC_PRE && (p) <= BC_INST_BOOL_NOT)
#define BC_PARSE_OP_PREFIX(t) ((t) == BC_LEX_OP_BOOL_NOT || (t) == BC_LEX_NEG)

// We can calculate the conversion between tokens and exprs by subtracting the
// position of the first operator in the lex enum and adding the position of
// the first in the expr enum. Note: This only works for binary operators.
#define BC_PARSE_TOKEN_INST(t) ((uchar) ((t) - BC_LEX_NEG + BC_INST_NEG))

#define bc_parse_posixErr(p, e) (bc_vm_posixError((e), (p)->l.line))

BcStatus bc_parse_expr(BcParse *p, uint8_t flags);

BcStatus bc_parse_parse(BcParse *p);
BcStatus bc_parse_expr_status(BcParse *p, uint8_t flags, BcParseNext next);

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
