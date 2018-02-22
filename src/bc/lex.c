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
 * The lexer.
 *
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <bc.h>
#include <lex.h>

static const char* const token_type_strs[] = {
  BC_LEX_TOKEN_FOREACH(BC_LEX_GEN_STR)
};

static const char* const keywords[] = {

  "auto",
  "break",
  "continue",
  "define",
  "else",
  "for",
  "halt",
  "ibase",
  "if",
  "last",
  "length",
  "limits",
  "obase",
  "print",
  "quit",
  "read",
  "return",
  "scale",
  "sqrt",
  "while",

};

static const uint32_t keyword_lens[] = {

  4, // auto
  5, // break
  8, // continue
  6, // define
  4, // else
  3, // for
  4, // halt
  5, // ibase
  2, // if
  4, // last
  6, // length
  6, // limits
  5, // obase
  5, // print
  4, // quit
  4, // read
  6, // return
  5, // scale
  4, // sqrt
  5, // while

};

static const bool keyword_posix[] = {

  true, // auto
  true, // break
  false, // continue
  true, // define
  false, // else
  true, // for
  false, // halt
  true, // ibase
  true, // if
  false, // last
  true, // length
  false, // limits
  true, // obase
  false, // print
  true, // quit
  false, // read
  true, // return
  true, // scale
  true, // sqrt
  true, // while

};

static BcStatus bc_lex_token(BcLex* lex, BcLexToken* token);
static BcStatus bc_lex_whitespace(BcLex* lex, BcLexToken* token);
static BcStatus bc_lex_string(BcLex* lex, BcLexToken* token);
static BcStatus bc_lex_comment(BcLex* lex, BcLexToken* token);
static BcStatus bc_lex_number(BcLex* lex, BcLexToken* token, char start);
static BcStatus bc_lex_name(BcLex* lex, BcLexToken* token);

BcStatus bc_lex_printToken(BcLexToken* token) {

  printf("<%s", token_type_strs[token->type]);

  switch (token->type) {

    case BC_LEX_STRING:
    case BC_LEX_NAME:
    case BC_LEX_NUMBER:
    {
      printf(":%s", token->string);
      break;
    }

    default:
    {
      // Do nothing.
      break;
    }
  }

  putchar('>');
  putchar('\n');

  return BC_STATUS_SUCCESS;
}

BcStatus bc_lex_init(BcLex* lex, const char* file) {

  if (lex == NULL ) return BC_STATUS_INVALID_PARAM;

  lex->line = 1;
  lex->newline = false;
  lex->file = file;

  return BC_STATUS_SUCCESS;
}

BcStatus bc_lex_text(BcLex* lex, const char* text) {

  if (lex == NULL || text == NULL) return BC_STATUS_INVALID_PARAM;

  lex->buffer = text;
  lex->idx = 0;
  lex->len = strlen(text);

  return BC_STATUS_SUCCESS;
}

BcStatus bc_lex_next(BcLex* lex, BcLexToken* token) {

  BcStatus status;

  if (lex == NULL || token == NULL) return BC_STATUS_INVALID_PARAM;

  if (lex->idx == lex->len) {
    token->type = BC_LEX_EOF;
    return BC_STATUS_LEX_EOF;
  }

  if (lex->newline) {
    ++lex->line;
    lex->newline = false;
  }

  // Loop until failure or we don't have whitespace. This
  // is so the parser doesn't get inundated with whitespace.
  do {
    status = bc_lex_token(lex, token);
  } while (!status && token->type == BC_LEX_WHITESPACE);

  return status;
}

