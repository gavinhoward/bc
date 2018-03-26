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
 * The parser.
 *
 */

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <lang.h>
#include <lex.h>
#include <parse.h>
#include <bc.h>

BcStatus bc_parse_else(BcParse *parse, BcVec *code);
BcStatus bc_parse_semicolonListEnd(BcParse *parse, BcVec *code);
BcStatus bc_parse_stmt(BcParse *parse, BcVec *code);

BcStatus bc_parse_pushName(BcVec *code, char *name) {

  BcStatus status;
  size_t len, i;

  status = BC_STATUS_SUCCESS;
  len = strlen(name);

  for (i = 0; !status && i < len; ++i)
    status = bc_vec_pushByte(code, (uint8_t) name[i]);

  if (status) return status;

  free(name);

  return bc_vec_pushByte(code, (uint8_t) ':');
}

BcStatus bc_parse_pushIndex(BcVec *code, size_t idx) {

  BcStatus status;
  uint8_t amt, i, nums[sizeof(size_t)];

  for (amt = 0; idx; ++amt) {
    nums[amt] = (uint8_t) idx;
    idx &= ~(UINT8_MAX);
    idx >>= sizeof(uint8_t) * CHAR_BIT;
  }

  if ((status = bc_vec_pushByte(code, amt))) return status;

  for (i = 0; !status && i < amt; ++i) status = bc_vec_pushByte(code, nums[i]);

  return status;
}

BcStatus bc_parse_operator(BcParse *parse, BcVec *code, BcVec *ops,
                           BcLexTokenType t, uint32_t *num_exprs, bool next)
{
  BcStatus status;
  BcLexTokenType top, *ptr;
  uint8_t lp, rp;
  bool rleft;

  rp = bc_parse_ops[t].prec;
  rleft = bc_parse_ops[t].left;

  if (ops->len) {

    ptr = bc_vec_top(ops);
    assert(ptr);
    top = *ptr;

    if (top != BC_LEX_LEFT_PAREN) {

      lp = bc_parse_ops[top].prec;

      while (lp < rp || (lp == rp && rleft)) {

        status = bc_vec_pushByte(code, BC_PARSE_TOKEN_TO_INST(top));
        if (status) return status;

        bc_vec_pop(ops);

        *num_exprs -= top != BC_LEX_OP_BOOL_NOT &&
                      top != BC_LEX_OP_NEG ? 1 : 0;

        if (!ops->len) break;

        ptr = bc_vec_top(ops);
        top = *ptr;

        if (top == BC_LEX_LEFT_PAREN) break;

        lp = bc_parse_ops[top].prec;
      }
    }
  }

  if ((status = bc_vec_push(ops, &t))) return status;
  if (next && (status = bc_lex_next(&parse->lex))) goto err;

  return status;

err:

  if (parse->lex.token.string) {
    free(parse->lex.token.string);
    parse->lex.token.string = NULL;
  }

  return status;
}

BcStatus bc_parse_rightParen(BcParse *parse, BcVec *code,
                             BcVec *ops, uint32_t *nexs)
{
  BcStatus status;
  BcLexTokenType top, *ptr;

  if (!ops->len) return BC_STATUS_PARSE_BAD_EXPR;

  ptr = bc_vec_top(ops);
  top = *ptr;

  while (top != BC_LEX_LEFT_PAREN) {

    status = bc_vec_pushByte(code, BC_PARSE_TOKEN_TO_INST(top));
    if (status) return status;

    bc_vec_pop(ops);

    *nexs -= top != BC_LEX_OP_BOOL_NOT &&
             top != BC_LEX_OP_NEG ? 1 : 0;

    if (!ops->len) return BC_STATUS_PARSE_BAD_EXPR;

    ptr = bc_vec_top(ops);
    assert(ptr);
    top = *ptr;
  }

  bc_vec_pop(ops);

  return bc_lex_next(&parse->lex);
}

BcStatus bc_parse_params(BcParse *parse, BcVec *code, uint8_t flags) {

  BcStatus status;
  bool comma;
  size_t nparams;

  if ((status = bc_lex_next(&parse->lex))) return status;

  if (parse->lex.token.type == BC_LEX_RIGHT_PAREN) {
    if ((status = bc_vec_pushByte(code, BC_INST_CALL))) return status;
    return bc_vec_pushByte(code, 0);
  }

  nparams = 0;

  do {

    ++nparams;

    status = bc_parse_expr(parse, code, flags & ~(BC_PARSE_EXPR_PRINT));
    if (status) return status;

    if (parse->lex.token.type == BC_LEX_COMMA) {
      comma = true;
      if ((status = bc_lex_next(&parse->lex))) return status;
    }
    else comma = false;

  } while (parse->lex.token.type != BC_LEX_RIGHT_PAREN);

  if (comma) return BC_STATUS_PARSE_BAD_TOKEN;

  if ((status = bc_vec_pushByte(code, BC_INST_CALL))) return status;

  return bc_parse_pushIndex(code, nparams);
}

BcStatus bc_parse_call(BcParse *parse, BcVec *code, char *name, uint8_t flags) {

  BcStatus status;
  BcEntry entry, *entry_ptr;
  size_t idx;

  entry.name = name;

  if ((status = bc_parse_params(parse, code, flags))) goto err;

  if (parse->lex.token.type != BC_LEX_RIGHT_PAREN) {
    status = BC_STATUS_PARSE_BAD_TOKEN;
    goto err;
  }

  idx = bc_veco_index(&parse->prog->func_map, &entry);

  if (idx == BC_INVALID_IDX) {

    status = bc_program_addFunc(parse->prog, name, &idx);
    if (status) return status;

    name = NULL;
  }
  else free(name);

  entry_ptr = bc_veco_item(&parse->prog->func_map, idx);

  assert(entry_ptr);

  if ((status = bc_parse_pushIndex(code, entry_ptr->idx))) return status;

  return bc_lex_next(&parse->lex);

err:

  if (name) free(name);

  return status;
}

BcStatus bc_parse_expr_name(BcParse *parse, BcVec *code,
                            BcInst *type, uint8_t flags)
{
  BcStatus status;
  char *name;

  name = parse->lex.token.string;

  if ((status = bc_lex_next(&parse->lex))) goto err;

  if (parse->lex.token.type == BC_LEX_LEFT_BRACKET) {

    *type = BC_INST_PUSH_ARRAY_ELEM;

    if ((status = bc_lex_next(&parse->lex))) goto err;
    if ((status = bc_parse_expr(parse, code, flags))) goto err;

    if (parse->lex.token.type != BC_LEX_RIGHT_BRACKET) {
      status = BC_STATUS_PARSE_BAD_TOKEN;
      goto err;
    }

    if ((status = bc_vec_pushByte(code, BC_INST_PUSH_ARRAY_ELEM))) goto err;

    status = bc_parse_pushName(code, name);
  }
  else if (parse->lex.token.type == BC_LEX_LEFT_PAREN) {

    if (flags & BC_PARSE_EXPR_NO_CALL) {
      status = BC_STATUS_PARSE_BAD_TOKEN;
      goto err;
    }

    *type = BC_INST_CALL;

    status = bc_parse_call(parse, code, name, flags);
  }
  else {

    *type = BC_INST_PUSH_VAR;

    if ((status = bc_vec_pushByte(code, BC_INST_PUSH_VAR))) goto err;

    status = bc_parse_pushName(code, name);
  }

  return status;

err:

  free(name);

  return status;
}

