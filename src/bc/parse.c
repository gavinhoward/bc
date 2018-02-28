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

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <data.h>
#include <lex.h>
#include <parse.h>
#include <instructions.h>

// This is an array that corresponds to token types. An entry is
// true if the token is valid in an expression, false otherwise.
static const bool bc_token_exprs[] = {

  true,
  true,

  true,

  true,
  true,
  true,

  true,
  true,

  true,
  true,
  true,
  true,
  true,
  true,
  true,

  true,
  true,
  true,
  true,
  true,
  true,

  true,

  true,
  true,

  true,

  false,

  false,

  true,
  true,

  false,
  false,

  false,
  false,

  false,
  false,

  false,
  true,
  true,

  false,
  false,
  false,
  false,
  false,
  false,
  false,
  true,
  false,
  true,
  true,
  true,
  true,
  false,
  false,
  true,
  false,
  true,
  true,
  false,

  false,

  false,
};

// This is an array of data for operators that correspond to token types.
// The last corresponds to BC_PARSE_OP_NEGATE_IDX since it doesn't have
// its own token type (it is the same token at the binary minus operator).
static const BcOp bc_ops[] = {

  { 0, false },
  { 0, false },

  { 2, false },

  { 3, true },
  { 3, true },
  { 3, true },

  { 4, true },
  { 4, true },

  { 5, false },
  { 5, false },
  { 5, false },
  { 5, false },
  { 5, false },
  { 5, false },
  { 5, false },

  { 1, false },

  { 6, true },
  { 6, true },
  { 6, true },
  { 6, true },
  { 6, true },
  { 6, true },

  { 7, false },

  { 8, true },
  { 8, true },

};

static const uint8_t bc_op_insts[] = {

  BC_INST_OP_POWER,

  BC_INST_OP_MULTIPLY,
  BC_INST_OP_DIVIDE,
  BC_INST_OP_MODULUS,

  BC_INST_OP_PLUS,
  BC_INST_OP_MINUS,

  BC_INST_OP_REL_EQUAL,
  BC_INST_OP_REL_LESS_EQ,
  BC_INST_OP_REL_GREATER_EQ,
  BC_INST_OP_REL_NOT_EQ,
  BC_INST_OP_REL_LESS,
  BC_INST_OP_REL_GREATER,

  BC_INST_OP_BOOL_NOT,

  BC_INST_OP_BOOL_NOT,
  BC_INST_OP_BOOL_AND,

  BC_INST_OP_NEGATE,

  BC_INST_OP_ASSIGN_POWER,
  BC_INST_OP_ASSIGN_MULTIPLY,
  BC_INST_OP_ASSIGN_DIVIDE,
  BC_INST_OP_ASSIGN_MODULUS,
  BC_INST_OP_ASSIGN_PLUS,
  BC_INST_OP_ASSIGN_MINUS,
  BC_INST_OP_ASSIGN,

};

static BcStatus bc_parse_semicolonListEnd(BcParse *parse, BcVec *code);
static BcStatus bc_parse_stmt(BcParse *parse, BcVec *code);

static BcStatus bc_parse_pushName(BcVec *code, char *name) {

  BcStatus status;
  size_t len;

  status = BC_STATUS_SUCCESS;
  len = strlen(name);

  for (size_t i = 0; !status && i < len; ++i)
    status = bc_vec_pushByte(code, (uint8_t) name[i]);

  if (status) return status;

  free(name);

  return bc_vec_pushByte(code, (uint8_t) ':');
}

static BcStatus bc_parse_pushIndex(BcVec *code, size_t idx) {

  BcStatus status;
  uint8_t amt;
  uint8_t nums[sizeof(size_t)];
  uint32_t i;

  amt = 0;
  i = 0;

  while (idx) {
    nums[amt] = (uint8_t) idx;
    idx &= ~(0xFF);
    idx >>= sizeof(uint8_t);
    ++amt;
  }

  status = bc_vec_pushByte(code, amt);

  if (status) return status;

  while (!status && i < amt) {
    status = bc_vec_pushByte(code, nums[idx]);
    ++i;
  }

  return status;
}

static BcStatus bc_parse_operator(BcParse *parse, BcVec *code, BcVec *ops,
                                  BcLexTokenType t, uint32_t *num_exprs,
                                  bool next)
{
  BcStatus status;
  BcLexTokenType top;
  BcLexTokenType *ptr;
  uint8_t lp;
  uint8_t rp;
  bool rleft;

  rp = bc_ops[t].prec;
  rleft = bc_ops[t].left;

  if (ops->len != 0) {

    ptr = bc_vec_top(ops);
    top = *ptr;

    if (top != BC_LEX_LEFT_PAREN) {

      lp = bc_ops[top - BC_LEX_OP_POWER].prec;

      while (lp < rp || (lp == rp && rleft)) {

        status = bc_vec_pushByte(code, bc_op_insts[top - BC_LEX_OP_POWER]);

        if (status) return status;

        status = bc_vec_pop(ops);

        if (status) return status;

        *num_exprs -= top != BC_LEX_OP_BOOL_NOT &&
                      top != BC_LEX_OP_NEGATE ? 1 : 0;

        if (ops->len == 0) break;

        ptr = bc_vec_top(ops);
        top = *ptr;

        if (top == BC_LEX_LEFT_PAREN) break;

        lp = bc_ops[top].prec;
      }
    }
  }

  status = bc_vec_push(ops, &t);

  if (status) return status;

  if (next) status = bc_lex_next(&parse->lex, &parse->token);

  return status;
}

static BcStatus bc_parse_rightParen(BcParse *parse, BcVec *code,
                                    BcVec *ops, uint32_t *nexs)
{
  BcStatus status;
  BcLexTokenType top;
  BcLexTokenType *ptr;

  if (ops->len == 0) return BC_STATUS_PARSE_INVALID_EXPR;

  ptr = bc_vec_top(ops);
  top = *ptr;

  while (top != BC_LEX_LEFT_PAREN) {

    status = bc_vec_pushByte(code, bc_op_insts[top - BC_LEX_OP_POWER]);

    if (status) return status;

    status = bc_vec_pop(ops);

    if (status) return status;

    *nexs -= top != BC_LEX_OP_BOOL_NOT &&
             top != BC_LEX_OP_NEGATE ? 1 : 0;

    if (ops->len == 0) return BC_STATUS_PARSE_INVALID_EXPR;

    ptr = bc_vec_top(ops);
    top = *ptr;
  }

  status = bc_vec_pop(ops);

  if (status) return status;

  return bc_lex_next(&parse->lex, &parse->token);
}