static BcStatus bc_lex_token(BcLex* lex, BcLexToken* token) {

  BcStatus status;
  char c;
  char c2;

  status = BC_STATUS_SUCCESS;

  c = lex->buffer[lex->idx];

  ++lex->idx;

  // This is the workhorse of the lexer.
  switch (c) {

    case '\0':
    {
      token->type = BC_LEX_EOF;
      break;
    }

    case '\t':
    {
      status = bc_lex_whitespace(lex, token);
      break;
    }

    case '\n':
    {
      lex->newline = true;
      token->type = BC_LEX_NEWLINE;
      break;
    }

    case '\v':
    case '\f':
    case '\r':
    case ' ':
    {
      status = bc_lex_whitespace(lex, token);
      break;
    }

    case '!':
    {
      c2 = lex->buffer[lex->idx];

      if (c2 == '=') {
        ++lex->idx;
        token->type = BC_LEX_OP_REL_NOT_EQ;
      }
      else {

        if ((status = bc_posix_error(BC_STATUS_POSIX_BOOL_OPS,
                                     lex->file, lex->line, "!")))
        {
          return status;
        }

        token->type = BC_LEX_OP_BOOL_NOT;
      }

      break;
    }

    case '"':
    {
      status = bc_lex_string(lex, token);
      break;
    }

    case '#':
    {
      if ((status = bc_posix_error(BC_STATUS_POSIX_SCRIPT_COMMENT,
                                  lex->file, lex->line, NULL)))
      {
        return status;
      }

      token->type = BC_LEX_WHITESPACE;

      ++lex->idx;
      while (lex->idx < lex->len && lex->buffer[lex->idx] != '\n') ++lex->idx;

      break;
    }

    case '%':
    {
      c2 = lex->buffer[lex->idx];

      if (c2 == '=') {
        ++lex->idx;
        token->type = BC_LEX_OP_ASSIGN_MODULUS;
      }
      else token->type = BC_LEX_OP_MODULUS;

      break;
    }

    case '&':
    {
      c2 = lex->buffer[lex->idx];

      if (c2 == '&') {

        if ((status = bc_posix_error(BC_STATUS_POSIX_BOOL_OPS,
                                     lex->file, lex->line, "&&")))
        {
          return status;
        }

        ++lex->idx;
        token->type = BC_LEX_OP_BOOL_AND;
      }
      else {
        token->type = BC_LEX_INVALID;
        status = BC_STATUS_LEX_INVALID_TOKEN;
      }

      break;
    }

    case '(':
    {
      token->type = BC_LEX_LEFT_PAREN;
      break;
    }

    case ')':
    {
      token->type = BC_LEX_RIGHT_PAREN;
      break;
    }

    case '*':
    {
      c2 = lex->buffer[lex->idx];

      if (c2 == '=') {
        ++lex->idx;
        token->type = BC_LEX_OP_ASSIGN_MULTIPLY;
      }
      else token->type = BC_LEX_OP_MULTIPLY;

      break;
    }

    case '+':
    {
      c2 = lex->buffer[lex->idx];

      if (c2 == '=') {
        ++lex->idx;
        token->type = BC_LEX_OP_ASSIGN_PLUS;
      }
      else if (c2 == '+') {
        ++lex->idx;
        token->type = BC_LEX_OP_INC;
      }
      else token->type = BC_LEX_OP_PLUS;

      break;
    }

    case ',':
    {
      token->type = BC_LEX_COMMA;
      break;
    }

    case '-':
    {
      c2 = lex->buffer[lex->idx];

      if (c2 == '=') {
        ++lex->idx;
        token->type = BC_LEX_OP_ASSIGN_MINUS;
      }
      else if (c2 == '-') {
        ++lex->idx;
        token->type = BC_LEX_OP_DEC;
      }
      else token->type = BC_LEX_OP_MINUS;

      break;
    }

    case '.':
    {
      status = bc_lex_number(lex, token, c);
      break;
    }

    case '/':
    {
      c2 = lex->buffer[lex->idx];

      if (c2 == '=') {
        ++lex->idx;
        token->type = BC_LEX_OP_ASSIGN_DIVIDE;
      }
      else if (c2 == '*') status = bc_lex_comment(lex, token);
      else token->type = BC_LEX_OP_DIVIDE;

      break;
    }

    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    {
      status = bc_lex_number(lex, token, c);
      break;
    }

    case ';':
    {
      token->type = BC_LEX_SEMICOLON;
      break;
    }

    case '<':
    {
      c2 = lex->buffer[lex->idx];

      if (c2 == '=') {
        ++lex->idx;
        token->type = BC_LEX_OP_REL_LESS_EQ;
      }
      else token->type = BC_LEX_OP_REL_LESS;

      break;
    }

    case '=':
    {
      c2 = lex->buffer[lex->idx];

      if (c2 == '=') {
        ++lex->idx;
        token->type = BC_LEX_OP_REL_EQUAL;
      }
      else token->type = BC_LEX_OP_ASSIGN;

      break;
    }

    case '>':
    {
      c2 = lex->buffer[lex->idx];

      if (c2 == '=') {
        ++lex->idx;
        token->type = BC_LEX_OP_REL_GREATER_EQ;
      }
      else token->type = BC_LEX_OP_REL_GREATER;

      break;
    }

    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
    {
      status = bc_lex_number(lex, token, c);
      break;
    }

    case '[':
    {
      token->type = BC_LEX_LEFT_BRACKET;
      break;
    }

    case '\\':
    {
      status = bc_lex_whitespace(lex, token);
      break;
    }

    case ']':
    {
      token->type = BC_LEX_RIGHT_BRACKET;
      break;
    }

    case '^':
    {
      c2 = lex->buffer[lex->idx];

      if (c2 == '=') {
        ++lex->idx;
        token->type = BC_LEX_OP_ASSIGN_POWER;
      }
      else token->type = BC_LEX_OP_POWER;

      break;
    }

    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
    case 'g':
    case 'h':
    case 'i':
    case 'j':
    case 'k':
    case 'l':
    case 'm':
    case 'n':
    case 'o':
    case 'p':
    case 'q':
    case 'r':
    case 's':
    case 't':
    case 'u':
    case 'v':
    case 'w':
    case 'x':
    case 'y':
    case 'z':
    {
      status = bc_lex_name(lex, token);
      break;
    }

    case '{':
    {
      token->type = BC_LEX_LEFT_BRACE;
      break;
    }

    case '|':
    {
      c2 = lex->buffer[lex->idx];

      if (c2 == '|') {

        if ((status = bc_posix_error(BC_STATUS_POSIX_BOOL_OPS,
                                     lex->file, lex->line, "||")))
        {
          return status;
        }

        ++lex->idx;
        token->type = BC_LEX_OP_BOOL_OR;
      }
      else {
        token->type = BC_LEX_INVALID;
        status = BC_STATUS_LEX_INVALID_TOKEN;
      }

      break;
    }

    case '}':
    {
      token->type = BC_LEX_RIGHT_BRACE;
      break;
    }

    default:
    {
      token->type = BC_LEX_INVALID;
      status = BC_STATUS_LEX_INVALID_TOKEN;
      break;
    }
  }

  return status;
}