BcStatus bc_parse_read(BcParse *parse, BcVec *code) {

  BcStatus status;

  if ((status = bc_lex_next(&parse->lex))) return status;

  if (parse->lex.token.type != BC_LEX_LEFT_PAREN)
    return BC_STATUS_PARSE_BAD_TOKEN;

  if ((status = bc_lex_next(&parse->lex))) return status;

  if (parse->lex.token.type != BC_LEX_RIGHT_PAREN)
    return BC_STATUS_PARSE_BAD_TOKEN;

  if ((status = bc_vec_pushByte(code, BC_INST_READ))) return status;

  return bc_lex_next(&parse->lex);
}

BcStatus bc_parse_builtin(BcParse *parse, BcVec *code,
                          BcLexTokenType type, uint8_t flags)
{
  BcStatus status;
  uint8_t inst;

  if ((status = bc_lex_next(&parse->lex))) return status;

  if (parse->lex.token.type != BC_LEX_LEFT_PAREN)
    return BC_STATUS_PARSE_BAD_TOKEN;

  if ((status = bc_lex_next(&parse->lex))) return status;

  status = bc_parse_expr(parse, code, flags & ~(BC_PARSE_EXPR_PRINT));
  if (status) return status;

  if (parse->lex.token.type != BC_LEX_RIGHT_PAREN)
    return BC_STATUS_PARSE_BAD_TOKEN;

  inst = type == BC_LEX_KEY_LENGTH ? BC_INST_LENGTH : BC_INST_SQRT;

  if ((status = bc_vec_pushByte(code, inst))) return status;

  return bc_lex_next(&parse->lex);
}

BcStatus bc_parse_scale(BcParse *parse, BcVec *code,
                        BcInst *type, uint8_t flags)
{
  BcStatus status;

  if ((status = bc_lex_next(&parse->lex))) return status;

  if (parse->lex.token.type != BC_LEX_LEFT_PAREN) {
    *type = BC_INST_PUSH_SCALE;
    return bc_vec_pushByte(code, BC_INST_PUSH_SCALE);
  }

  *type = BC_INST_SCALE_FUNC;

  if ((status = bc_lex_next(&parse->lex))) return status;

  if ((status = bc_parse_expr(parse, code, flags))) return status;

  if (parse->lex.token.type != BC_LEX_RIGHT_PAREN)
    return BC_STATUS_PARSE_BAD_TOKEN;

  if ((status = bc_vec_pushByte(code, BC_INST_SCALE_FUNC))) return status;

  return bc_lex_next(&parse->lex);
}

BcStatus bc_parse_incdec(BcParse *parse, BcVec *code, BcInst *prev,
                         uint32_t *nexprs, uint8_t flags)
{
  BcStatus status;
  BcLexTokenType type;
  BcInst etype;
  uint8_t inst;

  etype = *prev;

  if (etype == BC_INST_PUSH_VAR || etype == BC_INST_PUSH_ARRAY_ELEM ||
      etype == BC_INST_PUSH_SCALE || etype == BC_INST_PUSH_LAST ||
      etype == BC_INST_PUSH_IBASE || etype == BC_INST_PUSH_OBASE)
  {
    *prev = inst = parse->lex.token.type == BC_LEX_OP_INC ?
                     BC_INST_INC_POST : BC_INST_DEC_POST;

    if ((status = bc_vec_pushByte(code, inst))) return status;

    status = bc_lex_next(&parse->lex);
  }
  else {

    *prev = inst = parse->lex.token.type == BC_LEX_OP_INC ?
                     BC_INST_INC_PRE : BC_INST_DEC_PRE;

    if ((status = bc_lex_next(&parse->lex))) return status;

    type = parse->lex.token.type;

    // Because we parse the next part of the expression
    // right here, we need to increment this.
    *nexprs = *nexprs + 1;

    switch (type) {

      case BC_LEX_NAME:
      {
        status = bc_parse_expr_name(parse, code, prev,
                                    flags | BC_PARSE_EXPR_NO_CALL);
        break;
      }

      case BC_LEX_KEY_IBASE:
      {
        if ((status = bc_vec_pushByte(code, BC_INST_PUSH_IBASE))) return status;
        status = bc_lex_next(&parse->lex);
        break;
      }

      case BC_LEX_KEY_LAST:
      {
        if ((status = bc_vec_pushByte(code, BC_INST_PUSH_LAST))) return status;
        status = bc_lex_next(&parse->lex);
        break;
      }

      case BC_LEX_KEY_OBASE:
      {
        if ((status = bc_vec_pushByte(code, BC_INST_PUSH_OBASE))) return status;
        status = bc_lex_next(&parse->lex);
        break;
      }

      case BC_LEX_KEY_SCALE:
      {
        if ((status = bc_lex_next(&parse->lex))) return status;

        if (parse->lex.token.type == BC_LEX_LEFT_PAREN)
          return BC_STATUS_PARSE_BAD_TOKEN;

        status = bc_vec_pushByte(code, BC_INST_PUSH_SCALE);

        break;
      }

      default:
      {
        return BC_STATUS_PARSE_BAD_TOKEN;
      }
    }

    if (status) return status;

    status = bc_vec_pushByte(code, inst);
  }

  return status;
}