static BcStatus bc_parse_params(BcParse *parse, BcVec *code, uint8_t flags) {

  BcStatus status;
  bool comma;
  size_t nparams;

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) return status;

  if (parse->token.type == BC_LEX_RIGHT_PAREN) {

    status = bc_vec_pushByte(code, BC_INST_CALL);

    if (status) return status;

    return bc_vec_pushByte(code, 0);
  }

  nparams = 0;

  do {

    ++nparams;

    status = bc_parse_expr(parse, code, flags & ~(BC_PARSE_EXPR_PRINT));

    if (status) return status;

    if (parse->token.type == BC_LEX_COMMA) {

      comma = true;
      status = bc_lex_next(&parse->lex, &parse->token);

      if (status) return status;
    }
    else comma = false;

  } while (!status && parse->token.type != BC_LEX_RIGHT_PAREN);

  if (status) return status;

  if (comma) return BC_STATUS_PARSE_INVALID_TOKEN;

  status = bc_vec_pushByte(code, BC_INST_CALL);

  if (status) return status;

  return bc_parse_pushIndex(code, nparams);
}

static BcStatus bc_parse_call(BcParse *parse, BcVec *code,
                              char *name, uint8_t flags)
{
  BcStatus status;
  BcEntry entry;
  size_t idx;

  entry.name = name;

  status = bc_parse_params(parse, code, flags);

  if (status) return status;

  if (parse->token.type != BC_LEX_RIGHT_PAREN)
    return BC_STATUS_PARSE_INVALID_TOKEN;

  idx = bc_veco_index(&parse->program->func_map, &entry);

  if (idx == BC_INVALID_IDX) return BC_STATUS_EXEC_UNDEFINED_FUNC;

  status = bc_parse_pushIndex(code, idx);

  if (status) return status;

  return bc_lex_next(&parse->lex, &parse->token);
}

static BcStatus bc_parse_expr_name(BcParse *parse, BcVec *code,
                                   BcExprType *type, uint8_t flags)
{
  BcStatus status;
  char *name;

  name = parse->token.string;

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) return status;

  if (parse->token.type == BC_LEX_LEFT_BRACKET) {

    *type = BC_EXPR_ARRAY_ELEM;

    status = bc_lex_next(&parse->lex, &parse->token);

    if (status) return status;

    status = bc_parse_expr(parse, code, flags);

    if (status) return status;

    if (parse->token.type != BC_LEX_RIGHT_BRACKET)
      return BC_STATUS_PARSE_INVALID_TOKEN;

    status = bc_vec_pushByte(code, BC_INST_PUSH_ARRAY);

    if (status) return status;

    status = bc_parse_pushName(code, name);
  }
  else if (parse->token.type == BC_LEX_LEFT_PAREN) {

    if (flags & BC_PARSE_EXPR_NO_CALL) return BC_STATUS_PARSE_INVALID_TOKEN;

    *type = BC_EXPR_FUNC_CALL;

    status = bc_parse_call(parse, code, name, flags);
  }
  else {

    *type = BC_EXPR_VAR;

    status = bc_vec_pushByte(code, BC_INST_PUSH_VAR);

    if (status) return status;

    status = bc_parse_pushName(code, name);
  }

  return status;
}

static BcStatus bc_parse_read(BcParse *parse, BcVec *code) {

  BcStatus status;

  // TODO: Prevent recursive reads.

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) return status;

  if (parse->token.type != BC_LEX_LEFT_PAREN)
    return BC_STATUS_PARSE_INVALID_TOKEN;

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) return status;

  if (parse->token.type != BC_LEX_RIGHT_PAREN)
    return BC_STATUS_PARSE_INVALID_TOKEN;

  status = bc_vec_pushByte(code, BC_INST_READ);

  if (status) return status;

  return bc_lex_next(&parse->lex, &parse->token);
}

static BcStatus bc_parse_builtin(BcParse *parse, BcVec *code,
                                 BcLexTokenType type, uint8_t flags)
{
  BcStatus status;
  uint8_t inst;

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) return status;

  if (parse->token.type != BC_LEX_LEFT_PAREN)
    return BC_STATUS_PARSE_INVALID_TOKEN;

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) return status;

  status = bc_parse_expr(parse, code, flags & ~(BC_PARSE_EXPR_PRINT));

  if (status) return status;

  if (parse->token.type != BC_LEX_RIGHT_PAREN)
    return BC_STATUS_PARSE_INVALID_TOKEN;

  inst = type == BC_EXPR_LENGTH ? BC_INST_LENGTH : BC_INST_SQRT;

  status = bc_vec_pushByte(code, inst);

  if (status) return status;

  return bc_lex_next(&parse->lex, &parse->token);
}

static BcStatus bc_parse_scale(BcParse *parse, BcVec *code,
                               BcExprType *type, uint8_t flags)
{
  BcStatus status;

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) return status;

  if (parse->token.type != BC_LEX_LEFT_PAREN) {

    *type = BC_EXPR_SCALE;

    return bc_vec_pushByte(code, BC_INST_PUSH_SCALE);
  }

  *type = BC_EXPR_SCALE_FUNC;

  status = bc_parse_expr(parse, code, flags);

  if (status) return status;

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) return status;

  if (parse->token.type != BC_LEX_RIGHT_PAREN)
    return BC_STATUS_PARSE_INVALID_TOKEN;

  status = bc_vec_pushByte(code, BC_INST_SCALE_FUNC);

  if (status) return status;

  return bc_lex_next(&parse->lex, &parse->token);
}

static BcStatus bc_parse_incdec(BcParse *parse, BcVec *code,
                                BcExprType *prev, uint8_t flags)
{
  BcStatus status;
  BcLexTokenType type;
  BcExprType etype;
  uint8_t inst;

  etype = *prev;

  if (etype == BC_EXPR_VAR || etype == BC_EXPR_ARRAY_ELEM ||
      etype == BC_EXPR_SCALE || etype == BC_EXPR_LAST ||
      etype == BC_EXPR_IBASE || etype == BC_EXPR_OBASE)
  {
    *prev = parse->token.type == BC_LEX_OP_INC ?
              BC_EXPR_INC_POST : BC_EXPR_DEC_POST;

    inst = parse->token.type == BC_LEX_OP_INC ?
             BC_INST_INC_DUP : BC_INST_DEC_DUP;

    status = bc_vec_pushByte(code, inst);
  }
  else {

    inst = parse->token.type == BC_LEX_OP_INC ? BC_INST_INC : BC_INST_DEC;

    *prev = parse->token.type == BC_LEX_OP_INC ?
              BC_EXPR_INC_PRE : BC_EXPR_DEC_PRE;

    status = bc_lex_next(&parse->lex, &parse->token);

    if (status) return status;

    type = parse->token.type;

    switch (type) {

      case BC_LEX_NAME:
      {
        status = bc_parse_expr_name(parse, code, prev,
                                    flags | BC_PARSE_EXPR_NO_CALL);
        break;
      }

      case BC_LEX_KEY_IBASE:
      {
        status = bc_vec_pushByte(code, BC_INST_PUSH_IBASE);

        if (status) return status;

        status = bc_lex_next(&parse->lex, &parse->token);

        break;
      }

      case BC_LEX_KEY_LAST:
      {
        status = bc_vec_pushByte(code, BC_INST_PUSH_LAST);

        if (status) return status;

        status = bc_lex_next(&parse->lex, &parse->token);

        break;
      }

      case BC_LEX_KEY_OBASE:
      {
        status = bc_vec_pushByte(code, BC_INST_PUSH_OBASE);

        if (status) return status;

        status = bc_lex_next(&parse->lex, &parse->token);

        break;
      }

      case BC_LEX_KEY_SCALE:
      {
        status = bc_lex_next(&parse->lex, &parse->token);

        if (status) return status;

        if (parse->token.type == BC_LEX_LEFT_PAREN)
          return BC_STATUS_PARSE_INVALID_TOKEN;

        status = bc_vec_pushByte(code, BC_INST_PUSH_SCALE);

        break;
      }

      default:
      {
        return BC_STATUS_PARSE_INVALID_TOKEN;
      }
    }

    if (status) return status;

    status = bc_vec_pushByte(code, inst);
  }

  return status;
}

