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

#include <lang.h>

BcStatus bc_func_insert(BcFunc *func, char *name, bool var) {

  BcStatus status;
  BcAuto a;
  size_t i;
  BcAuto *ptr;

  if (!func || !name) return BC_STATUS_INVALID_PARAM;

  for (i = 0; i < func->autos.len; ++i) {
    ptr = bc_vec_item(&func->autos, i);
    if (!strcmp(name, ptr->name))
      return BC_STATUS_PARSE_DUPLICATE_LOCAL;
  }

  bc_auto_init(&a, name, var);

  status = bc_vec_push(&func->autos, &a);

  if (status) return status;

  return status;
}

BcStatus bc_func_init(BcFunc *func) {

  BcStatus status;

  if (!func) return BC_STATUS_INVALID_PARAM;

  status = bc_vec_init(&func->code, sizeof(uint8_t), NULL);

  if (status) return status;

  status = bc_vec_init(&func->autos, sizeof(BcAuto), bc_auto_free);

  if (status) goto auto_err;

  status = bc_vec_init(&func->auto_stack, sizeof(BcLocal), bc_local_free);

  if (status) goto auto_stack_err;

  status = bc_vec_init(&func->labels, sizeof(size_t), NULL);

  if (status) goto label_err;

  func->num_params = 0;

  return BC_STATUS_SUCCESS;

label_err:

  bc_vec_free(&func->auto_stack);

auto_stack_err:

  bc_vec_free(&func->autos);

auto_err:

  bc_vec_free(&func->code);

  return status;
}

void bc_func_free(void *func) {

  BcFunc *f;

  f = (BcFunc*) func;

  if (f == NULL) return;

  bc_vec_free(&f->code);
  bc_vec_free(&f->autos);
  bc_vec_free(&f->auto_stack);
  bc_vec_free(&f->labels);
}

BcStatus bc_array_copy(void *dest, void *src) {

  BcStatus status;
  size_t i;
  BcNum *dnum;
  BcNum *snum;

  BcVec *d;
  BcVec *s;

  d = (BcVec*) dest;
  s = (BcVec*) src;

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

BcStatus bc_array_expand(BcVec *a, size_t len) {

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

void bc_string_free(void *string) {
  char **s = string;
  if (s) free(*s);
}

int bc_entry_cmp(void *entry1, void *entry2) {

  BcEntry *e1;
  BcEntry *e2;
  int cmp;

  e1 = (BcEntry*) entry1;
  e2 = (BcEntry*) entry2;

  if (!strcmp(e1->name, bc_lang_func_main)) {
    if (!strcmp(e2->name, bc_lang_func_main)) cmp = 0;
    else cmp = -1;
  }
  else if (!strcmp(e1->name, bc_lang_func_read)) {
    if (!strcmp(e2->name, bc_lang_func_main)) cmp = 1;
    else if (!strcmp(e2->name, bc_lang_func_read)) cmp = 0;
    else cmp = -1;
  }
  else if (!strcmp(e2->name, bc_lang_func_main)) cmp = 1;
  else cmp = strcmp(e1->name, e2->name);

  return cmp;
}

void bc_entry_free(void *entry) {
  BcEntry *e = entry;
  if (e) free(e->name);
}

void bc_auto_init(BcAuto *a, char *name, bool var) {
  if (!a) return;
  a->var = var;
  a->name = name;
}

void bc_auto_free(void *auto1) {
  BcAuto *a = (BcAuto*) auto1;
  if (!a) return;
  if (a->name) free(a->name);
}

void bc_result_free(void *result) {

  BcResult *r = (BcResult*) result;

  if (!r) return;

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
      if (r->data.id.name) free(r->data.id.name);
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
  char **c = constant;
  if (c) free(*c);
}

BcStatus bc_local_init(BcLocal *l, bool var) {
  if (!l) return BC_STATUS_INVALID_PARAM;
  l->var = var;
  if (var) return bc_num_init(&l->data.num, BC_NUM_DEF_SIZE);
  else return bc_vec_init(&l->data.array, sizeof(BcNum), bc_num_free);
}

void bc_local_free(void *local) {
  BcLocal *l = local;
  if (!l) return;
  if (l->var) bc_num_free(&l->data.num);
  else bc_vec_free(&l->data.array);
}