static BcStatus bc_lex_whitespace(BcLex* lex, BcLexToken* token) {

  token->type = BC_LEX_WHITESPACE;

  char c = lex->buffer[lex->idx];

  while ((isspace(c) && c != '\n') || c == '\\') {
    ++lex->idx;
    c = lex->buffer[lex->idx];
  }

  return BC_STATUS_SUCCESS;
}

static BcStatus bc_lex_string(BcLex* lex, BcLexToken* token) {

  uint32_t newlines;

  newlines = 0;

  token->type = BC_LEX_STRING;

  size_t i = lex->idx;
  char c = lex->buffer[i];

  while (c != '"' && c != '\0') {

    if (c == '\n') ++newlines;

    c = lex->buffer[++i];
  }

  if (c == '\0') {
    lex->idx = i;
    return BC_STATUS_LEX_NO_STRING_END;
  }

  size_t len = i - lex->idx;

  token->string = malloc(len + 1);

  if (token->string == NULL) return BC_STATUS_MALLOC_FAIL;

  const char* start = lex->buffer + lex->idx;

  for (size_t j = 0; j < len; ++j) token->string[j] = start[j];

  token->string[len] = '\0';

  lex->idx = i + 1;
  lex->line += newlines;

  return BC_STATUS_SUCCESS;
}

