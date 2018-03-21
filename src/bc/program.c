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
 * Code to execute bc programs.
 *
 */

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <limits.h>
#include <unistd.h>

#include <io.h>
#include <program.h>
#include <parse.h>
#include <instructions.h>
#include <bc.h>

static BcStatus bc_program_search(BcProgram *p, BcResult *result,
                                  BcNum **ret, uint8_t flags)
{
  BcStatus status;
  BcFunc *func;
  BcEntry entry;
  BcVec *vec;
  BcVecO *veco;
  size_t idx, ip_idx;
  BcEntry *entry_ptr;
  BcAuto *a;

  for (ip_idx = 0; ip_idx < p->stack.len - 1; ++ip_idx) {

    BcInstPtr *ip = bc_vec_item_rev(&p->stack, ip_idx);

    if (!ip) return BC_STATUS_EXEC_BAD_STACK;

    if (ip->func == BC_PROGRAM_READ || ip->func == BC_PROGRAM_MAIN) continue;

    func = bc_vec_item(&p->funcs, ip->func);

    if (!func) return BC_STATUS_EXEC_BAD_STACK;

    for (idx = 0; idx < func->autos.len; ++idx) {
      a = bc_vec_item(&func->autos, idx);
      if (!a) return BC_STATUS_EXEC_UNDEFINED_VAR;
      if (!strcmp(a->name, result->data.id.name)) {

        BcResult *r;
        uint8_t cond;

        cond = flags & BC_PROGRAM_SEARCH_VAR;

        if (!a->var != !cond) return BC_STATUS_EXEC_BAD_TYPE;

        r = bc_vec_item(&p->expr_stack, ip->len + idx);

        if (!r) return cond ? BC_STATUS_EXEC_UNDEFINED_VAR :
                              BC_STATUS_EXEC_UNDEFINED_ARRAY;

        if (cond || flags & BC_PROGRAM_SEARCH_ARRAY_ONLY) *ret = &r->data.num;
        else {
          status = bc_array_expand(&r->data.array, result->data.id.idx + 1);
          if (status) return status;
          *ret = bc_vec_item(&r->data.array, result->data.id.idx);
        }

        return BC_STATUS_SUCCESS;
      }
    }
  }

  vec = (flags & BC_PROGRAM_SEARCH_VAR) ? &p->vars : &p->arrays;
  veco = (flags & BC_PROGRAM_SEARCH_VAR) ? &p->var_map : &p->array_map;

  entry.name = result->data.id.name;
  entry.idx = vec->len;

  status = bc_veco_insert(veco, &entry, &idx);

  if (status != BC_STATUS_VEC_ITEM_EXISTS) {

    // We use this because it has a union of BcNum and BcVec.
    BcResult data;
    size_t len;

    if (status) return status;

    len = strlen(entry.name) + 1;

    result->data.id.name = malloc(len);

    if (!result->data.id.name) return BC_STATUS_MALLOC_FAIL;

    strcpy(result->data.id.name, entry.name);

    if (flags & BC_PROGRAM_SEARCH_VAR)
      status = bc_num_init(&data.data.num, BC_NUM_DEF_SIZE);
    else status = bc_vec_init(&data.data.array, sizeof(BcNum), bc_num_free);

    if (status) return status;

    status = bc_vec_push(vec, &data.data);

    if (status) return status;
  }

  entry_ptr = bc_veco_item(veco, idx);

  if (!entry_ptr) return BC_STATUS_VEC_OUT_OF_BOUNDS;

  if (flags & BC_PROGRAM_SEARCH_VAR) {
    *ret = bc_vec_item(vec, entry_ptr->idx);
    if (!(*ret)) return BC_STATUS_EXEC_UNDEFINED_VAR;
  }
  else {

    BcVec *aptr;

    aptr = bc_vec_item(vec, entry_ptr->idx);

    if (!aptr) return BC_STATUS_EXEC_UNDEFINED_ARRAY;

    if (flags & BC_PROGRAM_SEARCH_ARRAY_ONLY) {
      *ret = (BcNum*) aptr;
      return BC_STATUS_SUCCESS;
    }

    status = bc_array_expand(aptr, result->data.id.idx + 1);

    if (status) return status;

    *ret = bc_vec_item(aptr, result->data.id.idx);
  }

  return BC_STATUS_SUCCESS;
}

static BcStatus bc_program_num(BcProgram *p, BcResult *result,
                               BcNum** num, bool hex)
{

  BcStatus status;

  status = BC_STATUS_SUCCESS;

  switch (result->type) {

    case BC_RESULT_INTERMEDIATE:
    case BC_RESULT_SCALE:
    {
      *num = &result->data.num;
      break;
    }

    case BC_RESULT_CONSTANT:
    {
      char** s;
      size_t idx;
      size_t len;
      size_t base;

      idx = result->data.id.idx;

      s = bc_vec_item(&p->constants, idx);

      if (!s) return BC_STATUS_EXEC_BAD_CONSTANT;

      len = strlen(*s);

      status = bc_num_init(&result->data.num, len);

      if (status) return status;

      base = hex && len == 1 ? 16 : p->ibase_t;

      status = bc_num_parse(&result->data.num, *s, &p->ibase, base);

      if (status) {
        bc_num_free(&result->data.num);
        return status;
      }

      *num = &result->data.num;

      result->type = BC_RESULT_INTERMEDIATE;

      break;
    }

    case BC_RESULT_VAR:
    case BC_RESULT_ARRAY:
    {
      uint8_t flags = result->type == BC_RESULT_VAR ? BC_PROGRAM_SEARCH_VAR : 0;
      status = bc_program_search(p, result, num, flags);
      break;
    }

    case BC_RESULT_LAST:
    {
      *num = &p->last;
      break;
    }

    case BC_RESULT_IBASE:
    {
      *num = &p->ibase;
      break;
    }

    case BC_RESULT_OBASE:
    {
      *num = &p->obase;
      break;
    }

    case BC_RESULT_ONE:
    {
      *num = &p->one;
      break;
    }

    default:
    {
      status = BC_STATUS_EXEC_BAD_EXPR;
      break;
    }
  }

  return status;
}

