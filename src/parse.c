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

BcStatus bc_parse_else(BcParse *p, BcVec *code);
BcStatus bc_parse_stmt(BcParse *p, BcVec *code);

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
    idx = (idx & ((unsigned long) ~(UINT8_MAX))) >> sizeof(uint8_t) * CHAR_BIT;
  }

  if ((status = bc_vec_pushByte(code, amt))) return status;
  for (i = 0; !status && i < amt; ++i) status = bc_vec_pushByte(code, nums[i]);

  return status;
}

BcStatus bc_parse_operator(BcParse *p, BcVec *code, BcVec *ops, BcLexToken t,
                           uint32_t *num_exprs, bool next)
{
  BcStatus status;
  BcLexToken top;
  uint8_t lp, rp;
  bool rleft;

  rp = bc_parse_ops[t].prec;
  rleft = bc_parse_ops[t].left;

  while (ops->len &&
         (top = *((BcLexToken*) bc_vec_top(ops))) != BC_LEX_LEFT_PAREN &&
         ((lp = bc_parse_ops[top].prec) < rp || (lp == rp && rleft)))
  {
    status = bc_vec_pushByte(code, (uint8_t) BC_PARSE_TOKEN_TO_INST(top));
    if (status) return status;

    bc_vec_pop(ops);

    *num_exprs -= top != BC_LEX_OP_BOOL_NOT && top != BC_LEX_OP_NEG;
  }

  if ((status = bc_vec_push(ops, &t))) return status;
  if (next && (status = bc_lex_next(&p->lex)) && p->lex.token.string) {
    free(p->lex.token.string);
    p->lex.token.string = NULL;
  }

  return status;
}

BcStatus bc_parse_rightParen(BcParse *p, BcVec *code,
                             BcVec *ops, uint32_t *nexs)
{
  BcStatus status;
  BcLexToken top;

  if (!ops->len) return BC_STATUS_PARSE_BAD_EXPR;

  while ((top = *((BcLexToken*) bc_vec_top(ops))) != BC_LEX_LEFT_PAREN) {

    status = bc_vec_pushByte(code, (uint8_t) BC_PARSE_TOKEN_TO_INST(top));
    if (status) return status;

    bc_vec_pop(ops);
    *nexs -= top != BC_LEX_OP_BOOL_NOT && top != BC_LEX_OP_NEG;

    if (!ops->len) return BC_STATUS_PARSE_BAD_EXPR;
  }

  bc_vec_pop(ops);

  return bc_lex_next(&p->lex);
}

BcStatus bc_parse_params(BcParse *p, BcVec *code, uint8_t flags) {

  BcStatus status;
  bool comma = false;
  size_t nparams;

  if ((status = bc_lex_next(&p->lex))) return status;

  for (nparams = 0; p->lex.token.type != BC_LEX_RIGHT_PAREN; ++nparams) {

    status = bc_parse_expr(p, code, flags & ~BC_PARSE_PRINT);
    if (status) return status;

    if (p->lex.token.type == BC_LEX_COMMA) {
      comma = true;
      if ((status = bc_lex_next(&p->lex))) return status;
    }
    else comma = false;
  }

  if (comma) return BC_STATUS_PARSE_BAD_TOKEN;
  if ((status = bc_vec_pushByte(code, BC_INST_CALL))) return status;

  return bc_parse_pushIndex(code, nparams);
}

BcStatus bc_parse_call(BcParse *p, BcVec *code, char *name, uint8_t flags) {

  BcStatus status;
  BcEntry entry, *entry_ptr;
  size_t idx;

  entry.name = name;

  if ((status = bc_parse_params(p, code, flags))) goto err;

  if (p->lex.token.type != BC_LEX_RIGHT_PAREN) {
    status = BC_STATUS_PARSE_BAD_TOKEN;
    goto err;
  }

  idx = bc_veco_index(&p->prog->func_map, &entry);

  if (idx == BC_INVALID_IDX) {
    if ((status = bc_program_addFunc(p->prog, name, &idx))) return status;
    name = NULL;
    idx = bc_veco_index(&p->prog->func_map, &entry);
    assert(idx != BC_INVALID_IDX);
  }
  else free(name);

  entry_ptr = bc_veco_item(&p->prog->func_map, idx);
  assert(entry_ptr);
  if ((status = bc_parse_pushIndex(code, entry_ptr->idx))) return status;

  return bc_lex_next(&p->lex);

err:
  if (name) free(name);
  return status;
}