BcStatus bc_parse_minus(BcParse *parse, BcVec *exs, BcVec *ops,
                        BcInst *prev, bool rparen, uint32_t *nexprs)
{
  BcStatus status;
  BcLexTokenType type;
  BcInst etype;

  etype = *prev;

  if ((status = bc_lex_next(&parse->lex))) return status;

  type = parse->lex.token.type;

  if (type != BC_LEX_NAME && type != BC_LEX_NUMBER &&
      type != BC_LEX_KEY_SCALE && type != BC_LEX_KEY_LAST &&
      type != BC_LEX_KEY_IBASE && type != BC_LEX_KEY_OBASE &&
      type != BC_LEX_LEFT_PAREN && type != BC_LEX_OP_MINUS &&
      type != BC_LEX_OP_INC && type != BC_LEX_OP_DEC &&
      type != BC_LEX_OP_BOOL_NOT)
  {
    return BC_STATUS_PARSE_BAD_TOKEN;
  }

  type = rparen || etype == BC_INST_INC_POST || etype == BC_INST_DEC_POST ||
         (etype >= BC_INST_PUSH_NUM && etype <= BC_INST_SQRT) ?
                  BC_LEX_OP_MINUS : BC_LEX_OP_NEG;

  *prev = BC_PARSE_TOKEN_TO_INST(type);

  if (type == BC_LEX_OP_MINUS)
    status = bc_parse_operator(parse, exs, ops, type, nexprs, false);
  else
    // We can just push onto the op stack because this is the largest
    // precedence operator that gets pushed. Inc/dec does not.
    status = bc_vec_push(ops, &type);

  return status;
}

BcStatus bc_parse_string(BcParse *parse, BcVec *code) {

  BcStatus status;
  size_t len;

  if (strlen(parse->lex.token.string) > (unsigned long) parse->prog->string_max) {
    status = BC_STATUS_EXEC_STRING_LEN;
    goto err;
  }

  len = parse->prog->strings.len;

  status = bc_vec_push(&parse->prog->strings, &parse->lex.token.string);
  if (status) goto err;

  if ((status = bc_vec_pushByte(code, BC_INST_STR))) return status;
  if ((status = bc_parse_pushIndex(code, len))) return status;
  if ((status = bc_lex_next(&parse->lex))) return status;

  return bc_parse_semicolonListEnd(parse, code);

err:

  free(parse->lex.token.string);

  return status;
}

BcStatus bc_parse_print(BcParse *parse, BcVec *code) {

  BcStatus status;
  BcLexTokenType type;
  bool comma;

  if ((status = bc_lex_next(&parse->lex))) return status;

  type = parse->lex.token.type;

  if (type == BC_LEX_SEMICOLON || type == BC_LEX_NEWLINE)
    return BC_STATUS_PARSE_BAD_PRINT;

  comma = false;

  while (!status && type != BC_LEX_SEMICOLON && type != BC_LEX_NEWLINE) {

    if (type == BC_LEX_STRING) {

      size_t len;

      len = parse->prog->strings.len;

      status = bc_vec_push(&parse->prog->strings, &parse->lex.token.string);
      if (status) {
        free(parse->lex.token.string);
        return status;
      }

      if ((status = bc_vec_pushByte(code, BC_INST_PRINT_STR))) return status;

      status = bc_parse_pushIndex(code, len);
    }
    else {
      if ((status = bc_parse_expr(parse, code, 0))) return status;
      status = bc_vec_pushByte(code, BC_INST_PRINT_EXPR);
    }

    if (status) return status;

    if ((status = bc_lex_next(&parse->lex))) return status;

    if (parse->lex.token.type == BC_LEX_COMMA) {
      comma = true;
      status = bc_lex_next(&parse->lex);
    }
    else comma = false;

    type = parse->lex.token.type;
  }

  if (status) return status;

  if (comma) return BC_STATUS_PARSE_BAD_TOKEN;

  return bc_lex_next(&parse->lex);
}

BcStatus bc_parse_return(BcParse *parse, BcVec *code) {

  BcStatus status;

  if (!BC_PARSE_FUNC(parse)) return BC_STATUS_PARSE_BAD_TOKEN;

  if ((status = bc_lex_next(&parse->lex))) return status;

  if (parse->lex.token.type != BC_LEX_NEWLINE &&
      parse->lex.token.type != BC_LEX_SEMICOLON &&
      parse->lex.token.type != BC_LEX_LEFT_PAREN &&
      (status = bc_posix_error(BC_STATUS_POSIX_RETURN_PARENS,
                               parse->lex.file, parse->lex.line, NULL)))
  {
     return status;
  }

  if (parse->lex.token.type == BC_LEX_NEWLINE ||
      parse->lex.token.type == BC_LEX_SEMICOLON)
  {
    status = bc_vec_pushByte(code, BC_INST_RETURN_ZERO);
  }
  else {
    if ((status = bc_parse_expr(parse, code, 0))) return status;
    status = bc_vec_pushByte(code, BC_INST_RETURN);
  }

  return status;
}

BcStatus bc_parse_endBody(BcParse *parse, BcVec *code, bool brace) {

  BcStatus status;
  uint8_t *flag_ptr;

  status = BC_STATUS_SUCCESS;

  if (parse->flags.len <= 1 || parse->num_braces == 0)
    return BC_STATUS_PARSE_BAD_TOKEN;

  if (brace) {

    if (parse->lex.token.type == BC_LEX_RIGHT_BRACE) {

      if (!parse->num_braces) return BC_STATUS_PARSE_BAD_TOKEN;

      --parse->num_braces;

      if ((status = bc_lex_next(&parse->lex))) return status;
    }
    else return BC_STATUS_PARSE_BAD_TOKEN;
  }

  if (BC_PARSE_IF(parse)) {

    while (parse->lex.token.type == BC_LEX_NEWLINE) {
      if ((status = bc_lex_next(&parse->lex))) return status;
    }

    bc_vec_pop(&parse->flags);

    flag_ptr = BC_PARSE_TOP_FLAG_PTR(parse);
    assert(flag_ptr);
    *flag_ptr = (*flag_ptr | BC_PARSE_FLAG_IF_END);

    if (parse->lex.token.type == BC_LEX_KEY_ELSE)
      status = bc_parse_else(parse, code);
  }
  else if (BC_PARSE_ELSE(parse)) {

    BcInstPtr *ip;
    BcFunc *func;
    size_t *label;

    bc_vec_pop(&parse->flags);

    ip = bc_vec_top(&parse->exit_labels);
    assert(ip);
    func = bc_vec_item(&parse->prog->funcs, parse->func);
    assert(func);
    label = bc_vec_item(&func->labels, ip->idx);
    assert(label);

    *label = code->len;

    bc_vec_pop(&parse->exit_labels);
  }
  else if (BC_PARSE_FUNC_INNER(parse)) {

    parse->func = 0;

    if ((status = bc_vec_pushByte(code, BC_INST_RETURN_ZERO))) return status;

    bc_vec_pop(&parse->flags);
  }
  else {

    BcInstPtr *ip;
    BcFunc *func;
    size_t *label;

    ip = bc_vec_top(&parse->exit_labels);

    assert(ip);

    if ((status = bc_vec_pushByte(code, BC_INST_JUMP))) return status;

    label = bc_vec_top(&parse->cond_labels);

    assert(label);

    if ((status = bc_parse_pushIndex(code, *label))) return status;

    func = bc_vec_item(&parse->prog->funcs, parse->func);
    assert(func);
    label = bc_vec_item(&func->labels, ip->idx);
    assert(label);

    *label = code->len;

    bc_vec_pop(&parse->flags);
    bc_vec_pop(&parse->exit_labels);
    bc_vec_pop(&parse->cond_labels);
  }

  return status;
}