static BcStatus bc_program_binaryOpPrep(BcProgram *p, BcResult **left,
                                        BcNum **lval, BcResult **right,
                                        BcNum **rval)
{
  BcStatus status;
  BcResult *l;
  BcResult *r;
  bool hex;

  if (!BC_PROGRAM_CHECK_EXPR_STACK(p, 2)) return BC_STATUS_EXEC_BAD_EXPR;

  r = bc_vec_item_rev(&p->expr_stack, 0);
  l = bc_vec_item_rev(&p->expr_stack, 1);

  if (!r || !l) return BC_STATUS_EXEC_BAD_EXPR;

  status = bc_program_num(p, l, lval, false);

  if (status) return status;

  hex = l->type == BC_RESULT_IBASE || l->type == BC_RESULT_OBASE;
  status = bc_program_num(p, r, rval, hex);

  if (status) return status;

  *left = l;
  *right = r;

  return BC_STATUS_SUCCESS;
}

static BcStatus bc_program_binaryOpRetire(BcProgram *p, BcResult *result,
                                          BcResultType type)
{
  BcStatus status;

  result->type = type;

  status = bc_vec_pop(&p->expr_stack);

  if (status) return status;

  status = bc_vec_pop(&p->expr_stack);

  if (status) return status;

  return bc_vec_push(&p->expr_stack, result);
}

static BcStatus bc_program_unaryOpPrep(BcProgram *p, BcResult **result,
                                       BcNum **val)
{
  BcStatus status;
  BcResult *r;

  if (!BC_PROGRAM_CHECK_EXPR_STACK(p, 1)) return BC_STATUS_EXEC_BAD_EXPR;

  r = bc_vec_item_rev(&p->expr_stack, 0);

  if (!r) return BC_STATUS_EXEC_BAD_EXPR;

  status = bc_program_num(p, r, val, false);

  if (status) return status;

  *result = r;

  return BC_STATUS_SUCCESS;
}

static BcStatus bc_program_unaryOpRetire(BcProgram *p, BcResult *result,
                                         BcResultType type)
{
  BcStatus status;

  result->type = type;

  status = bc_vec_pop(&p->expr_stack);

  if (status) return status;

  return bc_vec_push(&p->expr_stack, result);
}

static BcStatus bc_program_op(BcProgram *p, uint8_t inst) {

  BcStatus status;
  BcResult *operand1;
  BcResult *operand2;
  BcResult result;
  BcNum *num1;
  BcNum *num2;

  status = bc_program_binaryOpPrep(p, &operand1, &num1, &operand2, &num2);

  if (status) return status;

  status = bc_num_init(&result.data.num, BC_NUM_DEF_SIZE);

  if (status) return status;

  if (inst != BC_INST_OP_POWER) {
    BcNumBinaryFunc op = bc_program_math_ops[inst - BC_INST_OP_MODULUS];
    status = op(num1, num2, &result.data.num, p->scale);
  }
  else status = bc_num_pow(num1, num2, &result.data.num, p->scale);

  if (status) goto err;

  status = bc_program_binaryOpRetire(p, &result, BC_RESULT_INTERMEDIATE);

  if (status) goto err;

  return status;

err:

  bc_num_free(&result.data.num);

  return status;
}

static BcStatus bc_program_read(BcProgram *p) {

  BcStatus status;
  BcParse parse;
  char *buffer;
  size_t size;
  BcFunc *func;
  BcInstPtr ip;

  func = bc_vec_item(&p->funcs, BC_PROGRAM_READ);

  if (!func) return BC_STATUS_EXEC_UNDEFINED_FUNC;

  func->code.len = 0;

  buffer = malloc(BC_PROGRAM_BUF_SIZE + 1);

  if (!buffer) return BC_STATUS_MALLOC_FAIL;

  size = BC_PROGRAM_BUF_SIZE;

  status = bc_io_getline(&buffer, &size);

  if (status) goto io_err;

  status = bc_parse_init(&parse, p);

  if (status) goto io_err;

  status = bc_parse_file(&parse, "<stdin>");

  if (status) goto exec_err;

  status = bc_parse_text(&parse, buffer);

  if (status) goto exec_err;

  status = bc_parse_expr(&parse, &func->code, BC_PARSE_EXPR_NO_READ);

  if (status != BC_STATUS_LEX_EOF && parse.token.type != BC_LEX_NEWLINE) {
    status = status ? status : BC_STATUS_EXEC_BAD_READ_EXPR;
    goto exec_err;
  }

  ip.func = BC_PROGRAM_READ;
  ip.idx = 0;
  ip.len = p->expr_stack.len;

  status = bc_vec_push(&p->stack, &ip);

  if (status) goto exec_err;

  status = bc_program_exec(p);

  if (status) goto exec_err;

  status = bc_vec_pop(&p->stack);

exec_err:

  bc_parse_free(&parse);

io_err:

  free(buffer);

  return status;
}

static size_t bc_program_index(uint8_t *code, size_t *start) {

  uint8_t bytes, byte, i;
  size_t result;

  bytes = code[(*start)++];

  result = 0;

  for (i = 0; i < bytes; ++i) {
    byte = code[(*start)++];
    result |= (((size_t) byte) << (i * CHAR_BIT));
  }

  return result;
}

static char* bc_program_name(uint8_t *code, size_t *start) {

  char byte;
  char *s;
  char *string;
  char *ptr;
  size_t len;
  size_t i;

  string = (char*) (code + *start);

  ptr = strchr((char*) string, ':');

  if (ptr) len = ((unsigned long) ptr) - ((unsigned long) string);
  else len = strlen(string);

  s = malloc(len + 1);

  if (!s) return NULL;

  byte = code[(*start)++];
  i = 0;

  while (byte && byte != ':') {
    s[i++] = byte;
    byte = code[(*start)++];
  }

  s[i] = '\0';

  return s;
}

static BcStatus bc_program_printIndex(uint8_t *code, size_t *start) {

  uint8_t bytes, byte, i;

  bytes = code[(*start)++];
  byte = 1;

  if (printf(bc_program_byte_fmt, bytes) < 0) return BC_STATUS_IO_ERR;

  for (i = 0; byte && i < bytes; ++i) {
    byte = code[(*start)++];
    if (printf(bc_program_byte_fmt, byte) < 0) return BC_STATUS_IO_ERR;
  }

  return BC_STATUS_SUCCESS;
}

