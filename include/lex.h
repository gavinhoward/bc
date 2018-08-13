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
#include <stdlib.h>

#include <status.h>

// BC_LEX_OP_NEGATE is not used in lexing; it is only for parsing.
typedef enum BcLexToken {

  BC_LEX_OP_INC,
  BC_LEX_OP_DEC,

  BC_LEX_OP_NEG,

  BC_LEX_OP_POWER,
  BC_LEX_OP_MULTIPLY,
  BC_LEX_OP_DIVIDE,
  BC_LEX_OP_MODULUS,
  BC_LEX_OP_PLUS,
  BC_LEX_OP_MINUS,

  BC_LEX_OP_REL_EQUAL,
  BC_LEX_OP_REL_LESS_EQ,
  BC_LEX_OP_REL_GREATER_EQ,
  BC_LEX_OP_REL_NOT_EQ,
  BC_LEX_OP_REL_LESS,
  BC_LEX_OP_REL_GREATER,

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

  BC_LEX_NEWLINE,

  BC_LEX_WHITESPACE,

  BC_LEX_LEFT_PAREN,
  BC_LEX_RIGHT_PAREN,

  BC_LEX_LEFT_BRACKET,
  BC_LEX_COMMA,
  BC_LEX_RIGHT_BRACKET,

  BC_LEX_LEFT_BRACE,
  BC_LEX_SEMICOLON,
  BC_LEX_RIGHT_BRACE,

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

  BC_LEX_EOF,
  BC_LEX_INVALID,

} BcLexToken;

typedef struct BcLex {

  const char *buffer;
  size_t idx;
  size_t line;
  const char *file;
  size_t len;
  bool newline;

  struct {
    BcLexToken type;
    char *string;
  } token;

} BcLex;

typedef struct BcLexKeyword {

  const char name[9];
  const char len;
  const bool posix;

} BcLexKeyword;

#define BC_LEX_KW_ENTRY(a, b, c) { .name = a, .len = (b), .posix = (c) }

// ** Exclude start. **
void bc_lex_init(BcLex *lex, const char *file);

BcStatus bc_lex_text(BcLex *lex, const char *text);

BcStatus bc_lex_next(BcLex *lex);
// ** Exclude end. **

extern const char *bc_lex_token_type_strs[];
extern const BcLexKeyword bc_lex_keywords[20];

#endif // BC_LEX_H
