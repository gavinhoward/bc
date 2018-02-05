#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <arbprec/arbprec.h>

#include <bc/io.h>
#include <bc/program.h>
#include <bc/parse.h>

static fxdpnt* bc_program_add(fxdpnt* a, fxdpnt* b, fxdpnt* c, int base, size_t scale);
static fxdpnt* bc_program_sub(fxdpnt* a, fxdpnt* b, fxdpnt* c, int base, size_t scale);

static const BcMathOpFunc bc_math_ops[] = {

  NULL,//arb_exp,

  arb_mul,
  arb_alg_d,
  arb_mod,

  bc_program_add,
  bc_program_sub

};

static BcStatus bc_program_execList(BcProgram* p, BcStmtList* list);
static BcStatus bc_program_printString(const char* str);
static BcStatus bc_program_execExpr(BcProgram* p, BcVec* exprs,
                                    fxdpnt** num, bool print);
static BcStatus bc_program_assign(BcProgram* p, BcExpr* expr,
                                  BcExprType op, fxdpnt* amt);
static BcStatus bc_program_read(BcProgram* p);

BcStatus bc_program_init(BcProgram* p) {

  BcStatus st;

  if (p == NULL) {
    return BC_STATUS_INVALID_PARAM;
  }

  p->scale = 0;
  p->ibase = 10;
  p->obase = 10;

  p->last = arb_str2fxdpnt("0");
  p->zero = arb_str2fxdpnt("0");
  p->one = arb_str2fxdpnt("1");

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

  p->num_buf = malloc(BC_PROGRAM_BUF_SIZE + 1);

  if (!p->num_buf) {
    return BC_STATUS_MALLOC_FAIL;
  }

  p->buf_size = BC_PROGRAM_BUF_SIZE;

  p->list = bc_list_create();

  if (!p->list) {
    st = BC_STATUS_MALLOC_FAIL;
    goto list_err;
  }

  st = bc_segarray_init(&p->funcs, sizeof(BcFunc), bc_func_free, bc_func_cmp);

  if (st) {
    goto func_err;
  }

  st = bc_segarray_init(&p->vars, sizeof(BcVar), bc_var_free, bc_var_cmp);

  if (st) {
    goto var_err;
  }

  st = bc_segarray_init(&p->arrays, sizeof(BcArray),
                        bc_array_free, bc_array_cmp);

  if (st) {
    goto array_err;
  }

  st = bc_vec_init(&p->ctx_stack, sizeof(BcStmtList*), NULL);

  if (st) {
    goto ctx_err;
  }

  st = bc_vec_init(&p->locals, sizeof(BcLocal), bc_local_free);

  if (st) {
    goto local_err;
  }

  st = bc_vec_init(&p->temps, sizeof(BcTemp), bc_temp_free);

  if (st) {
    goto temps_err;
  }

  return st;

list_err:

  free(p->num_buf);

temps_err:

  bc_vec_free(&p->locals);

local_err:

  bc_vec_free(&p->ctx_stack);

ctx_err:

  bc_segarray_free(&p->arrays);

array_err:

  bc_segarray_free(&p->vars);

var_err:

  bc_segarray_free(&p->funcs);

func_err:

  bc_list_free(p->list);
  p->list = NULL;

  return st;
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

BcStatus bc_program_func_add(BcProgram* p, BcFunc* func) {

  if (!p || !func) {
    return BC_STATUS_INVALID_PARAM;
  }

  return bc_segarray_add(&p->funcs, func);
}

BcStatus bc_program_var_add(BcProgram* p, BcVar* var) {

  if (!p || !var) {
    return BC_STATUS_INVALID_PARAM;
  }

  return bc_segarray_add(&p->vars, var);
}

BcStatus bc_program_array_add(BcProgram* p, BcArray* array) {

  if (!p || !array) {
    return BC_STATUS_INVALID_PARAM;
  }

  return bc_segarray_add(&p->arrays, array);
}

BcStatus bc_program_exec(BcProgram* p) {

  BcStmtList* list;
  BcStatus status;

  status = bc_program_execList(p, p->list);

  if (status) {
    return status;
  }

  while (p->list->idx >= BC_PROGRAM_MAX_STMTS) {

    if (p->list->next) {

      list = p->list;
      p->list = list->next;

      bc_list_destruct(list);
    }
    else {

      bc_list_destruct(p->list);

      p->list = bc_list_create();

      if (!p->list) {
        return BC_STATUS_MALLOC_FAIL;
      }
    }
  }

  return BC_STATUS_SUCCESS;
}

void bc_program_free(BcProgram* p) {

  if (p == NULL) {
    return;
  }

  free(p->num_buf);
  p->buf_size = 0;

  bc_list_free(p->list);

  bc_segarray_free(&p->funcs);
  bc_segarray_free(&p->vars);
  bc_segarray_free(&p->arrays);

  bc_vec_free(&p->ctx_stack);
  bc_vec_free(&p->locals);
  bc_vec_free(&p->temps);

  arb_free(p->last);
  arb_free(p->zero);
  arb_free(p->one);
}

static BcStatus bc_program_execList(BcProgram* p, BcStmtList* list) {

  BcStatus status;
  BcStmtList* next;
  BcStmtList* cur;
  BcStmtType type;
  BcStmt* stmt;
  int pchars;
  fxdpnt* result;

  assert(list);

  status = BC_STATUS_SUCCESS;

  cur = list;

  do {

    next = cur->next;

    while (cur->idx < cur->num_stmts) {

      stmt = cur->stmts + cur->idx;
      type = stmt->type;

      ++cur->idx;

      switch (type) {

        case BC_STMT_EXPR:
        {
          status = bc_program_execExpr(p, stmt->data.exprs, &result, true);

          if (status) {
            break;
          }



          break;
        }

        case BC_STMT_STRING:
        {
          pchars = fprintf(stdout, "%s", stmt->data.string);
          status = pchars > 0 ? BC_STATUS_SUCCESS :
                                BC_STATUS_VM_PRINT_ERR;
          break;
        }

        case BC_STMT_STRING_PRINT:
        {
          status = bc_program_printString(stmt->data.string);
          break;
        }

        case BC_STMT_BREAK:
        {
          status = BC_STATUS_VM_BREAK;
          break;
        }

        case BC_STMT_CONTINUE:
        {
          status = BC_STATUS_VM_CONTINUE;
          break;
        }

        case BC_STMT_HALT:
        {
          status = BC_STATUS_VM_HALT;
          break;
        }

        case BC_STMT_RETURN:
        {
          break;
        }

        case BC_STMT_IF:
        {
          break;
        }

        case BC_STMT_WHILE:
        {
          break;
        }

        case BC_STMT_FOR:
        {
          break;
        }

        case BC_STMT_LIST:
        {
          status = bc_program_execList(p, stmt->data.list);
          break;
        }

        default:
        {
          return BC_STATUS_VM_INVALID_STMT;
        }
      }
    }

    if (cur->idx == cur->num_stmts) {
      cur = next;
    }
    else {
      cur = NULL;
    }

  } while (!status && cur);

  return status;
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
        return BC_STATUS_VM_INVALID_STRING;
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
      return BC_STATUS_VM_PRINT_ERR;
    }
  }

  return BC_STATUS_SUCCESS;
}