static BcStatus bc_program_printName(uint8_t *code, size_t *start) {

  BcStatus status;
  char byte;

  status = BC_STATUS_SUCCESS;
  byte = code[(*start)++];

  while (byte && byte != ':') {
    if (putchar(byte) == EOF) return BC_STATUS_IO_ERR;
    byte = code[(*start)++];
  }

  if (byte) {
    if (putchar(byte) == EOF) status = BC_STATUS_IO_ERR;
  }
  else status = BC_STATUS_PARSE_BUG;

  return status;
}

static BcStatus bc_program_printString(const char *str, size_t *nchars) {

  char c;
  char c2;
  size_t len, i;
  int err;

  err = 0;

  len = strlen(str);

  for (i = 0; i < len; ++i,  ++(*nchars)) {

    c = str[i];

    if (c != '\\') err = fputc(c, stdout);
    else {

      ++i;

      if (i >= len) return BC_STATUS_EXEC_BAD_STRING;

      c2 = str[i];

      switch (c2) {

        case 'a':
        {
          err = fputc('\a', stdout);
          break;
        }

        case 'b':
        {
          err = fputc('\b', stdout);
          break;
        }

        case 'e':
        {
          err = fputc('\\', stdout);
          break;
        }

        case 'f':
        {
          err = fputc('\f', stdout);
          break;
        }

        case 'n':
        {
          err = fputc('\n', stdout);
          *nchars = SIZE_MAX;
          break;
        }

        case 'r':
        {
          err = fputc('\r', stdout);
          break;
        }

        case 'q':
        {
          fputc('"', stdout);
          break;
        }

        case 't':
        {
          err = fputc('\t', stdout);
          break;
        }

        default:
        {
          // Do nothing.
          err = 0;
          break;
        }
      }
    }

    if (err == EOF) return BC_STATUS_IO_ERR;
  }

  return BC_STATUS_SUCCESS;
}

static BcStatus bc_program_push(BcProgram *p, uint8_t *code,
                                size_t *start, bool var)
{
  BcStatus status;
  BcResult result;
  char *s;

  s = bc_program_name(code, start);

  if (!s) return BC_STATUS_EXEC_UNDEFINED_VAR;

  result.data.id.name = s;

  if (var) {
    result.type = BC_RESULT_VAR;
    status = bc_vec_push(&p->expr_stack, &result);
  }
  else {

    BcResult *operand;
    BcNum *num;
    unsigned long temp;

    status = bc_program_unaryOpPrep(p, &operand, &num);

    if (status) goto err;

    status = bc_num_ulong(num, &temp);

    if (status) goto err;

    if (temp > (unsigned long) p->dim_max) {
      status = BC_STATUS_EXEC_ARRAY_LEN;
      goto err;
    }

    result.data.id.idx = (size_t) temp;

    status = bc_program_unaryOpRetire(p, &result, BC_RESULT_ARRAY);
  }

  if (status) goto err;

  return status;

err:

  free(s);

  return status;
}

static BcStatus bc_program_negate(BcProgram *p) {

  BcStatus status;
  BcResult result;
  BcResult *ptr;
  BcNum *num;

  status = bc_program_unaryOpPrep(p, &ptr, &num);

  if (status) return status;

  status = bc_num_init(&result.data.num, num->len);

  if (status) return status;

  status = bc_num_copy(&result.data.num, num);

  if (status) goto err;

  result.data.num.neg = !result.data.num.neg;

  status = bc_program_unaryOpRetire(p, &result, BC_RESULT_INTERMEDIATE);

  if (status) goto err;

  return status;

err:

  bc_num_free(&result.data.num);

  return status;
}

static BcStatus bc_program_logical(BcProgram *p, uint8_t inst) {

  BcStatus status;
  BcResult *operand1;
  BcResult *operand2;
  BcNum *num1;
  BcNum *num2;
  BcResult result;
  BcNumInitFunc init;
  bool cond;
  int cmp;

  status = bc_program_binaryOpPrep(p, &operand1, &num1, &operand2, &num2);

  if (status) return status;

  status = bc_num_init(&result.data.num, BC_NUM_DEF_SIZE);

  if (status) return status;

  if (inst == BC_INST_OP_BOOL_AND)
    cond = bc_num_cmp(num1, &p->zero, NULL) && bc_num_cmp(num2, &p->zero, NULL);
  else if (inst == BC_INST_OP_BOOL_OR)
    cond = bc_num_cmp(num1, &p->zero, NULL) || bc_num_cmp(num2, &p->zero, NULL);
  else {

    cmp = bc_num_cmp(num1, num2, NULL);

    switch (inst) {
      case BC_INST_OP_REL_EQUAL:
      {
        cond = cmp == 0;
        break;
      }

      case BC_INST_OP_REL_LESS_EQ:
      {
        cond = cmp <= 0;
        break;
      }

      case BC_INST_OP_REL_GREATER_EQ:
      {
        cond = cmp >= 0;
        break;
      }

      case BC_INST_OP_REL_NOT_EQ:
      {
        cond = cmp != 0;
        break;
      }

      case BC_INST_OP_REL_LESS:
      {
        cond = cmp < 0;
        break;
      }

      case BC_INST_OP_REL_GREATER:
      {
        cond = cmp > 0;
        break;
      }

      default:
      {
        return BC_STATUS_EXEC_BAD_EXPR;
      }
    }
  }

  init = cond ? bc_num_one : bc_num_zero;
  init(&result.data.num);

  status = bc_program_binaryOpRetire(p, &result, BC_RESULT_INTERMEDIATE);

  if (status) goto err;

  return status;

err:

  bc_num_free(&result.data.num);

  return status;
}

static BcNumBinaryFunc bc_program_assignOp(uint8_t inst) {

  switch (inst) {

    case BC_INST_OP_ASSIGN_POWER:
    {
      return bc_num_pow;
    }

    case BC_INST_OP_ASSIGN_MULTIPLY:
    {
      return bc_num_mul;
    }

    case BC_INST_OP_ASSIGN_DIVIDE:
    {
      return bc_num_div;
    }

    case BC_INST_OP_ASSIGN_MODULUS:
    {
      return bc_num_mod;
    }

    case BC_INST_OP_ASSIGN_PLUS:
    {
      return bc_num_add;
    }

    case BC_INST_OP_ASSIGN_MINUS:
    {
      return bc_num_sub;
    }

    default:
    {
      return NULL;
    }
  }
}

