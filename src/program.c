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

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <limits.h>
#include <unistd.h>

#include <io.h>
#include <program.h>
#include <parse.h>
#include <bc.h>

BcStatus bc_program_search(BcProgram *p, BcResult *result,
                           BcNum **ret, uint8_t flags)
{
  BcStatus status;
  BcEntry entry, *entry_ptr;
  BcVec *vec;
  BcVecO *veco;
  size_t idx, ip_idx;
  BcAuto *a;
  int var;

  for (ip_idx = 0; ip_idx < p->stack.len - 1; ++ip_idx) {

    BcFunc *func;
    BcInstPtr *ip;

    ip = bc_vec_item_rev(&p->stack, ip_idx);
    if (ip->func == BC_PROGRAM_READ || ip->func == BC_PROGRAM_MAIN) continue;

    func = bc_vec_item(&p->funcs, ip->func);

    for (idx = 0; idx < func->autos.len; ++idx) {

      a = bc_vec_item(&func->autos, idx);

      if (!strcmp(a->name, result->data.id.name)) {

        BcResult *r;
        uint8_t cond;

        cond = flags & BC_PROGRAM_SEARCH_VAR;

        if (!a->var != !cond) return BC_STATUS_EXEC_BAD_TYPE;

        r = bc_vec_item(&p->results, ip->len + idx);

        if (cond || flags & BC_PROGRAM_SEARCH_ARRAY) *ret = &r->data.num;
        else {
          status = bc_array_expand(&r->data.array, result->data.id.idx + 1);
          if (status) return status;
          *ret = bc_vec_item(&r->data.array, result->data.id.idx);
        }

        return BC_STATUS_SUCCESS;
      }
    }
  }

  var = flags & BC_PROGRAM_SEARCH_VAR;
  vec = var ? &p->vars : &p->arrays;
  veco = var ? &p->var_map : &p->array_map;

  entry.name = result->data.id.name;
  entry.idx = vec->len;

  status = bc_veco_insert(veco, &entry, &idx);

  if (status != BC_STATUS_VEC_ITEM_EXISTS) {

    // We use this because it has a union of BcNum and BcVec.
    BcResult data;
    size_t len;

    if (status) return status;

    len = strlen(entry.name) + 1;

    if (!(result->data.id.name = malloc(len))) return BC_STATUS_MALLOC_FAIL;

    strcpy(result->data.id.name, entry.name);

    if (flags & BC_PROGRAM_SEARCH_VAR)
      status = bc_num_init(&data.data.num, BC_NUM_DEF_SIZE);
    else status = bc_vec_init(&data.data.array, sizeof(BcNum), bc_num_free);

    if (status) return status;
    if ((status = bc_vec_push(vec, &data.data))) return status;
  }

  entry_ptr = bc_veco_item(veco, idx);

  if (var) *ret = bc_vec_item(vec, entry_ptr->idx);
  else {

    BcVec *ptr = bc_vec_item(vec, entry_ptr->idx);

    if (flags & BC_PROGRAM_SEARCH_ARRAY) {
      *ret = (BcNum*) ptr;
      return BC_STATUS_SUCCESS;
    }

    if ((status = bc_array_expand(ptr, result->data.id.idx + 1))) return status;

    *ret = bc_vec_item(ptr, result->data.id.idx);
  }

  return BC_STATUS_SUCCESS;
}

BcStatus bc_program_num(BcProgram *p, BcResult *result, BcNum** num, bool hex) {

  BcStatus status = BC_STATUS_SUCCESS;

  switch (result->type) {

    case BC_RESULT_TEMP:
    case BC_RESULT_SCALE:
    {
      *num = &result->data.num;
      break;
    }

    case BC_RESULT_CONSTANT:
    {
      char** s;
      size_t len, base;

      s = bc_vec_item(&p->constants, result->data.id.idx);
      len = strlen(*s);

      if ((status = bc_num_init(&result->data.num, len))) return status;

      base = hex && len == 1 ? BC_NUM_MAX_INPUT_BASE : p->ibase_t;

      if ((status = bc_num_parse(&result->data.num, *s, &p->ibase, base))) {
        bc_num_free(&result->data.num);
        return status;
      }

      *num = &result->data.num;
      result->type = BC_RESULT_TEMP;

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
      // This is here to prevent compiler warnings in release mode.
      *num = &result->data.num;
      assert(false);
      break;
    }
  }

  return status;
}