static BcStatus bc_parse_minus(BcParse *parse, BcVec *exs, BcVec *ops,
                               BcExprType *prev, bool rparen, uint32_t *nexprs)
{
  BcStatus status;
  BcLexTokenType type;
  BcExprType etype;

  etype = *prev;

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) return status;

  type = parse->token.type;

  if (type != BC_LEX_NAME && type != BC_LEX_NUMBER &&
      type != BC_LEX_KEY_SCALE && type != BC_LEX_KEY_LAST &&
      type != BC_LEX_KEY_IBASE && type != BC_LEX_KEY_OBASE &&
      type != BC_LEX_LEFT_PAREN)
  {
    return BC_STATUS_PARSE_INVALID_TOKEN;
  }

  type = rparen || etype == BC_EXPR_NUMBER ||
              etype == BC_EXPR_VAR || etype == BC_EXPR_ARRAY_ELEM ||
              etype == BC_EXPR_SCALE || etype == BC_EXPR_LAST ||
              etype == BC_EXPR_IBASE || etype == BC_EXPR_OBASE ?
                  BC_LEX_OP_MINUS : BC_LEX_OP_NEGATE;

  *prev = BC_PARSE_TOKEN_TO_EXPR(type);

  if (type == BC_LEX_OP_MINUS)
    status = bc_parse_operator(parse, exs, ops, type, nexprs, false);
  else
    // We can just push onto the op stack because this is the largest
    // precedence operator that gets pushed. Inc/dec does not.
    status = bc_vec_push(ops, &type);

  return status;
}

static BcStatus bc_parse_string(BcParse *parse, BcVec *code) {

  BcStatus status;
  size_t len;

  len = parse->program->strings.len;

  status = bc_vec_push(&parse->program->strings, &parse->token.string);

  if (status) return status;

  status = bc_vec_pushByte(code, BC_INST_STR);

  if (status) return status;

  status = bc_parse_pushIndex(code, len);

  if (status) return status;

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) return status;

  return bc_parse_semicolonListEnd(parse, code);
}

static BcStatus bc_parse_print(BcParse *parse, BcVec *code) {

  BcStatus status;
  BcLexTokenType type;
  bool comma;

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) return status;

  type = parse->token.type;

  if (type == BC_LEX_SEMICOLON || type == BC_LEX_NEWLINE)
    return BC_STATUS_PARSE_INVALID_PRINT;

  comma = false;

  while (!status && type != BC_LEX_SEMICOLON && type != BC_LEX_NEWLINE) {

    if (type == BC_LEX_STRING) {

      size_t len;

      len = parse->program->strings.len;

      status = bc_vec_push(&parse->program->strings, &parse->token.string);

      if (status) return status;

      status = bc_vec_pushByte(code, BC_INST_PRINT_STR);

      if (status) return status;

      status = bc_parse_pushIndex(code, len);
    }
    else {

      status = bc_parse_expr(parse, code, 0);

      if (status) return status;

      status = bc_vec_pushByte(code, BC_INST_PRINT);
    }

    if (status) return status;

    status = bc_lex_next(&parse->lex, &parse->token);

    if (status) return status;

    if (parse->token.type == BC_LEX_COMMA) {
      comma = true;
      status = bc_lex_next(&parse->lex, &parse->token);
    }
    else comma = false;

    type = parse->token.type;
  }

  if (status) return status;

  if (comma) return BC_STATUS_PARSE_INVALID_TOKEN;

  return bc_lex_next(&parse->lex, &parse->token);
}

static BcStatus bc_parse_return(BcParse *parse, BcVec *code) {

  BcStatus status;

  if (!BC_PARSE_FUNC(parse)) return BC_STATUS_PARSE_INVALID_TOKEN;

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) return status;

  if (parse->token.type != BC_LEX_NEWLINE &&
      parse->token.type != BC_LEX_SEMICOLON &&
      parse->token.type != BC_LEX_LEFT_PAREN &&
      (status = bc_posix_error(BC_STATUS_POSIX_RETURN_PARENS,
                               parse->lex.file, parse->lex.line, NULL)))
  {
     return status;
  }

  if (parse->token.type == BC_LEX_NEWLINE ||
      parse->token.type == BC_LEX_SEMICOLON)
  {
    status = bc_vec_pushByte(code, BC_INST_RETURN_ZERO);
  }
  else status = bc_parse_expr(parse, code, 0);

  return status;
}

static BcStatus bc_parse_startBody(BcParse *parse, BcVec *code, uint8_t flags) {

  BcStatus status;
  uint8_t *flag_ptr;

  if (parse->token.type == BC_LEX_LEFT_BRACE) {

    flag_ptr = BC_PARSE_TOP_FLAG_PTR(parse);

    if (!flag_ptr) return BC_STATUS_PARSE_BUG;

    flags |= (*flag_ptr & (BC_PARSE_FLAG_FUNC | BC_PARSE_FLAG_LOOP));

    status = bc_vec_push(&parse->flags, &flags);

    if (status) return status;

    ++parse->num_braces;

    status = bc_lex_next(&parse->lex, &parse->token);

    if (status) return status;
  }
  else {

    while (parse->token.type == BC_LEX_NEWLINE) {

      status = bc_lex_next(&parse->lex, &parse->token);

      if (status) return status;
    }
  }

  return bc_parse_stmt(parse, code);
}

static BcStatus bc_parse_endHeader(BcParse *parse, BcVec *code, BcFunc *func,
                                   size_t idx, uint8_t flags)
{
  BcStatus status;

  status = bc_vec_pushByte(code, BC_INST_JUMP_ZERO);

  if (status) return status;

  status = bc_parse_pushIndex(code, idx);

  if (status) return status;

  if (parse->token.type == BC_LEX_LEFT_BRACE)
    status = bc_parse_startBody(parse, code, flags);
  else if (parse->token.type == BC_LEX_NEWLINE) {

    flags |= BC_PARSE_FLAG_HEADER;

    status = bc_vec_push(&parse->flags, &flags);
  }
  else {

    BcInstPtr *ip_ptr;

    status = bc_parse_stmt(parse, code);

    if (status) return status;

    ip_ptr = bc_vec_item(&func->labels, idx);

    if (!ip_ptr) return BC_STATUS_PARSE_BUG;

    ip_ptr->idx = code->len;

    status = bc_vec_pop(&parse->exit_labels);
  }

  return status;
}