static BcStatus bc_program_assignScale(BcProgram *p, BcNum *scale,
                                       BcNum *rval, uint8_t inst)
{
  BcStatus status;
  unsigned long result;

  switch (inst) {

    case BC_INST_OP_ASSIGN_POWER:
    case BC_INST_OP_ASSIGN_MULTIPLY:
    case BC_INST_OP_ASSIGN_DIVIDE:
    case BC_INST_OP_ASSIGN_MODULUS:
    case BC_INST_OP_ASSIGN_PLUS:
    case BC_INST_OP_ASSIGN_MINUS:
    {
      BcNumBinaryFunc op = bc_program_assignOp(inst);
      status = op(scale, rval, scale, p->scale);
      if (status) return status;
      break;
    }

    case BC_INST_OP_ASSIGN:
    {
      status = bc_num_copy(scale, rval);
      if (status) return status;
      break;
    }

    default:
    {
      return BC_STATUS_EXEC_BAD_EXPR;
    }
  }

  status = bc_num_ulong(scale, &result);

  if (status) return status;

  if (result > (unsigned long) p->scale_max) return BC_STATUS_EXEC_BAD_SCALE;

  p->scale = (size_t) result;

  return status;
}

static BcStatus bc_program_assign(BcProgram *p, uint8_t inst) {

  BcStatus status;
  BcResult *left;
  BcResult *right;
  BcResult result;
  BcNum *lval;
  BcNum *rval;

  status = bc_program_binaryOpPrep(p, &left, &lval, &right, &rval);

  if (status) return status;

  if (left->type == BC_RESULT_CONSTANT || left->type == BC_RESULT_INTERMEDIATE)
    return BC_STATUS_PARSE_BAD_ASSIGN;

  if (inst == BC_EXPR_ASSIGN_DIVIDE && !bc_num_cmp(rval, &p->zero, NULL))
    return BC_STATUS_MATH_DIVIDE_BY_ZERO;

  if (left->type != BC_RESULT_SCALE) {

    switch (inst) {

      case BC_INST_OP_ASSIGN_POWER:
      case BC_INST_OP_ASSIGN_MULTIPLY:
      case BC_INST_OP_ASSIGN_DIVIDE:
      case BC_INST_OP_ASSIGN_MODULUS:
      case BC_INST_OP_ASSIGN_PLUS:
      case BC_INST_OP_ASSIGN_MINUS:
      {
        BcNumBinaryFunc op;

        op = bc_program_assignOp(inst);
        status = op(lval, rval, lval, p->scale);

        break;
      }

      case BC_INST_OP_ASSIGN:
      {
        status = bc_num_copy(lval, rval);
        break;
      }

      default:
      {
        status = BC_STATUS_EXEC_BAD_EXPR;
        break;
      }
    }

    if (status) return status;

    if (left->type == BC_RESULT_IBASE || left->type == BC_RESULT_OBASE) {

      long base, max;
      size_t *ptr;

      ptr = left->type == BC_RESULT_IBASE ? &p->ibase_t : &p->obase_t;

      status = bc_num_long(lval, &base);

      if (status) return status;

      max = left->type == BC_RESULT_IBASE ? BC_NUM_MAX_INPUT_BASE : p->base_max;

      if (base < BC_NUM_MIN_BASE || base > max)
        return left->type - BC_RESULT_IBASE + BC_STATUS_EXEC_BAD_IBASE;

      *ptr = (size_t) base;
    }
  }
  else {
    status = bc_program_assignScale(p, lval, rval, inst);
    if (status) return status;
  }

  status = bc_num_init(&result.data.num, lval->len);

  if (status) return status;

  status = bc_num_copy(&result.data.num, lval);

  if (status) goto err;

  status = bc_program_binaryOpRetire(p, &result, BC_RESULT_INTERMEDIATE);

  if (status) goto err;

  return status;

err:

  bc_num_free(&result.data.num);

  return status;
}

static BcStatus bc_program_call(BcProgram *p, uint8_t *code, size_t *idx) {

  BcStatus status;
  BcInstPtr ip;
  size_t nparams, i;
  BcFunc *func;
  BcAuto *auto_ptr;
  BcResult param;
  BcResult *arg;

  nparams = bc_program_index(code, idx);

  ip.idx = 0;
  ip.len = p->expr_stack.len;

  ip.func = bc_program_index(code, idx);

  func = bc_vec_item(&p->funcs, ip.func);

  if (!func || !func->code.len) return BC_STATUS_EXEC_UNDEFINED_FUNC;

  if (nparams != func->num_params) return BC_STATUS_EXEC_MISMATCHED_PARAMS;

  for (i = 0; i < nparams; ++i) {

    auto_ptr = bc_vec_item(&func->autos, i);
    arg = bc_vec_item_rev(&p->expr_stack, nparams - 1);

    if (!auto_ptr || !arg) return BC_STATUS_EXEC_UNDEFINED_VAR;

    param.type = auto_ptr->var + BC_RESULT_ARRAY_AUTO;

    if (auto_ptr->var) {

      BcNum *num;

      status = bc_program_num(p, arg, &num, false);

      if (status) return status;

      status = bc_num_init(&param.data.num, num->len);

      if (status) return status;

      status = bc_num_copy(&param.data.num, num);
    }
    else {

      BcVec *array;

      if (arg->type != BC_RESULT_VAR || arg->type != BC_RESULT_ARRAY)
        return BC_STATUS_EXEC_BAD_TYPE;

      status = bc_program_search(p, arg, (BcNum**) &array,
                                 BC_PROGRAM_SEARCH_ARRAY_ONLY);

      if (status) return status;

      status = bc_vec_init(&param.data.array, sizeof(BcNum), bc_num_free);

      if (status) return status;

      status = bc_array_copy(&param.data.array, array);
    }

    if (status) goto err;

    status = bc_vec_push(&p->expr_stack, &param);

    if (status) goto err;
  }

  for (; i < func->autos.len; ++i) {

    auto_ptr = bc_vec_item_rev(&func->autos, i);

    if (!auto_ptr) return BC_STATUS_EXEC_UNDEFINED_VAR;

    param.type = auto_ptr->var + BC_RESULT_ARRAY_AUTO;

    if (auto_ptr->var) status = bc_num_init(&param.data.num, BC_NUM_DEF_SIZE);
    else status = bc_vec_init(&param.data.array, sizeof(BcNum), bc_num_free);

    if (status) return status;

    status = bc_vec_push(&p->expr_stack, &param);

    if (status) goto err;
  }

  return bc_vec_push(&p->stack, &ip);

err:

  bc_result_free(&param);

  return status;
}