BcStatus bc_program_binaryOpPrep(BcProgram *p, BcResult **left, BcNum **lval,
                                 BcResult **right, BcNum **rval)
{
  BcStatus status;
  BcResult *l, *r;
  bool hex;

  assert(p && left && lval && right && rval && BC_PROGRAM_CHECK_RESULTS(p, 2));

  r = bc_vec_item_rev(&p->results, 0);
  l = bc_vec_item_rev(&p->results, 1);

  *left = l;
  *right = r;

  hex = l->type == BC_RESULT_IBASE || l->type == BC_RESULT_OBASE;

  if ((status = bc_program_num(p, l, lval, false))) return status;
  if ((status = bc_program_num(p, r, rval, hex))) return status;

  return BC_STATUS_SUCCESS;
}

BcStatus bc_program_binaryOpRetire(BcProgram *p, BcResult *result,
                                   BcResultType type)
{
  result->type = type;
  bc_vec_pop(&p->results);
  bc_vec_pop(&p->results);
  return bc_vec_push(&p->results, result);
}

BcStatus bc_program_unaryOpPrep(BcProgram *p, BcResult **result, BcNum **val) {

  BcStatus status;
  BcResult *r;

  assert(p && result && val && BC_PROGRAM_CHECK_RESULTS(p, 1));

  r = bc_vec_item_rev(&p->results, 0);

  if ((status = bc_program_num(p, r, val, false))) return status;

  *result = r;

  return BC_STATUS_SUCCESS;
}

BcStatus bc_program_unaryOpRetire(BcProgram *p, BcResult *result,
                                  BcResultType type)
{
  result->type = type;
  bc_vec_pop(&p->results);
  return bc_vec_push(&p->results, result);
}

BcStatus bc_program_op(BcProgram *p, uint8_t inst) {

  BcStatus status;
  BcResult *operand1, *operand2, res;
  BcNum *num1, *num2;
  BcNumBinaryFunc op;

  status = bc_program_binaryOpPrep(p, &operand1, &num1, &operand2, &num2);
  if (status) return status;

  if ((status = bc_num_init(&res.data.num, BC_NUM_DEF_SIZE))) return status;

  op = bc_program_math_ops[inst - BC_INST_POWER];
  if ((status = op(num1, num2, &res.data.num, p->scale))) goto err;
  if ((status = bc_program_binaryOpRetire(p, &res, BC_RESULT_TEMP))) goto err;

  return status;

err:
  bc_num_free(&res.data.num);
  return status;
}

BcStatus bc_program_read(BcProgram *p) {

  BcStatus status;
  BcParse parse;
  char *buffer;
  size_t size;
  BcFunc *func;
  BcInstPtr ip;

  func = bc_vec_item(&p->funcs, BC_PROGRAM_READ);
  func->code.len = 0;

  if (!(buffer = malloc(BC_PROGRAM_BUF_SIZE + 1))) return BC_STATUS_MALLOC_FAIL;

  size = BC_PROGRAM_BUF_SIZE;

  if ((status = bc_io_getline(&buffer, &size)))goto io_err;

  if ((status = bc_parse_init(&parse, p))) goto io_err;
  bc_lex_init(&parse.lex, "<stdin>");
  if ((status = bc_lex_text(&parse.lex, buffer))) goto exec_err;

  if ((status = bc_parse_expr(&parse, &func->code, BC_PARSE_EXPR_NOREAD))) return status;

  if (parse.lex.token.type != BC_LEX_NEWLINE &&
      parse.lex.token.type != BC_LEX_EOF)
  {
    status = BC_STATUS_EXEC_BAD_READ_EXPR;
    goto exec_err;
  }

  ip.func = BC_PROGRAM_READ;
  ip.idx = 0;
  ip.len = p->results.len;

  if ((status = bc_vec_push(&p->stack, &ip))) goto exec_err;
  if ((status = bc_program_exec(p))) goto exec_err;

  bc_vec_pop(&p->stack);

exec_err:
  bc_parse_free(&parse);
io_err:
  free(buffer);
  return status;
}

size_t bc_program_index(uint8_t *code, size_t *start) {

  uint8_t bytes, i;
  size_t result;

  for (bytes = code[(*start)++], result = 0, i = 0; i < bytes; ++i)
    result |= (((size_t) code[(*start)++]) << (i * CHAR_BIT));

  return result;
}

char* bc_program_name(uint8_t *code, size_t *start) {

  char byte, *s, *string, *ptr;
  size_t len, i;

  string = (char*) (code + *start);
  ptr = strchr((char*) string, ':');

  if (ptr) len = ((unsigned long) ptr) - ((unsigned long) string);
  else len = strlen(string);

  if (!(s = malloc(len + 1))) return NULL;

  for (byte = code[(*start)++], i = 0; byte && byte != ':'; ++i) {
    s[i] = byte;
    byte = code[(*start)++];
  }

  s[i] = '\0';

  return s;
}

