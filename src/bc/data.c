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
 * Code to manipulate data structures in programs.
 *
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <data.h>

static BcStatus bc_func_insert(char *name, bool var, BcVec *vec) {

  BcStatus status;
  BcAuto a;

  if (!name || !vec) return BC_STATUS_INVALID_PARAM;

  a.name = name;
  a.var = var;

  if (var) status = bc_num_init(&a.data.num, BC_NUM_DEF_SIZE);
  else status = bc_vec_init(&a.data.array, sizeof(BcNum), bc_num_free);

  if (status) goto init_err;

  status = bc_vec_push(vec, &a);

  if (status) goto err;

  return status;

err:

  if (var) bc_num_free(&a.data.num);
  else bc_vec_free(&a.data.array);

init_err:

  free(name);

  return status;
}

BcStatus bc_func_init(BcFunc *func) {

  BcStatus status;

  if (!func) return BC_STATUS_INVALID_PARAM;

  status = bc_vec_init(&func->code, sizeof(uint8_t), NULL);

  if (status) return status;

  status = bc_vec_init(&func->params, sizeof(BcAuto), bc_auto_free);

  if (status) goto param_err;

  status = bc_vec_init(&func->autos, sizeof(BcAuto), bc_auto_free);

  if (status) goto auto_err;

  status = bc_vec_init(&func->labels, sizeof(size_t), NULL);

  if (status) goto label_err;

  return BC_STATUS_SUCCESS;

label_err:

  bc_vec_free(&func->autos);

auto_err:

  bc_vec_free(&func->params);

param_err:

  bc_vec_free(&func->code);

  return status;
}

BcStatus bc_func_insertParam(BcFunc *func, char *name, bool var) {
  return bc_func_insert(name, var, &func->params);
}

BcStatus bc_func_insertAuto(BcFunc *func, char *name, bool var) {
  return bc_func_insert(name, var, &func->autos);
}

void bc_func_free(void *func) {

  BcFunc *f;

  f = (BcFunc*) func;

  if (f == NULL) return;

  bc_vec_free(&f->code);
  bc_vec_free(&f->params);
  bc_vec_free(&f->autos);
  bc_vec_free(&f->labels);
}

BcStatus bc_var_init(BcVar *var) {

  if (!var) return BC_STATUS_INVALID_PARAM;

  return bc_num_init(var, BC_NUM_DEF_SIZE);
}

void bc_var_free(void *var) {

  BcVar *v;

  v = (BcVar*) var;

  if (v == NULL) return;

  bc_num_free(v);
}

BcStatus bc_array_init(BcArray *array) {

  if (!array) return BC_STATUS_INVALID_PARAM;

  return bc_vec_init(array, sizeof(BcNum), bc_num_free);
}

void bc_array_free(void *array) {

  BcArray *a;

  a = (BcArray*) array;

  if (a == NULL) return;

  bc_vec_free(a);
}

void bc_string_free(void *string) {

  char *s;

  s = *((char**) string);

  free(s);
}

int bc_entry_cmp(void *entry1, void *entry2) {

  BcEntry *e1;
  BcEntry *e2;

  e1 = (BcEntry*) entry1;
  e2 = (BcEntry*) entry2;

  return strcmp(e1->name, e2->name);
}

void bc_entry_free(void *entry) {

  BcEntry *e;

  if (!entry) return;

  e = (BcEntry*) entry;

  free(e->name);
}

void bc_auto_free(void *auto1) {

  BcAuto *a;

  if (!auto1) return;

  a = (BcAuto*) auto1;

  free(a->name);

  if (a->var) bc_num_free(&a->data.num);
  else bc_vec_free(&a->data.array);
}

void bc_result_free(void *result) {

  BcResult *r;

  if (!result) return;

  r = (BcResult*) result;

  switch (r->type) {

    case BC_RESULT_INTERMEDIATE:
    case BC_RESULT_SCALE:
    {
      bc_num_free(&r->data.num);
      break;
    }

    case BC_RESULT_VAR:
    case BC_RESULT_ARRAY:
    {
      free(r->data.id.name);
      break;
    }

    default:
    {
      // Do nothing.
      break;
    }
  }
}

void bc_constant_free(void *constant) {

  char *c;

  c = *((char**) constant);

  free(c);
}