static BcStatus bc_program_return(BcProgram *p, uint8_t inst) {

  BcStatus status;
  BcResult result;
  BcResult *operand;
  size_t req;
  BcInstPtr *ip;
  BcFunc *func;

  if (!BC_PROGRAM_CHECK_STACK(p)) return BC_STATUS_EXEC_BAD_RETURN;

  ip = bc_vec_top(&p->stack);

  if (!ip) return BC_STATUS_EXEC_BAD_STACK;

  req = ip->len + (inst == BC_INST_RETURN ? 1 : 0);

  if (!BC_PROGRAM_CHECK_EXPR_STACK(p, req))
    return BC_STATUS_EXEC_BAD_EXPR;

  func = bc_vec_item(&p->funcs, ip->func);

  if (!func) return BC_STATUS_EXEC_BAD_STMT;

  result.type = BC_RESULT_INTERMEDIATE;

  if (inst == BC_INST_RETURN) {

    BcNum *num;

    operand = bc_vec_top(&p->expr_stack);

    if (!operand) return BC_STATUS_EXEC_BAD_EXPR;

    status = bc_program_num(p, operand, &num, false);

    if (status) return status;

    status = bc_num_init(&result.data.num, num->len);

    if (status) return status;

    status = bc_num_copy(&result.data.num, num);

    if (status) goto err;
  }
  else {

    status = bc_num_init(&result.data.num, BC_NUM_DEF_SIZE);

    if (status) return status;

    bc_num_zero(&result.data.num);
  }

  // We need to pop arguments as well, so this takes that into account.
  status = bc_vec_npop(&p->expr_stack, p->expr_stack.len - (ip->len - func->num_params));
  if (status) goto err;

  status = bc_vec_push(&p->expr_stack, &result);

  if (status) goto err;

  return bc_vec_pop(&p->stack);

err:

  bc_num_free(&result.data.num);

  return status;
}

static unsigned long bc_program_scale(BcNum *n) {
  return (unsigned long) n->rdx;
}

static unsigned long bc_program_length(BcNum *n) {

  unsigned long len = n->len;

  if (n->rdx == n->len) {
    size_t i;
    for (i = n->len - 1; i < n->len && !n->num[i]; --len, --i);
  }

  return len;
}

static BcStatus bc_program_builtin(BcProgram *p, uint8_t inst) {

  BcStatus status;
  BcResult *operand;
  BcNum *num1;
  BcResult result;

  status = bc_program_unaryOpPrep(p, &operand, &num1);

  if (status) return status;

  status = bc_num_init(&result.data.num, BC_NUM_DEF_SIZE);

  if (status) return status;

  if (inst == BC_INST_SQRT) {
    status = bc_num_sqrt(num1, &result.data.num, p->scale);
  }
  else {

    BcProgramBuiltInFunc func;
    unsigned long ans;

    func = inst == BC_INST_LENGTH ? bc_program_length : bc_program_scale;

    ans = func(num1);

    status = bc_num_ulong2num(&result.data.num, ans);
  }

  if (status) goto err;

  status = bc_program_unaryOpRetire(p, &result, BC_RESULT_INTERMEDIATE);

  if (status) goto err;

  return status;

err:

  bc_num_free(&result.data.num);

  return status;
}

static BcStatus bc_program_pushScale(BcProgram *p) {

  BcStatus status;
  BcResult result;

  result.type = BC_RESULT_SCALE;
  status = bc_num_init(&result.data.num, BC_NUM_DEF_SIZE);

  if (status) return status;

  status = bc_num_ulong2num(&result.data.num, (unsigned long) p->scale);

  if (status) goto err;

  status = bc_vec_push(&p->expr_stack, &result);

  if (status) goto err;

  return status;

err:

  bc_num_free(&result.data.num);

  return status;
}

static BcStatus bc_program_incdec(BcProgram *p, uint8_t inst) {

  BcStatus status;
  BcResult *ptr;
  BcNum *num;
  BcResult copy;
  uint8_t inst2;
  BcResult result;

  status = bc_program_unaryOpPrep(p, &ptr, &num);

  if (status) return status;

  inst2 = inst == BC_INST_INC || inst == BC_INST_INC_DUP ?
            BC_INST_OP_ASSIGN_PLUS : BC_INST_OP_ASSIGN_MINUS;

  if (inst == BC_INST_INC_DUP || inst == BC_INST_DEC_DUP) {
    copy.type = BC_RESULT_INTERMEDIATE;
    status = bc_num_init(&copy.data.num, num->len);
    if (status) return status;
  }

  result.type = BC_RESULT_ONE;

  status = bc_vec_push(&p->expr_stack, &result);

  if (status) goto err;

  status = bc_program_assign(p, inst2);

  if (status) goto err;

  if (inst == BC_INST_INC_DUP || inst == BC_INST_DEC_DUP) {

    status = bc_vec_pop(&p->expr_stack);

    if (status) goto err;

    status = bc_vec_push(&p->expr_stack, &copy);

    if (status) goto err;
  }

  return status;

err:

  if (inst == BC_INST_INC_DUP || inst == BC_INST_DEC_DUP)
    bc_num_free(&copy.data.num);

  return status;
}

