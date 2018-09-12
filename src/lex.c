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

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <status.h>
#include <lex.h>
#include <bc.h>

BcStatus bc_lex_string(BcLex *lex) {

  BcStatus status;
  const char *buf;
  size_t len, newlines = 0, i = lex->idx;
  char c;

  lex->token.type = BC_LEX_STRING;

  for (c = lex->buffer[i]; c != '"' && c != '\0'; c = lex->buffer[++i])
    newlines += (c == '\n');

  if (c == '\0') {
    lex->idx = i;
    return BC_STATUS_LEX_NO_STRING_END;
  }

  len = i - lex->idx;
  if (len > (unsigned long) BC_MAX_STRING) return BC_STATUS_EXEC_STRING_LEN;

  buf = lex->buffer + lex->idx;
  if ((status = bc_vec_setToString(&lex->token.data, len, buf))) return status;

  lex->idx = i + 1;
  lex->line += newlines;

  return BC_STATUS_SUCCESS;
}

BcStatus bc_lex_comment(BcLex *lex) {

  size_t i, newlines = 0;
  const char *buffer = lex->buffer;
  bool end = false;

  lex->token.type = BC_LEX_WHITESPACE;

  for (i = ++lex->idx; !end; i += !end) {

    char c;

    for (; (c = buffer[i]) != '*' && c != '\0'; ++i)
      newlines += (c == '\n');

    if (c == '\0' || buffer[i + 1] == '\0') {
      lex->idx = i;
      return BC_STATUS_LEX_NO_COMMENT_END;
    }

    end = buffer[i + 1] == '/';
  }

  lex->idx = i + 2;
  lex->line += newlines;

  return BC_STATUS_SUCCESS;
}

BcStatus bc_lex_number(BcLex *lex, char start) {

  BcStatus status;
  const char *buf, *buffer = lex->buffer + lex->idx;
  size_t len, hits, backslashes = 0, i = 0, j;
  char c = buffer[i];
  bool point = start == '.';

  lex->token.type = BC_LEX_NUMBER;

  while (c && ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') ||
               (c == '.' && !point) || (c == '\\' && buffer[i + 1] == '\n')))
  {
    if (c == '\\') {
      ++i;
      backslashes += 1;
    }

    point = point || c == '.';
    c = buffer[++i];
  }

  len = i + 1 * (*(buffer + i - 1) != '.');
  if (len > BC_MAX_NUM) return BC_STATUS_EXEC_NUM_LEN;

  bc_vec_npop(&lex->token.data, lex->token.data.len);
  if ((status = bc_vec_expand(&lex->token.data, len - backslashes * 2 + 1)))
    return status;
  if ((status = bc_vec_push(&lex->token.data, 1, &start))) return status;

  buf = buffer - 1;
  hits = 0;

  for (j = 1; j < len; ++j) {

    c = buf[j];

    // If we have hit a backslash, skip it. We don't have
    // to check for a newline because it's guaranteed.
    if (hits < backslashes && c == '\\') {
      ++hits;
      ++j;
      continue;
    }

    if ((status = bc_vec_push(&lex->token.data, 1, &c))) return status;
  }

  if ((status = bc_vec_pushByte(&lex->token.data, '\0'))) return status;
  lex->idx += i;

  return BC_STATUS_SUCCESS;
}

BcStatus bc_lex_name(BcLex *lex) {

  BcStatus status;
  size_t i;
  char c;
  const char *buffer = lex->buffer + lex->idx - 1;

  for (i = 0; i < sizeof(bc_lex_keywords) / sizeof(bc_lex_keywords[0]); ++i) {

    unsigned long len = (unsigned long) bc_lex_keywords[i].len;
    if (!strncmp(buffer, bc_lex_keywords[i].name, len)) {

      lex->token.type = BC_LEX_KEY_AUTO + (BcLexToken) i;

      if (!bc_lex_keywords[i].posix &&
          (status = bc_posix_error(BC_STATUS_POSIX_BAD_KEYWORD, lex->file,
                                   lex->line, bc_lex_keywords[i].name)))
      {
        return status;
      }

      // We need to minus one because the index has already been incremented.
      lex->idx += len - 1;

      return BC_STATUS_SUCCESS;
    }
  }

  lex->token.type = BC_LEX_NAME;

  i = 0;
  c = buffer[i];

  while ((c >= 'a' && c<= 'z') || (c >= '0' && c <= '9') || c == '_')
    c = buffer[++i];

  if (i > 1 && (status = bc_posix_error(BC_STATUS_POSIX_NAME_LEN,
                                        lex->file, lex->line, buffer)))
  {
    return status;
  }

  if (i > (unsigned long) BC_MAX_STRING) return BC_STATUS_EXEC_NAME_LEN;
  if ((status = bc_vec_setToString(&lex->token.data, i, buffer))) return status;

  // Increment the index. We minus one because it has already been incremented.
  lex->idx += i - 1;

  return BC_STATUS_SUCCESS;
}