static BcStatus bc_program_execExpr(BcProgram* p, BcVec* exprs,
                                    fxdpnt** num, bool print)
{
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
      return BC_STATUS_VM_INVALID_EXPR;
    }

    etype = expr->type;

    switch (etype) {

      case BC_EXPR_INC_PRE:
      case BC_EXPR_DEC_PRE:
      {
        if (idx - 1 >= exprs->len) {
          status = BC_STATUS_VM_INVALID_EXPR;
          break;
        }

        expr = bc_vec_item(exprs, idx - 1);

        if (!expr) {
          status = BC_STATUS_VM_INVALID_EXPR;
          break;
        }

        if (expr->type != BC_EXPR_VAR && expr->type != BC_EXPR_ARRAY_ELEM &&
            !(expr->type < BC_EXPR_SCALE || expr->type > BC_EXPR_LAST))
        {
          status = BC_STATUS_VM_INVALID_EXPR;
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
          status = BC_STATUS_VM_INVALID_EXPR;
          break;
        }

        temp_ptr = bc_vec_top(&p->temps);

        if (!temp_ptr) {
          status = BC_STATUS_VM_INVALID_EXPR;
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
          status = BC_STATUS_VM_INVALID_EXPR;
          break;
        }

        b = bc_vec_top(&p->temps);

        if (!b) {
          status = BC_STATUS_VM_INVALID_EXPR;
          break;
        }

        status = bc_vec_pop(&p->temps);

        if (status) {
          break;
        }

        a = bc_vec_top(&p->temps);

        if (!a) {
          status = BC_STATUS_VM_INVALID_EXPR;
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
          status = BC_STATUS_VM_NEG_SQRT;
        }

        break;
      }

      case BC_EXPR_PRINT:
      {
        if (!print) {
          break;
        }

        if (p->temps.len != temp_len + 1) {
          status = BC_STATUS_VM_INVALID_EXPR;
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
        status = BC_STATUS_VM_INVALID_EXPR;
        break;
      }
    }

    --idx;
  }

  if (status) {
    return status;
  }

  if (p->temps.len != temp_len) {
    return BC_STATUS_VM_INVALID_EXPR;
  }

  return status;
}

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
      return BC_STATUS_VM_INVALID_EXPR;
    }
  }

  switch (op) {

    case BC_EXPR_ASSIGN_DIVIDE:
      if (!arb_compare(amt, p->zero, 10)) {
        return BC_STATUS_VM_DIVIDE_BY_ZERO;
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
      status = BC_STATUS_VM_INVALID_EXPR;
      break;
    }
  }

  return status;
}

static BcStatus bc_program_read(BcProgram* p) {

  BcStatus status;
  BcParse parse;
  char* buffer;
  BcVec exprs;
  BcTemp temp;
  size_t size;

  buffer = malloc(BC_PROGRAM_BUF_SIZE + 1);

  if (!buffer) {
    return BC_STATUS_MALLOC_FAIL;
  }

  status = bc_vec_init(&exprs, sizeof(BcExpr), bc_expr_free);

  if (status) {
    goto stack_err;
  }

  size = BC_PROGRAM_BUF_SIZE;

  if (bc_io_getline(&buffer, &size) == (size_t) -1) {
    status = BC_STATUS_VM_FILE_READ_ERR;
    goto io_err;
  }

  status = bc_parse_init(&parse, p->list);

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

  status = bc_parse_expr(&parse, &exprs, false);

  if (status != BC_STATUS_LEX_EOF && status != BC_STATUS_PARSE_EOF) {
    status = status ? status : BC_STATUS_VM_INVALID_READ_EXPR;
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

  bc_vec_free(&exprs);

stack_err:

  free(buffer);

  return status;
}

static fxdpnt* bc_program_add(fxdpnt* a, fxdpnt* b, fxdpnt* c,
                              int base, size_t scale)
{
  (void) scale;
  return arb_add(a, b, c, base);
}

static fxdpnt* bc_program_sub(fxdpnt* a, fxdpnt* b, fxdpnt* c,
                              int base, size_t scale)
{
  (void) scale;
  return arb_sub(a, b, c, base);
}