BcStatus bc_parse_name(BcParse *p, BcVec *code, BcInst *type, uint8_t flags)
{
  BcStatus status;
  char *name = p->lex.token.string;

  if ((status = bc_lex_next(&p->lex))) goto err;

  if (p->lex.token.type == BC_LEX_LEFT_BRACKET) {

    if ((status = bc_lex_next(&p->lex))) goto err;

    if (p->lex.token.type == BC_LEX_RIGHT_BRACKET) *type = BC_INST_PUSH_ARRAY;
    else {

      *type = BC_INST_PUSH_ARRAY_ELEM;

      if ((status = bc_parse_expr(p, code, flags & ~BC_PARSE_PRINT))) goto err;

      if (p->lex.token.type != BC_LEX_RIGHT_BRACKET) {
        status = BC_STATUS_PARSE_BAD_TOKEN;
        goto err;
      }
    }

    if ((status = bc_lex_next(&p->lex))) goto err;
    if ((status = bc_vec_pushByte(code, (uint8_t) *type))) goto err;
    status = bc_parse_pushName(code, name);
  }
  else if (p->lex.token.type == BC_LEX_LEFT_PAREN) {

    if (flags & BC_PARSE_NOCALL) {
      status = BC_STATUS_PARSE_BAD_TOKEN;
      goto err;
    }

    *type = BC_INST_CALL;
    status = bc_parse_call(p, code, name, flags);
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

BcStatus bc_parse_read(BcParse *p, BcVec *code) {

  BcStatus status;

  if ((status = bc_lex_next(&p->lex))) return status;
  if (p->lex.token.type != BC_LEX_LEFT_PAREN) return BC_STATUS_PARSE_BAD_TOKEN;

  if ((status = bc_lex_next(&p->lex))) return status;
  if (p->lex.token.type != BC_LEX_RIGHT_PAREN) return BC_STATUS_PARSE_BAD_TOKEN;

  if ((status = bc_vec_pushByte(code, BC_INST_READ))) return status;

  return bc_lex_next(&p->lex);
}

BcStatus bc_parse_builtin(BcParse *p, BcVec *code, BcLexToken type,
                          uint8_t flags, BcInst *prev)
{
  BcStatus status;

  if ((status = bc_lex_next(&p->lex))) return status;
  if (p->lex.token.type != BC_LEX_LEFT_PAREN) return BC_STATUS_PARSE_BAD_TOKEN;

  if ((status = bc_lex_next(&p->lex))) return status;
  if ((status = bc_parse_expr(p, code, flags & ~BC_PARSE_PRINT))) return status;
  if (p->lex.token.type != BC_LEX_RIGHT_PAREN) return BC_STATUS_PARSE_BAD_TOKEN;

  *prev = (type == BC_LEX_KEY_LENGTH) ? BC_INST_LENGTH : BC_INST_SQRT;
  if ((status = bc_vec_pushByte(code, (uint8_t) *prev))) return status;

  return bc_lex_next(&p->lex);
}

BcStatus bc_parse_scale(BcParse *p, BcVec *code, BcInst *type, uint8_t flags) {

  BcStatus status;

  if ((status = bc_lex_next(&p->lex))) return status;

  if (p->lex.token.type != BC_LEX_LEFT_PAREN) {
    *type = BC_INST_PUSH_SCALE;
    return bc_vec_pushByte(code, BC_INST_PUSH_SCALE);
  }

  *type = BC_INST_SCALE_FUNC;

  if ((status = bc_lex_next(&p->lex))) return status;
  if ((status = bc_parse_expr(p, code, flags & ~BC_PARSE_PRINT))) return status;
  if (p->lex.token.type != BC_LEX_RIGHT_PAREN) return BC_STATUS_PARSE_BAD_TOKEN;
  if ((status = bc_vec_pushByte(code, BC_INST_SCALE_FUNC))) return status;

  return bc_lex_next(&p->lex);
}

BcStatus bc_parse_incdec(BcParse *p, BcVec *code, BcInst *prev,
                         uint32_t *nexprs, uint8_t flags)
{
  BcStatus status;
  BcLexToken type;
  BcInst etype;
  uint8_t inst;

  etype = *prev;

  if (etype == BC_INST_PUSH_VAR || etype == BC_INST_PUSH_ARRAY_ELEM ||
      etype == BC_INST_PUSH_SCALE || etype == BC_INST_PUSH_LAST ||
      etype == BC_INST_PUSH_IBASE || etype == BC_INST_PUSH_OBASE)
  {
    *prev = inst = BC_INST_INC_POST + (p->lex.token.type != BC_LEX_OP_INC);
    if ((status = bc_vec_pushByte(code, inst))) return status;
    status = bc_lex_next(&p->lex);
  }
  else {

    *prev = inst = BC_INST_INC_PRE + (p->lex.token.type != BC_LEX_OP_INC);

    if ((status = bc_lex_next(&p->lex))) return status;
    type = p->lex.token.type;

    // Because we parse the next part of the expression
    // right here, we need to increment this.
    *nexprs = *nexprs + 1;

    switch (type) {

      case BC_LEX_NAME:
      {
        status = bc_parse_name(p, code, prev, flags | BC_PARSE_NOCALL);
        break;
      }

      case BC_LEX_KEY_IBASE:
      {
        if ((status = bc_vec_pushByte(code, BC_INST_PUSH_IBASE))) return status;
        status = bc_lex_next(&p->lex);
        break;
      }

      case BC_LEX_KEY_LAST:
      {
        if ((status = bc_vec_pushByte(code, BC_INST_PUSH_LAST))) return status;
        status = bc_lex_next(&p->lex);
        break;
      }

      case BC_LEX_KEY_OBASE:
      {
        if ((status = bc_vec_pushByte(code, BC_INST_PUSH_OBASE))) return status;
        status = bc_lex_next(&p->lex);
        break;
      }

      case BC_LEX_KEY_SCALE:
      {
        if ((status = bc_lex_next(&p->lex))) return status;
        if (p->lex.token.type == BC_LEX_LEFT_PAREN)
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

BcStatus bc_parse_minus(BcParse *p, BcVec *exs, BcVec *ops, BcInst *prev,
                        bool rparen, uint32_t *nexprs)
{
  BcStatus status;
  BcLexToken type;
  BcInst etype;

  if ((status = bc_lex_next(&p->lex))) return status;

  etype = *prev;
  type = p->lex.token.type;

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
    status = bc_parse_operator(p, exs, ops, type, nexprs, false);
  else
    // We can just push onto the op stack because this is the largest
    // precedence operator that gets pushed. Inc/dec does not.
    status = bc_vec_push(ops, &type);

  return status;
}

BcStatus bc_parse_string(BcParse *p, BcVec *code) {

  BcStatus status;
  size_t len;

  if (strlen(p->lex.token.string) > (unsigned long) BC_MAX_STRING) {
    status = BC_STATUS_EXEC_STRING_LEN;
    goto err;
  }

  len = p->prog->strings.len;

  if ((status = bc_vec_push(&p->prog->strings, &p->lex.token.string))) goto err;
  if ((status = bc_vec_pushByte(code, BC_INST_STR))) return status;
  if ((status = bc_parse_pushIndex(code, len))) return status;

  return bc_lex_next(&p->lex);

err:
  free(p->lex.token.string);
  return status;
}

BcStatus bc_parse_print(BcParse *p, BcVec *code) {

  BcStatus status;
  BcLexToken type;
  bool comma;

  if ((status = bc_lex_next(&p->lex))) return status;

  type = p->lex.token.type;

  if (type == BC_LEX_SEMICOLON || type == BC_LEX_NEWLINE)
    return BC_STATUS_PARSE_BAD_PRINT;

  comma = false;

  while (!status && type != BC_LEX_SEMICOLON && type != BC_LEX_NEWLINE) {

    if (type == BC_LEX_STRING) {

      size_t len = p->prog->strings.len;

      status = bc_vec_push(&p->prog->strings, &p->lex.token.string);
      if (status) {
        free(p->lex.token.string);
        return status;
      }

      if ((status = bc_vec_pushByte(code, BC_INST_PRINT_STR))) return status;
      status = bc_parse_pushIndex(code, len);
    }
    else {
      if ((status = bc_parse_expr(p, code, 0))) return status;
      status = bc_vec_pushByte(code, BC_INST_PRINT_EXPR);
    }

    if (status) return status;
    if ((status = bc_lex_next(&p->lex))) return status;

    if (p->lex.token.type == BC_LEX_COMMA) {
      comma = true;
      status = bc_lex_next(&p->lex);
    }
    else comma = false;

    type = p->lex.token.type;
  }

  if (status) return status;
  if (comma) return BC_STATUS_PARSE_BAD_TOKEN;

  return bc_lex_next(&p->lex);
}

BcStatus bc_parse_return(BcParse *p, BcVec *code) {

  BcStatus status;

  if (!BC_PARSE_FUNC(p)) return BC_STATUS_PARSE_BAD_TOKEN;

  if ((status = bc_lex_next(&p->lex))) return status;

  if (p->lex.token.type != BC_LEX_NEWLINE &&
      p->lex.token.type != BC_LEX_SEMICOLON &&
      p->lex.token.type != BC_LEX_LEFT_PAREN &&
      (status = bc_posix_error(BC_STATUS_POSIX_RETURN_PARENS,
                               p->lex.file, p->lex.line, NULL)))
  {
     return status;
  }

  if (p->lex.token.type == BC_LEX_NEWLINE ||
      p->lex.token.type == BC_LEX_SEMICOLON)
  {
    status = bc_vec_pushByte(code, BC_INST_RETURN_ZERO);
  }
  else {
    if ((status = bc_parse_expr(p, code, 0))) return status;
    status = bc_vec_pushByte(code, BC_INST_RETURN);
  }

  return status;
}

BcStatus bc_parse_endBody(BcParse *p, BcVec *code, bool brace) {

  BcStatus status = BC_STATUS_SUCCESS;

  if (p->flags.len <= 1 || p->num_braces == 0) return BC_STATUS_PARSE_BAD_TOKEN;

  if (brace) {

    if (p->lex.token.type == BC_LEX_RIGHT_BRACE) {

      if (!p->num_braces) return BC_STATUS_PARSE_BAD_TOKEN;

      --p->num_braces;

      if ((status = bc_lex_next(&p->lex))) return status;
    }
    else return BC_STATUS_PARSE_BAD_TOKEN;
  }

  if (BC_PARSE_IF(p)) {

    uint8_t *flag_ptr;

    while (p->lex.token.type == BC_LEX_NEWLINE) {
      if ((status = bc_lex_next(&p->lex))) return status;
    }

    bc_vec_pop(&p->flags);

    flag_ptr = BC_PARSE_TOP_FLAG_PTR(p);
    *flag_ptr = (*flag_ptr | BC_PARSE_FLAG_IF_END);

    if (p->lex.token.type == BC_LEX_KEY_ELSE) status = bc_parse_else(p, code);
  }
  else if (BC_PARSE_ELSE(p)) {

    BcInstPtr *ip;
    BcFunc *func;
    size_t *label;

    bc_vec_pop(&p->flags);

    ip = bc_vec_top(&p->exits);
    func = bc_vec_item(&p->prog->funcs, p->func);
    label = bc_vec_item(&func->labels, ip->idx);
    *label = code->len;

    bc_vec_pop(&p->exits);
  }
  else if (BC_PARSE_FUNC_INNER(p)) {
    p->func = 0;
    if ((status = bc_vec_pushByte(code, BC_INST_RETURN_ZERO))) return status;
    bc_vec_pop(&p->flags);
  }
  else {

    BcInstPtr *ip;
    BcFunc *func;
    size_t *label;

    if ((status = bc_vec_pushByte(code, BC_INST_JUMP))) return status;

    ip = bc_vec_top(&p->exits);
    label = bc_vec_top(&p->conds);

    if ((status = bc_parse_pushIndex(code, *label))) return status;

    func = bc_vec_item(&p->prog->funcs, p->func);
    label = bc_vec_item(&func->labels, ip->idx);
    *label = code->len;

    bc_vec_pop(&p->flags);
    bc_vec_pop(&p->exits);
    bc_vec_pop(&p->conds);
  }

  return status;
}

BcStatus bc_parse_startBody(BcParse *p, uint8_t flags) {
  uint8_t *flag_ptr = BC_PARSE_TOP_FLAG_PTR(p);
  flags |= (*flag_ptr & (BC_PARSE_FLAG_FUNC | BC_PARSE_FLAG_LOOP));
  flags |= BC_PARSE_FLAG_BODY;
  return bc_vec_push(&p->flags, &flags);
}

void bc_parse_noElse(BcParse *p, BcVec *code) {

  uint8_t *flag_ptr;
  BcInstPtr *ip;
  BcFunc *func;
  size_t *label;

  flag_ptr = BC_PARSE_TOP_FLAG_PTR(p);
  *flag_ptr = (*flag_ptr & ~(BC_PARSE_FLAG_IF_END));

  ip = bc_vec_top(&p->exits);
  assert(!ip->func && !ip->len);
  func = bc_vec_item(&p->prog->funcs, p->func);
  assert(code == &func->code);
  label = bc_vec_item(&func->labels, ip->idx);
  *label = code->len;

  bc_vec_pop(&p->exits);
}

BcStatus bc_parse_if(BcParse *p, BcVec *code) {

  BcStatus status;
  BcInstPtr ip;
  BcFunc *func;

  if ((status = bc_lex_next(&p->lex))) return status;
  if (p->lex.token.type != BC_LEX_LEFT_PAREN) return BC_STATUS_PARSE_BAD_TOKEN;

  if ((status = bc_lex_next(&p->lex))) return status;
  status = bc_parse_expr(p, code, BC_PARSE_POSIX_REL);
  if (status) return status;
  if (p->lex.token.type != BC_LEX_RIGHT_PAREN) return BC_STATUS_PARSE_BAD_TOKEN;

  if ((status = bc_lex_next(&p->lex))) return status;
  if ((status = bc_vec_pushByte(code, BC_INST_JUMP_ZERO))) return status;

  func = bc_vec_item(&p->prog->funcs, p->func);

  ip.idx = func->labels.len;
  ip.func = 0;
  ip.len = 0;

  if ((status = bc_parse_pushIndex(code, ip.idx))) return status;
  if ((status = bc_vec_push(&p->exits, &ip))) return status;
  if ((status = bc_vec_push(&func->labels, &ip.idx))) return status;

  return bc_parse_startBody(p, BC_PARSE_FLAG_IF);
}

BcStatus bc_parse_else(BcParse *p, BcVec *code) {

  BcStatus status;
  BcInstPtr ip;
  BcFunc *func;

  if (!BC_PARSE_IF_END(p)) return BC_STATUS_PARSE_BAD_TOKEN;

  func = bc_vec_item(&p->prog->funcs, p->func);

  ip.idx = func->labels.len;
  ip.func = 0;
  ip.len = 0;

  if ((status = bc_vec_pushByte(code, BC_INST_JUMP))) return status;
  if ((status = bc_parse_pushIndex(code, ip.idx))) return status;

  bc_parse_noElse(p, code);

  if ((status = bc_vec_push(&p->exits, &ip))) return status;
  if ((status = bc_vec_push(&func->labels, &ip.idx))) return status;
  if ((status = bc_lex_next(&p->lex))) return status;

  return bc_parse_startBody(p, BC_PARSE_FLAG_ELSE);
}

BcStatus bc_parse_while(BcParse *p, BcVec *code) {

  BcStatus status;
  BcFunc *func;
  BcInstPtr ip;

  if ((status = bc_lex_next(&p->lex))) return status;
  if (p->lex.token.type != BC_LEX_LEFT_PAREN) return BC_STATUS_PARSE_BAD_TOKEN;
  if ((status = bc_lex_next(&p->lex))) return status;

  func = bc_vec_item(&p->prog->funcs, p->func);
  ip.idx = func->labels.len;

  if ((status = bc_vec_push(&func->labels, &code->len))) return status;
  if ((status = bc_vec_push(&p->conds, &ip.idx))) return status;

  ip.idx = func->labels.len;
  ip.func = 1;
  ip.len = 0;

  if ((status = bc_vec_push(&p->exits, &ip))) return status;
  if ((status = bc_vec_push(&func->labels, &ip.idx))) return status;

  if ((status = bc_parse_expr(p, code, BC_PARSE_POSIX_REL))) return status;
  if (p->lex.token.type != BC_LEX_RIGHT_PAREN) return BC_STATUS_PARSE_BAD_TOKEN;

  if ((status = bc_lex_next(&p->lex))) return status;
  if ((status = bc_vec_pushByte(code, BC_INST_JUMP_ZERO))) return status;
  if ((status = bc_parse_pushIndex(code, ip.idx))) return status;

  return bc_parse_startBody(p, BC_PARSE_FLAG_LOOP | BC_PARSE_FLAG_LOOP_INNER);
}

BcStatus bc_parse_for(BcParse *p, BcVec *code) {

  BcStatus status;
  BcFunc *func;
  BcInstPtr ip;
  size_t cond_idx, exit_idx, body_idx, update_idx;

  if ((status = bc_lex_next(&p->lex))) return status;
  if (p->lex.token.type != BC_LEX_LEFT_PAREN) return BC_STATUS_PARSE_BAD_TOKEN;
  if ((status = bc_lex_next(&p->lex))) return status;

  if (p->lex.token.type != BC_LEX_SEMICOLON)
    status = bc_parse_expr(p, code, 0);
  else
    status = bc_posix_error(BC_STATUS_POSIX_NO_FOR_INIT,
                            p->lex.file, p->lex.line, NULL);

  if (status) return status;
  if (p->lex.token.type != BC_LEX_SEMICOLON) return BC_STATUS_PARSE_BAD_TOKEN;
  if ((status = bc_lex_next(&p->lex))) return status;

  func = bc_vec_item(&p->prog->funcs, p->func);

  cond_idx = func->labels.len;
  update_idx = cond_idx + 1;
  body_idx = update_idx + 1;
  exit_idx = body_idx + 1;

  if ((status = bc_vec_push(&func->labels, &code->len))) return status;

  if (p->lex.token.type != BC_LEX_SEMICOLON)
    status = bc_parse_expr(p, code, BC_PARSE_POSIX_REL);
  else status = bc_posix_error(BC_STATUS_POSIX_NO_FOR_COND,
                               p->lex.file, p->lex.line, NULL);

  if (status) return status;
  if (p->lex.token.type != BC_LEX_SEMICOLON) return BC_STATUS_PARSE_BAD_TOKEN;

  if ((status = bc_lex_next(&p->lex))) return status;
  if ((status = bc_vec_pushByte(code, BC_INST_JUMP_ZERO))) return status;
  if ((status = bc_parse_pushIndex(code, exit_idx))) return status;
  if ((status = bc_vec_pushByte(code, BC_INST_JUMP))) return status;
  if ((status = bc_parse_pushIndex(code, body_idx))) return status;

  ip.idx = func->labels.len;

  if ((status = bc_vec_push(&p->conds, &update_idx))) return status;
  if ((status = bc_vec_push(&func->labels, &code->len))) return status;

  if (p->lex.token.type != BC_LEX_RIGHT_PAREN)
    status = bc_parse_expr(p, code, 0);
  else
    status = bc_posix_error(BC_STATUS_POSIX_NO_FOR_UPDATE,
                            p->lex.file, p->lex.line, NULL);

  if (status) return status;

  if (p->lex.token.type != BC_LEX_RIGHT_PAREN) {
    status = bc_parse_expr(p, code, BC_PARSE_POSIX_REL);
    if (status) return status;
  }

  if (p->lex.token.type != BC_LEX_RIGHT_PAREN) return BC_STATUS_PARSE_BAD_TOKEN;
  if ((status = bc_vec_pushByte(code, BC_INST_JUMP))) return status;
  if ((status = bc_parse_pushIndex(code, cond_idx))) return status;
  if ((status = bc_vec_push(&func->labels, &code->len))) return status;

  ip.idx = exit_idx;
  ip.func = 1;
  ip.len = 0;

  if ((status = bc_vec_push(&p->exits, &ip))) return status;
  if ((status = bc_vec_push(&func->labels, &ip.idx))) return status;
  if ((status = bc_lex_next(&p->lex))) return status;

  return bc_parse_startBody(p, BC_PARSE_FLAG_LOOP | BC_PARSE_FLAG_LOOP_INNER);
}

BcStatus bc_parse_loopExit(BcParse *p, BcVec *code, BcLexToken type) {

  BcStatus status;
  size_t idx;
  BcInstPtr *ip;

  if (!BC_PARSE_LOOP(p)) return BC_STATUS_PARSE_BAD_TOKEN;

  if (type == BC_LEX_KEY_BREAK) {

    size_t top;

    if (!p->exits.len) return BC_STATUS_PARSE_BAD_TOKEN;

    top = p->exits.len - 1;
    ip = bc_vec_item(&p->exits, top);

    while (top < p->exits.len && ip && !ip->func)
      ip = bc_vec_item(&p->exits, top--);

    if (top >= p->exits.len || !ip) return BC_STATUS_PARSE_BAD_TOKEN;

    idx = ip->idx;
  }
  else idx = *((size_t*) bc_vec_top(&p->conds));

  if ((status = bc_vec_pushByte(code, BC_INST_JUMP))) return status;
  if ((status = bc_parse_pushIndex(code, idx))) return status;
  if ((status = bc_lex_next(&p->lex))) return status;

  if (p->lex.token.type != BC_LEX_SEMICOLON &&
      p->lex.token.type != BC_LEX_NEWLINE)
  {
    return BC_STATUS_PARSE_BAD_TOKEN;
  }

  return bc_lex_next(&p->lex);
}

BcStatus bc_parse_func(BcParse *p) {

  BcStatus status;
  BcFunc *fptr;
  bool var, comma = false;
  uint8_t flags;
  char *name;

  if ((status = bc_lex_next(&p->lex))) return status;

  name = p->lex.token.string;

  if (p->lex.token.type != BC_LEX_NAME) {
    status = BC_STATUS_PARSE_BAD_FUNC;
    goto err;
  }

  assert(p->prog->funcs.len == p->prog->func_map.vec.len);

  if ((status = bc_program_addFunc(p->prog, name, &p->func))) goto err;
  assert(p->func);
  fptr = bc_vec_item(&p->prog->funcs, p->func);

  if ((status = bc_lex_next(&p->lex))) return status;
  if (p->lex.token.type != BC_LEX_LEFT_PAREN) return BC_STATUS_PARSE_BAD_FUNC;
  if ((status = bc_lex_next(&p->lex))) return status;

  while (!status && p->lex.token.type != BC_LEX_RIGHT_PAREN) {

    if (p->lex.token.type != BC_LEX_NAME) {
      status = BC_STATUS_PARSE_BAD_FUNC;
      goto err;
    }

    ++fptr->nparams;
    name = p->lex.token.string;

    if ((status = bc_lex_next(&p->lex))) goto err;

    var = p->lex.token.type != BC_LEX_LEFT_BRACKET;

    if (!var) {
      if ((status = bc_lex_next(&p->lex))) goto err;
      if (p->lex.token.type != BC_LEX_RIGHT_BRACKET)
        return BC_STATUS_PARSE_BAD_FUNC;
      if ((status = bc_lex_next(&p->lex))) goto err;
    }

    comma = p->lex.token.type == BC_LEX_COMMA;
    if (comma && (status = bc_lex_next(&p->lex))) goto err;

    if ((status = bc_func_insert(fptr, name, var))) goto err;
  }

  if (comma) return BC_STATUS_PARSE_BAD_FUNC;

  flags = BC_PARSE_FLAG_FUNC | BC_PARSE_FLAG_FUNC_INNER | BC_PARSE_FLAG_BODY;

  if ((status = bc_parse_startBody(p, flags))) return status;
  if ((status = bc_lex_next(&p->lex))) return status;

  if (p->lex.token.type != BC_LEX_LEFT_BRACE)
    return bc_posix_error(BC_STATUS_POSIX_HEADER_BRACE,
                          p->lex.file, p->lex.line, NULL);

  return status;

err:
  free(name);
  return status;
}

BcStatus bc_parse_auto(BcParse *p) {

  BcStatus status;
  bool comma, var, one;
  char *name;
  BcFunc *func;

  if (!p->auto_part) return BC_STATUS_PARSE_BAD_TOKEN;
  if ((status = bc_lex_next(&p->lex))) return status;

  p->auto_part = comma = one = false;
  func = bc_vec_item(&p->prog->funcs, p->func);

  while (!status && p->lex.token.type == BC_LEX_NAME) {

    name = p->lex.token.string;

    if ((status = bc_lex_next(&p->lex))) return status;

    one = true;

    var = p->lex.token.type != BC_LEX_LEFT_BRACKET;

    if (!var) {

      if ((status = bc_lex_next(&p->lex))) goto err;
      if (p->lex.token.type != BC_LEX_RIGHT_BRACKET)
        return BC_STATUS_PARSE_BAD_FUNC;

      if ((status = bc_lex_next(&p->lex))) goto err;
    }

    comma = p->lex.token.type == BC_LEX_COMMA;
    if (comma && (status = bc_lex_next(&p->lex))) goto err;

    if ((status = bc_func_insert(func, name, var))) goto err;
  }

  if (status) return status;
  if (comma) return BC_STATUS_PARSE_BAD_FUNC;
  if (!one) return BC_STATUS_PARSE_NO_AUTO;

  if (p->lex.token.type != BC_LEX_NEWLINE &&
      p->lex.token.type != BC_LEX_SEMICOLON)
  {
    return BC_STATUS_PARSE_BAD_TOKEN;
  }

  return bc_lex_next(&p->lex);

err:
  free(name);
  return status;
}

BcStatus bc_parse_body(BcParse *p, BcVec *code, bool brace) {

  BcStatus status;
  uint8_t *flag_ptr;

  assert(p->flags.len >= 2);

  flag_ptr = bc_vec_top(&p->flags);
  *flag_ptr &= ~(BC_PARSE_FLAG_BODY);

  if (*flag_ptr & BC_PARSE_FLAG_FUNC_INNER) {
    if (!brace) return BC_STATUS_PARSE_BAD_TOKEN;
    p->auto_part = true;
    status = bc_lex_next(&p->lex);
  }
  else {
    assert(*flag_ptr);
    if ((status = bc_parse_stmt(p, code))) return status;
    if (!brace) status = bc_parse_endBody(p, code, false);
  }

  return status;
}

BcStatus bc_parse_stmt(BcParse *p, BcVec *code) {

  BcStatus status;

  switch (p->lex.token.type) {

    case BC_LEX_NEWLINE:
    {
      return bc_lex_next(&p->lex);
    }

    case BC_LEX_KEY_ELSE:
    {
      p->auto_part = false;
      break;
    }

    case BC_LEX_LEFT_BRACE:
    {
      if (!BC_PARSE_BODY(p)) return BC_STATUS_PARSE_BAD_TOKEN;

      ++p->num_braces;
      if ((status = bc_lex_next(&p->lex))) return status;

      return bc_parse_body(p, code, true);
    }

    case BC_LEX_KEY_AUTO:
    {
      return bc_parse_auto(p);
    }

    default:
    {
      p->auto_part = false;

      if (BC_PARSE_IF_END(p)) {
        bc_parse_noElse(p, code);
        return BC_STATUS_SUCCESS;
      }
      else if (BC_PARSE_BODY(p)) return bc_parse_body(p, code, false);

      break;
    }
  }

  switch (p->lex.token.type) {

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
      status = bc_parse_expr(p, code, BC_PARSE_PRINT);
      break;
    }

    case BC_LEX_KEY_ELSE:
    {
      status = bc_parse_else(p, code);
      break;
    }

    case BC_LEX_SEMICOLON:
    {
      status = BC_STATUS_SUCCESS;

      while (!status && p->lex.token.type == BC_LEX_SEMICOLON)
        status = bc_lex_next(&p->lex);

      break;
    }

    case BC_LEX_RIGHT_BRACE:
    {
      status = bc_parse_endBody(p, code, true);
      break;
    }

    case BC_LEX_STRING:
    {
      status = bc_parse_string(p, code);
      break;
    }

    case BC_LEX_KEY_BREAK:
    case BC_LEX_KEY_CONTINUE:
    {
      status = bc_parse_loopExit(p, code, p->lex.token.type);
      break;
    }

    case BC_LEX_KEY_FOR:
    {
      status = bc_parse_for(p, code);
      break;
    }

    case BC_LEX_KEY_HALT:
    {
      if ((status = bc_vec_pushByte(code, BC_INST_HALT))) return status;
      status = bc_lex_next(&p->lex);
      break;
    }

    case BC_LEX_KEY_IF:
    {
      status = bc_parse_if(p, code);
      break;
    }

    case BC_LEX_KEY_LIMITS:
    {
      if ((status = bc_lex_next(&p->lex))) return status;
      status = BC_STATUS_LIMITS;
      break;
    }

    case BC_LEX_KEY_PRINT:
    {
      status = bc_parse_print(p, code);
      break;
    }

    case BC_LEX_KEY_QUIT:
    {
      // Quit is a compile-time command, so we send an exit command. We don't
      // exit directly, so the vm can clean up. Limits do the same thing.
      status = BC_STATUS_QUIT;
      break;
    }

    case BC_LEX_KEY_RETURN:
    {
      if ((status = bc_parse_return(p, code))) return status;
      break;
    }

    case BC_LEX_KEY_WHILE:
    {
      status = bc_parse_while(p, code);
      break;
    }

    case BC_LEX_EOF:
    {
      status = (p->flags.len > 0) * BC_STATUS_LEX_BAD_CHARACTER;
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

BcStatus bc_parse_init(BcParse *p, BcProgram *program) {

  BcStatus status;
  uint8_t flags = 0;

  assert(p && program);

  if ((status = bc_vec_init(&p->flags, sizeof(uint8_t), NULL))) return status;
  if ((status = bc_vec_init(&p->exits, sizeof(BcInstPtr), NULL))) goto exit_err;
  if ((status = bc_vec_init(&p->conds, sizeof(size_t), NULL))) goto cond_err;
  if ((status = bc_vec_push(&p->flags, &flags))) goto push_err;
  if ((status = bc_vec_init(&p->ops, sizeof(BcLexToken), NULL))) goto push_err;

  p->prog = program;
  p->func = p->num_braces = 0;
  p->auto_part = false;

  return status;

push_err:
  bc_vec_free(&p->conds);
cond_err:
  bc_vec_free(&p->exits);
exit_err:
  bc_vec_free(&p->flags);
  return status;
}

BcStatus bc_parse_parse(BcParse *p) {

  BcStatus status;

  assert(p);

  if (p->lex.token.type == BC_LEX_EOF) status = BC_STATUS_LEX_EOF;
  else if (p->lex.token.type == BC_LEX_KEY_DEFINE) {
    if (!BC_PARSE_CAN_EXEC(p)) return BC_STATUS_PARSE_BAD_TOKEN;
    status = bc_parse_func(p);
  }
  else {
    BcFunc *func = bc_vec_item(&p->prog->funcs, p->func);
    status = bc_parse_stmt(p, &func->code);
  }

  if (status || bcg.signe) {

    if (p->func) {

      BcFunc *func = bc_vec_item(&p->prog->funcs, p->func);

      func->nparams = 0;
      bc_vec_npop(&func->code, func->code.len);
      bc_vec_npop(&func->autos, func->autos.len);
      bc_vec_npop(&func->labels, func->labels.len);

      p->func = 0;
    }

    p->lex.idx = p->lex.len;
    p->lex.token.type = BC_LEX_EOF;
    p->auto_part = false;
    p->num_braces = 0;

    bc_vec_npop(&p->flags, p->flags.len - 1);
    bc_vec_npop(&p->exits, p->exits.len);
    bc_vec_npop(&p->conds, p->conds.len);
    bc_vec_npop(&p->ops, p->ops.len);

    status = bc_program_reset(p->prog, status);
  }

  return status;
}

void bc_parse_free(BcParse *p) {

  if (!p) return;

  bc_vec_free(&p->flags);
  bc_vec_free(&p->exits);
  bc_vec_free(&p->conds);
  bc_vec_free(&p->ops);

  if ((p->lex.token.type == BC_LEX_STRING || p->lex.token.type == BC_LEX_NAME ||
       p->lex.token.type == BC_LEX_NUMBER) && p->lex.token.string)
  {
    free(p->lex.token.string);
  }
}

BcStatus bc_parse_expr(BcParse *p, BcVec *code, uint8_t flags) {

  BcStatus status = BC_STATUS_SUCCESS;
  BcInst prev = BC_INST_PRINT;
  BcLexToken top, type = p->lex.token.type;
  uint32_t nexprs, nparens, nrelops, ops_start = (uint32_t) p->ops.len;
  bool paren_first, paren_expr, rparen, done, get_token, assign;

  paren_first = p->lex.token.type == BC_LEX_LEFT_PAREN;
  nexprs = nparens = nrelops = 0;
  paren_expr = rparen = done = get_token = assign = false;

  while (!bcg.signe && !status && !done && bc_parse_token_exprs[type]) {

    switch (type) {

      case BC_LEX_OP_INC:
      case BC_LEX_OP_DEC:
      {
        status = bc_parse_incdec(p, code, &prev, &nexprs, flags);
        rparen = get_token = false;
        break;
      }

      case BC_LEX_OP_MINUS:
      {
        status = bc_parse_minus(p, code, &p->ops, &prev, rparen, &nexprs);
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
        nrelops += type >= BC_LEX_OP_REL_EQUAL && type <= BC_LEX_OP_REL_GREATER;
        prev = BC_PARSE_TOKEN_TO_INST(type);
        status = bc_parse_operator(p, code, &p->ops, type, &nexprs, true);
        rparen = get_token = false;
        break;
      }

      case BC_LEX_LEFT_PAREN:
      {
        ++nparens;
        paren_expr = rparen = false;
        get_token = true;
        status = bc_vec_push(&p->ops, &type);
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

        status = bc_parse_rightParen(p, code, &p->ops, &nexprs);

        break;
      }

      case BC_LEX_NAME:
      {
        paren_expr = true;
        rparen = get_token = false;
        status = bc_parse_name(p, code, &prev, flags & ~BC_PARSE_NOCALL);
        ++nexprs;
        break;
      }

      case BC_LEX_NUMBER:
      {
        size_t idx = p->prog->constants.len;

        status = bc_vec_push(&p->prog->constants, &p->lex.token.string);
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
        prev = (uint8_t) (type - BC_LEX_KEY_IBASE + BC_INST_PUSH_IBASE);
        status = bc_vec_pushByte(code, (uint8_t) prev);

        paren_expr = get_token = true;
        rparen = false;
        ++nexprs;

        break;
      }

      case BC_LEX_KEY_LENGTH:
      case BC_LEX_KEY_SQRT:
      {
        status = bc_parse_builtin(p, code, type, flags, &prev);
        paren_expr = true;
        rparen = get_token = false;
        ++nexprs;
        break;
      }

      case BC_LEX_KEY_READ:
      {
        if (flags & BC_PARSE_NOREAD) status = BC_STATUS_EXEC_NESTED_READ;
        else status = bc_parse_read(p, code);

        paren_expr = true;
        rparen = get_token = false;
        ++nexprs;
        prev = BC_INST_READ;

        break;
      }

      case BC_LEX_KEY_SCALE:
      {
        status = bc_parse_scale(p, code, &prev, flags);
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

    if (status) goto err;
    if (get_token) status = bc_lex_next(&p->lex);

    type = p->lex.token.type;
  }

  if (status) goto err;
  if (bcg.signe) {
    status = BC_STATUS_EXEC_SIGNAL;
    goto err;
  }

  while (!status && p->ops.len > ops_start) {

    top = *((BcLexToken*) bc_vec_top(&p->ops));
    assign = top >= BC_LEX_OP_ASSIGN_POWER && top <= BC_LEX_OP_ASSIGN;

    if (top == BC_LEX_LEFT_PAREN || top == BC_LEX_RIGHT_PAREN) {
      status = BC_STATUS_PARSE_BAD_EXPR;
      goto err;
    }

    if ((status = bc_vec_pushByte(code, (uint8_t) BC_PARSE_TOKEN_TO_INST(top))))
      goto err;

    nexprs -= top != BC_LEX_OP_BOOL_NOT && top != BC_LEX_OP_NEG;
    bc_vec_pop(&p->ops);
  }

  if (nexprs != 1) {
    status = BC_STATUS_PARSE_BAD_EXPR;
    goto err;
  }

  if (!(flags & BC_PARSE_POSIX_REL) && nrelops &&
      (status = bc_posix_error(BC_STATUS_POSIX_REL_OUTSIDE,
                               p->lex.file, p->lex.line, NULL)))
  {
    goto err;
  }
  else if ((flags & BC_PARSE_POSIX_REL) && nrelops != 1 &&
           (status = bc_posix_error(BC_STATUS_POSIX_MULTIPLE_REL,
                                    p->lex.file, p->lex.line, NULL)))
  {
    goto err;
  }

  if (flags & BC_PARSE_PRINT) {
    if (paren_first || !assign) status = bc_vec_pushByte(code, BC_INST_PRINT);
    else status = bc_vec_pushByte(code, BC_INST_POP);
  }

  return status;

err:

  if (p->lex.token.string) {
    free(p->lex.token.string);
    p->lex.token.string = NULL;
  }

  return status;
}