static BcStatus bc_lex_comment(BcLex* lex, BcLexToken* token) {

  uint32_t newlines;
  bool end;
  size_t i;
  const char* buffer;
  char c;

  newlines = 0;

  token->type = BC_LEX_WHITESPACE;

  ++lex->idx;

  i = lex->idx;
  buffer = lex->buffer;

  end = false;

  while (!end) {

    c = buffer[i];

    while (c != '*' && c != '\0') {
      if (c == '\n') ++newlines;
      c = buffer[++i];
    }

    if (c == '\0' || buffer[i + 1] == '\0') {
      lex->idx = i;
      return BC_STATUS_LEX_NO_COMMENT_END;
    }

    end = buffer[i + 1] == '/';
    i += end ? 0 : 1;
  }

  lex->idx = i + 2;
  lex->line += newlines;

  return BC_STATUS_SUCCESS;
}

static BcStatus bc_lex_number(BcLex* lex, BcLexToken* token, char start) {

  token->type = BC_LEX_NUMBER;

  int point = start == '.';

  const char* buffer = lex->buffer + lex->idx;

  size_t backslashes = 0;
  size_t i = 0;
  char c = buffer[i];

  while (c && ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') ||
               (c == '.' && !point) || (c == '\\' && buffer[i + 1] == '\n')))
  {
    if (c == '\\') {
      ++i;
      backslashes += 1;
    }

    c = buffer[++i];
  }

  size_t len = i + 1;

  token->string = malloc(len - backslashes + 1);

  if (token->string == NULL) return BC_STATUS_MALLOC_FAIL;

  token->string[0] = start;

  const char* buf = buffer - 1;
  size_t hits = 0;

  for (size_t j = 1; j < len; ++j) {

    char c = buf[j];

    // If we have hit a backslash, skip it.
    // We don't have to check for a newline
    // because it's guaranteed.
    if (hits < backslashes && c == '\\') {
      ++hits;
      ++j;
      continue;
    }

    token->string[j - (hits * 2)] = c;
  }

  token->string[len] = '\0';

  lex->idx += i;

  return BC_STATUS_SUCCESS;
}

static BcStatus bc_lex_name(BcLex* lex, BcLexToken* token) {

  BcStatus status;

  const char* buffer = lex->buffer + lex->idx - 1;

  for (uint32_t i = 0; i < sizeof(keywords) / sizeof(char*); ++i) {

    if (!strncmp(buffer, keywords[i], keyword_lens[i])) {

      token->type = BC_LEX_KEY_AUTO + i;

      if (!keyword_posix[i] &&
          (status = bc_posix_error(BC_STATUS_POSIX_INVALID_KEYWORD,
                                   lex->file, lex->line, keywords[i])))
      {
        return status;
      }

      // We need to minus one because the
      // index has already been incremented.
      lex->idx += keyword_lens[i] - 1;

      return BC_STATUS_SUCCESS;
    }
  }

  token->type = BC_LEX_NAME;

  size_t i = 0;
  char c = buffer[i];

  while ((c >= 'a' && c<= 'z') || (c >= '0' && c <= '9') || c == '_') {
    ++i;
    c = buffer[i];
  }

  if (i > 1 && (status = bc_posix_error(BC_STATUS_POSIX_NAME_LEN,
                                        lex->file, lex->line, buffer)))
  {
    return status;
  }

  token->string = malloc(i + 1);

  if (token->string == NULL) {
    return BC_STATUS_MALLOC_FAIL;
  }

  strncpy(token->string, buffer, i);
  token->string[i] = '\0';

  // Increment the index. It is minus one
  // because it has already been incremented.
  lex->idx += i - 1;

  return BC_STATUS_SUCCESS;
}