BcStatus bc_program_init(BcProgram *p) {

  BcStatus s;
  size_t idx;
  char *name;
  char *read_name;
  BcInstPtr ip;

  if (!p) return BC_STATUS_INVALID_ARG;

  name = NULL;

  p->nchars = 0;

#ifdef _POSIX_BC_BASE_MAX
  p->base_max = _POSIX_BC_BASE_MAX;
#elif defined(_BC_BASE_MAX)
  p->base_max = _BC_BASE_MAX;
#else
  errno = 0;
  p->base_max = sysconf(_SC_BC_BASE_MAX);

  if (p->base_max == -1) {
    if (errno) return BC_STATUS_NO_LIMIT;
    p->base_max = BC_BASE_MAX_DEF;
  }
  else if (p->base_max > BC_BASE_MAX_DEF) return BC_STATUS_BAD_LIMIT;
  else p->base_max = BC_BASE_MAX_DEF;
#endif

#ifdef _POSIX_BC_DIM_MAX
  p->dim_max = _POSIX_BC_DIM_MAX;
#elif defined(_BC_DIM_MAX)
  p->dim_max = _BC_DIM_MAX;
#else
  errno = 0;
  p->dim_max = sysconf(_SC_BC_DIM_MAX);

  if (p->dim_max == -1) {
    if (errno) return BC_STATUS_NO_LIMIT;
    p->dim_max = BC_DIM_MAX_DEF;
  }
  else if (p->dim_max > BC_DIM_MAX_DEF) return BC_STATUS_BAD_LIMIT;
  else p->dim_max = BC_DIM_MAX_DEF;
#endif

#ifdef _POSIX_BC_SCALE_MAX
  p->scale_max = _POSIX_BC_SCALE_MAX;
#elif defined(_BC_SCALE_MAX)
  p->scale_max = _BC_SCALE_MAX;
#else
  errno = 0;
  p->scale_max = sysconf(_SC_BC_SCALE_MAX);

  if (p->scale_max == -1) {
    if (errno) return BC_STATUS_NO_LIMIT;
    p->scale_max = BC_SCALE_MAX_DEF;
  }
  else if (p->scale_max > BC_SCALE_MAX_DEF) return BC_STATUS_BAD_LIMIT;
  else p->scale_max = BC_SCALE_MAX_DEF;
#endif

#ifdef _POSIX_BC_STRING_MAX
  p->string_max = _POSIX_BC_STRING_MAX;
#elif defined(_BC_STRING_MAX)
  p->string_max = _BC_STRING_MAX;
#else
  errno = 0;
  p->string_max = sysconf(_SC_BC_STRING_MAX);

  if (p->string_max == -1) {
    if (errno) return BC_STATUS_NO_LIMIT;
    p->string_max = BC_STRING_MAX_DEF;
  }
  else if (p->string_max > BC_STRING_MAX_DEF) return BC_STATUS_BAD_LIMIT;
  else p->string_max = BC_STRING_MAX_DEF;
#endif

  p->scale = 0;

  s = bc_num_init(&p->ibase, BC_NUM_DEF_SIZE);

  if (s) return s;

  bc_num_ten(&p->ibase);
  p->ibase_t = 10;

  s = bc_num_init(&p->obase, BC_NUM_DEF_SIZE);

  if (s) goto obase_err;

  bc_num_ten(&p->obase);
  p->obase_t = 10;

  s = bc_num_init(&p->last, BC_NUM_DEF_SIZE);

  if (s) goto last_err;

  bc_num_zero(&p->last);

  s = bc_num_init(&p->zero, BC_NUM_DEF_SIZE);

  if (s) goto zero_err;

  bc_num_zero(&p->zero);

  s = bc_num_init(&p->one, BC_NUM_DEF_SIZE);

  if (s) goto one_err;

  bc_num_one(&p->one);

  p->num_buf = malloc(BC_PROGRAM_BUF_SIZE + 1);

  if (!p->num_buf) {
    s = BC_STATUS_MALLOC_FAIL;
    goto num_buf_err;
  }

  p->buf_size = BC_PROGRAM_BUF_SIZE;

  s = bc_vec_init(&p->funcs, sizeof(BcFunc), bc_func_free);

  if (s) goto func_err;

  s = bc_veco_init(&p->func_map, sizeof(BcEntry), bc_entry_free, bc_entry_cmp);

  if (s) goto func_map_err;

  name = malloc(strlen(bc_lang_func_main) + 1);

  if (!name) {
    s = BC_STATUS_MALLOC_FAIL;
    goto name_err;
  }

  strcpy(name, bc_lang_func_main);

  s = bc_program_addFunc(p, name, &idx);

  name = NULL;

  if (s || idx != BC_PROGRAM_MAIN) goto read_err;

  read_name = malloc(strlen(bc_lang_func_read) + 1);

  if (!read_name) {
    s = BC_STATUS_MALLOC_FAIL;
    goto read_err;
  }

  strcpy(read_name, bc_lang_func_read);

  s = bc_program_addFunc(p, read_name, &idx);

  read_name = NULL;

  if (s || idx != BC_PROGRAM_READ) goto var_err;

  s = bc_vec_init(&p->vars, sizeof(BcNum), bc_num_free);

  if (s) goto var_err;

  s = bc_veco_init(&p->var_map, sizeof(BcEntry), bc_entry_free, bc_entry_cmp);

  if (s) goto var_map_err;

  s = bc_vec_init(&p->arrays, sizeof(BcVec), bc_vec_free);

  if (s) goto array_err;

  s = bc_veco_init(&p->array_map, sizeof(BcEntry), bc_entry_free, bc_entry_cmp);

  if (s) goto array_map_err;

  s = bc_vec_init(&p->strings, sizeof(char*), bc_string_free);

  if (s) goto string_err;

  s = bc_vec_init(&p->constants, sizeof(char*), bc_constant_free);

  if (s) goto const_err;

  s = bc_vec_init(&p->expr_stack, sizeof(BcResult), bc_result_free);

  if (s) goto expr_err;

  s = bc_vec_init(&p->stack, sizeof(BcInstPtr), NULL);

  if (s) goto stack_err;

  ip.idx = 0;
  ip.func = 0;
  ip.len = 0;

  s = bc_vec_push(&p->stack, &ip);

  if (s) goto push_err;

  return s;

push_err:

  bc_vec_free(&p->stack);

stack_err:

  bc_vec_free(&p->expr_stack);

expr_err:

  bc_vec_free(&p->constants);

const_err:

  bc_vec_free(&p->strings);

string_err:

  bc_veco_free(&p->array_map);

array_map_err:

  bc_vec_free(&p->arrays);

array_err:

  bc_veco_free(&p->var_map);

var_map_err:

  bc_vec_free(&p->vars);

var_err:

  if (read_name) free(read_name);

read_err:

  if (name) free(name);

name_err:

  bc_veco_free(&p->func_map);

func_map_err:

  bc_vec_free(&p->funcs);

func_err:

  free(p->num_buf);

num_buf_err:

  bc_num_free(&p->one);

one_err:

  bc_num_free(&p->zero);

zero_err:

  bc_num_free(&p->last);

last_err:

  bc_num_free(&p->obase);

obase_err:

  bc_num_free(&p->ibase);

  return s;
}

