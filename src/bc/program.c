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
 * Code to execute bc programs.
 *
 */

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <arbprec/arbprec.h>

#include <bc/io.h>
#include <bc/program.h>
#include <bc/parse.h>
#include <bc/instructions.h>

static const char* const bc_byte_fmt = "%02x";

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

static BcStatus bc_program_printString(const char* str);
static BcStatus bc_program_execExpr(BcProgram* p, BcVec* exprs,
                                    BcNum* num, bool print);
#if 0
static BcStatus bc_program_assign(BcProgram* p, BcExpr* expr,
                                  BcExprType op, fxdpnt* amt);
#endif
static BcStatus bc_program_read(BcProgram* p);
static size_t bc_program_index(uint8_t* code, size_t* start);
static void bc_program_printIndex(uint8_t* code, size_t* start);
static void bc_program_printName(uint8_t* code, size_t* start);

BcStatus bc_program_init(BcProgram* p) {

  BcStatus s;
  size_t idx;
  char* name;
  BcInstPtr ip;

  if (p == NULL) {
    return BC_STATUS_INVALID_PARAM;
  }

  name = NULL;

  p->scale = 0;
  p->ibase = 10;
  p->obase = 10;

#ifdef _POSIX_BC_BASE_MAX
  p->base_max = _POSIX_BC_BASE_MAX;
#elif defined(_BC_BASE_MAX)
  p->base_max = _BC_BASE_MAX;
#else
  errno = 0;
  p->base_max = sysconf(_SC_BC_BASE_MAX);

  if (p->base_max == -1) {

    if (errno) {
      return BC_STATUS_NO_LIMIT;
    }

    p->base_max = BC_BASE_MAX_DEF;
  }
  else if (p->base_max > BC_BASE_MAX_DEF) {
    return BC_STATUS_INVALID_LIMIT;
  }
#endif

#ifdef _POSIX_BC_DIM_MAX
  p->dim_max = _POSIX_BC_DIM_MAX;
#elif defined(_BC_DIM_MAX)
  p->dim_max = _BC_DIM_MAX;
#else
  errno = 0;
  p->dim_max = sysconf(_SC_BC_DIM_MAX);

  if (p->dim_max == -1) {

    if (errno) {
      return BC_STATUS_NO_LIMIT;
    }

    p->dim_max = BC_DIM_MAX_DEF;
  }
  else if (p->dim_max > BC_DIM_MAX_DEF) {
    return BC_STATUS_INVALID_LIMIT;
  }
#endif

#ifdef _POSIX_BC_SCALE_MAX
  p->scale_max = _POSIX_BC_SCALE_MAX;
#elif defined(_BC_SCALE_MAX)
  p->scale_max = _BC_SCALE_MAX;
#else
  errno = 0;
  p->scale_max = sysconf(_SC_BC_SCALE_MAX);

  if (p->scale_max == -1) {

    if (errno) {
      return BC_STATUS_NO_LIMIT;
    }

    p->scale_max = BC_SCALE_MAX_DEF;
  }
  else if (p->scale_max > BC_SCALE_MAX_DEF) {
    return BC_STATUS_INVALID_LIMIT;
  }
#endif

#ifdef _POSIX_BC_STRING_MAX
  p->string_max = _POSIX_BC_STRING_MAX;
#elif defined(_BC_STRING_MAX)
  p->string_max = _BC_STRING_MAX;
#else
  errno = 0;
  p->string_max = sysconf(_SC_BC_STRING_MAX);

  if (p->string_max == -1) {

    if (errno) {
      return BC_STATUS_NO_LIMIT;
    }

    p->string_max = BC_STRING_MAX_DEF;
  }
  else if (p->string_max > BC_STRING_MAX_DEF) {
    return BC_STATUS_INVALID_LIMIT;
  }
#endif

  s = bc_num_parse(&p->last, "0", 10, 0);

  if (s) return s;

  s = bc_num_parse(&p->zero, "0", 10, 0);

  if (s) goto zero_err;

  s = bc_num_parse(&p->one, "1", 10, 0);

  if (s) goto one_err;

  p->num_buf = malloc(BC_PROGRAM_BUF_SIZE + 1);

  if (!p->num_buf) {
    s = BC_STATUS_MALLOC_FAIL;
    goto num_buf_err;
  }

  p->buf_size = BC_PROGRAM_BUF_SIZE;

  s = bc_vec_init(&p->funcs, sizeof(BcFunc), bc_func_free);

  if (s) {
    goto func_err;
  }

  s = bc_veco_init(&p->func_map, sizeof(BcEntry), bc_entry_free, bc_entry_cmp);

  if (s) {
    goto func_map_err;
  }

  name = malloc(1);

  if (!name) {
    s = BC_STATUS_MALLOC_FAIL;
    goto name_err;
  }

  name[0] = '\0';

  s = bc_program_func_add(p, name, &idx);

  if (s) {
    goto var_err;
  }

  name = NULL;

  s = bc_vec_init(&p->vars, sizeof(BcVar), bc_var_free);

  if (s) {
    goto var_err;
  }

  s = bc_veco_init(&p->var_map, sizeof(BcEntry), bc_entry_free, bc_entry_cmp);

  if (s) {
    goto var_map_err;
  }

  s = bc_vec_init(&p->arrays, sizeof(BcArray), bc_array_free);

  if (s) {
    goto array_err;
  }

  s = bc_veco_init(&p->array_map, sizeof(BcEntry), bc_entry_free, bc_entry_cmp);

  if (s) {
    goto array_map_err;
  }

  s = bc_vec_init(&p->strings, sizeof(char*), bc_string_free);

  if (s) {
    goto string_err;
  }

  s = bc_vec_init(&p->constants, sizeof(char*), bc_constant_free);

  if (s) {
    goto const_err;
  }

  s = bc_vec_init(&p->expr_stack, sizeof(BcNum), bc_result_free);

  if (s) {
    goto expr_err;
  }

  s = bc_vec_init(&p->stack, sizeof(BcInstPtr), NULL);

  if (s) {
    goto stack_err;
  }

  ip.idx = 0;
  ip.func = 0;

  s = bc_vec_push(&p->stack, &ip);

  if (s) {
    goto local_err;
  }

  s = bc_vec_init(&p->locals, sizeof(BcLocal), bc_local_free);

  if (s) {
    goto local_err;
  }

  s = bc_vec_init(&p->temps, sizeof(BcTemp), bc_temp_free);

  if (s) {
    goto temps_err;
  }

  return s;

temps_err:

  bc_vec_free(&p->locals);

local_err:

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

  if (name) {
    free(name);
  }

name_err:

  bc_veco_free(&p->func_map);

func_map_err:

  bc_vec_free(&p->funcs);

func_err:

  free(p->num_buf);

num_buf_err:

  bc_num_destruct(&p->one);

one_err:

  bc_num_destruct(&p->zero);

zero_err:

  bc_num_destruct(&p->last);

  return s;
}