BcStatus bc_parse_startBody(BcParse *parse, uint8_t flags) {

  uint8_t *flag_ptr = BC_PARSE_TOP_FLAG_PTR(parse);

  assert(flag_ptr);

  flags |= (*flag_ptr & (BC_PARSE_FLAG_FUNC | BC_PARSE_FLAG_LOOP));
  flags |= BC_PARSE_FLAG_BODY;

  return bc_vec_push(&parse->flags, &flags);
}

void bc_parse_noElse(BcParse *parse, BcVec *code) {

  uint8_t *flag_ptr;
  BcInstPtr *ip;
  BcFunc *func;
  size_t *label;

  flag_ptr = BC_PARSE_TOP_FLAG_PTR(parse);
  assert(flag_ptr);
  *flag_ptr = (*flag_ptr & ~(BC_PARSE_FLAG_IF_END));

  ip = bc_vec_top(&parse->exit_labels);
  assert(ip && !ip->func && !ip->len);
  func = bc_vec_item(&parse->prog->funcs, parse->func);
  assert(func && code == &func->code);
  label = bc_vec_item(&func->labels, ip->idx);
  assert(label);

  *label = code->len;

  bc_vec_pop(&parse->exit_labels);
}

BcStatus bc_parse_if(BcParse *parse, BcVec *code) {

  BcStatus status;
  BcInstPtr ip;
  BcFunc *func;

  if ((status = bc_lex_next(&parse->lex))) return status;

  if (parse->lex.token.type != BC_LEX_LEFT_PAREN)
    return BC_STATUS_PARSE_BAD_TOKEN;

  if ((status = bc_lex_next(&parse->lex))) return status;

  status = bc_parse_expr(parse, code, BC_PARSE_EXPR_POSIX_REL);
  if (status) return status;

  if (parse->lex.token.type != BC_LEX_RIGHT_PAREN)
    return BC_STATUS_PARSE_BAD_TOKEN;

  if ((status = bc_lex_next(&parse->lex))) return status;
  if ((status = bc_vec_pushByte(code, BC_INST_JUMP_ZERO))) return status;

  func = bc_vec_item(&parse->prog->funcs, parse->func);

  assert(func);

  ip.idx = func->labels.len;
  ip.func = 0;
  ip.len = 0;

  if ((status = bc_parse_pushIndex(code, ip.idx))) return status;
  if ((status = bc_vec_push(&parse->exit_labels, &ip))) return status;
  if ((status = bc_vec_push(&func->labels, &ip.idx))) return status;

  return bc_parse_startBody(parse, BC_PARSE_FLAG_IF);
}

BcStatus bc_parse_else(BcParse *parse, BcVec *code) {

  BcStatus status;
  BcInstPtr ip;
  BcFunc *func;

  if (!BC_PARSE_IF_END(parse)) return BC_STATUS_PARSE_BAD_TOKEN;

  func = bc_vec_item(&parse->prog->funcs, parse->func);

  assert(func);

  ip.idx = func->labels.len;
  ip.func = 0;
  ip.len = 0;

  if ((status = bc_vec_pushByte(code, BC_INST_JUMP))) return status;
  if ((status = bc_parse_pushIndex(code, ip.idx))) return status;

  bc_parse_noElse(parse, code);

  if ((status = bc_vec_push(&parse->exit_labels, &ip))) return status;
  if ((status = bc_vec_push(&func->labels, &ip.idx))) return status;
  if ((status = bc_lex_next(&parse->lex))) return status;

  return bc_parse_startBody(parse, BC_PARSE_FLAG_ELSE);
}

BcStatus bc_parse_while(BcParse *parse, BcVec *code) {

  BcStatus status;
  uint8_t flags;
  BcFunc *func;
  BcInstPtr ip;

  if ((status = bc_lex_next(&parse->lex))) return status;

  if (parse->lex.token.type != BC_LEX_LEFT_PAREN)
    return BC_STATUS_PARSE_BAD_TOKEN;

  if ((status = bc_lex_next(&parse->lex))) return status;

  func = bc_vec_item(&parse->prog->funcs, parse->func);

  assert(func);

  ip.idx = func->labels.len;

  if ((status = bc_vec_push(&func->labels, &code->len))) return status;
  if ((status = bc_vec_push(&parse->cond_labels, &ip.idx))) return status;

  ip.idx = func->labels.len;
  ip.func = 1;
  ip.len = 0;

  if ((status = bc_vec_push(&parse->exit_labels, &ip))) return status;
  if ((status = bc_vec_push(&func->labels, &ip.idx))) return status;

  status = bc_parse_expr(parse, code, BC_PARSE_EXPR_POSIX_REL);
  if (status) return status;

  if (parse->lex.token.type != BC_LEX_RIGHT_PAREN)
    return BC_STATUS_PARSE_BAD_TOKEN;

  if ((status = bc_lex_next(&parse->lex))) return status;
  if ((status = bc_vec_pushByte(code, BC_INST_JUMP_ZERO))) return status;
  if ((status = bc_parse_pushIndex(code, ip.idx))) return status;

  flags = BC_PARSE_FLAG_LOOP | BC_PARSE_FLAG_LOOP_INNER;

  return bc_parse_startBody(parse, flags);
}