static BcStatus bc_parse_noElse(BcParse *parse, BcVec *code) {

  uint8_t *flag_ptr;
  BcInstPtr *ip;
  BcFunc *func;
  size_t *label;

  flag_ptr = BC_PARSE_TOP_FLAG_PTR(parse);
  *flag_ptr = (*flag_ptr & ~(BC_PARSE_FLAG_IF_END));

  ip = bc_vec_top(&parse->exit_labels);

  if (!ip || ip->func || ip->len) return BC_STATUS_PARSE_BUG;

  func = bc_vec_item(&parse->program->funcs, parse->func);

  if (!func || code != &func->code) return BC_STATUS_PARSE_BUG;

  label = bc_vec_item(&func->labels, ip->idx);

  if (!label) return BC_STATUS_PARSE_BUG;

  *label = code->len;

  return bc_vec_pop(&parse->exit_labels);
}

static BcStatus bc_parse_if(BcParse *parse, BcVec *code) {

  BcStatus status;
  BcInstPtr ip;
  BcFunc *func;

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) return status;

  if (parse->token.type != BC_LEX_LEFT_PAREN)
    return BC_STATUS_PARSE_INVALID_TOKEN;

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) return status;

  func = bc_vec_item(&parse->program->funcs, parse->func);

  if (!func) return BC_STATUS_EXEC_UNDEFINED_FUNC;

  ip.idx = func->labels.len;
  ip.func = 0;
  ip.len = 0;

  status = bc_vec_push(&parse->exit_labels, &ip);

  if (status) return status;

  status = bc_vec_push(&func->labels, &ip.idx);

  if (status) return status;

  status = bc_parse_expr(parse, code, BC_PARSE_EXPR_POSIX_REL);

  if (status) return status;

  if (parse->token.type != BC_LEX_RIGHT_PAREN)
    return BC_STATUS_PARSE_INVALID_TOKEN;

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) return status;

  status = bc_vec_pushByte(code, BC_INST_JUMP_ZERO);

  if (status) return status;

  status = bc_parse_pushIndex(code, ip.idx);

  if (status) return status;

  return bc_parse_endHeader(parse, code, func, ip.idx, BC_PARSE_FLAG_IF);
}

static BcStatus bc_parse_else(BcParse *parse, BcVec *code) {

  BcStatus status;
  BcInstPtr ip;
  BcFunc *func;

  if (!BC_PARSE_IF_END(parse)) return BC_STATUS_PARSE_INVALID_TOKEN;

  func = bc_vec_item(&parse->program->funcs, parse->func);

  if (!func) return BC_STATUS_PARSE_BUG;

  ip.idx = func->labels.len;
  ip.func = 0;
  ip.len = 0;

  status = bc_vec_pushByte(code, BC_INST_JUMP);

  if (status) return status;

  status = bc_parse_pushIndex(code, ip.idx);

  if (status) return status;

  status = bc_parse_noElse(parse, code);

  if (status) return status;

  status = bc_vec_push(&parse->exit_labels, &ip);

  if (status) return status;

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) return status;

  return bc_parse_endHeader(parse, code, func, ip.idx, BC_PARSE_FLAG_ELSE);
}

static BcStatus bc_parse_while(BcParse *parse, BcVec *code) {

  BcStatus status;
  uint8_t flags;
  BcFunc *func;
  BcInstPtr ip;

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) return status;

  if (parse->token.type != BC_LEX_LEFT_PAREN)
    return BC_STATUS_PARSE_INVALID_TOKEN;

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) return status;

  func = bc_vec_item(&parse->program->funcs, parse->func);

  if (!func) return BC_STATUS_EXEC_UNDEFINED_FUNC;

  ip.idx = func->labels.len;

  status = bc_vec_push(&func->labels, &code->len);

  if (status) return status;

  status = bc_vec_push(&parse->cond_labels, &ip.idx);

  if (status) return status;

  ip.idx = func->labels.len;
  ip.func = 1;
  ip.len = 0;

  status = bc_vec_push(&parse->exit_labels, &ip);

  if (status) return status;

  status = bc_vec_push(&func->labels, &ip.idx);

  if (status) return status;

  status = bc_parse_expr(parse, code, BC_PARSE_EXPR_POSIX_REL);

  if (status) return status;

  if (parse->token.type != BC_LEX_RIGHT_PAREN)
    return BC_STATUS_PARSE_INVALID_TOKEN;

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) return status;

  status = bc_vec_pushByte(code, BC_INST_JUMP_ZERO);

  if (status) return status;

  status = bc_parse_pushIndex(code, ip.idx);

  if (status) return status;

  flags = BC_PARSE_FLAG_LOOP | BC_PARSE_FLAG_LOOP_INNER;

  return bc_parse_endHeader(parse, code, func, ip.idx, flags);
}