void bc_program_limits(BcProgram* p) {

  putchar('\n');

  printf("BC_BASE_MAX     = %ld\n", p->base_max);
  printf("BC_DIM_MAX      = %ld\n", p->dim_max);
  printf("BC_SCALE_MAX    = %ld\n", p->scale_max);
  printf("BC_STRING_MAX   = %ld\n", p->string_max);
  printf("Max Exponent    = %ld\n", INT64_MAX);
  printf("Number of Vars  = %u\n", UINT32_MAX);

  putchar('\n');
}

BcStatus bc_program_func_add(BcProgram* p, char* name, size_t* idx) {

  BcStatus status;
  BcEntry entry;
  BcFunc f;

  if (!p || !name || !idx) {
    return BC_STATUS_INVALID_PARAM;
  }

  entry.name = name;
  entry.idx = p->funcs.len;

  status = bc_veco_insert(&p->func_map, &entry, idx);

  if (status == BC_STATUS_VECO_ITEM_EXISTS) {

    BcFunc* func;

    func = bc_vec_item(&p->funcs, *idx);

    if (!func) {
      return BC_STATUS_EXEC_UNDEFINED_FUNC;
    }

    // We need to reset these so the function can be repopulated.
    func->num_autos = 0;
    func->num_params = 0;
    func->code.len = 0;
    func->labels.len = 0;

    return BC_STATUS_SUCCESS;
  }
  else if (status) {
    return status;
  }

  status = bc_func_init(&f);

  if (status) {
    return status;
  }

  return bc_vec_push(&p->funcs, &f);
}