BcStatus bc_lex_token(BcLex *lex) {

  BcStatus status = BC_STATUS_SUCCESS;
  char c, c2;

  // This is the workhorse of the lexer.
  switch ((c = lex->buffer[lex->idx++])) {

    case '\0':
    case '\n':
    {
      lex->newline = true;
      lex->token.type = BC_LEX_NEWLINE  + (!c) * (BC_LEX_EOF - BC_LEX_NEWLINE);
      break;
    }

    case '\t':
    case '\v':
    case '\f':
    case '\r':
    case ' ':
    case '\\':
    {
      lex->token.type = BC_LEX_WHITESPACE;
      c = lex->buffer[lex->idx];

      while ((isspace(c) && c != '\n') || c == '\\')
        c = lex->buffer[++lex->idx];

      break;
    }

    case '!':
    {
      c2 = lex->buffer[lex->idx];

      if (c2 == '=') {
        ++lex->idx;
        lex->token.type = BC_LEX_OP_REL_NOT_EQ;
      }
      else {

        if ((status = bc_posix_error(BC_STATUS_POSIX_BOOL_OPS,
                                     lex->file, lex->line, "!")))
        {
          return status;
        }

        lex->token.type = BC_LEX_OP_BOOL_NOT;
      }

      break;
    }

    case '"':
    {
      status = bc_lex_string(lex);
      break;
    }

    case '#':
    {
      if ((status = bc_posix_error(BC_STATUS_POSIX_SCRIPT_COMMENT,
                                  lex->file, lex->line, NULL)))
      {
        return status;
      }

      lex->token.type = BC_LEX_WHITESPACE;
      while (++lex->idx < lex->len && lex->buffer[lex->idx] != '\n');

      break;
    }

    case '%':
    {
      if ((c2 = lex->buffer[lex->idx]) == '=') {
        ++lex->idx;
        lex->token.type = BC_LEX_OP_ASSIGN_MODULUS;
      }
      else lex->token.type = BC_LEX_OP_MODULUS;
      break;
    }

    case '&':
    {
      if ((c2 = lex->buffer[lex->idx]) == '&') {

        if ((status = bc_posix_error(BC_STATUS_POSIX_BOOL_OPS,
                                     lex->file, lex->line, "&&")))
        {
          return status;
        }

        ++lex->idx;
        lex->token.type = BC_LEX_OP_BOOL_AND;
      }
      else {
        lex->token.type = BC_LEX_INVALID;
        status = BC_STATUS_LEX_BAD_CHARACTER;
      }

      break;
    }

    case '(':
    case ')':
    {
      lex->token.type = (BcLexToken) (c - '(' + BC_LEX_LEFT_PAREN);
      break;
    }

    case '*':
    {
      if ((c2 = lex->buffer[lex->idx]) == '=') {
        ++lex->idx;
        lex->token.type = BC_LEX_OP_ASSIGN_MULTIPLY;
      }
      else lex->token.type = BC_LEX_OP_MULTIPLY;
      break;
    }

    case '+':
    {
      if ((c2 = lex->buffer[lex->idx]) == '=') {
        ++lex->idx;
        lex->token.type = BC_LEX_OP_ASSIGN_PLUS;
      }
      else if (c2 == '+') {
        ++lex->idx;
        lex->token.type = BC_LEX_OP_INC;
      }
      else lex->token.type = BC_LEX_OP_PLUS;
      break;
    }

    case ',':
    {
      lex->token.type = BC_LEX_COMMA;
      break;
    }

    case '-':
    {
      if ((c2 = lex->buffer[lex->idx]) == '=') {
        ++lex->idx;
        lex->token.type = BC_LEX_OP_ASSIGN_MINUS;
      }
      else if (c2 == '-') {
        ++lex->idx;
        lex->token.type = BC_LEX_OP_DEC;
      }
      else lex->token.type = BC_LEX_OP_MINUS;
      break;
    }

    case '.':
    {
      c2 = lex->buffer[lex->idx];
      if (isdigit(c2)) status = bc_lex_number(lex, c);
      else {
        status = bc_posix_error(BC_STATUS_POSIX_DOT_LAST,
                                lex->file, lex->line, NULL);
        lex->token.type = BC_LEX_KEY_LAST;
      }
      break;
    }

    case '/':
    {
      if ((c2 = lex->buffer[lex->idx]) == '=') {
        ++lex->idx;
        lex->token.type = BC_LEX_OP_ASSIGN_DIVIDE;
      }
      else if (c2 == '*') status = bc_lex_comment(lex);
      else lex->token.type = BC_LEX_OP_DIVIDE;
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
      status = bc_lex_number(lex, c);
      break;
    }

    case ';':
    {
      lex->token.type = BC_LEX_SEMICOLON;
      break;
    }

    case '<':
    {
      if ((c2 = lex->buffer[lex->idx]) == '=') {
        ++lex->idx;
        lex->token.type = BC_LEX_OP_REL_LESS_EQ;
      }
      else lex->token.type = BC_LEX_OP_REL_LESS;
      break;
    }

    case '=':
    {
      if ((c2 = lex->buffer[lex->idx]) == '=') {
        ++lex->idx;
        lex->token.type = BC_LEX_OP_REL_EQUAL;
      }
      else lex->token.type = BC_LEX_OP_ASSIGN;
      break;
    }

    case '>':
    {
      if ((c2 = lex->buffer[lex->idx]) == '=') {
        ++lex->idx;
        lex->token.type = BC_LEX_OP_REL_GREATER_EQ;
      }
      else lex->token.type = BC_LEX_OP_REL_GREATER;
      break;
    }

    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
    {
      status = bc_lex_number(lex, c);
      break;
    }

    case '[':
    case ']':
    {
      lex->token.type = (BcLexToken) (c - '[' + BC_LEX_LEFT_BRACKET);
      break;
    }

    case '^':
    {
      if ((c2 = lex->buffer[lex->idx]) == '=') {
        ++lex->idx;
        lex->token.type = BC_LEX_OP_ASSIGN_POWER;
      }
      else lex->token.type = BC_LEX_OP_POWER;
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
      status = bc_lex_name(lex);
      break;
    }

    case '{':
    case '}':
    {
      lex->token.type = (BcLexToken) (c - '{' + BC_LEX_LEFT_BRACE);
      break;
    }

    case '|':
    {
      if ((c2 = lex->buffer[lex->idx]) == '|') {

        if ((status = bc_posix_error(BC_STATUS_POSIX_BOOL_OPS,
                                     lex->file, lex->line, "||")))
        {
          return status;
        }

        ++lex->idx;
        lex->token.type = BC_LEX_OP_BOOL_OR;
      }
      else {
        lex->token.type = BC_LEX_INVALID;
        status = BC_STATUS_LEX_BAD_CHARACTER;
      }

      break;
    }

    default:
    {
      lex->token.type = BC_LEX_INVALID;
      status = BC_STATUS_LEX_BAD_CHARACTER;
      break;
    }
  }

  return status;
}