static BcStatus bc_parse_for(BcParse *parse, BcVec *code) {

  BcStatus status;
  BcFunc *func;
  BcInstPtr ip;
  size_t cond_idx;
  uint8_t flags;

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) return status;

  if (parse->token.type != BC_LEX_LEFT_PAREN)
    return BC_STATUS_PARSE_INVALID_TOKEN;

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) return status;

  if (parse->token.type != BC_LEX_SEMICOLON) {
    status = bc_parse_expr(parse, code, 0);
  }
  else {
    status = bc_posix_error(BC_STATUS_POSIX_MISSING_FOR_INIT,
                            parse->lex.file, parse->lex.line, NULL);
  }

  if (status) return status;

  if (parse->token.type != BC_LEX_SEMICOLON)
    return BC_STATUS_PARSE_INVALID_TOKEN;

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) return status;

  func = bc_vec_item(&parse->program->funcs, parse->func);

  if (!func) return BC_STATUS_PARSE_BUG;

  cond_idx = func->labels.len;

  status = bc_vec_push(&func->labels, &code->len);

  if (status) return status;

  if (parse->token.type != BC_LEX_SEMICOLON)
    status = bc_parse_expr(parse, code, BC_PARSE_EXPR_POSIX_REL);
  else status = bc_posix_error(BC_STATUS_POSIX_MISSING_FOR_COND,
                               parse->lex.file, parse->lex.line, NULL);

  if (status) return status;

  if (parse->token.type != BC_LEX_SEMICOLON)
    return BC_STATUS_PARSE_INVALID_TOKEN;

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) return status;

  status = bc_vec_pushByte(code, BC_INST_JUMP_ZERO);

  if (status) return status;

  status = bc_parse_pushIndex(code, cond_idx + 3);

  if (status) return status;

  status = bc_vec_pushByte(code, BC_INST_JUMP);

  if (status) return status;

  status = bc_parse_pushIndex(code, cond_idx + 2);

  if (status) return status;

  ip.idx = func->labels.len;

  status = bc_vec_push(&parse->cond_labels, &ip.idx);

  if (status) return status;

  status = bc_vec_push(&func->labels, &code->len);

  if (status) return status;

  if (parse->token.type != BC_LEX_RIGHT_PAREN)
    status = bc_parse_expr(parse, code, 0);
  else
    status = bc_posix_error(BC_STATUS_POSIX_MISSING_FOR_UPDATE,
                            parse->lex.file, parse->lex.line, NULL);

  if (status) return status;

  if (parse->token.type != BC_LEX_RIGHT_PAREN)
    return BC_STATUS_PARSE_INVALID_TOKEN;

  status = bc_vec_pushByte(code, BC_INST_JUMP);

  if (status) return status;

  status = bc_parse_pushIndex(code, cond_idx);

  if (status) return status;

  status = bc_vec_push(&func->labels, &code->len);

  if (status) return status;

  ip.idx = func->labels.len;
  ip.func = 1;
  ip.len = 0;

  status = bc_vec_push(&parse->exit_labels, &ip);

  if (status) return status;

  status = bc_vec_push(&func->labels, &ip.idx);

  if (status) return status;

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) return status;

  flags = BC_PARSE_FLAG_LOOP | BC_PARSE_FLAG_LOOP_INNER;

  return bc_parse_endHeader(parse, code, func, ip.idx, flags);
}

static BcStatus bc_parse_loopExit(BcParse *parse, BcVec *code,
                                  BcLexTokenType type)
{
  BcStatus status;
  size_t idx;

  if (!BC_PARSE_LOOP(parse))
    return BC_STATUS_PARSE_INVALID_TOKEN;

  if (type == BC_LEX_KEY_BREAK) {

    size_t top;
    BcInstPtr *ip;

    top = parse->exit_labels.len - 1;
    ip = bc_vec_item(&parse->exit_labels, top);

    while (top < parse->exit_labels.len && ip && !ip->func) {
      ip = bc_vec_item(&parse->exit_labels, top);
      --top;
    }

    if (top >= parse->exit_labels.len || !ip) return BC_STATUS_PARSE_BUG;

    idx = ip->idx;
  }
  else {

    size_t *ptr;

    ptr = bc_vec_top(&parse->cond_labels);

    if (!ptr) return BC_STATUS_PARSE_BUG;

    idx = *ptr;
  }

  status = bc_vec_pushByte(code, BC_INST_JUMP);

  if (status) return status;

  status = bc_parse_pushIndex(code, idx);

  if (status) return status;

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) return status;

  if (parse->token.type != BC_LEX_SEMICOLON &&
      parse->token.type != BC_LEX_NEWLINE)
  {
    return BC_STATUS_PARSE_INVALID_TOKEN;
  }

  return bc_lex_next(&parse->lex, &parse->token);
}

static BcStatus bc_parse_rightBrace(BcParse *parse, BcVec *code) {

  BcStatus status;

  if (parse->flags.len <= 1 || parse->num_braces == 0)
    return BC_STATUS_PARSE_INVALID_TOKEN;

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) return status;

  if (BC_PARSE_IF(parse)) {

    while (parse->token.type == BC_LEX_NEWLINE) {
      status = bc_lex_next(&parse->lex, &parse->token);
      if (status) return status;
    }

    if (parse->token.type == BC_LEX_KEY_ELSE) {
      status = bc_parse_else(parse, code);
    }
    else {

      uint8_t *flag_ptr;

      status = bc_vec_pop(&parse->flags);

      if (status) return status;

      flag_ptr = BC_PARSE_TOP_FLAG_PTR(parse);

      *flag_ptr = (*flag_ptr | BC_PARSE_FLAG_IF_END);

      return BC_STATUS_SUCCESS;
    }
  }
  else if (BC_PARSE_FUNC_INNER(parse)) {

    parse->func = 0;

    status = bc_vec_pushByte(code, BC_INST_RETURN_ZERO);
  }
  else {

    BcInstPtr *ip;
    BcFunc *func;
    size_t *label;

    ip = bc_vec_top(&parse->exit_labels);

    if (!ip) return BC_STATUS_PARSE_BUG;

    func = bc_vec_item(&parse->program->funcs, parse->func);

    if (!func) return BC_STATUS_PARSE_BUG;

    label = bc_vec_item(&func->labels, ip->idx);

    if (!label) return BC_STATUS_PARSE_BUG;

    *label = code->len;
  }

  if (status) return status;

  --parse->num_braces;

  return bc_vec_pop(&parse->flags);
}

static BcStatus bc_parse_func(BcParse *parse) {

  BcLexTokenType type;
  BcStatus status;
  BcFunc func;
  bool comma;
  uint8_t flags;
  char *name;
  bool var;

  // TODO: Make sure parameter and auto names do not clash.

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) return status;

  if (parse->token.type != BC_LEX_NAME) return BC_STATUS_PARSE_INVALID_FUNC;

  if (parse->program->funcs.len != parse->program->func_map.vec.len)
    return BC_STATUS_PARSE_MISMATCH_NUM_FUNCS;

  status = bc_program_func_add(parse->program, parse->token.string,
                               &parse->func);

  if (status) return status;

  if (!parse->func) return BC_STATUS_PARSE_BUG;

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) return status;

  if (parse->token.type != BC_LEX_LEFT_PAREN)
    return BC_STATUS_PARSE_INVALID_FUNC;

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) return status;

  comma = false;

  type = parse->token.type;

  while (!status && type != BC_LEX_RIGHT_PAREN) {

    if (type != BC_LEX_NAME) return BC_STATUS_PARSE_INVALID_FUNC;

    name = parse->token.string;

    status = bc_lex_next(&parse->lex, &parse->token);

    if (status) return status;

    if (parse->token.type == BC_LEX_LEFT_BRACKET) {

      status = bc_lex_next(&parse->lex, &parse->token);

      if (status) return status;

      if (parse->token.type != BC_LEX_RIGHT_BRACKET)
        return BC_STATUS_PARSE_INVALID_FUNC;

      status = bc_lex_next(&parse->lex, &parse->token);

      if (status) return status;

      var = false;
    }
    else {
      var = true;
    }

    if (parse->token.type == BC_LEX_COMMA) {

      comma = true;
      status = bc_lex_next(&parse->lex, &parse->token);

      if (status) return status;
    }
    else comma = false;

    status = bc_func_insertParam(&func, name, var);

    if (status) return status;

    type = parse->token.type;
  }

  if (comma) return BC_STATUS_PARSE_INVALID_FUNC;

  flags = BC_PARSE_FLAG_FUNC | BC_PARSE_FLAG_FUNC_INNER | BC_PARSE_FLAG_HEADER;
  status = bc_vec_push(&parse->flags, &flags);

  if (status) return status;

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) return status;

  if (parse->token.type != BC_LEX_LEFT_BRACE)
    return bc_posix_error(BC_STATUS_POSIX_FUNC_HEADER_LEFT_BRACE,
                          parse->lex.file, parse->lex.line, NULL);

  return status;
}

