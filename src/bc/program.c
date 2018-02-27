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

static const char *bc_program_read_func = "read()";

static const char *bc_byte_fmt = "%02x";

static const BcMathOpFunc bc_math_ops[] = {

  bc_num_mod,

  NULL, // &
  NULL, // '
  NULL, // (
  NULL, // )

  bc_num_mul,

  bc_num_add,

  NULL, // ,

  bc_num_sub,

  NULL, // .

  bc_num_div,

};

static BcStatus bc_program_op(BcProgram *p, uint8_t inst);
static BcStatus bc_program_num(BcProgram *p, BcResult *result,
                               BcNum** num, bool ibase);
static BcStatus bc_program_printString(const char *str);
static BcStatus bc_program_read(BcProgram *p);
static size_t bc_program_index(uint8_t *code, size_t *start);
static BcStatus bc_program_printIndex(uint8_t *code, size_t *start);
static BcStatus bc_program_printName(uint8_t *code, size_t *start);
static BcStatus bc_program_assign(BcProgram *p, uint8_t inst);
static BcStatus bc_program_assignScale(BcProgram *p, BcNum *rval, uint8_t inst);
static BcMathOpFunc bc_program_assignOp(uint8_t inst);

static BcStatus bc_program_binaryOpPrep(BcProgram *p, BcResult **left,
                                        BcNum **lval, BcResult **right,
                                        BcNum **rval);
static BcStatus bc_program_binaryOpRetire(BcProgram *p, BcResult *result);
static BcStatus bc_program_unaryOpPrep(BcProgram *p, BcResult **result,
                                       BcNum **val);
static BcStatus bc_program_unaryOpRetire(BcProgram *p, BcResult *result);

static BcStatus bc_program_call(BcProgram *p, uint8_t *code, size_t *idx);
static BcStatus bc_program_return(BcProgram *p, uint8_t inst);

static BcStatus bc_program_builtin(BcProgram *p, uint8_t inst);
static unsigned long bc_program_scale(BcNum *n);
static unsigned long bc_program_length(BcNum *n);

static BcStatus bc_program_pushScale(BcProgram *p);

static BcStatus bc_program_incdec(BcProgram *p, uint8_t inst);

BcStatus bc_program_init(BcProgram *p) {

  BcStatus s;
  size_t idx;
  char *name;
  char *read_name;
  BcInstPtr ip;

  if (p == NULL) return BC_STATUS_INVALID_PARAM;

  name = NULL;

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
  else if (p->base_max > BC_BASE_MAX_DEF) return BC_STATUS_INVALID_LIMIT;
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
  else if (p->dim_max > BC_DIM_MAX_DEF) return BC_STATUS_INVALID_LIMIT;
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
  else if (p->scale_max > BC_SCALE_MAX_DEF) return BC_STATUS_INVALID_LIMIT;
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
  else if (p->string_max > BC_STRING_MAX_DEF) return BC_STATUS_INVALID_LIMIT;
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

  name = malloc(1);

  if (!name) {
    s = BC_STATUS_MALLOC_FAIL;
    goto name_err;
  }

  name[0] = '\0';

  s = bc_program_func_add(p, name, &idx);

  if (s || idx != BC_PROGRAM_ROOT_FUNC) goto read_err;

  name = NULL;

  read_name = malloc(strlen(bc_program_read_func));

  if (!read_name) {
    s = BC_STATUS_MALLOC_FAIL;
    goto read_err;
  }

  s = bc_program_func_add(p, read_name, &idx);

  if (s || idx != BC_PROGRAM_READ_FUNC) goto var_err;

  read_name = NULL;

  s = bc_vec_init(&p->vars, sizeof(BcVar), bc_var_free);

  if (s) goto var_err;

  s = bc_veco_init(&p->var_map, sizeof(BcEntry), bc_entry_free, bc_entry_cmp);

  if (s) goto var_map_err;

  s = bc_vec_init(&p->arrays, sizeof(BcArray), bc_array_free);

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
  printf("Number of Vars  = %u\n", UINT32_MAX);

  putchar('\n');
}