BcStatus bc_program_printString(const char *str, size_t *nchars) {

  char c, c2;
  size_t len, i;
  int err;

  len = strlen(str);

  for (i = 0; i < len; ++i,  ++(*nchars)) {

    if ((c = str[i]) != '\\') err = putchar(c);
    else {

      ++i;
      assert(i < len);
      c2 = str[i];

      switch (c2) {

        case 'a':
        {
          err = putchar('\a');
          break;
        }

        case 'b':
        {
          err = putchar('\b');
          break;
        }

        case 'e':
        {
          err = putchar('\\');
          break;
        }

        case 'f':
        {
          err = putchar('\f');
          break;
        }

        case 'n':
        {
          err = putchar('\n');
          *nchars = SIZE_MAX;
          break;
        }

        case 'r':
        {
          err = putchar('\r');
          break;
        }

        case 'q':
        {
          err = putchar('"');
          break;
        }

        case 't':
        {
          err = putchar('\t');
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

BcStatus bc_program_push(BcProgram *p, uint8_t *code, size_t *start, bool var) {

  BcStatus status;
  BcResult result;

  result.data.id.name = bc_program_name(code, start);

  assert(result.data.id.name);

  if (var) {
    result.type = BC_RESULT_VAR;
    status = bc_vec_push(&p->results, &result);
  }
  else {

    BcResult *operand;
    BcNum *num;
    unsigned long temp;

    if ((status = bc_program_unaryOpPrep(p, &operand, &num))) goto err;
    if ((status = bc_num_ulong(num, &temp))) goto err;

    if (temp > (unsigned long) BC_MAX_DIM) {
      status = BC_STATUS_EXEC_ARRAY_LEN;
      goto err;
    }

    result.data.id.idx = (size_t) temp;

    status = bc_program_unaryOpRetire(p, &result, BC_RESULT_ARRAY);
  }

  if (status) goto err;

  return status;

err:
  free(result.data.id.name);
  return status;
}

BcStatus bc_program_negate(BcProgram *p) {

  BcStatus status;
  BcResult result, *ptr;
  BcNum *num;

  if ((status = bc_program_unaryOpPrep(p, &ptr, &num))) return status;
  if ((status = bc_num_init(&result.data.num, num->len))) return status;
  if ((status = bc_num_copy(&result.data.num, num))) goto err;

  result.data.num.neg = !result.data.num.neg;

  if ((status = bc_program_unaryOpRetire(p, &result, BC_RESULT_TEMP))) goto err;

  return status;

err:
  bc_num_free(&result.data.num);
  return status;
}

BcStatus bc_program_logical(BcProgram *p, uint8_t inst) {

  BcStatus status;
  BcResult *operand1, *operand2, res;
  BcNum *num1, *num2;
  bool cond;
  int cmp;

  status = bc_program_binaryOpPrep(p, &operand1, &num1, &operand2, &num2);
  if (status) return status;

  if ((status = bc_num_init(&res.data.num, BC_NUM_DEF_SIZE))) return status;

  if (inst == BC_INST_BOOL_AND)
    cond = bc_num_cmp(num1, &p->zero) && bc_num_cmp(num2, &p->zero);
  else if (inst == BC_INST_BOOL_OR)
    cond = bc_num_cmp(num1, &p->zero) || bc_num_cmp(num2, &p->zero);
  else {

    cmp = bc_num_cmp(num1, num2);

    switch (inst) {
      case BC_INST_REL_EQ:
      {
        cond = cmp == 0;
        break;
      }

      case BC_INST_REL_LE:
      {
        cond = cmp <= 0;
        break;
      }

      case BC_INST_REL_GE:
      {
        cond = cmp >= 0;
        break;
      }

      case BC_INST_REL_NE:
      {
        cond = cmp != 0;
        break;
      }

      case BC_INST_REL_LT:
      {
        cond = cmp < 0;
        break;
      }

      case BC_INST_REL_GT:
      {
        cond = cmp > 0;
        break;
      }

      default:
      {
        // This is here to silence a compiler warning in release mode.
        cond = 0;
        assert(cond);
        break;
      }
    }
  }

  (cond ? bc_num_one : bc_num_zero)(&res.data.num);

  if ((status = bc_program_binaryOpRetire(p, &res, BC_RESULT_TEMP))) goto err;

  return status;

err:
  bc_num_free(&res.data.num);
  return status;
}

BcStatus bc_program_assign(BcProgram *p, uint8_t inst) {

  BcStatus status;
  BcResult *left, *right, res;
  BcNum *l, *r;
  BcNumBinaryFunc op;
  unsigned long val, max;
  size_t *ptr;

  status = bc_program_binaryOpPrep(p, &left, &l, &right, &r);
  if (status) return status;

  if (left->type == BC_RESULT_CONSTANT || left->type == BC_RESULT_TEMP)
    return BC_STATUS_PARSE_BAD_ASSIGN;

  if (inst == BC_INST_ASSIGN_DIVIDE && !bc_num_cmp(r, &p->zero))
    return BC_STATUS_MATH_DIVIDE_BY_ZERO;

  if (inst == BC_INST_ASSIGN)  status = bc_num_copy(l, r);
  else {
    op = bc_program_math_ops[inst - BC_INST_ASSIGN_POWER];
    status = op(l, r, l, p->scale);
  }

  if (status) return status;

  if (left->type == BC_RESULT_IBASE || left->type == BC_RESULT_OBASE) {

    ptr = left->type == BC_RESULT_IBASE ? &p->ibase_t : &p->obase_t;
    max = left->type == BC_RESULT_IBASE ? BC_NUM_MAX_INPUT_BASE : BC_MAX_BASE;

    if ((status = bc_num_ulong(l, &val))) return status;

    if (val < BC_NUM_MIN_BASE || val > max)
      return left->type - BC_RESULT_IBASE + BC_STATUS_EXEC_BAD_IBASE;

    *ptr = (size_t) val;
  }
  else if (left->type == BC_RESULT_SCALE) {

    if ((status = bc_num_ulong(l, &val))) return status;
    if (val > (unsigned long) BC_MAX_SCALE) return BC_STATUS_EXEC_BAD_SCALE;

    p->scale = (size_t) val;
  }

  if ((status = bc_num_init(&res.data.num, l->len))) return status;
  if ((status = bc_num_copy(&res.data.num, l))) goto err;

  if ((status = bc_program_binaryOpRetire(p, &res, BC_RESULT_TEMP))) goto err;

  return status;

err:
  bc_num_free(&res.data.num);
  return status;
}

BcStatus bc_program_call(BcProgram *p, uint8_t *code, size_t *idx) {

  BcStatus status;
  BcInstPtr ip;
  size_t nparams, i;
  BcFunc *func;
  BcAuto *auto_ptr;
  BcResult param, *arg;

  status = BC_STATUS_SUCCESS;
  nparams = bc_program_index(code, idx);

  ip.idx = 0;
  ip.len = p->results.len;
  ip.func = bc_program_index(code, idx);

  func = bc_vec_item(&p->funcs, ip.func);

  if (!func->code.len) return BC_STATUS_EXEC_UNDEFINED_FUNC;
  if (nparams != func->nparams) return BC_STATUS_EXEC_MISMATCHED_PARAMS;

  for (i = 0; i < nparams; ++i) {

    auto_ptr = bc_vec_item(&func->autos, i);
    arg = bc_vec_item_rev(&p->results, nparams - 1);
    param.type = auto_ptr->var + BC_RESULT_ARRAY_AUTO;

    if (auto_ptr->var) {

      BcNum *n;

      if ((status = bc_program_num(p, arg, &n, false))) return status;
      if ((status = bc_num_init(&param.data.num, n->len))) return status;

      status = bc_num_copy(&param.data.num, n);
    }
    else {

      BcVec *a;

      if (arg->type != BC_RESULT_VAR || arg->type != BC_RESULT_ARRAY)
        return BC_STATUS_EXEC_BAD_TYPE;

      status = bc_program_search(p, arg, (BcNum**) &a, BC_PROGRAM_SEARCH_ARRAY);
      if (status) return status;

      status = bc_vec_init(&param.data.array, sizeof(BcNum), bc_num_free);
      if (status) return status;

      status = bc_array_copy(&param.data.array, a);
    }

    if (status || (status = bc_vec_push(&p->results, &param))) goto err;
  }

  for (; !status && i < func->autos.len; ++i) {

    auto_ptr = bc_vec_item_rev(&func->autos, i);
    param.type = auto_ptr->var + BC_RESULT_ARRAY_AUTO;

    if (auto_ptr->var) status = bc_num_init(&param.data.num, BC_NUM_DEF_SIZE);
    else status = bc_vec_init(&param.data.array, sizeof(BcNum), bc_num_free);

    if (status) return status;

    status = bc_vec_push(&p->results, &param);
  }

  if (status) goto err;

  return bc_vec_push(&p->stack, &ip);

err:
  bc_result_free(&param);
  return status;
}

BcStatus bc_program_return(BcProgram *p, uint8_t inst) {

  BcStatus status;
  BcResult result, *operand;
  BcInstPtr *ip;
  BcFunc *func;

  assert(BC_PROGRAM_CHECK_STACK(p));

  ip = bc_vec_top(&p->stack);
  assert(BC_PROGRAM_CHECK_RESULTS(p, ip->len + inst == BC_INST_RETURN));
  func = bc_vec_item(&p->funcs, ip->func);

  result.type = BC_RESULT_TEMP;

  if (inst == BC_INST_RETURN) {

    BcNum *num;

    operand = bc_vec_top(&p->results);

    if ((status = bc_program_num(p, operand, &num, false))) return status;
    if ((status = bc_num_init(&result.data.num, num->len))) return status;
    if ((status = bc_num_copy(&result.data.num, num))) goto err;
  }
  else {
    status = bc_num_init(&result.data.num, BC_NUM_DEF_SIZE);
    if (status) return status;
    bc_num_zero(&result.data.num);
  }

  // We need to pop arguments as well, so this takes that into account.
  bc_vec_npop(&p->results, p->results.len - (ip->len - func->nparams));

  if ((status = bc_vec_push(&p->results, &result))) goto err;
  bc_vec_pop(&p->stack);

  return status;

err:
  bc_num_free(&result.data.num);
  return status;
}

unsigned long bc_program_scale(BcNum *n) {
  return (unsigned long) n->rdx;
}

unsigned long bc_program_length(BcNum *n) {

  size_t i;
  unsigned long len = n->len;

  if (n->rdx == n->len) {
    for (i = n->len - 1; i < n->len && !n->num[i]; --len, --i);
  }

  return len;
}

BcStatus bc_program_builtin(BcProgram *p, uint8_t inst) {

  BcStatus status;
  BcResult *operand;
  BcNum *num1;
  BcResult result;

  if ((status = bc_program_unaryOpPrep(p, &operand, &num1))) return status;
  if ((status = bc_num_init(&result.data.num, BC_NUM_DEF_SIZE))) return status;

  if (inst == BC_INST_SQRT)
    status = bc_num_sqrt(num1, &result.data.num, p->scale);
  else {

    BcProgramBuiltInFunc func;
    unsigned long ans;

    func = inst == BC_INST_LENGTH ? bc_program_length : bc_program_scale;
    ans = func(num1);

    status = bc_num_ulong2num(&result.data.num, ans);
  }

  if (status || (status = bc_program_unaryOpRetire(p, &result, BC_RESULT_TEMP)))
    goto err;

  return status;

err:
  bc_num_free(&result.data.num);
  return status;
}

BcStatus bc_program_pushScale(BcProgram *p) {

  BcStatus status;
  BcResult result;

  result.type = BC_RESULT_SCALE;

  if ((status = bc_num_init(&result.data.num, BC_NUM_DEF_SIZE))) return status;
  status = bc_num_ulong2num(&result.data.num, (unsigned long) p->scale);
  if (status || (status = bc_vec_push(&p->results, &result))) goto err;

  return status;

err:
  bc_num_free(&result.data.num);
  return status;
}

BcStatus bc_program_incdec(BcProgram *p, uint8_t inst) {

  BcStatus status;
  BcResult *ptr, result, copy;
  BcNum *num;

  if ((status = bc_program_unaryOpPrep(p, &ptr, &num))) return status;

  if (inst == BC_INST_INC_POST || inst == BC_INST_DEC_POST) {
    copy.type = BC_RESULT_TEMP;
    if ((status = bc_num_init(&copy.data.num, num->len))) return status;
  }

  result.type = BC_RESULT_ONE;
  inst = inst == BC_INST_INC_PRE || inst == BC_INST_INC_POST ?
            BC_INST_ASSIGN_PLUS : BC_INST_ASSIGN_MINUS;

  if ((status = bc_vec_push(&p->results, &result))) goto err;
  if ((status = bc_program_assign(p, inst))) goto err;

  if (inst == BC_INST_INC_POST || inst == BC_INST_DEC_POST) {
    bc_vec_pop(&p->results);
    if ((status = bc_vec_push(&p->results, &copy))) goto err;
  }

  return status;

err:

  if (inst == BC_INST_INC_POST || inst == BC_INST_DEC_POST)
    bc_num_free(&copy.data.num);

  return status;
}

BcStatus bc_program_init(BcProgram *p, size_t line_len) {

  BcStatus s;
  size_t idx;
  char *main_name, *read_name;
  BcInstPtr ip;

  assert(p);

  assert(sysconf(_SC_BC_BASE_MAX) <= BC_MAX_BASE);
  assert(sysconf(_SC_BC_DIM_MAX) <= BC_MAX_DIM);
  assert(sysconf(_SC_BC_SCALE_MAX) <= BC_MAX_SCALE);
  assert(sysconf(_SC_BC_STRING_MAX) <= BC_MAX_STRING);

  main_name = read_name = NULL;
  p->nchars = p->scale = 0;
  p->line_len = line_len;

  if ((s = bc_num_init(&p->ibase, BC_NUM_DEF_SIZE))) return s;
  bc_num_ten(&p->ibase);
  p->ibase_t = 10;

  if ((s = bc_num_init(&p->obase, BC_NUM_DEF_SIZE))) goto obase_err;
  bc_num_ten(&p->obase);
  p->obase_t = 10;

  if ((s = bc_num_init(&p->last, BC_NUM_DEF_SIZE))) goto last_err;
  bc_num_zero(&p->last);

  if ((s = bc_num_init(&p->zero, BC_NUM_DEF_SIZE))) goto zero_err;
  bc_num_zero(&p->zero);

  if ((s = bc_num_init(&p->one, BC_NUM_DEF_SIZE))) goto one_err;
  bc_num_one(&p->one);

  if ((s = bc_vec_init(&p->funcs, sizeof(BcFunc), bc_func_free))) goto func_err;

  s = bc_veco_init(&p->func_map, sizeof(BcEntry), bc_entry_free, bc_entry_cmp);
  if (s) goto func_map_err;

  if (!(main_name = malloc(sizeof(bc_lang_func_main)))) {
    s = BC_STATUS_MALLOC_FAIL;
    goto name_err;
  }

  strcpy(main_name, bc_lang_func_main);
  s = bc_program_addFunc(p, main_name, &idx);
  main_name = NULL;
  if (s || idx != BC_PROGRAM_MAIN) goto read_err;

  if (!(read_name = malloc(sizeof(bc_lang_func_read)))) {
    s = BC_STATUS_MALLOC_FAIL;
    goto read_err;
  }

  strcpy(read_name, bc_lang_func_read);
  s = bc_program_addFunc(p, read_name, &idx);
  read_name = NULL;
  if (s || idx != BC_PROGRAM_READ) goto var_err;

  if ((s = bc_vec_init(&p->vars, sizeof(BcNum), bc_num_free))) goto var_err;
  s = bc_veco_init(&p->var_map, sizeof(BcEntry), bc_entry_free, bc_entry_cmp);
  if (s) goto var_map_err;

  if ((s = bc_vec_init(&p->arrays, sizeof(BcVec), bc_vec_free))) goto array_err;
  s = bc_veco_init(&p->array_map, sizeof(BcEntry), bc_entry_free, bc_entry_cmp);
  if (s) goto array_map_err;

  s = bc_vec_init(&p->strings, sizeof(char*), bc_string_free);
  if (s) goto string_err;

  s = bc_vec_init(&p->constants, sizeof(char*), bc_string_free);
  if (s) goto const_err;

  s = bc_vec_init(&p->results, sizeof(BcResult), bc_result_free);
  if (s) goto expr_err;

  if ((s = bc_vec_init(&p->stack, sizeof(BcInstPtr), NULL))) goto stack_err;

  memset(&ip, 0, sizeof(BcInstPtr));

  if ((s = bc_vec_push(&p->stack, &ip))) goto push_err;

  return s;

push_err:
  bc_vec_free(&p->stack);
stack_err:
  bc_vec_free(&p->results);
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
  if (main_name) free(main_name);
name_err:
  bc_veco_free(&p->func_map);
func_map_err:
  bc_vec_free(&p->funcs);
func_err:
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

BcStatus bc_program_addFunc(BcProgram *p, char *name, size_t *idx) {

  BcStatus status;
  BcEntry entry, *entry_ptr;
  BcFunc f;

  assert(p && name && idx);

  entry.name = name;
  entry.idx = p->funcs.len;

  if ((status = bc_veco_insert(&p->func_map, &entry, idx))) {
    free(name);
    if (status != BC_STATUS_VEC_ITEM_EXISTS) return status;
  }

  entry_ptr = bc_veco_item(&p->func_map, *idx);
  *idx = entry_ptr->idx;

  if (status == BC_STATUS_VEC_ITEM_EXISTS) {

    BcFunc *func = bc_vec_item(&p->funcs, entry_ptr->idx);

    status = BC_STATUS_SUCCESS;

    // We need to reset these, so the function can be repopulated.
    func->nparams = 0;
    bc_vec_npop(&func->autos, func->autos.len);
    bc_vec_npop(&func->code, func->code.len);
    bc_vec_npop(&func->labels, func->labels.len);
  }
  else {
    if ((status = bc_func_init(&f))) return status;
    if ((status = bc_vec_push(&p->funcs, &f))) bc_func_free(&f);
  }

  return status;
}

BcStatus bc_program_reset(BcProgram *p, BcStatus status, bool sig) {

  BcFunc *func;
  BcInstPtr *ip;

  bc_vec_npop(&p->stack, p->stack.len - 1);
  bc_vec_npop(&p->results, p->results.len);

  func = bc_vec_item(&p->funcs, 0);
  ip = bc_vec_top(&p->stack);
  ip->idx = func->code.len;

  bcg.sig_int_catches += sig;

  if (!status && sig && !bcg.interactive) return BC_STATUS_QUIT;

  if (!status && bcg.interactive) {
    fprintf(stderr, "%s", bc_program_ready_prompt);
    fflush(stderr);
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
  bool cond, sig;
  const char **string, *s;

  status = BC_STATUS_SUCCESS;
  cond = false;

  ip = bc_vec_top(&p->stack);
  func = bc_vec_item(&p->funcs, ip->func);
  code = func->code.array;

  while (!status && !bcg.sig_other && ip->idx < func->code.len) {

    uint8_t inst = code[(ip->idx)++];

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

      case BC_INST_JUMP_ZERO:
      {
        BcResult *operand;
        BcNum *num;

        if ((status = bc_program_unaryOpPrep(p, &operand, &num))) return status;
        cond = !bc_num_cmp(num, &p->zero);
        bc_vec_pop(&p->results);
      }
      // Fallthrough.
      case BC_INST_JUMP:
      {
        size_t idx, *addr;

        idx = bc_program_index(code, &ip->idx);
        addr = bc_vec_item(&func->labels, idx);

        if (inst == BC_INST_JUMP || cond) ip->idx = *addr;

        break;
      }

      case BC_INST_PUSH_VAR:
      case BC_INST_PUSH_ARRAY_ELEM:
      {
        status = bc_program_push(p, code, &ip->idx, inst == BC_INST_PUSH_VAR);
        break;
      }

      case BC_INST_PUSH_LAST:
      {
        result.type = BC_RESULT_LAST;
        status = bc_vec_push(&p->results, &result);
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
        status = bc_vec_push(&p->results, &result);
        break;
      }

      case BC_INST_PUSH_OBASE:
      {
        result.type = BC_RESULT_OBASE;
        status = bc_vec_push(&p->results, &result);
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
        result.type = BC_RESULT_CONSTANT;
        result.data.id.idx = bc_program_index(code, &ip->idx);
        status = bc_vec_push(&p->results, &result);
        break;
      }

      case BC_INST_POP:
      {
        bc_vec_pop(&p->results);
        break;
      }

      case BC_INST_INC_POST:
      case BC_INST_DEC_POST:
      case BC_INST_INC_PRE:
      case BC_INST_DEC_PRE:
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

        if ((status = bc_program_unaryOpPrep(p, &operand, &num))) return status;

        status = bc_num_print(num, &p->obase, p->obase_t, inst == BC_INST_PRINT,
                              &p->nchars, p->line_len);
        if (status) return status;
        if ((status = bc_num_copy(&p->last, num))) return status;

        bc_vec_pop(&p->results);

        break;
      }

      case BC_INST_STR:
      {
        size_t len;

        idx = bc_program_index(code, &ip->idx);
        assert(idx < p->strings.len);
        string = bc_vec_item(&p->strings, idx);

        s = *string;
        len = strlen(s);

        for (idx = 0; idx < len; ++idx) {
          char c = s[idx];
          if (putchar(c) == EOF) return BC_STATUS_IO_ERR;
          if (c == '\n') p->nchars = SIZE_MAX;
          ++p->nchars;
        }

        break;
      }

      case BC_INST_PRINT_STR:
      {
        idx = bc_program_index(code, &ip->idx);
        assert(idx < p->strings.len);
        string = bc_vec_item(&p->strings, idx);
        status = bc_program_printString(*string, &p->nchars);
        break;
      }

      case BC_INST_POWER:
      case BC_INST_MULTIPLY:
      case BC_INST_DIVIDE:
      case BC_INST_MODULUS:
      case BC_INST_PLUS:
      case BC_INST_MINUS:
      {
        status = bc_program_op(p, inst);
        break;
      }

      case BC_INST_REL_EQ:
      case BC_INST_REL_LE:
      case BC_INST_REL_GE:
      case BC_INST_REL_NE:
      case BC_INST_REL_LT:
      case BC_INST_REL_GT:
      {
        status = bc_program_logical(p, inst);
        break;
      }

      case BC_INST_BOOL_NOT:
      {
        BcResult *ptr;
        BcNum *num;

        if ((status = bc_program_unaryOpPrep(p, &ptr, &num))) return status;
        status = bc_num_init(&result.data.num, BC_NUM_DEF_SIZE);
        if (status) return status;

        if (bc_num_cmp(num, &p->zero)) bc_num_one(&result.data.num);
        else bc_num_zero(&result.data.num);

        status = bc_program_unaryOpRetire(p, &result, BC_RESULT_TEMP);
        if (status) bc_num_free(&result.data.num);

        break;
      }

      case BC_INST_BOOL_OR:
      case BC_INST_BOOL_AND:
      {
        status = bc_program_logical(p, inst);
        break;
      }

      case BC_INST_NEG:
      {
        status = bc_program_negate(p);
        break;
      }

      case BC_INST_ASSIGN_POWER:
      case BC_INST_ASSIGN_MULTIPLY:
      case BC_INST_ASSIGN_DIVIDE:
      case BC_INST_ASSIGN_MODULUS:
      case BC_INST_ASSIGN_PLUS:
      case BC_INST_ASSIGN_MINUS:
      case BC_INST_ASSIGN:
      {
        status = bc_program_assign(p, inst);
        break;
      }

      default:
      {
        assert(false);
        break;
      }
    }

    sig = bcg.sig_int != bcg.sig_int_catches;

    if ((status && status != BC_STATUS_QUIT) || sig)
      status = bc_program_reset(p, status, sig);

    // We keep getting these because if the size of the
    // stack changes, pointers may end up being invalid.
    ip = bc_vec_top(&p->stack);
    func = bc_vec_item(&p->funcs, ip->func);
    code = func->code.array;
  }

  return status;
}

void bc_program_free(BcProgram *p) {

  if (!p) return;

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

  bc_vec_free(&p->results);
  bc_vec_free(&p->stack);

  bc_num_free(&p->last);
  bc_num_free(&p->zero);
  bc_num_free(&p->one);

  memset(p, 0, sizeof(BcProgram));
}

#ifndef NDEBUG
BcStatus bc_program_printIndex(uint8_t *code, size_t *start) {

  uint8_t bytes, byte, i;

  bytes = code[(*start)++];

  if (printf(bc_program_byte_fmt, bytes) < 0) return BC_STATUS_IO_ERR;

  for (byte = 1, i = 0; byte && i < bytes; ++i) {
    byte = code[(*start)++];
    if (printf(bc_program_byte_fmt, byte) < 0) return BC_STATUS_IO_ERR;
  }

  return BC_STATUS_SUCCESS;
}

BcStatus bc_program_printName(uint8_t *code, size_t *start) {

  BcStatus status = BC_STATUS_SUCCESS;
  char byte;

  for (byte = code[(*start)++]; byte && byte != ':'; byte = code[(*start)++]) {
    if (putchar(byte) == EOF) return BC_STATUS_IO_ERR;
  }

  assert(byte);

  if (putchar(byte) == EOF) status = BC_STATUS_IO_ERR;

  return status;
}

BcStatus bc_program_print(BcProgram *p) {

  BcStatus status;
  BcFunc *func;
  uint8_t *code;
  BcInstPtr ip;
  size_t i;
  bool sig;

  status = BC_STATUS_SUCCESS;

  for (i = 0; !status && !bcg.sig_other && i < p->funcs.len; ++i) {

    ip.idx = ip.len = 0;
    ip.func = i;

    func = bc_vec_item(&p->funcs, ip.func);
    code = func->code.array;

    if (printf("func[%zu]: ", ip.func) < 0) return BC_STATUS_IO_ERR;

    while (ip.idx < func->code.len) {

      uint8_t inst = code[ip.idx++];

      if (putchar(bc_lang_inst_chars[inst]) == EOF) return BC_STATUS_IO_ERR;

      if (inst == BC_INST_PUSH_VAR || inst == BC_INST_PUSH_ARRAY_ELEM) {
        if ((status = bc_program_printName(code, &ip.idx))) return status;
      }
      else if (inst == BC_INST_PUSH_NUM || inst == BC_INST_CALL ||
               (inst >= BC_INST_STR && inst <= BC_INST_JUMP_ZERO))
      {
        if ((status = bc_program_printIndex(code, &ip.idx))) return status;
        if (inst == BC_INST_CALL && (status = bc_program_printIndex(code, &ip.idx)))
          return status;
      }
    }

    if (putchar('\n') == EOF) status = BC_STATUS_IO_ERR;

    sig = bcg.sig_int != bcg.sig_int_catches;
    if (status || sig) status = bc_program_reset(p, status, sig);
  }

  return status;
}
#endif // NDEBUG
