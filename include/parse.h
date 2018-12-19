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
#include <lang.h>

#define BC_PARSE_STREND ((char) UCHAR_MAX)

#define BC_PARSE_REL (1<<0)
#define BC_PARSE_PRINT (1<<1)
#define BC_PARSE_NOCALL (1<<2)
#define BC_PARSE_NOREAD (1<<3)
#define BC_PARSE_ARRAY (1<<4)

#define BC_PARSE_TOP_FLAG_PTR(parse) ((uint16_t*) bc_vec_top(&(parse)->flags))
#define BC_PARSE_TOP_FLAG(parse) (*(BC_PARSE_TOP_FLAG_PTR(parse)))

#define BC_PARSE_FLAG_BRACE (1<<0)
#define BC_PARSE_BRACE(parse) \
	(BC_PARSE_TOP_FLAG(parse) & BC_PARSE_FLAG_BRACE)

#define BC_PARSE_FLAG_FUNC_INNER (1<<1)
#define BC_PARSE_FUNC_INNER(parse) \
	(BC_PARSE_TOP_FLAG(parse) & BC_PARSE_FLAG_FUNC_INNER)

#define BC_PARSE_FLAG_FUNC (1<<2)
#define BC_PARSE_FUNC(parse) \
	(BC_PARSE_TOP_FLAG(parse) & BC_PARSE_FLAG_FUNC)

#define BC_PARSE_FLAG_BODY (1<<3)
#define BC_PARSE_BODY(parse) \
	(BC_PARSE_TOP_FLAG(parse) & BC_PARSE_FLAG_BODY)

#define BC_PARSE_FLAG_LOOP (1<<4)
#define BC_PARSE_LOOP(parse) \
	(BC_PARSE_TOP_FLAG(parse) & BC_PARSE_FLAG_LOOP)

#define BC_PARSE_FLAG_LOOP_INNER (1<<5)
#define BC_PARSE_LOOP_INNER(parse) \
	(BC_PARSE_TOP_FLAG(parse) & BC_PARSE_FLAG_LOOP_INNER)

#define BC_PARSE_FLAG_IF (1<<6)
#define BC_PARSE_IF(parse) \
	(BC_PARSE_TOP_FLAG(parse) & BC_PARSE_FLAG_IF)

#define BC_PARSE_FLAG_ELSE (1<<7)
#define BC_PARSE_ELSE(parse) \
	(BC_PARSE_TOP_FLAG(parse) & BC_PARSE_FLAG_ELSE)

#define BC_PARSE_FLAG_IF_END (1<<8)
#define BC_PARSE_IF_END(parse) \
	(BC_PARSE_TOP_FLAG(parse) & BC_PARSE_FLAG_IF_END)

#define BC_PARSE_CAN_EXEC(parse) (BC_PARSE_TOP_FLAG(parse) == 0)

#define BC_PARSE_VALID_END_TOKEN(t) \
	((t) == BC_LEX_SCOLON || (t) == BC_LEX_NLINE || (t) == BC_LEX_RBRACE)

#define bc_parse_push(p, i) (bc_vec_pushByte(&(p)->func->code, (char) (i)))

#define bc_parse_number(p) \
	(bc_parse_addId((p), &(p)->prog->const_map, &(p)->prog->consts, BC_INST_NUM))

#define bc_parse_string(p) \
	(bc_parse_addId((p), &(p)->prog->str_map, &(p)->prog->strs, BC_INST_STR))

typedef struct BcOp {
	char prec;
	bool left;
} BcOp;

typedef struct BcParseNext {
	uint32_t len;
	BcLexType tokens[4];
} BcParseNext;

#define BC_PARSE_NEXT_TOKENS(...) .tokens = { __VA_ARGS__ }
#define BC_PARSE_NEXT(a, ...) { .len = (a), BC_PARSE_NEXT_TOKENS(__VA_ARGS__) }

// ** Exclude start. **
struct BcParse;
// ** Exclude end. **

struct BcProgram;

// ** Exclude start. **
typedef void (*BcParseInit)(struct BcParse*, struct BcProgram*, size_t);
typedef BcStatus (*BcParseParse)(struct BcParse*);
typedef BcStatus (*BcParseExpr)(struct BcParse*, uint8_t);
// ** Exclude end. **

typedef struct BcParse {

	// ** Exclude start. **
	BcParseParse parse;
	// ** Exclude end. **

	BcLex l;

	BcVec flags;

	BcVec exits;
	BcVec conds;

	BcVec ops;

	struct BcProgram *prog;
	BcFunc *func;
	size_t fidx;

	bool auto_part;

} BcParse;

// ** Exclude start. **
// ** Busybox exclude start. **

void bc_parse_create(BcParse *p, struct BcProgram *prog, size_t func,
                     BcParseParse parse, BcLexNext next);
void bc_parse_free(BcParse *p);
BcStatus bc_parse_reset(BcParse *p, BcStatus s);

void bc_parse_addId(BcParse *p, BcVec *map, BcVec *vec, char inst);
void bc_parse_updateFunc(BcParse *p, size_t fidx);
size_t bc_parse_addFunc(BcParse *p, char *name);
void bc_parse_pushName(BcParse* p, char *name);
void bc_parse_pushIndex(BcParse* p, size_t idx);
BcStatus bc_parse_text(BcParse *p, const char *text);

// ** Busybox exclude end. **
// ** Exclude end. **

#endif // BC_PARSE_H