BcStatus bc_parse_for(BcParse *parse, BcVec *code) {

  BcStatus status;
  BcFunc *func;
  BcInstPtr ip;
  size_t cond_idx, exit_idx, body_idx, update_idx;
  uint8_t flags;

  if ((status = bc_lex_next(&parse->lex))) return status;

  if (parse->lex.token.type != BC_LEX_LEFT_PAREN)
    return BC_STATUS_PARSE_BAD_TOKEN;

  if ((status = bc_lex_next(&parse->lex))) return status;

  if (parse->lex.token.type != BC_LEX_SEMICOLON)
    status = bc_parse_expr(parse, code, 0);
  else
    status = bc_posix_error(BC_STATUS_POSIX_MISSING_FOR_INIT,
                            parse->lex.file, parse->lex.line, NULL);

  if (status) return status;

  if (parse->lex.token.type != BC_LEX_SEMICOLON)
    return BC_STATUS_PARSE_BAD_TOKEN;

  if ((status = bc_lex_next(&parse->lex))) return status;

  func = bc_vec_item(&parse->prog->funcs, parse->func);

  assert(func);

  cond_idx = func->labels.len;
  update_idx = cond_idx + 1;
  body_idx = update_idx + 1;
  exit_idx = body_idx + 1;

  if ((status = bc_vec_push(&func->labels, &code->len))) return status;

  if (parse->lex.token.type != BC_LEX_SEMICOLON)
    status = bc_parse_expr(parse, code, BC_PARSE_EXPR_POSIX_REL);
  else status = bc_posix_error(BC_STATUS_POSIX_MISSING_FOR_COND,
                               parse->lex.file, parse->lex.line, NULL);

  if (status) return status;

  if (parse->lex.token.type != BC_LEX_SEMICOLON)
    return BC_STATUS_PARSE_BAD_TOKEN;

  if ((status = bc_lex_next(&parse->lex))) return status;
  if ((status = bc_vec_pushByte(code, BC_INST_JUMP_ZERO))) return status;
  if ((status = bc_parse_pushIndex(code, exit_idx))) return status;
  if ((status = bc_vec_pushByte(code, BC_INST_JUMP))) return status;
  if ((status = bc_parse_pushIndex(code, body_idx))) return status;

  ip.idx = func->labels.len;

  if ((status = bc_vec_push(&parse->cond_labels, &update_idx))) return status;
  if ((status = bc_vec_push(&func->labels, &code->len))) return status;

  if (parse->lex.token.type != BC_LEX_RIGHT_PAREN)
    status = bc_parse_expr(parse, code, 0);
  else
    status = bc_posix_error(BC_STATUS_POSIX_MISSING_FOR_UPDATE,
                            parse->lex.file, parse->lex.line, NULL);

  if (status) return status;

  if (parse->lex.token.type != BC_LEX_RIGHT_PAREN) {
    status = bc_parse_expr(parse, code, BC_PARSE_EXPR_POSIX_REL);
    if (status) return status;
  }

  if (parse->lex.token.type != BC_LEX_RIGHT_PAREN)
    return BC_STATUS_PARSE_BAD_TOKEN;

  if ((status = bc_vec_pushByte(code, BC_INST_JUMP))) return status;
  if ((status = bc_parse_pushIndex(code, cond_idx))) return status;
  if ((status = bc_vec_push(&func->labels, &code->len))) return status;

  ip.idx = exit_idx;
  ip.func = 1;
  ip.len = 0;

  if ((status = bc_vec_push(&parse->exit_labels, &ip))) return status;
  if ((status = bc_vec_push(&func->labels, &ip.idx))) return status;
  if ((status = bc_lex_next(&parse->lex))) return status;

  flags = BC_PARSE_FLAG_LOOP | BC_PARSE_FLAG_LOOP_INNER;

  return bc_parse_startBody(parse, flags);
}

BcStatus bc_parse_loopExit(BcParse *parse, BcVec *code, BcLexTokenType type) {

  BcStatus status;
  size_t idx;

  if (!BC_PARSE_LOOP(parse)) return BC_STATUS_PARSE_BAD_TOKEN;

  if (type == BC_LEX_KEY_BREAK) {

    size_t top;
    BcInstPtr *ip;

    if (!parse->exit_labels.len) return BC_STATUS_PARSE_BAD_TOKEN;

    top = parse->exit_labels.len - 1;
    ip = bc_vec_item(&parse->exit_labels, top);

    while (top < parse->exit_labels.len && ip && !ip->func) {
      ip = bc_vec_item(&parse->exit_labels, top);
      --top;
    }

    if (top >= parse->exit_labels.len || !ip) return BC_STATUS_PARSE_BAD_TOKEN;

    idx = ip->idx;
  }
  else {
    size_t *ptr = bc_vec_top(&parse->cond_labels);
    assert(ptr);
    idx = *ptr;
  }

  if ((status = bc_vec_pushByte(code, BC_INST_JUMP))) return status;
  if ((status = bc_parse_pushIndex(code, idx))) return status;
  if ((status = bc_lex_next(&parse->lex))) return status;

  if (parse->lex.token.type != BC_LEX_SEMICOLON &&
      parse->lex.token.type != BC_LEX_NEWLINE)
  {
    return BC_STATUS_PARSE_BAD_TOKEN;
  }

  return bc_lex_next(&parse->lex);
}

BcStatus bc_parse_func(BcParse *parse) {

  BcLexTokenType type;
  BcStatus status;
  BcFunc *fptr;
  bool comma, var;
  uint8_t flags;
  char *name;

  if ((status = bc_lex_next(&parse->lex))) return status;

  name = parse->lex.token.string;

  if (parse->lex.token.type != BC_LEX_NAME) {
    status = BC_STATUS_PARSE_BAD_FUNC;
    goto err;
  }

  assert(parse->prog->funcs.len == parse->prog->func_map.vec.len);

  status = bc_program_addFunc(parse->prog, name, &parse->func);
  if (status) goto err;

  assert(parse->func);

  fptr = bc_vec_item(&parse->prog->funcs, parse->func);

  assert(fptr);

  if ((status = bc_lex_next(&parse->lex))) return status;

  if (parse->lex.token.type != BC_LEX_LEFT_PAREN) return BC_STATUS_PARSE_BAD_FUNC;

  if ((status = bc_lex_next(&parse->lex))) return status;

  comma = false;

  type = parse->lex.token.type;
  name = parse->lex.token.string;

  while (!status && type != BC_LEX_RIGHT_PAREN) {

    if (type != BC_LEX_NAME) {
      status = BC_STATUS_PARSE_BAD_FUNC;
      goto err;
    }

    ++fptr->nparams;

    if ((status = bc_lex_next(&parse->lex))) goto err;

    if (parse->lex.token.type == BC_LEX_LEFT_BRACKET) {

      if ((status = bc_lex_next(&parse->lex))) goto err;

      if (parse->lex.token.type != BC_LEX_RIGHT_BRACKET)
        return BC_STATUS_PARSE_BAD_FUNC;

      if ((status = bc_lex_next(&parse->lex))) goto err;

      var = false;
    }
    else var = true;

    if (parse->lex.token.type == BC_LEX_COMMA) {
      comma = true;
      if ((status = bc_lex_next(&parse->lex))) goto err;
    }
    else comma = false;

    if ((status = bc_func_insert(fptr, name, var))) goto err;

    type = parse->lex.token.type;
    name = parse->lex.token.string;
  }

  if (comma) return BC_STATUS_PARSE_BAD_FUNC;

  flags = BC_PARSE_FLAG_FUNC | BC_PARSE_FLAG_FUNC_INNER | BC_PARSE_FLAG_BODY;

  if ((status = bc_parse_startBody(parse, flags))) return status;
  if ((status = bc_lex_next(&parse->lex))) return status;

  if (parse->lex.token.type != BC_LEX_LEFT_BRACE)
    return bc_posix_error(BC_STATUS_POSIX_FUNC_HEADER_LEFT_BRACE,
                          parse->lex.file, parse->lex.line, NULL);

  return status;

err:

  free(name);

  return status;
}