static BcStatus bc_parse_funcStart(BcParse *parse) {

  BcStatus status;

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) return status;

  parse->auto_part = true;

  return BC_STATUS_SUCCESS;
}

static BcStatus bc_parse_auto(BcParse *parse) {

  BcLexTokenType type;
  BcStatus status;
  bool comma;
  char *name;
  bool var;
  bool one;
  BcFunc *func;

  if (!parse->auto_part) return BC_STATUS_PARSE_INVALID_TOKEN;

  parse->auto_part = false;

  status = bc_lex_next(&parse->lex, &parse->token);

  if (status) return status;

  comma = false;
  one = false;

  func = bc_vec_item(&parse->program->funcs, parse->func);

  if (!func) return BC_STATUS_EXEC_UNDEFINED_FUNC;

  type = parse->token.type;

  while (!status && type == BC_LEX_NAME) {

    name = parse->token.string;

    status = bc_lex_next(&parse->lex, &parse->token);

    if (status) return status;

    one = true;

    if (parse->token.type == BC_LEX_LEFT_BRACKET) {

      status = bc_lex_next(&parse->lex, &parse->token);

      if (status) goto err;

      if (parse->token.type != BC_LEX_RIGHT_BRACKET)
        return BC_STATUS_PARSE_INVALID_FUNC;

      status = bc_lex_next(&parse->lex, &parse->token);

      if (status) goto err;

      var = false;
    }
    else var = true;

    if (parse->token.type == BC_LEX_COMMA) {

      comma = true;
      status = bc_lex_next(&parse->lex, &parse->token);

      if (status) goto err;
    }
    else comma = false;

    status = bc_func_insertAuto(func, name, var);

    if (status) goto err;

    type = parse->token.type;
  }

  if (status) return status;

  if (comma) return BC_STATUS_PARSE_INVALID_FUNC;

  if (!one) return BC_STATUS_PARSE_NO_AUTO;

  if (type != BC_LEX_NEWLINE && type != BC_LEX_SEMICOLON)
    return BC_STATUS_PARSE_INVALID_TOKEN;

  return bc_lex_next(&parse->lex, &parse->token);

err:

  free(name);

  return status;
}

static BcStatus bc_parse_semicolonList(BcParse *parse, BcVec *code) {

  BcStatus status;

  status = BC_STATUS_SUCCESS;

  switch (parse->token.type) {

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
      status = bc_lex_next(&parse->lex, &parse->token);
      break;
    }

    case BC_LEX_SEMICOLON:
    {
      status = bc_parse_semicolonListEnd(parse, code);
      break;
    }

    case BC_LEX_EOF:
    {
      if (parse->flags.len > 0) status = BC_STATUS_PARSE_EOF;
      break;
    }

    default:
    {
      status = BC_STATUS_PARSE_INVALID_TOKEN;
      break;
    }
  }

  return status;
}

static BcStatus bc_parse_semicolonListEnd(BcParse *parse, BcVec *code) {

  BcStatus status;

  if (parse->token.type == BC_LEX_SEMICOLON) {

    status = bc_lex_next(&parse->lex, &parse->token);

    if (status) return status;

    status = bc_parse_semicolonList(parse, code);
  }
  else if (parse->token.type == BC_LEX_NEWLINE)
    status = bc_lex_next(&parse->lex, &parse->token);
  else if (parse->token.type == BC_LEX_EOF)
    status = BC_STATUS_SUCCESS;
  else
    status = BC_STATUS_PARSE_INVALID_TOKEN;

  return status;
}