BcStatus bc_program_func_add(BcProgram *p, char *name, size_t *idx) {

  BcStatus status;
  BcEntry entry;
  BcFunc f;

  if (!p || !name || !idx) return BC_STATUS_INVALID_PARAM;

  entry.name = name;
  entry.idx = p->funcs.len;

  status = bc_veco_insert(&p->func_map, &entry, idx);

  if (status == BC_STATUS_VECO_ITEM_EXISTS) {

    BcFunc *func;

    func = bc_vec_item(&p->funcs, *idx);

    if (!func) return BC_STATUS_EXEC_UNDEFINED_FUNC;

    status = BC_STATUS_SUCCESS;

    // We need to reset these so the function can be repopulated.
    while (!status && func->autos.len) status = bc_vec_pop(&func->autos);
    while (!status && func->params.len) status = bc_vec_pop(&func->params);

    return status;
  }
  else if (status) return status;

  status = bc_func_init(&f);

  if (status) return status;

  return bc_vec_push(&p->funcs, &f);
}

BcStatus bc_program_var_add(BcProgram *p, char *name, size_t *idx) {

  BcStatus status;
  BcEntry entry;
  BcVar v;

  if (!p || !name || !idx) return BC_STATUS_INVALID_PARAM;

  entry.name = name;
  entry.idx = p->vars.len;

  status = bc_veco_insert(&p->var_map, &entry, idx);

  if (status) return status == BC_STATUS_VECO_ITEM_EXISTS ?
                               BC_STATUS_SUCCESS : status;

  status = bc_var_init(&v);

  if (status) return status;

  return bc_vec_push(&p->vars, &v);
}

BcStatus bc_program_array_add(BcProgram *p, char *name, size_t *idx) {

  BcStatus status;
  BcEntry entry;
  BcArray a;

  if (!p || !name || !idx) return BC_STATUS_INVALID_PARAM;

  entry.name = name;
  entry.idx = p->arrays.len;

  status = bc_veco_insert(&p->array_map, &entry, idx);

  if (status) return status == BC_STATUS_VECO_ITEM_EXISTS ?
                               BC_STATUS_SUCCESS : status;

  status = bc_array_init(&a);

  if (status) return status;

  return bc_vec_push(&p->arrays, &a);
}

