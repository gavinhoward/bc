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

#define BC_LEX_GEN_ENUM(ENUM) ENUM,
#define BC_LEX_GEN_STR(STRING) #STRING,

// BC_LEX_OP_NEGATE is not used in lexing;
// it is only for parsing.
#define BC_LEX_TOKEN_FOREACH(TOKEN) \
  TOKEN(BC_LEX_OP_INC)  \
  TOKEN(BC_LEX_OP_DEC)  \
                        \
  TOKEN(BC_LEX_OP_NEG)  \
                           \
  TOKEN(BC_LEX_OP_POWER)  \
                          \
  TOKEN(BC_LEX_OP_MULTIPLY)  \
  TOKEN(BC_LEX_OP_DIVIDE)    \
  TOKEN(BC_LEX_OP_MODULUS)   \
                             \
  TOKEN(BC_LEX_OP_PLUS)   \
  TOKEN(BC_LEX_OP_MINUS)  \
                          \
  TOKEN(BC_LEX_OP_REL_EQUAL)       \
  TOKEN(BC_LEX_OP_REL_LESS_EQ)     \
  TOKEN(BC_LEX_OP_REL_GREATER_EQ)  \
  TOKEN(BC_LEX_OP_REL_NOT_EQ)      \
  TOKEN(BC_LEX_OP_REL_LESS)        \
  TOKEN(BC_LEX_OP_REL_GREATER)     \
                                   \
  TOKEN(BC_LEX_OP_BOOL_NOT)  \
                             \
  TOKEN(BC_LEX_OP_BOOL_OR)   \
  TOKEN(BC_LEX_OP_BOOL_AND)  \
                             \
  TOKEN(BC_LEX_OP_ASSIGN_POWER)     \
  TOKEN(BC_LEX_OP_ASSIGN_MULTIPLY)  \
  TOKEN(BC_LEX_OP_ASSIGN_DIVIDE)    \
  TOKEN(BC_LEX_OP_ASSIGN_MODULUS)   \
  TOKEN(BC_LEX_OP_ASSIGN_PLUS)      \
  TOKEN(BC_LEX_OP_ASSIGN_MINUS)     \
  TOKEN(BC_LEX_OP_ASSIGN)           \
                                    \
  TOKEN(BC_LEX_NEWLINE)  \
                         \
  TOKEN(BC_LEX_WHITESPACE)  \
                            \
  TOKEN(BC_LEX_LEFT_PAREN)   \
  TOKEN(BC_LEX_RIGHT_PAREN)  \
                             \
  TOKEN(BC_LEX_LEFT_BRACKET)   \
  TOKEN(BC_LEX_RIGHT_BRACKET)  \
                               \
  TOKEN(BC_LEX_LEFT_BRACE)   \
  TOKEN(BC_LEX_RIGHT_BRACE)  \
                             \
  TOKEN(BC_LEX_COMMA)      \
  TOKEN(BC_LEX_SEMICOLON)  \
                           \
  TOKEN(BC_LEX_STRING)  \
  TOKEN(BC_LEX_NAME)    \
  TOKEN(BC_LEX_NUMBER)  \
                        \
  TOKEN(BC_LEX_KEY_AUTO)      \
  TOKEN(BC_LEX_KEY_BREAK)     \
  TOKEN(BC_LEX_KEY_CONTINUE)  \
  TOKEN(BC_LEX_KEY_DEFINE)    \
  TOKEN(BC_LEX_KEY_ELSE)      \
  TOKEN(BC_LEX_KEY_FOR)       \
  TOKEN(BC_LEX_KEY_HALT)      \
  TOKEN(BC_LEX_KEY_IBASE)     \
  TOKEN(BC_LEX_KEY_IF)        \
  TOKEN(BC_LEX_KEY_LAST)      \
  TOKEN(BC_LEX_KEY_LENGTH)    \
  TOKEN(BC_LEX_KEY_LIMITS)    \
  TOKEN(BC_LEX_KEY_OBASE)     \
  TOKEN(BC_LEX_KEY_PRINT)     \
  TOKEN(BC_LEX_KEY_QUIT)      \
  TOKEN(BC_LEX_KEY_READ)      \
  TOKEN(BC_LEX_KEY_RETURN)    \
  TOKEN(BC_LEX_KEY_SCALE)     \
  TOKEN(BC_LEX_KEY_SQRT)      \
  TOKEN(BC_LEX_KEY_WHILE)     \
                              \
  TOKEN(BC_LEX_EOF)           \
                              \
  TOKEN(BC_LEX_INVALID)       \

typedef enum BcLexTokenType {
  BC_LEX_TOKEN_FOREACH(BC_LEX_GEN_ENUM)
} BcLexTokenType;

typedef struct BcLex {

  const char *buffer;
  size_t idx;
  size_t line;
  bool newline;
  const char *file;
  size_t len;

  struct {
    BcLexTokenType type;
    char *string;
  } token;

} BcLex;

typedef struct BcLexKeyword {

  const char name[9];
  const char len;
  const bool posix;

} BcLexKeyword;

#define BC_LEX_KW_ENTRY(a, b, c) { .name = a, .len = b, .posix = c }

// ** Exclude start. **
void bc_lex_init(BcLex *lex, const char *file);

BcStatus bc_lex_text(BcLex *lex, const char *text);

BcStatus bc_lex_next(BcLex *lex);
// ** Exclude end. **

extern const char *bc_lex_token_type_strs[];
extern const BcLexKeyword bc_lex_keywords[20];

#endif // BC_LEX_H