BcStatus bc_parse_auto(BcParse *parse) {

  BcLexTokenType type;
  BcStatus status;
  bool comma, var, one;
  char *name;
  BcFunc *func;

  if (!parse->auto_part) return BC_STATUS_PARSE_BAD_TOKEN;

  if ((status = bc_lex_next(&parse->lex))) return status;

  parse->auto_part = comma = one = false;
  type = parse->lex.token.type;

  func = bc_vec_item(&parse->prog->funcs, parse->func);

  assert(func);

  while (!status && type == BC_LEX_NAME) {

    name = parse->lex.token.string;

    if ((status = bc_lex_next(&parse->lex))) return status;

    one = true;

    if (parse->lex.token.type == BC_LEX_LEFT_BRACKET) {

      if ((status = bc_lex_next(&parse->lex))) goto err;

      if (parse->lex.token.type != BC_LEX_RIGHT_BRACKET)
        return BC_STATUS_PARSE_BAD_FUNC;

      if ((status = bc_lex_next(&parse->lex))) goto err;

      var = false;
    }
    else var = true;

    if (parse->lex.token.type == BC_LEX_COMMA) {
      comma = true;
      if ((status = bc_lex_next(&parse->lex))) goto err;
    }
    else comma = false;

    if ((status = bc_func_insert(func, name, var))) goto err;

    type = parse->lex.token.type;
  }

  if (status) return status;

  if (comma) return BC_STATUS_PARSE_BAD_FUNC;

  if (!one) return BC_STATUS_PARSE_NO_AUTO;

  if (type != BC_LEX_NEWLINE && type != BC_LEX_SEMICOLON)
    return BC_STATUS_PARSE_BAD_TOKEN;

  return bc_lex_next(&parse->lex);

err:

  free(name);

  return status;
}

BcStatus bc_parse_body(BcParse *parse, BcVec *code, bool brace) {

  BcStatus status;
  uint8_t *flag_ptr, flags;

  assert(parse->flags.len >= 2);

  flag_ptr = bc_vec_top(&parse->flags);
  assert(flag_ptr);
  *flag_ptr &= ~(BC_PARSE_FLAG_BODY);
  flags = *flag_ptr;

  if (flags & BC_PARSE_FLAG_FUNC_INNER) {
    if (!brace) return BC_STATUS_PARSE_BAD_TOKEN;
    parse->auto_part = true;
    status = bc_lex_next(&parse->lex);
  }
  else {
    assert(flags);
    if ((status = bc_parse_stmt(parse, code))) return status;
    if (!brace) status = bc_parse_endBody(parse, code, false);
  }

  return status;
}

BcStatus bc_parse_semicolonList(BcParse *parse, BcVec *code) {

  BcStatus status = BC_STATUS_SUCCESS;

  switch (parse->lex.token.type) {

    case BC_LEX_OP_INC:
    case BC_LEX_OP_DEC:
    case BC_LEX_OP_MINUS:
    case BC_LEX_OP_BOOL_NOT:
    case BC_LEX_LEFT_PAREN:
    case BC_LEX_STRING:
    case BC_LEX_NAME:
    case BC_LEX_NUMBER:
    case BC_LEX_KEY_BREAK:
    case BC_LEX_KEY_CONTINUE:
    case BC_LEX_KEY_FOR:
    case BC_LEX_KEY_HALT:
    case BC_LEX_KEY_IBASE:
    case BC_LEX_KEY_IF:
    case BC_LEX_KEY_LAST:
    case BC_LEX_KEY_LENGTH:
    case BC_LEX_KEY_LIMITS:
    case BC_LEX_KEY_OBASE:
    case BC_LEX_KEY_PRINT:
    case BC_LEX_KEY_QUIT:
    case BC_LEX_KEY_READ:
    case BC_LEX_KEY_RETURN:
    case BC_LEX_KEY_SCALE:
    case BC_LEX_KEY_SQRT:
    case BC_LEX_KEY_WHILE:
    {
      status = bc_parse_stmt(parse, code);
      break;
    }

    case BC_LEX_NEWLINE:
    {
      status = bc_lex_next(&parse->lex);
      break;
    }

    case BC_LEX_SEMICOLON:
    {
      status = bc_parse_semicolonListEnd(parse, code);
      break;
    }

    case BC_LEX_EOF:
    {
      if (parse->flags.len > 0) status = BC_STATUS_LEX_BAD_CHARACTER;
      break;
    }

    default:
    {
      status = BC_STATUS_PARSE_BAD_TOKEN;
      break;
    }
  }

  return status;
}

BcStatus bc_parse_semicolonListEnd(BcParse *parse, BcVec *code) {

  BcStatus status;

  if (parse->lex.token.type == BC_LEX_SEMICOLON) {
    if ((status = bc_lex_next(&parse->lex))) return status;
    status = bc_parse_semicolonList(parse, code);
  }
  else if (parse->lex.token.type == BC_LEX_NEWLINE)
    status = bc_lex_next(&parse->lex);
  else if (parse->lex.token.type == BC_LEX_EOF)
    status = BC_STATUS_SUCCESS;
  else
    status = BC_STATUS_PARSE_BAD_TOKEN;

  return status;
}