BcStatus bc_program_var_add(BcProgram* p, char* name, size_t* idx) {

  BcStatus status;
  BcEntry entry;
  BcVar v;

  if (!p || !name || !idx) {
    return BC_STATUS_INVALID_PARAM;
  }

  entry.name = name;
  entry.idx = p->vars.len;

  status = bc_veco_insert(&p->var_map, &entry, idx);

  if (status) {
    return status == BC_STATUS_VECO_ITEM_EXISTS ?
          BC_STATUS_SUCCESS : status;
  }

  status = bc_var_init(&v);

  if (status) {
    return status;
  }

  return bc_vec_push(&p->vars, &v);
}

BcStatus bc_program_array_add(BcProgram* p, char* name, size_t* idx) {

  BcStatus status;
  BcEntry entry;
  BcArray a;

  if (!p || !name || !idx) {
    return BC_STATUS_INVALID_PARAM;
  }

  entry.name = name;
  entry.idx = p->arrays.len;

  status = bc_veco_insert(&p->array_map, &entry, idx);

  if (status) {
    return status == BC_STATUS_VECO_ITEM_EXISTS ?
          BC_STATUS_SUCCESS : status;
  }

  status = bc_array_init(&a);

  if (status) {
    return status;
  }

  return bc_vec_push(&p->arrays, &a);
}