BcStatus bc_program_exec(BcProgram *p) {

  BcStatus status;
  uint8_t *code;
  size_t idx;
  int pchars;
  BcResult result;
  BcFunc *func;
  BcInstPtr *ip;

  ip = bc_vec_top(&p->stack);

  assert(ip);

  if (!ip) return BC_STATUS_EXEC_INVALID_STMT;

  func = bc_vec_item(&p->funcs, ip->func);

  assert(func);

  status = BC_STATUS_SUCCESS;

  code = func->code.array;

  while (ip->idx < func->code.len) {

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
      case BC_INST_JUMP:
      {
        // TODO: Fill this out.
        break;
      }

      case BC_INST_PUSH_VAR:
      {
        // TODO: Fill this out.
        break;
      }

      case BC_INST_PUSH_ARRAY:
      {
        // TODO: Fill this out.
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
        status = BC_STATUS_EXEC_HALT;
        break;
      }

      case BC_INST_PRINT:
      {
        BcResult *result;
        BcNum *num;

        if (!BC_PROGRAM_CHECK_EXPR_STACK(p, 1))
          return BC_STATUS_EXEC_INVALID_EXPR;

        result = bc_vec_top(&p->expr_stack);

        status = bc_program_num(p, result, &num, false);

        status = bc_num_print(num, p->obase_t);

        if (status) return status;

        status = bc_num_copy(&p->last, num);

        if (status) return status;

        status = bc_vec_pop(&p->expr_stack);

        break;
      }

      case BC_INST_STR:
      {
        const char *string;

        idx = bc_program_index(code, &ip->idx);

        if (idx >= p->strings.len) return BC_STATUS_EXEC_INVALID_STRING;

        string = bc_vec_item(&p->strings, idx);

        pchars = fprintf(stdout, "%s", string);
        status = pchars > 0 ? BC_STATUS_SUCCESS :
                              BC_STATUS_EXEC_PRINT_ERR;

        break;
      }

      case BC_INST_PRINT_STR:
      {
        const char *string;

        idx = bc_program_index(code, &ip->idx);

        if (idx >= p->strings.len) return BC_STATUS_EXEC_INVALID_STRING;

        string = bc_vec_item(&p->strings, idx);

        status = bc_program_printString(string);

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
        // TODO: Fill this out.
        break;
      }

      case BC_INST_OP_BOOL_NOT:
      {
        BcResult *ptr;
        BcNum *num;

        status = bc_program_unaryOpPrep(p, &ptr, &num);

        if (status) return status;

        result.type = BC_RESULT_INTERMEDIATE;

        status = bc_num_init(&result.data.num, BC_NUM_DEF_SIZE);

        if (status) return status;

        if (bc_num_compare(num, &p->zero)) bc_num_one(&result.data.num);
        else bc_num_zero(&result.data.num);

        status = bc_program_unaryOpRetire(p, &result);

        if (status) bc_num_free(&result.data.num);

        break;
      }

      case BC_INST_OP_BOOL_OR:
      case BC_INST_OP_BOOL_AND:
      {
        // TODO: Fill this out.
        break;
      }

      case BC_INST_OP_NEGATE:
      {
        BcResult *ptr;
        BcNum *num;

        if (!BC_PROGRAM_CHECK_EXPR_STACK(p, 1))
          return BC_STATUS_EXEC_INVALID_EXPR;

        ptr = bc_vec_top(&p->expr_stack);

        if (!ptr) return BC_STATUS_EXEC_INVALID_EXPR;

        status = bc_program_num(p, ptr, &num, false);

        if (status) return status;

        num->neg = !num->neg;

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
        status = BC_STATUS_EXEC_INVALID_STMT;
        break;
      }
    }

    if (status) return status;

    // We keep getting these because if the size of the
    // stack changes, pointers may end up being invalid.
    ip = bc_vec_top(&p->stack);

    if (!ip) return BC_STATUS_EXEC_INVALID_STMT;

    func = bc_vec_item(&p->funcs, ip->func);

    assert(func);
  }

  return status;
}

