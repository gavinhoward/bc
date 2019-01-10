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

#define BC_PARSE_STREND ((uchar) UCHAR_MAX)

#define BC_PARSE_REL (1<<0)
#define BC_PARSE_PRINT (1<<1)
#define BC_PARSE_NOCALL (1<<2)
#define BC_PARSE_NOREAD (1<<3)
#define BC_PARSE_ARRAY (1<<4)

#define bc_parse_push(p, i) (bc_vec_pushByte(&(p)->func->code, (uchar) (i)))
#define bc_parse_number(p)(bc_parse_addId((p), BC_INST_NUM))
#define bc_parse_string(p)(bc_parse_addId((p), BC_INST_STR))

#define bc_parse_err(p, e) (bc_vm_error((e), (p)->l.line))
#define bc_parse_verr(p, e, ...) (bc_vm_error((e), (p)->l.line, __VA_ARGS__))

typedef struct BcParseNext {
	uchar len;
	uchar tokens[4];
} BcParseNext;

#define BC_PARSE_NEXT_TOKENS(...) .tokens = { __VA_ARGS__ }
#define BC_PARSE_NEXT(a, ...) \
	{ .len = (uchar) (a), BC_PARSE_NEXT_TOKENS(__VA_ARGS__) }

// ** Exclude start. **
struct BcParse;
// ** Exclude end. **

struct BcProgram;

// ** Exclude start. **
typedef BcStatus (*BcParseParse)(struct BcParse*);
typedef BcStatus (*BcParseExpr)(struct BcParse*, uint8_t);
// ** Exclude end. **

typedef struct BcParse {

	BcLex l;

#if BC_ENABLED
	BcVec flags;
	BcVec exits;
	BcVec conds;
	BcVec ops;
#endif // BC_ENABLED

	struct BcProgram *prog;
	BcFunc *func;
	size_t fidx;

	bool auto_part;

} BcParse;

// ** Exclude start. **
// ** Busybox exclude start. **

void bc_parse_init(BcParse *p, struct BcProgram *prog, size_t func);
void bc_parse_free(BcParse *p);
BcStatus bc_parse_reset(BcParse *p, BcStatus s);

void bc_parse_addId(BcParse *p, uchar inst);
void bc_parse_updateFunc(BcParse *p, size_t fidx);
void bc_parse_pushName(BcParse* p, char *name);
void bc_parse_pushIndex(BcParse* p, size_t idx);
BcStatus bc_parse_text(BcParse *p, const char *text);

// ** Busybox exclude end. **
// ** Exclude end. **

#endif // BC_PARSE_H