BcStatus bc_program_exec(BcProgram* p) {

  BcStatus status;
  int pchars;
  BcFunc* func;
  uint8_t* code;
  BcInstPtr* ip;
  size_t idx;

  status = BC_STATUS_SUCCESS;

  ip = bc_vec_top(&p->stack);

  func = bc_vec_item(&p->funcs, ip->func);

  assert(func);

  code = func->code.array;

  for (; ip->idx < func->code.len; ++ip->idx) {

    uint8_t inst;

    inst = code[ip->idx];

    // TODO: Add all instructions.
    switch (inst) {

      case BC_INST_READ:
      {
        status = bc_program_read(p);
        break;
      }

      case BC_INST_PRINT:
      {
        BcResult* num;

        num = bc_vec_top(&p->expr_stack);

        bc_num_print(&num->num, p->obase);

        bc_vec_pop(&p->expr_stack);

        break;
      }

      case BC_INST_STR:
      {
        const char* string;

        idx = bc_program_index(code, &ip->idx);

        if (idx >= p->strings.len) {
          return BC_STATUS_EXEC_INVALID_STRING;
        }

        string = bc_vec_item(&p->strings, idx);

        pchars = fprintf(stdout, "%s", string);
        status = pchars > 0 ? BC_STATUS_SUCCESS :
                              BC_STATUS_EXEC_PRINT_ERR;

        break;
      }

      case BC_INST_PRINT_STR:
      {
        const char* string;

        idx = bc_program_index(code, &ip->idx);

        if (idx >= p->strings.len) {
          return BC_STATUS_EXEC_INVALID_STRING;
        }

        string = bc_vec_item(&p->strings, idx);

        status = bc_program_printString(string);

        break;
      }

      case BC_INST_HALT:
      {
        status = BC_STATUS_EXEC_HALT;
        break;
      }

      case BC_INST_PUSH_NUM:
      {
        size_t idx;
        BcResult result;
        char* str;

        idx = bc_program_index(code, &ip->idx);

        str = *((char**) bc_vec_item(&p->constants, idx));

        if (!str) {
          return BC_STATUS_EXEC_INVALID_EXPR;
        }

        //result.num = arb_str2fxdpnt(str);

        //if (!result.num) {
        //  return BC_STATUS_EXEC_INVALID_EXPR;
        //}

        result.type = BC_NUM_CONSTANT;

        status = bc_vec_push(&p->expr_stack, &result);

        break;
      }

      case BC_INST_OP_MODULUS:
      case BC_INST_OP_MULTIPLY:
      case BC_INST_OP_PLUS:
      case BC_INST_OP_MINUS:
      case BC_INST_OP_DIVIDE:
      case BC_INST_OP_POWER:
      {
        BcResult* ptr;
        BcResult num1;
        BcResult num2;
        BcResult result;

        ptr = bc_vec_top(&p->expr_stack);

        if (!ptr) return BC_STATUS_EXEC_INVALID_EXPR;

        num2 = *ptr;

        status = bc_vec_pop(&p->expr_stack);

        if (status) return status;

        ptr = bc_vec_top(&p->expr_stack);

        if (!ptr) return BC_STATUS_EXEC_INVALID_EXPR;

        num1 = *ptr;

        status = bc_vec_pop(&p->expr_stack);

        if (status) return status;

        result.type = BC_NUM_RESULT;

        status = bc_num_construct(&result.num, BC_NUM_DEF_SIZE);

        if (status) return status;

        if (inst != BC_INST_OP_POWER) {

          BcMathOpFunc op;

          op = bc_math_ops[inst - BC_INST_OP_MODULUS];

          status = op(&num1.num, &num2.num, &result.num, p->scale);
        }
        else {
          // TODO: Power.
        }

        if (status) return status;

        status = bc_vec_push(&p->expr_stack, &result);

        break;
      }

      default:
      {
        return BC_STATUS_EXEC_INVALID_STMT;
      }
    }
  }

  return status;

  return BC_STATUS_SUCCESS;
}

void bc_program_printCode(BcProgram* p) {

  BcFunc* func;
  uint8_t* code;
  BcInstPtr* ip;

  ip = bc_vec_top(&p->stack);

  func = bc_vec_item(&p->funcs, ip->func);

  assert(func);

  code = func->code.array;

  for (; ip->idx < func->code.len; ++ip->idx) {

    uint8_t inst;

    inst = code[ip->idx];

    // TODO: Fill this out.
    switch (inst) {

      case BC_INST_PUSH_VAR:
      case BC_INST_PUSH_ARRAY:
      {
        putchar(inst);
        bc_program_printName(code, &ip->idx);
        break;
      }

      case BC_INST_CALL:
      {
        putchar(inst);

        bc_program_printIndex(code, &ip->idx);
        bc_program_printIndex(code, &ip->idx);

        break;
      }

      case BC_INST_JUMP:
      case BC_INST_JUMP_NOT_ZERO:
      case BC_INST_JUMP_ZERO:
      case BC_INST_PUSH_NUM:
      case BC_INST_STR:
      case BC_INST_PRINT_STR:
      {
        putchar(inst);
        bc_program_printIndex(code, &ip->idx);
        break;
      }

      default:
      {
        putchar(inst);
        break;
      }
    }
  }

  putchar('\n');
}

void bc_program_free(BcProgram* p) {

  if (p == NULL) {
    return;
  }

  free(p->num_buf);

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
  bc_vec_free(&p->locals);
  bc_vec_free(&p->temps);

  bc_num_destruct(&p->last);
  bc_num_destruct(&p->zero);
  bc_num_destruct(&p->one);

  memset(p, 0, sizeof(BcProgram));
}