BcStatus bc_program_printCode(BcProgram *p) {

  BcStatus status;
  BcFunc *func;
  uint8_t *code;
  BcInstPtr *ip;

  ip = bc_vec_top(&p->stack);

  if (!ip) return BC_STATUS_EXEC_INVALID_STMT;

  func = bc_vec_item(&p->funcs, ip->func);

  assert(func);

  code = func->code.array;
  status = BC_STATUS_SUCCESS;

  for (; ip->idx < func->code.len; ++ip->idx) {

    uint8_t inst;

    inst = code[ip->idx];

    switch (inst) {

      case BC_INST_PUSH_VAR:
      case BC_INST_PUSH_ARRAY:
      {
        if (putchar(inst) == EOF) return BC_STATUS_IO_ERR;
        status = bc_program_printName(code, &ip->idx);
        break;
      }

      case BC_INST_CALL:
      {
        if (putchar(inst) == EOF) return BC_STATUS_IO_ERR;

        status = bc_program_printIndex(code, &ip->idx);

        if (status) return status;

        status = bc_program_printIndex(code, &ip->idx);

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
        bc_program_printIndex(code, &ip->idx);
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

  if (putchar('\n') == EOF) status = BC_STATUS_SUCCESS;

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

static BcStatus bc_program_op(BcProgram *p, uint8_t inst) {

  BcStatus status;
  BcResult *result1;
  BcResult *result2;
  BcResult result;
  BcNum *num1;
  BcNum *num2;

  status = bc_program_binaryOpPrep(p, &result1, &num1, &result2, &num2);

  if (status) return status;

  result.type = BC_RESULT_INTERMEDIATE;

  status = bc_num_init(&result.data.num, BC_NUM_DEF_SIZE);

  if (status) return status;

  if (inst != BC_INST_OP_POWER) {

    BcMathOpFunc op;

    op = bc_math_ops[inst - BC_INST_OP_MODULUS];

    status = op(&result1->data.num, &result2->data.num,
                &result.data.num, p->scale);
  }
  else status = bc_num_pow(&result1->data.num, &result2->data.num,
                           &result.data.num, p->scale);

  if (status) goto err;

  status = bc_program_binaryOpRetire(p, &result);

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

  func = bc_vec_item(&p->funcs, BC_PROGRAM_READ_FUNC);

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

  status = bc_parse_expr(&parse, &func->code, false, false);

  if (status != BC_STATUS_LEX_EOF && status != BC_STATUS_PARSE_EOF) {
    status = status ? status : BC_STATUS_EXEC_INVALID_READ_EXPR;
    goto exec_err;
  }

  ip.func = BC_PROGRAM_READ_FUNC;
  ip.idx = 0;
  ip.len = p->expr_stack.len;

  status = bc_vec_push(&p->stack, &ip);

  if (status) goto exec_err;

  status = bc_program_exec(p);

exec_err:

  bc_parse_free(&parse);

io_err:

  free(buffer);

  return status;
}

static size_t bc_program_index(uint8_t *code, size_t *start) {

  uint8_t bytes;
  uint8_t byte;
  size_t result;

  bytes = code[(*start)++];

  result = 0;

  for (uint8_t i = 0; i < bytes; ++i) {
    byte = code[(*start)++];
    result |= (((size_t) byte) << (i * 8));
  }

  return result;
}

static BcStatus bc_program_printIndex(uint8_t *code, size_t *start) {

  uint8_t bytes;
  uint8_t byte;

  bytes = code[(*start)++];

  if (printf(bc_byte_fmt, bytes) < 0) return BC_STATUS_IO_ERR;

  for (uint8_t i = 0; i < bytes; ++i) {
    byte = code[(*start)++];
    if (printf(bc_byte_fmt, byte) < 0) return BC_STATUS_IO_ERR;
  }

  return BC_STATUS_SUCCESS;
}

static BcStatus bc_program_printName(uint8_t *code, size_t *start) {

  char byte;

  byte = code[(*start)++];

  while (byte != ':') {
    if (putchar(byte) == EOF) return BC_STATUS_IO_ERR;
    byte = code[(*start)++];
  }

  return putchar(byte) == EOF ? BC_STATUS_IO_ERR : BC_STATUS_SUCCESS;
}

static BcStatus bc_program_num(BcProgram *p, BcResult *result,
                               BcNum** num, bool ibase)
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

      if (!s) return BC_STATUS_EXEC_INVALID_CONSTANT;

      len = strlen(*s);

      status = bc_num_init(&result->data.num, len);

      if (status) return status;

      base = ibase && len == 1 ? 16 : p->ibase_t;

      status = bc_num_parse(&result->data.num, *s, base);

      if (status) return status;

      *num = &result->data.num;

      result->type = BC_RESULT_INTERMEDIATE;

      break;
    }

    case BC_RESULT_VAR:
    {
      // TODO: Search for var.
      break;
    }

    case BC_RESULT_ARRAY:
    {
      // TODO Search for array.
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
      status = BC_STATUS_EXEC_INVALID_EXPR;
      break;
    }
  }

  return status;
}

static BcStatus bc_program_printString(const char *str) {

  char c;
  char c2;
  size_t len;
  int err;

  err = 0;

  len = strlen(str);

  for (size_t i = 0; i < len; ++i) {

    c = str[i];

    if (c != '\\') err = fputc(c, stdout);
    else {

      ++i;

      if (i >= len) return BC_STATUS_EXEC_INVALID_STRING;

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

    if (err == EOF) return BC_STATUS_EXEC_PRINT_ERR;
  }

  return BC_STATUS_SUCCESS;
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
    return BC_STATUS_EXEC_INVALID_LVALUE;

  if (inst == BC_EXPR_ASSIGN_DIVIDE && !bc_num_compare(rval, &p->zero))
    return BC_STATUS_MATH_DIVIDE_BY_ZERO;

  if (left->type != BC_RESULT_SCALE) {

    status = bc_program_num(p, left, &lval, false);

    if (status) return status;

    switch (inst) {

      case BC_INST_OP_ASSIGN_POWER:
      case BC_INST_OP_ASSIGN_MULTIPLY:
      case BC_INST_OP_ASSIGN_DIVIDE:
      case BC_INST_OP_ASSIGN_MODULUS:
      case BC_INST_OP_ASSIGN_PLUS:
      case BC_INST_OP_ASSIGN_MINUS:
      {
        BcMathOpFunc op;

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
        status = BC_STATUS_EXEC_INVALID_EXPR;
        break;
      }
    }
  }
  else status = bc_program_assignScale(p, rval, inst);

  if (status) return status;

  if (left->type == BC_RESULT_IBASE || left->type == BC_RESULT_OBASE) {

    unsigned long base;
    size_t *ptr;

    ptr = left->type == BC_RESULT_IBASE ? &p->ibase_t : &p->obase_t;

    status = bc_num_ulong(lval, &base);

    if (status) return status;

    *ptr = (size_t) base;
  }

  memcpy(&result, left, sizeof(BcResult));

  return bc_program_binaryOpRetire(p, &result);
}

static BcStatus bc_program_assignScale(BcProgram *p, BcNum *rval, uint8_t inst)
{
  BcStatus status;
  BcNum scale;
  unsigned long result;

  status = bc_num_init(&scale, BC_NUM_DEF_SIZE);

  if (status) return status;

  status = bc_num_ulong2num(&scale, (unsigned long) p->scale);

  if (status) goto err;

  switch (inst) {

    case BC_INST_OP_ASSIGN_POWER:
    case BC_INST_OP_ASSIGN_MULTIPLY:
    case BC_INST_OP_ASSIGN_DIVIDE:
    case BC_INST_OP_ASSIGN_MODULUS:
    case BC_INST_OP_ASSIGN_PLUS:
    case BC_INST_OP_ASSIGN_MINUS:
    {
      BcMathOpFunc op;

      op = bc_program_assignOp(inst);

      status = op(&scale, rval, &scale, p->scale);

      if (status) goto err;

      break;
    }

    case BC_INST_OP_ASSIGN:
    {
      status = bc_num_copy(&scale, rval);
      if (status) goto err;
      break;
    }

    default:
    {
      status = BC_STATUS_EXEC_INVALID_EXPR;
      break;
    }
  }

  status = bc_num_ulong(&scale, &result);

  if (status) goto err;

  p->scale = (size_t) result;

err:

  bc_num_free(&scale);

  return status;
}

static BcMathOpFunc bc_program_assignOp(uint8_t inst) {

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

static BcStatus bc_program_binaryOpPrep(BcProgram *p, BcResult **left,
                                        BcNum **lval, BcResult **right,
                                        BcNum **rval)
{
  BcStatus status;
  BcResult *l;
  BcResult *r;

  if (!BC_PROGRAM_CHECK_EXPR_STACK(p, 2)) return BC_STATUS_EXEC_INVALID_EXPR;

  r = bc_vec_item_rev(&p->expr_stack, 0);
  l = bc_vec_item_rev(&p->expr_stack, 1);

  if (!r || !l) return BC_STATUS_EXEC_INVALID_EXPR;

  status = bc_program_num(p, l, lval, false);

  if (status) return status;

  status = bc_program_num(p, r, rval, l->type == BC_RESULT_IBASE);

  if (status) return status;

  *left = l;
  *right = r;

  return BC_STATUS_SUCCESS;
}

static BcStatus bc_program_binaryOpRetire(BcProgram *p, BcResult *result) {

  BcStatus status;

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

  if (!BC_PROGRAM_CHECK_EXPR_STACK(p, 1)) return BC_STATUS_EXEC_INVALID_EXPR;

  r = bc_vec_item_rev(&p->expr_stack, 0);

  if (!r) return BC_STATUS_EXEC_INVALID_EXPR;

  status = bc_program_num(p, r, val, false);

  if (status) return status;

  *result = r;

  return BC_STATUS_SUCCESS;
}

static BcStatus bc_program_unaryOpRetire(BcProgram *p, BcResult *result) {

  BcStatus status;

  status = bc_vec_pop(&p->expr_stack);

  if (status) return status;

  return bc_vec_push(&p->expr_stack, result);
}

static BcStatus bc_program_call(BcProgram *p, uint8_t *code, size_t *idx) {

  BcStatus status;
  BcInstPtr ip;
  size_t nparams;
  BcFunc *func;

  nparams = bc_program_index(code, idx);

  ip.idx = 0;
  ip.len = p->expr_stack.len;

  ip.func = bc_program_index(code, idx);

  func = bc_vec_item(&p->funcs, ip.func);

  if (!func) return BC_STATUS_EXEC_UNDEFINED_FUNC;

  if (func->params.len != nparams) return BC_STATUS_EXEC_MISMATCHED_PARAMS;

  // TODO: Handle arguments and autos.

  return bc_vec_push(&p->stack, &ip);
}

static BcStatus bc_program_return(BcProgram *p, uint8_t inst) {

  BcStatus status;
  BcResult result;
  BcResult *result1;
  size_t req;
  BcInstPtr *ip;

  if (!BC_PROGRAM_CHECK_STACK(p)) return BC_STATUS_EXEC_INVALID_RETURN;

  ip = bc_vec_top(&p->stack);

  if (!ip) return BC_STATUS_EXEC_INVALID_EXPR;

  req = ip->len + (inst == BC_INST_RETURN ? 1 : 0);

  if (!BC_PROGRAM_CHECK_EXPR_STACK(p, req))
    return BC_STATUS_EXEC_INVALID_EXPR;

  result.type = BC_RESULT_INTERMEDIATE;

  if (inst == BC_INST_RETURN) {

    BcNum *num;

    result1 = bc_vec_top(&p->expr_stack);

    if (!result1) return BC_STATUS_EXEC_INVALID_EXPR;

    status = bc_program_num(p, result1, &num, false);

    if (status) return status;

    status = bc_num_init(&result.data.num, num->len);

    if (status) return status;

    status = bc_num_copy(&result.data.num, num);
  }
  else {

    status = bc_num_init(&result.data.num, BC_NUM_DEF_SIZE);

    if (status) return status;

    bc_num_zero(&result.data.num);
  }

  if (status) goto err;

  status = bc_vec_pushAt(&p->expr_stack, &result, ip->len);

  if (status) goto err;

  while (p->expr_stack.len > ip->len + 1) {
    status = bc_vec_pop(&p->expr_stack);
    if (status) return status;
  }

  return bc_vec_pop(&p->stack);

err:

  bc_num_free(&result.data.num);

  return status;
}

static BcStatus bc_program_builtin(BcProgram *p, uint8_t inst) {

  BcStatus status;
  BcResult *result1;
  BcNum *num1;
  BcResult result;

  status = bc_program_unaryOpPrep(p, &result1, &num1);

  if (status) return status;

  result.type = BC_RESULT_INTERMEDIATE;

  status = bc_num_init(&result.data.num, BC_NUM_DEF_SIZE);

  if (status) return status;

  if (inst == BC_INST_SQRT) {
    status = bc_num_sqrt(num1, &result.data.num, p->scale);
  }
  else {

    BcBuiltInFunc func;
    unsigned long ans;

    func = inst == BC_INST_LENGTH ? bc_program_length : bc_program_scale;

    ans = func(num1);

    status = bc_num_ulong2num(&result.data.num, ans);
  }

  if (status) goto err;

  status = bc_program_unaryOpRetire(p, &result);

  if (status) goto err;

  return status;

err:

  bc_num_free(&result.data.num);

  return status;
}

static unsigned long bc_program_scale(BcNum *n) {
  return (unsigned long) n->rdx;
}

static unsigned long bc_program_length(BcNum *n) {
  return (unsigned long) n->len;
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

  if (!BC_PROGRAM_CHECK_EXPR_STACK(p, 1))
    return BC_STATUS_EXEC_INVALID_EXPR;

  ptr = bc_vec_top(&p->expr_stack);

  if (!ptr) return BC_STATUS_EXEC_INVALID_EXPR;

  status = bc_program_num(p, ptr, &num, false);

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