void bc_program_limits(BcProgram *p) {

  putchar('\n');

  printf("BC_BASE_MAX     = %ld\n", p->base_max);
  printf("BC_DIM_MAX      = %ld\n", p->dim_max);
  printf("BC_SCALE_MAX    = %ld\n", p->scale_max);
  printf("BC_STRING_MAX   = %ld\n", p->string_max);
  printf("Max Exponent    = %ld\n", LONG_MAX);
  printf("Number of Vars  = %zu\n", SIZE_MAX);

  putchar('\n');
}

BcStatus bc_program_addFunc(BcProgram *p, char *name, size_t *idx) {

  BcStatus status;
  BcEntry entry;
  BcEntry *entry_ptr;
  BcFunc f;

  if (!p || !name || !idx) return BC_STATUS_INVALID_ARG;

  entry.name = name;
  entry.idx = p->funcs.len;

  status = bc_veco_insert(&p->func_map, &entry, idx);

  if (status) {
    free(name);
    if (status != BC_STATUS_VEC_ITEM_EXISTS) return status;
  }

  entry_ptr = bc_veco_item(&p->func_map, *idx);

  if (!entry_ptr) return BC_STATUS_EXEC_UNDEFINED_FUNC;

  *idx = entry_ptr->idx;

  if (status == BC_STATUS_VEC_ITEM_EXISTS) {

    BcFunc *func = bc_vec_item(&p->funcs, entry_ptr->idx);

    if (!func) return BC_STATUS_EXEC_UNDEFINED_FUNC;

    status = BC_STATUS_SUCCESS;

    // We need to reset these, so the function can be repopulated.
    func->num_params = 0;
    status = bc_vec_npop(&func->autos, func->autos.len);
    if (status) return status;
    status = bc_vec_npop(&func->code, func->code.len);
    if (status) return status;
    status = bc_vec_npop(&func->labels, func->labels.len);
  }
  else {
    status = bc_func_init(&f);
    if (status) return status;
    status = bc_vec_push(&p->funcs, &f);
    if (status) bc_func_free(&f);
  }

  return status;
}

BcStatus bc_program_exec(BcProgram *p) {

  BcStatus status;
  uint8_t *code;
  size_t idx;
  BcResult result;
  BcFunc *func;
  BcInstPtr *ip;
  bool cond;

  ip = bc_vec_top(&p->stack);

  if (!ip) return BC_STATUS_EXEC_BAD_STACK;

  func = bc_vec_item(&p->funcs, ip->func);

  if (!func) return BC_STATUS_EXEC_BAD_STACK;

  status = BC_STATUS_SUCCESS;

  code = func->code.array;
  cond = false;

  while (!bcg.bc_sig && ip->idx < func->code.len) {

    uint8_t inst;

    inst = code[(ip->idx)++];

    switch (inst) {

      case BC_INST_CALL:
      {
        status = bc_program_call(p, code, &ip->idx);
        break;
      }

      case BC_INST_RETURN:
      case BC_INST_RETURN_ZERO:
      {
        status = bc_program_return(p, inst);
        break;
      }

      case BC_INST_READ:
      {
        status = bc_program_read(p);
        break;
      }

      case BC_INST_JUMP_NOT_ZERO:
      case BC_INST_JUMP_ZERO:
      {
        BcResult *operand;
        BcNum *num;

        status = bc_program_unaryOpPrep(p, &operand, &num);

        if (status) return status;

        cond = bc_num_cmp(num, &p->zero, NULL) == 0;

        status = bc_vec_pop(&p->expr_stack);

        if (status) return status;
      }
      // Fallthrough.
      case BC_INST_JUMP:
      {
        size_t idx;
        size_t *addr;

        idx = bc_program_index(code, &ip->idx);
        addr = bc_vec_item(&func->labels, idx);

        if (!addr) return BC_STATUS_EXEC_BAD_LABEL;

        if (inst == BC_INST_JUMP ||
            (inst == BC_INST_JUMP_ZERO && cond) ||
            (inst == BC_INST_JUMP_NOT_ZERO && !cond))
        {
          ip->idx = *addr;
        }

        break;
      }

      case BC_INST_PUSH_VAR:
      case BC_INST_PUSH_ARRAY:
      {
        status = bc_program_push(p, code, &ip->idx, inst == BC_INST_PUSH_VAR);
        break;
      }

      case BC_INST_PUSH_LAST:
      {
        result.type = BC_RESULT_LAST;
        status = bc_vec_push(&p->expr_stack, &result);
        break;
      }

      case BC_INST_PUSH_SCALE:
      {
        status = bc_program_pushScale(p);
        break;
      }

      case BC_INST_PUSH_IBASE:
      {
        result.type = BC_RESULT_IBASE;
        status = bc_vec_push(&p->expr_stack, &result);
        break;
      }

      case BC_INST_PUSH_OBASE:
      {
        result.type = BC_RESULT_OBASE;
        status = bc_vec_push(&p->expr_stack, &result);
        break;
      }

      case BC_INST_SCALE_FUNC:
      case BC_INST_LENGTH:
      case BC_INST_SQRT:
      {
        status = bc_program_builtin(p, inst);
        break;
      }

      case BC_INST_PUSH_NUM:
      {
        BcResult result;

        result.type = BC_RESULT_CONSTANT;
        result.data.id.idx = bc_program_index(code, &ip->idx);

        status = bc_vec_push(&p->expr_stack, &result);

        break;
      }

      case BC_INST_POP:
      {
        status = bc_vec_pop(&p->expr_stack);
        break;
      }

      case BC_INST_INC_DUP:
      case BC_INST_DEC_DUP:
      case BC_INST_INC:
      case BC_INST_DEC:
      {
        status = bc_program_incdec(p, inst);
        break;
      }

      case BC_INST_HALT:
      {
        status = BC_STATUS_QUIT;
        break;
      }

      case BC_INST_PRINT:
      case BC_INST_PRINT_EXPR:
      {
        BcResult *operand;
        BcNum *num;

        status = bc_program_unaryOpPrep(p, &operand, &num);

        if (status) return status;

        status = bc_num_print(num, &p->obase, p->obase_t,
                              inst == BC_INST_PRINT, &p->nchars);

        if (status) return status;

        status = bc_num_copy(&p->last, num);

        if (status) return status;

        status = bc_vec_pop(&p->expr_stack);

        break;
      }

      case BC_INST_STR:
      {
        const char **string;
        const char *s;
        size_t len;

        idx = bc_program_index(code, &ip->idx);

        if (idx >= p->strings.len) return BC_STATUS_EXEC_BAD_STRING;

        string = bc_vec_item(&p->strings, idx);

        if (!string) return BC_STATUS_EXEC_BAD_STRING;

        s = *string;
        len = strlen(s);

        for (idx = 0; idx < len; ++idx) {
          char c = s[idx];
          if (fputc(c, stdout) == EOF) return BC_STATUS_IO_ERR;
          if (c == '\n') p->nchars = SIZE_MAX;
          ++p->nchars;
        }

        break;
      }

      case BC_INST_PRINT_STR:
      {
        const char **string;

        idx = bc_program_index(code, &ip->idx);

        if (idx >= p->strings.len) return BC_STATUS_EXEC_BAD_STRING;

        string = bc_vec_item(&p->strings, idx);

        if (!string) return BC_STATUS_EXEC_BAD_STRING;

        status = bc_program_printString(*string, &p->nchars);

        break;
      }

      case BC_INST_OP_POWER:
      case BC_INST_OP_MULTIPLY:
      case BC_INST_OP_DIVIDE:
      case BC_INST_OP_MODULUS:
      case BC_INST_OP_PLUS:
      case BC_INST_OP_MINUS:
      {
        status = bc_program_op(p, inst);
        break;
      }

      case BC_INST_OP_REL_EQUAL:
      case BC_INST_OP_REL_LESS_EQ:
      case BC_INST_OP_REL_GREATER_EQ:
      case BC_INST_OP_REL_NOT_EQ:
      case BC_INST_OP_REL_LESS:
      case BC_INST_OP_REL_GREATER:
      {
        status = bc_program_logical(p, inst);
        break;
      }

      case BC_INST_OP_BOOL_NOT:
      {
        BcResult *ptr;
        BcNum *num;

        status = bc_program_unaryOpPrep(p, &ptr, &num);

        if (status) return status;

        status = bc_num_init(&result.data.num, BC_NUM_DEF_SIZE);

        if (status) return status;

        if (bc_num_cmp(num, &p->zero, NULL)) bc_num_one(&result.data.num);
        else bc_num_zero(&result.data.num);

        status = bc_program_unaryOpRetire(p, &result, BC_RESULT_INTERMEDIATE);

        if (status) bc_num_free(&result.data.num);

        break;
      }

      case BC_INST_OP_BOOL_OR:
      case BC_INST_OP_BOOL_AND:
      {
        status = bc_program_logical(p, inst);
        break;
      }

      case BC_INST_OP_NEGATE:
      {
        status = bc_program_negate(p);
        break;
      }

      case BC_INST_OP_ASSIGN_POWER:
      case BC_INST_OP_ASSIGN_MULTIPLY:
      case BC_INST_OP_ASSIGN_DIVIDE:
      case BC_INST_OP_ASSIGN_MODULUS:
      case BC_INST_OP_ASSIGN_PLUS:
      case BC_INST_OP_ASSIGN_MINUS:
      case BC_INST_OP_ASSIGN:
      {
        status = bc_program_assign(p, inst);
        break;
      }

      default:
      {
        status = BC_STATUS_EXEC_BAD_STMT;
        break;
      }
    }

    if (status) return status;

    // We keep getting these because if the size of the
    // stack changes, pointers may end up being invalid.
    ip = bc_vec_top(&p->stack);

    if (!ip) return BC_STATUS_EXEC_BAD_STACK;

    func = bc_vec_item(&p->funcs, ip->func);

    if (!func) return BC_STATUS_EXEC_BAD_STACK;

    code = func->code.array;
  }

  return status;
}