BcStatus bc_parse_stmt(BcParse *parse, BcVec *code) {

  BcStatus status;

  switch (parse->lex.token.type) {

    case BC_LEX_NEWLINE:
    {
      status = bc_lex_next(&parse->lex);
      return status;
    }

    case BC_LEX_KEY_ELSE:
    {
      parse->auto_part = false;
      break;
    }

    case BC_LEX_LEFT_BRACE:
    {
      if (!BC_PARSE_BODY(parse)) return BC_STATUS_PARSE_BAD_TOKEN;

      ++parse->num_braces;

      if ((status = bc_lex_next(&parse->lex))) return status;

      return bc_parse_body(parse, code, true);
    }

    case BC_LEX_KEY_AUTO:
    {
      return bc_parse_auto(parse);
    }

    default:
    {
      parse->auto_part = false;

      if (BC_PARSE_IF_END(parse)) {
        bc_parse_noElse(parse, code);
        return BC_STATUS_SUCCESS;
      }
      else if (BC_PARSE_BODY(parse)) return bc_parse_body(parse, code, false);

      break;
    }
  }

  switch (parse->lex.token.type) {

    case BC_LEX_OP_INC:
    case BC_LEX_OP_DEC:
    case BC_LEX_OP_MINUS:
    case BC_LEX_OP_BOOL_NOT:
    case BC_LEX_LEFT_PAREN:
    case BC_LEX_NAME:
    case BC_LEX_NUMBER:
    case BC_LEX_KEY_IBASE:
    case BC_LEX_KEY_LAST:
    case BC_LEX_KEY_LENGTH:
    case BC_LEX_KEY_OBASE:
    case BC_LEX_KEY_READ:
    case BC_LEX_KEY_SCALE:
    case BC_LEX_KEY_SQRT:
    {
      if ((status = bc_parse_expr(parse, code, BC_PARSE_EXPR_PRINT))) break;
      status = bc_parse_semicolonListEnd(parse, code);
      break;
    }

    case BC_LEX_KEY_ELSE:
    {
      status = bc_parse_else(parse, code);
      break;
    }

    case BC_LEX_RIGHT_BRACE:
    {
      status = bc_parse_endBody(parse, code, true);
      break;
    }

    case BC_LEX_STRING:
    {
      status = bc_parse_string(parse, code);
      break;
    }

    case BC_LEX_KEY_BREAK:
    case BC_LEX_KEY_CONTINUE:
    {
      status = bc_parse_loopExit(parse, code, parse->lex.token.type);
      break;
    }

    case BC_LEX_KEY_FOR:
    {
      status = bc_parse_for(parse, code);
      break;
    }

    case BC_LEX_KEY_HALT:
    {
      if ((status = bc_vec_pushByte(code, BC_INST_HALT))) return status;
      if ((status = bc_lex_next(&parse->lex))) return status;

      status = bc_parse_semicolonListEnd(parse, code);

      break;
    }

    case BC_LEX_KEY_IF:
    {
      status = bc_parse_if(parse, code);
      break;
    }

    case BC_LEX_KEY_LIMITS:
    {
      status = bc_lex_next(&parse->lex);
      if (status && status != BC_STATUS_LEX_EOF) return status;

      status = bc_parse_semicolonListEnd(parse, code);
      if (status && status != BC_STATUS_LEX_EOF) return status;

      status = BC_STATUS_LIMITS;

      break;
    }

    case BC_LEX_KEY_PRINT:
    {
      status = bc_parse_print(parse, code);
      break;
    }

    case BC_LEX_KEY_QUIT:
    {
      // Quit is a compile-time command,
      // so we send an exit command. We
      // don't exit directly, so the vm
      // can clean up.
      status = BC_STATUS_QUIT;
      break;
    }

    case BC_LEX_KEY_RETURN:
    {
      if ((status = bc_parse_return(parse, code))) return status;
      status = bc_parse_semicolonListEnd(parse, code);
      break;
    }

    case BC_LEX_KEY_WHILE:
    {
      status = bc_parse_while(parse, code);
      break;
    }

    default:
    {
      status = BC_STATUS_PARSE_BAD_TOKEN;
      break;
    }
  }

  return status;
}

BcStatus bc_parse_init(BcParse *parse, BcProgram *program) {

  BcStatus status;

  assert(parse && program);

  status = bc_vec_init(&parse->flags, sizeof(uint8_t), NULL);
  if (status) return status;

  status = bc_vec_init(&parse->exit_labels, sizeof(BcInstPtr), NULL);
  if (status) goto exit_label_err;

  status = bc_vec_init(&parse->cond_labels, sizeof(size_t), NULL);
  if (status) goto cond_label_err;

  uint8_t flags = 0;

  if ((status = bc_vec_push(&parse->flags, &flags))) goto push_err;

  status = bc_vec_init(&parse->ops, sizeof(BcLexTokenType), NULL);
  if (status) goto push_err;

  parse->prog = program;
  parse->func = 0;
  parse->num_braces = 0;
  parse->auto_part = false;

  return status;

push_err:

  bc_vec_free(&parse->cond_labels);

cond_label_err:

  bc_vec_free(&parse->exit_labels);

exit_label_err:

  bc_vec_free(&parse->flags);

  return status;
}

BcStatus bc_parse_parse(BcParse *parse) {

  BcStatus status;

  assert(parse);

  switch (parse->lex.token.type) {

    case BC_LEX_NEWLINE:
    {
      status = bc_lex_next(&parse->lex);
      break;
    }

    case BC_LEX_KEY_DEFINE:
    {
      if (!BC_PARSE_CAN_EXEC(parse)) return BC_STATUS_PARSE_BAD_TOKEN;
      status = bc_parse_func(parse);
      break;
    }

    case BC_LEX_EOF:
    {
      status = BC_STATUS_LEX_EOF;
      break;
    }

    default:
    {
      BcFunc *func = bc_vec_item(&parse->prog->funcs, parse->func);
      assert(func);
      status = bc_parse_stmt(parse, &func->code);
      break;
    }
  }

  return status;
}

void bc_parse_free(BcParse *parse) {

  if (!parse) return;

  bc_vec_free(&parse->flags);
  bc_vec_free(&parse->exit_labels);
  bc_vec_free(&parse->cond_labels);
  bc_vec_free(&parse->ops);

  if ((parse->lex.token.type == BC_LEX_STRING || parse->lex.token.type == BC_LEX_NAME ||
      parse->lex.token.type == BC_LEX_NUMBER) && parse->lex.token.string)
  {
    free(parse->lex.token.string);
  }
}

