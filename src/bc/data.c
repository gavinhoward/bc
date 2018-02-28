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

static BcStatus bc_func_insert(BcFunc *func, char *name, bool var, BcVec *vec) {

  BcStatus status;
  BcAuto a;
  size_t i;
  BcAuto *ptr;

  if (!name || !vec) return BC_STATUS_INVALID_PARAM;

  for (i = 0; i < func->params.len; ++i) {
    ptr = bc_vec_item(&func->params, i);
    if (!strcmp(name, ptr->name)) return BC_STATUS_PARSE_DUPLICATE_LOCAL;
  }

  for (i = 0; i < func->autos.len; ++i) {
    ptr = bc_vec_item(&func->autos, i);
    if (!strcmp(name, ptr->name)) return BC_STATUS_PARSE_DUPLICATE_LOCAL;
  }

  status = bc_auto_init(&a, name, var);

  if (status) goto err;

  status = bc_vec_push(vec, &a);

  if (status) goto err;

  return status;

err:

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
  return bc_func_insert(func, name, var, &func->params);
}

BcStatus bc_func_insertAuto(BcFunc *func, char *name, bool var) {
  return bc_func_insert(func, name, var, &func->autos);
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

BcStatus bc_var_init(void *var) {

  if (!var) return BC_STATUS_INVALID_PARAM;

  return bc_num_init((BcVar*) var, BC_NUM_DEF_SIZE);
}

void bc_var_free(void *var) {

  BcVar *v;

  v = (BcVar*) var;

  if (v == NULL) return;

  bc_num_free(v);
}

BcStatus bc_array_init(void *array) {

  if (!array) return BC_STATUS_INVALID_PARAM;

  return bc_vec_init((BcArray*) array, sizeof(BcNum), bc_num_free);
}

BcStatus bc_array_copy(void *dest, void *src) {

  BcStatus status;
  size_t i;
  BcNum *dnum;
  BcNum *snum;

  BcArray *d;
  BcArray *s;

  d = (BcArray*) dest;
  s = (BcArray*) src;

  if (!d || !s || d == s || d->size != s->size || d->dtor != s->dtor)
    return BC_STATUS_INVALID_PARAM;

  while (d->len) {
    status = bc_vec_pop(d);
    if (status) return status;
  }

  status = bc_vec_expand(d, s->cap);

  if (status) return status;

  d->len = s->len;

  for (i = 0; i < s->len; ++i) {

    dnum = bc_vec_item(d, i);
    snum = bc_vec_item(s, i);

    if (!dnum || !snum) return BC_STATUS_VEC_OUT_OF_BOUNDS;

    status = bc_num_init(dnum, snum->len);

    if (status) return status;

    status = bc_num_copy(dnum, snum);

    if (status) {
      bc_num_free(dnum);
      return status;
    }
  }

  return status;
}

BcStatus bc_array_zero(BcArray *a) {

  BcStatus status;

  status = BC_STATUS_SUCCESS;

  while (!status && a->len) status = bc_vec_pop(a);

  return status;
}

BcStatus bc_array_expand(BcArray *a, size_t len) {

  BcStatus status;

  status = BC_STATUS_SUCCESS;

  while (len > a->len) {

    BcNum num;

    status = bc_num_init(&num, BC_NUM_DEF_SIZE);

    if (status) return status;

    bc_num_zero(&num);

    status = bc_vec_push(a, &num);

    if (status) {
      bc_num_free(&num);
      return status;
    }
  }

  return status;
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

BcStatus bc_auto_init(void *auto1, char *name, bool var) {

  BcStatus status;
  BcAuto *a;

  if (!auto1) return BC_STATUS_INVALID_PARAM;

  a = (BcAuto*) auto1;

  a->var = var;
  a->name = name;

  if (var) status = bc_num_init(&a->data.num, BC_NUM_DEF_SIZE);
  else status = bc_vec_init(&a->data.array, sizeof(BcNum), bc_num_free);

  return status;
}

void bc_auto_free(void *auto1) {

  BcAuto *a;

  if (!auto1) return;

  a = (BcAuto*) auto1;

  if (a->name) free(a->name);

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
