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
 * Definitions for bc's lexer.
 *
 */

#ifndef BC_LEX_H
#define BC_LEX_H

#include <stdbool.h>
#include <stddef.h>

#include <status.h>
#include <vector.h>
#include <lang.h>

// BC_LEX_NEG is not used in lexing; it is only for parsing.
typedef enum BcLexType {

	BC_LEX_EOF,
	BC_LEX_INVALID,

	BC_LEX_OP_INC,
	BC_LEX_OP_DEC,

	BC_LEX_NEG,

	BC_LEX_OP_POWER,
	BC_LEX_OP_MULTIPLY,
	BC_LEX_OP_DIVIDE,
	BC_LEX_OP_MODULUS,
	BC_LEX_OP_PLUS,
	BC_LEX_OP_MINUS,

	BC_LEX_OP_REL_EQ,
	BC_LEX_OP_REL_LE,
	BC_LEX_OP_REL_GE,
	BC_LEX_OP_REL_NE,
	BC_LEX_OP_REL_LT,
	BC_LEX_OP_REL_GT,

	BC_LEX_OP_BOOL_NOT,
	BC_LEX_OP_BOOL_OR,
	BC_LEX_OP_BOOL_AND,

	BC_LEX_OP_ASSIGN_POWER,
	BC_LEX_OP_ASSIGN_MULTIPLY,
	BC_LEX_OP_ASSIGN_DIVIDE,
	BC_LEX_OP_ASSIGN_MODULUS,
	BC_LEX_OP_ASSIGN_PLUS,
	BC_LEX_OP_ASSIGN_MINUS,
	BC_LEX_OP_ASSIGN,

	BC_LEX_NLINE,

	BC_LEX_WHITESPACE,

	BC_LEX_LPAREN,
	BC_LEX_RPAREN,

	BC_LEX_LBRACKET,
	BC_LEX_COMMA,
	BC_LEX_RBRACKET,

	BC_LEX_LBRACE,
	BC_LEX_SCOLON,
	BC_LEX_RBRACE,

	BC_LEX_STRING,
	BC_LEX_NAME,
	BC_LEX_NUMBER,

	BC_LEX_KEY_AUTO,
	BC_LEX_KEY_BREAK,
	BC_LEX_KEY_CONTINUE,
	BC_LEX_KEY_DEFINE,
	BC_LEX_KEY_ELSE,
	BC_LEX_KEY_FOR,
	BC_LEX_KEY_HALT,
	BC_LEX_KEY_IBASE,
	BC_LEX_KEY_IF,
	BC_LEX_KEY_LAST,
	BC_LEX_KEY_LENGTH,
	BC_LEX_KEY_LIMITS,
	BC_LEX_KEY_OBASE,
	BC_LEX_KEY_PRINT,
	BC_LEX_KEY_QUIT,
	BC_LEX_KEY_READ,
	BC_LEX_KEY_RETURN,
	BC_LEX_KEY_SCALE,
	BC_LEX_KEY_SQRT,
	BC_LEX_KEY_WHILE,

#ifdef DC_CONFIG
	BC_LEX_OP_MODEXP,
	BC_LEX_OP_DIVMOD,

	BC_LEX_COLON,
	BC_LEX_EXECUTE,
	BC_LEX_PRINT_STACK,
	BC_LEX_CLEAR_STACK,
	BC_LEX_STACK_LEVEL,
	BC_LEX_DUPLICATE,
	BC_LEX_SWAP,
	BC_LEX_POP,

	BC_LEX_LOAD,
	BC_LEX_LOAD_POP,
	BC_LEX_STORE,
	BC_LEX_STORE_PUSH,
	BC_LEX_STORE_IBASE,
	BC_LEX_STORE_OBASE,
	BC_LEX_STORE_SCALE,
	BC_LEX_PRINT_POP,
	BC_LEX_PRINT_STREAM,
	BC_LEX_NQUIT,
	BC_LEX_SCALE_FACTOR,
#endif // DC_CONFIG

} BcLexType;

// ** Exclude start. **
struct BcLex;

typedef BcStatus (*BcLexNext)(struct BcLex*);
// ** Exclude end. **

typedef struct BcLex {

	const char *buffer;
	size_t idx;
	size_t line;
	const char *file;
	size_t len;
	bool newline;

	struct {
		BcLexType t;
		BcLexType last;
		BcVec v;
	} t;

	// ** Exclude start. **
	BcLexNext next;
	// ** Exclude end. **

} BcLex;

typedef struct BcLexKeyword {

	const char name[9];
	const char len;
	const bool posix;

} BcLexKeyword;

#define BC_LEX_KW_ENTRY(a, b, c) { .name = a, .len = (b), .posix = (c) }

extern const BcLexKeyword bc_lex_kws[20];

// ** Exclude start. **

extern const BcLexType dc_lex_tokens[];
extern const BcInst dc_parse_insts[];

// Common code.

BcStatus bc_lex_init(BcLex *l, BcLexNext next);
void bc_lex_free(BcLex *l);
void bc_lex_file(BcLex *l, const char *file);
BcStatus bc_lex_text(BcLex *l, const char *text);
BcStatus bc_lex_next(BcLex *l);

void bc_lex_lineComment(BcLex *l);
void bc_lex_whitespace(BcLex *l);
BcStatus bc_lex_number(BcLex *l, char start);

// bc lex code.

BcStatus bc_lex_token(BcLex *l);

// dc lex code.

BcStatus dc_lex_token(BcLex *l);

// ** Exclude end. **

#endif // BC_LEX_H
