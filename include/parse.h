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
 * Definitions for bc's parser.
 *
 */

#ifndef BC_PARSE_H
#define BC_PARSE_H

#include <stdbool.h>
#include <stdint.h>

#include <status.h>
#include <vector.h>
#include <lex.h>

#define BC_PARSE_POSIX_REL (1<<0)
#define BC_PARSE_PRINT (1<<1)
#define BC_PARSE_NOCALL (1<<2)
#define BC_PARSE_NOREAD (1<<3)
#define BC_PARSE_ARRAY (1<<4)

#define BC_PARSE_TOP_FLAG_PTR(parse) ((uint8_t*) bc_vec_top(&(parse)->flags))
#define BC_PARSE_TOP_FLAG(parse) (*(BC_PARSE_TOP_FLAG_PTR(parse)))

#define BC_PARSE_FLAG_FUNC_INNER (1<<0)
#define BC_PARSE_FUNC_INNER(parse) \
	(BC_PARSE_TOP_FLAG(parse) & BC_PARSE_FLAG_FUNC_INNER)

#define BC_PARSE_FLAG_FUNC (1<<1)
#define BC_PARSE_FUNC(parse) \
	(BC_PARSE_TOP_FLAG(parse) & BC_PARSE_FLAG_FUNC)

#define BC_PARSE_FLAG_BODY (1<<2)
#define BC_PARSE_BODY(parse) \
	(BC_PARSE_TOP_FLAG(parse) & BC_PARSE_FLAG_BODY)

#define BC_PARSE_FLAG_LOOP (1<<3)
#define BC_PARSE_LOOP(parse) \
	(BC_PARSE_TOP_FLAG(parse) & BC_PARSE_FLAG_LOOP)

#define BC_PARSE_FLAG_LOOP_INNER (1<<4)
#define BC_PARSE_LOOP_INNER(parse) \
	(BC_PARSE_TOP_FLAG(parse) & BC_PARSE_FLAG_LOOP_INNER)

#define BC_PARSE_FLAG_IF (1<<5)
#define BC_PARSE_IF(parse) \
	(BC_PARSE_TOP_FLAG(parse) & BC_PARSE_FLAG_IF)

#define BC_PARSE_FLAG_ELSE (1<<6)
#define BC_PARSE_ELSE(parse) \
	(BC_PARSE_TOP_FLAG(parse) & BC_PARSE_FLAG_ELSE)

#define BC_PARSE_FLAG_IF_END (1<<7)
#define BC_PARSE_IF_END(parse) \
	(BC_PARSE_TOP_FLAG(parse) & BC_PARSE_FLAG_IF_END)

#define BC_PARSE_CAN_EXEC(parse) \
	(!(BC_PARSE_TOP_FLAG(parse) & (BC_PARSE_FLAG_FUNC_INNER |  \
	                               BC_PARSE_FLAG_FUNC |        \
	                               BC_PARSE_FLAG_BODY |        \
	                               BC_PARSE_FLAG_LOOP |        \
	                               BC_PARSE_FLAG_LOOP_INNER |  \
	                               BC_PARSE_FLAG_IF |          \
	                               BC_PARSE_FLAG_ELSE |        \
	                               BC_PARSE_FLAG_IF_END)))

#define BC_PARSE_LEAF(p, rparen) \
	(((p) >= BC_INST_NUM && (p) <= BC_INST_SQRT) || (rparen) || \
	(p) == BC_INST_INC_POST || (p) == BC_INST_DEC_POST)

// We can calculate the conversion between tokens and exprs
// by subtracting the position of the first operator in the
// lex enum and adding the position of the first in the expr
// enum. Note: This only works for binary operators.
#define BC_PARSE_TOKEN_TO_INST(type) ((type) - BC_LEX_OP_NEG + BC_INST_NEG)

typedef struct BcOp {

	uint8_t prec;
	bool left;

} BcOp;

typedef struct BcParseNext {

	uint32_t len;
	BcLexToken tokens[3];

} BcParseNext;

#define BC_PARSE_NEXT_TOKENS(...) .tokens = { __VA_ARGS__ }
#define BC_PARSE_NEXT(a, ...) { .len = (a), BC_PARSE_NEXT_TOKENS(__VA_ARGS__) }

struct BcParse;
struct BcProgram;

typedef BcStatus (*BcParseInit)(struct BcParse*, struct BcProgram*);
typedef BcStatus (*BcParseParse)(struct BcParse*);
typedef BcStatus (*BcParseExpr)(struct BcParse*, BcVec*, uint8_t, BcParseNext);

typedef struct BcParse {

	// ** Exclude start. **
	BcParseParse parse;
	// ** Exclude end. **

	BcLex lex;

	BcVec flags;

	BcVec exits;
	BcVec conds;

	BcVec ops;

	struct BcProgram *prog;
	size_t func;

	size_t nbraces;
	bool auto_part;

} BcParse;

// ** Exclude start. **

// Common code.

BcStatus bc_parse_create(BcParse *p, struct BcProgram *prog, BcParseParse parse, BcLexNext next);
void bc_parse_free(BcParse *p);
BcStatus bc_parse_reset(BcParse *p, BcStatus s);
BcStatus bc_parse_pushName(BcVec *code, char *name);
BcStatus bc_parse_pushIndex(BcVec *code, size_t idx);

// bc parse code.

BcStatus bc_parse_init(BcParse *p, struct BcProgram *prog);
BcStatus bc_parse_expr(BcParse *p, BcVec *code, uint8_t flags, BcParseNext next);

// dc parse code.

BcStatus dc_parse_init(BcParse *p, struct BcProgram *prog);
BcStatus dc_parse_expr(BcParse *p, BcVec *code, uint8_t flags, BcParseNext next);

// ** Exclude end. **

#define BC_PARSE_STREND ((char) '#')

extern const bool bc_parse_token_exprs[];
extern const BcParseNext bc_parse_next_expr;
extern const BcParseNext bc_parse_next_param;
extern const BcParseNext bc_parse_next_print;
extern const BcParseNext bc_parse_next_cond;
extern const BcParseNext bc_parse_next_elem;
extern const BcParseNext bc_parse_next_for;
extern const BcParseNext bc_parse_next_read;
extern const BcOp bc_parse_ops[];

#endif // BC_PARSE_H
