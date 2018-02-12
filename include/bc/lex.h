/*
 * Copyright 2018 Contributors
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.

 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *******************************************************************************
 *
 * A special license exemption is granted to the Toybox project and to the
 * Android Open Source Project to use this source under the following BSD
 * 0-Clause License:
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
 *******************************************************************************
 *
 * Definitions for bc's lexer.
 *
 */

#ifndef BC_LEX_H
#define BC_LEX_H

#include <stdbool.h>
#include <stdlib.h>

#include <bc/bc.h>

#define BC_LEX_GEN_ENUM(ENUM) ENUM,
#define BC_LEX_GEN_STR(STRING) #STRING,

// BC_LEX_OP_NEGATE is not used in lexing;
// it is only for parsing.
#define BC_LEX_TOKEN_FOREACH(TOKEN) \
  TOKEN(BC_LEX_OP_INC)  \
  TOKEN(BC_LEX_OP_DEC)  \
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
  TOKEN(BC_LEX_OP_NEGATE)    \
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

typedef struct BcLexToken {

  BcLexTokenType type;
  char* string;

} BcLexToken;

typedef struct BcLex {

  const char* buffer;
  size_t idx;
  uint32_t line;
  bool newline;
  const char* file;
  size_t len;

} BcLex;

BcStatus bc_lex_init(BcLex* lex, const char* file);

BcStatus bc_lex_text(BcLex* lex, const char* text);

BcStatus bc_lex_next(BcLex* lex, BcLexToken* token);

BcStatus bc_lex_printToken(BcLexToken* token);

#endif // BC_LEX_H