BcStatus bc_lex_init(BcLex *lex) {
  assert(lex);
  return bc_vec_init(&lex->token.data, sizeof(uint8_t), NULL);
}

void bc_lex_free(BcLex *lex) {
  assert(lex);
  bc_vec_free(&lex->token.data);
}

void bc_lex_file(BcLex *lex, const char *file) {
  assert(lex && file);
  lex->line = 1;
  lex->newline = false;
  lex->file = file;
}

BcStatus bc_lex_next(BcLex *lex) {

  BcStatus status;

  assert(lex);

  if (lex->token.type == BC_LEX_EOF) return BC_STATUS_LEX_EOF;

  if (lex->idx == lex->len) {
    lex->newline = true;
    lex->token.type = BC_LEX_EOF;
    return BC_STATUS_SUCCESS;
  }

  if (lex->newline) {
    ++lex->line;
    lex->newline = false;
  }

  // Loop until failure or we don't have whitespace. This
  // is so the parser doesn't get inundated with whitespace.
  do {
    status = bc_lex_token(lex);
  } while (!status && lex->token.type == BC_LEX_WHITESPACE);

  return status;
}

BcStatus bc_lex_text(BcLex *lex, const char *text) {
  assert(lex && text);
  lex->buffer = text;
  lex->idx = 0;
  lex->len = strlen(text);
  lex->token.type = BC_LEX_INVALID;
  return bc_lex_next(lex);
}