BcStatus bc_program_print(BcProgram *p) {

  BcStatus status;
  BcFunc *func;
  uint8_t *code;
  BcInstPtr ip;
  size_t i;

  status = BC_STATUS_SUCCESS;

  for (i = 0; !status && i < p->funcs.len; ++i) {

    ip.idx = 0;
    ip.func = i;
    ip.len = 0;

    func = bc_vec_item(&p->funcs, ip.func);

    if (!func) return BC_STATUS_EXEC_BAD_STACK;

    code = func->code.array;

    if (printf("func[%zu]: ", ip.func) < 0) return BC_STATUS_IO_ERR;

    while (ip.idx < func->code.len) {

      uint8_t inst;

      inst = code[ip.idx++];

      switch (inst) {

        case BC_INST_PUSH_VAR:
        case BC_INST_PUSH_ARRAY:
        {
          if (putchar(inst) == EOF) return BC_STATUS_IO_ERR;
          status = bc_program_printName(code, &ip.idx);
          break;
        }

        case BC_INST_CALL:
        {
          if (putchar(inst) == EOF) return BC_STATUS_IO_ERR;

          status = bc_program_printIndex(code, &ip.idx);

          if (status) return status;

          status = bc_program_printIndex(code, &ip.idx);

          break;
        }

        case BC_INST_JUMP:
        case BC_INST_JUMP_NOT_ZERO:
        case BC_INST_JUMP_ZERO:
        case BC_INST_PUSH_NUM:
        case BC_INST_STR:
        case BC_INST_PRINT_STR:
        {
          if (putchar(inst) == EOF) return BC_STATUS_IO_ERR;
          bc_program_printIndex(code, &ip.idx);
          break;
        }

        default:
        {
          if (putchar(inst) == EOF) return BC_STATUS_IO_ERR;
          break;
        }
      }
    }

    if (status) return status;

    if (putchar('\n') == EOF) status = BC_STATUS_IO_ERR;
  }

  return status;
}

void bc_program_free(BcProgram *p) {

  if (p == NULL) return;

  free(p->num_buf);

  bc_num_free(&p->ibase);
  bc_num_free(&p->obase);

  bc_vec_free(&p->funcs);
  bc_veco_free(&p->func_map);

  bc_vec_free(&p->vars);
  bc_veco_free(&p->var_map);

  bc_vec_free(&p->arrays);
  bc_veco_free(&p->array_map);

  bc_vec_free(&p->strings);
  bc_vec_free(&p->constants);

  bc_vec_free(&p->expr_stack);
  bc_vec_free(&p->stack);

  bc_num_free(&p->last);
  bc_num_free(&p->zero);
  bc_num_free(&p->one);

  memset(p, 0, sizeof(BcProgram));
}