static BcStatus bc_parse_stmt(BcParse *parse, BcVec *code) {

  BcStatus status;
  uint8_t *flag_ptr;

  switch (parse->token.type) {

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
      if (BC_PARSE_IF_END(parse)) {
        status = bc_parse_noElse(parse, code);
        if (status) return status;
      }

      parse->auto_part = false;

      status = bc_parse_expr(parse, code, BC_PARSE_EXPR_PRINT);

      if (status) break;

      status = bc_parse_semicolonListEnd(parse, code);

      break;
    }

    case BC_LEX_NEWLINE:
    {
      status = bc_lex_next(&parse->lex, &parse->token);
      break;
    }

    case BC_LEX_KEY_ELSE:
    {
      parse->auto_part = false;

      status = bc_parse_else(parse, code);

      break;
    }

    case BC_LEX_LEFT_BRACE:
    {
      uint8_t flags;

      if (BC_PARSE_IF_END(parse)) {
        status = bc_parse_noElse(parse, code);
        if (status) return status;
      }

      parse->auto_part = false;

      ++parse->num_braces;

      if (BC_PARSE_HEADER(parse)) {

        flag_ptr = bc_vec_top(&parse->flags);
        flags = *flag_ptr & ~(BC_PARSE_FLAG_HEADER);

        if (flags & BC_PARSE_FLAG_FUNC_INNER)
          status = bc_parse_funcStart(parse);
        else if (flags) status = bc_parse_startBody(parse, code, flags);
        else status = BC_STATUS_PARSE_BUG;
      }
      else status = bc_parse_startBody(parse, code, 0);

      break;
    }

    case BC_LEX_RIGHT_BRACE:
    {
      if (BC_PARSE_IF_END(parse)) {
        status = bc_parse_noElse(parse, code);
        if (status) return status;
      }

      parse->auto_part = false;

      status = bc_parse_rightBrace(parse, code);

      break;
    }

    case BC_LEX_STRING:
    {
      if (BC_PARSE_IF_END(parse)) {
        status = bc_parse_noElse(parse, code);
        if (status) return status;
      }

      parse->auto_part = false;

      status = bc_parse_string(parse, code);

      break;
    }

    case BC_LEX_KEY_AUTO:
    {
      if (BC_PARSE_IF_END(parse)) {
        status = bc_parse_noElse(parse, code);
        if (status) return status;
      }

      status = bc_parse_auto(parse);

      break;
    }

    case BC_LEX_KEY_BREAK:
    case BC_LEX_KEY_CONTINUE:
    {
      if (BC_PARSE_IF_END(parse)) {
        status = bc_parse_noElse(parse, code);
        if (status) return status;
      }

      parse->auto_part = false;

      status = bc_parse_loopExit(parse, code, parse->token.type);

      break;
    }

    case BC_LEX_KEY_FOR:
    {
      if (BC_PARSE_IF_END(parse)) {
        status = bc_parse_noElse(parse, code);
        if (status) return status;
      }

      parse->auto_part = false;

      status = bc_parse_for(parse, code);

      break;
    }

    case BC_LEX_KEY_HALT:
    {
      if (BC_PARSE_IF_END(parse)) {
        status = bc_parse_noElse(parse, code);
        if (status) return status;
      }

      parse->auto_part = false;

      status = bc_vec_pushByte(code, BC_INST_HALT);

      if (status) return status;

      status = bc_lex_next(&parse->lex, &parse->token);

      if (status) return status;

      status = bc_parse_semicolonListEnd(parse, code);

      break;
    }

    case BC_LEX_KEY_IF:
    {
      if (BC_PARSE_IF_END(parse)) {
        status = bc_parse_noElse(parse, code);
        if (status) return status;
      }

      parse->auto_part = false;

      status = bc_parse_if(parse, code);

      break;
    }

    case BC_LEX_KEY_LIMITS:
    {
      if (BC_PARSE_IF_END(parse)) {
        status = bc_parse_noElse(parse, code);
        if (status) return status;
      }

      status = bc_lex_next(&parse->lex, &parse->token);

      if (status && status != BC_STATUS_LEX_EOF) return status;

      status = bc_parse_semicolonListEnd(parse, code);

      if (status && status != BC_STATUS_LEX_EOF) return status;

      status = BC_STATUS_PARSE_LIMITS;

      break;
    }

    case BC_LEX_KEY_PRINT:
    {
      if (BC_PARSE_IF_END(parse)) {
        status = bc_parse_noElse(parse, code);
        if (status) return status;
      }

      parse->auto_part = false;

      status = bc_parse_print(parse, code);

      break;
    }

    case BC_LEX_KEY_QUIT:
    {
      if (BC_PARSE_IF_END(parse)) {
        status = bc_parse_noElse(parse, code);
        if (status) return status;
      }

      parse->auto_part = false;

      // Quit is a compile-time command,
      // so we send an exit command. We
      // don't exit directly, so the vm
      // can clean up.
      status = BC_STATUS_PARSE_QUIT;
      break;
    }

    case BC_LEX_KEY_RETURN:
    {
      if (BC_PARSE_IF_END(parse)) {
        status = bc_parse_noElse(parse, code);
        if (status) return status;
      }

      parse->auto_part = false;

      status = bc_parse_return(parse, code);

      if (status) return status;

      status = bc_parse_semicolonListEnd(parse, code);

      break;
    }

    case BC_LEX_KEY_WHILE:
    {
      if (BC_PARSE_IF_END(parse)) {
        status = bc_parse_noElse(parse, code);
        if (status) return status;
      }

      parse->auto_part = false;

      status = bc_parse_while(parse, code);

      break;
    }

    default:
    {
      if (BC_PARSE_IF_END(parse)) {
        status = bc_parse_noElse(parse, code);
        if (status) return status;
      }

      parse->auto_part = false;

      status = BC_STATUS_PARSE_INVALID_TOKEN;

      break;
    }
  }

  return status;
}