BcStatus bc_parse_expr(BcParse *parse, BcVec *code, uint8_t flags) {

  BcStatus status;
  uint32_t nexprs, nparens, ops_start, nrelops;
  bool paren_first, paren_expr, rparen, done, get_token, assign;
  BcInst prev;
  BcLexTokenType type, top, *ptr;

  status = BC_STATUS_SUCCESS;
  prev = BC_INST_PRINT;

  ops_start = parse->ops.len;
  paren_first = parse->lex.token.type == BC_LEX_LEFT_PAREN;
  nexprs = nparens = nrelops = 0;
  paren_expr = rparen = done = get_token = assign = false;

  type = parse->lex.token.type;

  while (!status && !done && bc_parse_token_exprs[type]) {

    switch (type) {

      case BC_LEX_OP_INC:
      case BC_LEX_OP_DEC:
      {
        status = bc_parse_incdec(parse, code, &prev, &nexprs, flags);
        rparen = get_token = false;
        break;
      }

      case BC_LEX_OP_MINUS:
      {
        status = bc_parse_minus(parse, code, &parse->ops, &prev,
                                rparen, &nexprs);
        rparen = get_token = false;
        break;
      }

      case BC_LEX_OP_ASSIGN_POWER:
      case BC_LEX_OP_ASSIGN_MULTIPLY:
      case BC_LEX_OP_ASSIGN_DIVIDE:
      case BC_LEX_OP_ASSIGN_MODULUS:
      case BC_LEX_OP_ASSIGN_PLUS:
      case BC_LEX_OP_ASSIGN_MINUS:
      case BC_LEX_OP_ASSIGN:
        if (prev != BC_INST_PUSH_VAR && prev != BC_INST_PUSH_ARRAY_ELEM &&
            prev != BC_INST_PUSH_SCALE && prev != BC_INST_PUSH_IBASE &&
            prev != BC_INST_PUSH_OBASE && prev != BC_INST_PUSH_LAST)
        {
          status = BC_STATUS_PARSE_BAD_ASSIGN;
          break;
        }
        // Fallthrough.
      case BC_LEX_OP_POWER:
      case BC_LEX_OP_MULTIPLY:
      case BC_LEX_OP_DIVIDE:
      case BC_LEX_OP_MODULUS:
      case BC_LEX_OP_PLUS:
      case BC_LEX_OP_REL_EQUAL:
      case BC_LEX_OP_REL_LESS_EQ:
      case BC_LEX_OP_REL_GREATER_EQ:
      case BC_LEX_OP_REL_NOT_EQ:
      case BC_LEX_OP_REL_LESS:
      case BC_LEX_OP_REL_GREATER:
      case BC_LEX_OP_BOOL_NOT:
      case BC_LEX_OP_BOOL_OR:
      case BC_LEX_OP_BOOL_AND:
      {
        if (type >= BC_LEX_OP_REL_EQUAL && type <= BC_LEX_OP_REL_GREATER)
          nrelops += 1;

        prev = BC_PARSE_TOKEN_TO_INST(type);
        status = bc_parse_operator(parse, code, &parse->ops,
                                   type, &nexprs, true);
        rparen = get_token = false;

        break;
      }

      case BC_LEX_LEFT_PAREN:
      {
        ++nparens;
        paren_expr = rparen = false;
        get_token = true;
        status = bc_vec_push(&parse->ops, &type);
        break;
      }

      case BC_LEX_RIGHT_PAREN:
      {
        if (nparens == 0) {
          status = BC_STATUS_SUCCESS;
          done = true;
          get_token = false;
          break;
        }
        else if (!paren_expr) {
          status = BC_STATUS_PARSE_BAD_EXPR;
          goto err;
        }

        --nparens;
        paren_expr = rparen = true;
        get_token = false;

        status = bc_parse_rightParen(parse, code, &parse->ops, &nexprs);

        break;
      }

      case BC_LEX_NAME:
      {
        paren_expr = true;
        rparen = get_token = false;
        status = bc_parse_expr_name(parse, code, &prev,
                                    flags & ~(BC_PARSE_EXPR_NO_CALL));
        ++nexprs;
        break;
      }

      case BC_LEX_NUMBER:
      {
        size_t idx = parse->prog->constants.len;

        status = bc_vec_push(&parse->prog->constants, parse->lex.token.string);
        if (status) goto err;

        if ((status = bc_vec_pushByte(code, BC_INST_PUSH_NUM))) return status;
        if ((status = bc_parse_pushIndex(code, idx))) return status;

        paren_expr = get_token = true;
        rparen = false;
        ++nexprs;
        prev = BC_INST_PUSH_NUM;

        break;
      }

      case BC_LEX_KEY_IBASE:
      case BC_LEX_KEY_LAST:
      case BC_LEX_KEY_OBASE:
      {
        uint8_t inst = type - BC_LEX_KEY_IBASE + BC_INST_PUSH_IBASE;
        status = bc_vec_pushByte(code, inst);

        paren_expr = get_token = true;
        rparen = false;
        ++nexprs;
        prev = BC_INST_PUSH_OBASE;

        break;
      }

      case BC_LEX_KEY_LENGTH:
      case BC_LEX_KEY_SQRT:
      {
        status = bc_parse_builtin(parse, code, type, flags);
        paren_expr = true;
        rparen = get_token = false;
        ++nexprs;
        prev = type == BC_LEX_KEY_LENGTH ? BC_INST_LENGTH : BC_INST_SQRT;
        break;
      }

      case BC_LEX_KEY_READ:
      {
        if (flags & BC_PARSE_EXPR_NO_READ)
          status = BC_STATUS_EXEC_RECURSIVE_READ;
        else status = bc_parse_read(parse, code);

        paren_expr = true;
        rparen = get_token = false;
        ++nexprs;
        prev = BC_INST_READ;

        break;
      }

      case BC_LEX_KEY_SCALE:
      {
        status = bc_parse_scale(parse, code, &prev, flags);
        paren_expr = true;
        rparen = get_token = false;
        ++nexprs;
        prev = BC_INST_PUSH_SCALE;
        break;
      }

      default:
      {
        status = BC_STATUS_PARSE_BAD_TOKEN;
        break;
      }
    }

    if (status && status != BC_STATUS_LEX_EOF) goto err;

    if (get_token) status = bc_lex_next(&parse->lex);

    type = parse->lex.token.type;
  }

  if (status && status != BC_STATUS_LEX_EOF) goto err;

  status = BC_STATUS_SUCCESS;

  while (!status && parse->ops.len > ops_start) {

    ptr = bc_vec_top(&parse->ops);
    assert(ptr);
    top = *ptr;

    assign = top >= BC_LEX_OP_ASSIGN_POWER && top <= BC_LEX_OP_ASSIGN;

    if (top == BC_LEX_LEFT_PAREN || top == BC_LEX_RIGHT_PAREN) {
      status = BC_STATUS_PARSE_BAD_EXPR;
      goto err;
    }

    status = bc_vec_pushByte(code, BC_PARSE_TOKEN_TO_INST(top));
    if (status) goto err;

    nexprs -= top != BC_LEX_OP_BOOL_NOT && top != BC_LEX_OP_NEG ? 1 : 0;

    bc_vec_pop(&parse->ops);
  }

  if (nexprs != 1) {
    status = BC_STATUS_PARSE_BAD_EXPR;
    goto err;
  }

  if (!(flags & BC_PARSE_EXPR_POSIX_REL) && nrelops &&
      (status = bc_posix_error(BC_STATUS_POSIX_REL_OUTSIDE,
                               parse->lex.file, parse->lex.line, NULL)))
  {
    goto err;
  }
  else if ((flags & BC_PARSE_EXPR_POSIX_REL) && nrelops != 1 &&
           (status = bc_posix_error(BC_STATUS_POSIX_MULTIPLE_REL,
                                    parse->lex.file, parse->lex.line, NULL)))
  {
    goto err;
  }

  if (flags & BC_PARSE_EXPR_PRINT) {
    if (paren_first || !assign) status = bc_vec_pushByte(code, BC_INST_PRINT);
    else status = bc_vec_pushByte(code, BC_INST_POP);
  }

  return type == BC_LEX_EOF ? BC_STATUS_LEX_EOF : status;

err:

  if (parse->lex.token.string) {
    free(parse->lex.token.string);
    parse->lex.token.string = NULL;
  }

  return status;
}