static BcStatus bc_program_printString(const char* str) {

  char c;
  char c2;
  size_t len;
  int err;

  len = strlen(str);

  for (size_t i = 0; i < len; ++i) {

    c = str[i];

    if (c != '\\') {
      err = fputc(c, stdout);
    }
    else {

      ++i;

      if (i >= len) {
        return BC_STATUS_EXEC_INVALID_STRING;
      }

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

    if (err == EOF) {
      return BC_STATUS_EXEC_PRINT_ERR;
    }
  }

  return BC_STATUS_SUCCESS;
}

static BcStatus bc_program_execExpr(BcProgram* p, BcVec* exprs,
                                    BcNum* num, bool print)
{
#if 0
  BcStatus status;
  uint32_t idx;
  BcExpr* expr;
  BcTemp temp;
  uint32_t temp_len;
  fxdpnt* temp_num;
  BcExprType etype;
  BcTempType ttype;
  BcTemp* temp_ptr;

  status = BC_STATUS_SUCCESS;

  temp_len = p->temps.len;

  idx = exprs->len - 1;

  while (idx < exprs->len) {

    expr = bc_vec_item(exprs, idx);

    if (!expr) {
      return BC_STATUS_EXEC_INVALID_EXPR;
    }

    etype = expr->type;

    switch (etype) {

      case BC_EXPR_INC_PRE:
      case BC_EXPR_DEC_PRE:
      {
        if (idx - 1 >= exprs->len) {
          status = BC_STATUS_EXEC_INVALID_EXPR;
          break;
        }

        expr = bc_vec_item(exprs, idx - 1);

        if (!expr) {
          status = BC_STATUS_EXEC_INVALID_EXPR;
          break;
        }

        if (expr->type != BC_EXPR_VAR && expr->type != BC_EXPR_ARRAY_ELEM &&
            !(expr->type < BC_EXPR_SCALE || expr->type > BC_EXPR_LAST))
        {
          status = BC_STATUS_EXEC_INVALID_EXPR;
          break;
        }

        status = bc_program_assign(p, expr, etype + BC_EXPR_ASSIGN_PLUS, p->one);

        break;
      }

      case BC_EXPR_INC_POST:
      case BC_EXPR_DEC_POST:
      {
        // TODO: Do this.
        break;
      }

      case BC_EXPR_NEGATE:
      {
        char sign;

        if (p->temps.len <= temp_len) {
          status = BC_STATUS_EXEC_INVALID_EXPR;
          break;
        }

        temp_ptr = bc_vec_top(&p->temps);

        if (!temp_ptr) {
          status = BC_STATUS_EXEC_INVALID_EXPR;
          break;
        }

        sign = temp_ptr->num->sign;

        temp_ptr->num->sign = sign == '+' ? '-' : '+';

        break;
      }

      case BC_EXPR_POWER:
      case BC_EXPR_MULTIPLY:
      case BC_EXPR_DIVIDE:
      case BC_EXPR_MODULUS:
      case BC_EXPR_PLUS:
      case BC_EXPR_MINUS:
      {
        BcTemp* a;
        BcTemp* b;
        BcTemp result;
        BcMathOpFunc op;

        if (p->temps.len < temp_len + 2) {
          status = BC_STATUS_EXEC_INVALID_EXPR;
          break;
        }

        b = bc_vec_top(&p->temps);

        if (!b) {
          status = BC_STATUS_EXEC_INVALID_EXPR;
          break;
        }

        status = bc_vec_pop(&p->temps);

        if (status) {
          break;
        }

        a = bc_vec_top(&p->temps);

        if (!a) {
          status = BC_STATUS_EXEC_INVALID_EXPR;
          break;
        }

        status = bc_vec_pop(&p->temps);

        if (status) {
          break;
        }

        result.type = BC_TEMP_NUM;
        result.num = arb_alloc(a->num->len + b->num->len);

        // TODO: Handle scale.
        op = bc_math_ops[etype - BC_EXPR_POWER];
        result.num = op(a->num, b->num, result.num, 10, 0);

        status = bc_vec_push(&p->temps, &result);

        break;
      }

      case BC_EXPR_ASSIGN_POWER:
      case BC_EXPR_ASSIGN_MULTIPLY:
      case BC_EXPR_ASSIGN_DIVIDE:
      case BC_EXPR_ASSIGN_MODULUS:
      case BC_EXPR_ASSIGN_PLUS:
      case BC_EXPR_ASSIGN_MINUS:
      case BC_EXPR_ASSIGN:
      {
        // TODO: Get the amount.
        status = bc_program_assign(p, expr, etype, NULL);
        break;
      }

      case BC_EXPR_REL_EQUAL:
      case BC_EXPR_REL_LESS_EQ:
      case BC_EXPR_REL_GREATER_EQ:
      case BC_EXPR_REL_NOT_EQ:
      case BC_EXPR_REL_LESS:
      case BC_EXPR_REL_GREATER:
      {
        break;
      }

      case BC_EXPR_BOOL_NOT:
      {
        break;
      }

      case BC_EXPR_BOOL_OR:
      {
        break;
      }

      case BC_EXPR_BOOL_AND:
      {
        break;
      }

      case BC_EXPR_NUMBER:
      {
        status = bc_temp_initNum(&temp, expr->string);

        if (status) {
          break;
        }

        status = bc_vec_push(&p->temps, &temp);

        break;
      }

      case BC_EXPR_VAR:
      {
        break;
      }

      case BC_EXPR_ARRAY_ELEM:
      {
        break;
      }

      case BC_EXPR_FUNC_CALL:
      {
        break;
      }

      case BC_EXPR_SCALE_FUNC:
      {
        break;
      }

      case BC_EXPR_SCALE:
      case BC_EXPR_IBASE:
      case BC_EXPR_OBASE:
      {
        ttype = etype - BC_EXPR_SCALE + BC_TEMP_SCALE;

        status = bc_temp_init(&temp, ttype);

        if (status) {
          break;
        }

        status = bc_vec_push(&p->temps, &temp);

        break;
      }

      case BC_EXPR_LAST:
      {
        break;
      }

      case BC_EXPR_LENGTH:
      {
        break;
      }

      case BC_EXPR_READ:
      {
        status = bc_program_read(p);
        break;
      }

      case BC_EXPR_SQRT:
      {
        status = bc_program_execExpr(p, expr->exprs, &temp_num, false);

        if (status) {
          break;
        }

        if (temp_num->sign != '-') {

          status = bc_temp_initNum(&temp, NULL);

          if (status) {
            break;
          }

          arb_newton_sqrt(temp_num, temp.num, 10, p->scale);
        }
        else {
          status = BC_STATUS_MATH_NEG_SQRT;
        }

        break;
      }

      case BC_EXPR_PRINT:
      {
        if (!print) {
          break;
        }

        if (p->temps.len != temp_len + 1) {
          status = BC_STATUS_EXEC_INVALID_EXPR;
          break;
        }

        temp_ptr = bc_vec_top(&p->temps);

        arb_copy(p->last, temp_ptr->num);

        arb_print(p->last);

        status = bc_vec_pop(&p->temps);

        break;
      }

      default:
      {
        status = BC_STATUS_EXEC_INVALID_EXPR;
        break;
      }
    }

    --idx;
  }

  if (status) {
    return status;
  }

  if (p->temps.len != temp_len) {
    return BC_STATUS_EXEC_INVALID_EXPR;
  }

  return status;
#endif
  return BC_STATUS_SUCCESS;
}

#if 0
static BcStatus bc_program_assign(BcProgram* p, BcExpr* expr,
                                  BcExprType op, fxdpnt* amt)
{
  BcStatus status;
  fxdpnt* left;

  switch (expr->type) {

    case BC_EXPR_VAR:
    {
      break;
    }

    case BC_EXPR_ARRAY_ELEM:
    {
      break;
    }

    case BC_EXPR_SCALE:
    {
      break;
    }

    case BC_EXPR_IBASE:
    {
      break;
    }

    case BC_EXPR_OBASE:
    {
      break;
    }

    case BC_EXPR_LAST:
    {
      break;
    }

    default:
    {
      return BC_STATUS_EXEC_INVALID_EXPR;
    }
  }

  switch (op) {

    case BC_EXPR_ASSIGN_DIVIDE:
      if (!arb_compare(amt, p->zero, 10)) {
        return BC_STATUS_MATH_DIVIDE_BY_ZERO;
      }
      // Fallthrough.
    case BC_EXPR_ASSIGN_POWER:
    case BC_EXPR_ASSIGN_MULTIPLY:
    case BC_EXPR_ASSIGN_MODULUS:
    case BC_EXPR_ASSIGN_PLUS:
    case BC_EXPR_ASSIGN_MINUS:
    {
      // TODO: Get the right scale.
      left = bc_math_ops[op - BC_EXPR_ASSIGN_POWER](left, amt, left, 10, 10);
      status = BC_STATUS_SUCCESS;
      break;
    }

    case BC_EXPR_ASSIGN:
    {
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
#endif

static BcStatus bc_program_read(BcProgram* p) {

  BcStatus status;
  BcParse parse;
  char* buffer;
  BcVec exprs;
  BcTemp temp;
  size_t size;
  BcVec code;

  buffer = malloc(BC_PROGRAM_BUF_SIZE + 1);

  if (!buffer) {
    return BC_STATUS_MALLOC_FAIL;
  }

  status = bc_vec_init(&code, sizeof(uint8_t), NULL);

  if (status) {
    goto vec_err;
  }

  size = BC_PROGRAM_BUF_SIZE;

  if (bc_io_getline(&buffer, &size) == BC_INVALID_IDX) {
    status = BC_STATUS_IO_ERR;
    goto io_err;
  }

  status = bc_parse_init(&parse, p);

  if (status) {
    goto io_err;
  }

  status = bc_parse_file(&parse, "<stdin>");

  if (status) {
    goto exec_err;
  }

  status = bc_parse_text(&parse, buffer);

  if (status) {
    goto exec_err;
  }

  status = bc_parse_expr(&parse, &code, false, false);

  if (status != BC_STATUS_LEX_EOF && status != BC_STATUS_PARSE_EOF) {
    status = status ? status : BC_STATUS_EXEC_INVALID_READ_EXPR;
    goto exec_err;
  }

  temp.type = BC_TEMP_NUM;

  status = bc_program_execExpr(p, &exprs, &temp.num, false);

  if (status) {
    goto exec_err;
  }

  status = bc_vec_push(&p->temps, &temp);

exec_err:

  bc_parse_free(&parse);

io_err:

  bc_vec_free(&code);

vec_err:

  free(buffer);

  return status;
}

static size_t bc_program_index(uint8_t* code, size_t* start) {

  uint8_t bytes;
  uint8_t byte;
  size_t result;

  bytes = code[++(*start)];

  result = 0;

  for (uint8_t i = 0; i < bytes; ++i) {

    byte = code[++(*start)];

    result |= (((size_t) byte) << (i * 8));
  }

  return result;
}

static void bc_program_printIndex(uint8_t* code, size_t* start) {

  uint8_t bytes;
  uint8_t byte;

  bytes = code[++(*start)];

  printf(bc_byte_fmt, bytes);

  for (uint8_t i = 0; i < bytes; ++i) {

    byte = code[++(*start)];

    printf(bc_byte_fmt, byte);
  }
}

static void bc_program_printName(uint8_t* code, size_t* start) {

  char byte;

  byte = code[++(*start)];

  while (byte != ':') {
    putchar(byte);
    byte = code[++(*start)];
  }

  putchar(byte);
}