BcStatus bc_parse_init(BcParse *parse, BcProgram *program) {

  BcStatus status;

  if (parse == NULL || program == NULL) return BC_STATUS_INVALID_PARAM;

  status = bc_vec_init(&parse->flags, sizeof(uint8_t), NULL);

  if (status != BC_STATUS_SUCCESS) return status;

  status = bc_vec_init(&parse->exit_labels, sizeof(BcInstPtr), NULL);

  if (status) goto exit_label_err;

  status = bc_vec_init(&parse->cond_labels, sizeof(size_t), NULL);

  if (status) goto cond_label_err;

  uint8_t flags = 0;

  status = bc_vec_push(&parse->flags, &flags);

  if (status != BC_STATUS_SUCCESS) goto push_err;

  status = bc_vec_init(&parse->ops, sizeof(BcLexTokenType), NULL);

  if (status) goto push_err;

  parse->program = program;
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

BcStatus bc_parse_file(BcParse *parse, const char *file) {

  if (parse == NULL || file == NULL) return BC_STATUS_INVALID_PARAM;

  return bc_lex_init(&parse->lex, file);
}

BcStatus bc_parse_text(BcParse *parse, const char *text) {

  BcStatus status;

  if (parse == NULL || text == NULL) return BC_STATUS_INVALID_PARAM;

  status = bc_lex_text(&parse->lex, text);

  if (status) return status;

  return bc_lex_next(&parse->lex, &parse->token);
}

BcStatus bc_parse_parse(BcParse *parse) {

  BcStatus status;

  if (parse == NULL) return BC_STATUS_INVALID_PARAM;

  switch (parse->token.type) {

    case BC_LEX_NEWLINE:
    {
      status = bc_lex_next(&parse->lex, &parse->token);
      break;
    }

    case BC_LEX_KEY_DEFINE:
    {
      if (!BC_PARSE_CAN_EXEC(parse))
        return BC_STATUS_PARSE_INVALID_TOKEN;

      status = bc_parse_func(parse);

      break;
    }

    case BC_LEX_EOF:
    {
      status = BC_STATUS_PARSE_EOF;
      break;
    }

    default:
    {
      BcFunc *func;

      func = bc_vec_item(&parse->program->funcs, parse->func);

      if (!func) return BC_STATUS_EXEC_UNDEFINED_FUNC;

      status = bc_parse_stmt(parse, &func->code);

      break;
    }
  }

  return status;
}

void bc_parse_free(BcParse *parse) {

  if (!parse) {
    return;
  }

  bc_vec_free(&parse->flags);
  bc_vec_free(&parse->exit_labels);
  bc_vec_free(&parse->cond_labels);
  bc_vec_free(&parse->ops);

  switch (parse->token.type) {

    case BC_LEX_STRING:
    case BC_LEX_NAME:
    case BC_LEX_NUMBER:
    {
      if (parse->token.string) {
        free(parse->token.string);
      }

      break;
    }

    default:
    {
      // We don't have have to free anything.
      break;
    }
  }
}

BcStatus bc_parse_expr(BcParse *parse, BcVec *code, uint8_t flags) {

  BcStatus status;
  uint32_t nexprs;
  uint32_t num_parens;
  uint32_t ops_start_len;
  bool paren_first;
  bool paren_expr;
  bool rparen;
  bool done;
  bool get_token;
  uint32_t num_rel_ops;
  BcExprType prev;
  BcLexTokenType type;
  BcLexTokenType *ptr;
  BcLexTokenType top;
  uint8_t inst;

  status = BC_STATUS_SUCCESS;

  ops_start_len = parse->ops.len;

  prev = BC_EXPR_PRINT;

  paren_first = parse->token.type == BC_LEX_LEFT_PAREN;

  nexprs = 0;
  num_parens = 0;
  paren_expr = false;
  rparen = false;
  done = false;
  get_token = false;
  num_rel_ops = 0;

  type = parse->token.type;

  while (!status && !done && bc_token_exprs[type]) {

    switch (type) {

      case BC_LEX_OP_INC:
      case BC_LEX_OP_DEC:
      {
        status = bc_parse_incdec(parse, code, &prev, flags);
        rparen = false;
        get_token = false;
        break;
      }

      case BC_LEX_OP_MINUS:
      {
        status = bc_parse_minus(parse, code, &parse->ops, &prev,
                                rparen, &nexprs);
        rparen = false;
        get_token = false;
        break;
      }

      case BC_LEX_OP_ASSIGN_POWER:
      case BC_LEX_OP_ASSIGN_MULTIPLY:
      case BC_LEX_OP_ASSIGN_DIVIDE:
      case BC_LEX_OP_ASSIGN_MODULUS:
      case BC_LEX_OP_ASSIGN_PLUS:
      case BC_LEX_OP_ASSIGN_MINUS:
      case BC_LEX_OP_ASSIGN:
        if (prev != BC_EXPR_VAR && prev != BC_EXPR_ARRAY_ELEM &&
            prev != BC_EXPR_SCALE && prev != BC_EXPR_IBASE &&
            prev != BC_EXPR_OBASE && prev != BC_EXPR_LAST)
        {
          status = BC_STATUS_PARSE_INVALID_ASSIGN;
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
        if (type >= BC_LEX_OP_REL_EQUAL && type <= BC_LEX_OP_REL_GREATER) {
          num_rel_ops += 1;
        }

        prev = BC_PARSE_TOKEN_TO_EXPR(type);
        status = bc_parse_operator(parse, code, &parse->ops,
                                   type, &nexprs, true);
        rparen = false;
        get_token = false;

        break;
      }

      case BC_LEX_LEFT_PAREN:
      {
        ++num_parens;
        paren_expr = false;
        rparen = false;
        get_token = true;
        status = bc_vec_push(&parse->ops, &type);
        break;
      }

      case BC_LEX_RIGHT_PAREN:
      {
        if (num_parens == 0) {
          status = BC_STATUS_SUCCESS;
          done = true;
          get_token = false;
          break;
        }
        else if (!paren_expr) return BC_STATUS_PARSE_INVALID_EXPR;

        --num_parens;
        paren_expr = true;
        rparen = true;
        get_token = false;

        status = bc_parse_rightParen(parse, code, &parse->ops, &nexprs);

        break;
      }

      case BC_LEX_NAME:
      {
        paren_expr = true;
        rparen = false;
        get_token = false;
        status = bc_parse_expr_name(parse, code, &prev,
                                    flags & ~(BC_PARSE_EXPR_NO_CALL));
        ++nexprs;
        break;
      }

      case BC_LEX_NUMBER:
      {
        size_t idx;

        idx = parse->program->constants.len;

        status = bc_vec_push(&parse->program->constants, &parse->token.string);

        if (status) return status;

        status = bc_vec_pushByte(code, BC_INST_PUSH_NUM);

        if (status) return status;

        status = bc_parse_pushIndex(code, idx);

        if (status) return status;

        paren_expr = true;
        rparen = false;
        get_token = true;
        ++nexprs;
        prev = BC_EXPR_NUMBER;

        break;
      }

      case BC_LEX_KEY_IBASE:
      {
        status = bc_vec_pushByte(code, BC_INST_PUSH_IBASE);

        paren_expr = true;
        rparen = false;
        get_token = true;
        ++nexprs;
        prev = BC_EXPR_IBASE;

        break;
      }

      case BC_LEX_KEY_LENGTH:
      case BC_LEX_KEY_SQRT:
      {
        status = bc_parse_builtin(parse, code, type, flags);
        paren_expr = true;
        rparen = false;
        get_token = true;
        ++nexprs;
        prev = type == BC_LEX_KEY_LENGTH ? BC_EXPR_LENGTH : BC_EXPR_SQRT;
        break;
      }

      case BC_LEX_KEY_OBASE:
      {
        status = bc_vec_pushByte(code, BC_INST_PUSH_OBASE);

        paren_expr = true;
        rparen = false;
        get_token = true;
        ++nexprs;
        prev = BC_EXPR_OBASE;

        break;
      }

      case BC_LEX_KEY_READ:
      {
        status = bc_parse_read(parse, code);
        paren_expr = true;
        rparen = false;
        get_token = false;
        ++nexprs;
        prev = BC_EXPR_READ;
        break;
      }

      case BC_LEX_KEY_SCALE:
      {
        status = bc_parse_scale(parse, code, &prev, flags);
        paren_expr = true;
        rparen = false;
        get_token = false;
        ++nexprs;
        prev = BC_EXPR_SCALE;
        break;
      }

      default:
      {
        status = BC_STATUS_PARSE_INVALID_TOKEN;
        break;
      }
    }

    if (get_token) status = bc_lex_next(&parse->lex, &parse->token);

    type = parse->token.type;
  }

  if (status) return status;

  while (!status && parse->ops.len > ops_start_len) {

    ptr = bc_vec_top(&parse->ops);
    top = *ptr;

    if (top == BC_LEX_LEFT_PAREN || top == BC_LEX_RIGHT_PAREN)
      return BC_STATUS_PARSE_INVALID_EXPR;

    inst = bc_op_insts[top - BC_LEX_OP_POWER];

    status = bc_vec_pushByte(code, inst);

    if (status) return status;

    nexprs -= top != BC_LEX_OP_BOOL_NOT && top != BC_LEX_OP_NEGATE ? 1 : 0;

    status = bc_vec_pop(&parse->ops);
  }

  if (nexprs != 1) return BC_STATUS_PARSE_INVALID_EXPR;

  if (!(flags & BC_PARSE_EXPR_POSIX_REL) && num_rel_ops &&
      (status = bc_posix_error(BC_STATUS_POSIX_REL_OUTSIDE,
                               parse->lex.file, parse->lex.line, NULL)))
  {
    return status;
  }
  else if (num_rel_ops > 1 &&
           (status = bc_posix_error(BC_STATUS_POSIX_REL_OUTSIDE,
                                    parse->lex.file, parse->lex.line, NULL)))
  {
    return status;
  }

  inst = *((uint8_t*) bc_vec_top(code));

  if (flags & BC_PARSE_EXPR_PRINT) {
    if (paren_first ||
        (inst != BC_INST_OP_ASSIGN_POWER &&
         inst != BC_INST_OP_ASSIGN_MULTIPLY &&
         inst != BC_INST_OP_ASSIGN_DIVIDE &&
         inst != BC_INST_OP_ASSIGN_MODULUS &&
         inst != BC_INST_OP_ASSIGN_PLUS &&
         inst != BC_INST_OP_ASSIGN_MINUS &&
         inst != BC_INST_OP_ASSIGN))
    {
      status = bc_vec_pushByte(code, BC_INST_PRINT);
    }
    else status = bc_vec_pushByte(code, BC_INST_POP);
  }

  return status;
}
