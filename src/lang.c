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

BcStatus bc_func_insert(BcFunc *f, char *name, bool var) {

  BcAuto a;
  size_t i;

  assert(f && name);

  for (i = 0; i < f->autos.len; ++i) {
    if (!strcmp(name, ((BcAuto*) bc_vec_item(&f->autos, i))->name))
      return BC_STATUS_PARSE_DUPLICATE_LOCAL;
  }

  a.var = var;
  a.name = name;

  return bc_vec_push(&f->autos, &a);
}

BcStatus bc_func_init(BcFunc *f) {

  BcStatus status;

  assert(f);

  if ((status = bc_vec_init(&f->code, sizeof(uint8_t), NULL))) return status;
  if ((status = bc_vec_init(&f->autos, sizeof(BcAuto), bc_auto_free))) goto err;
  if ((status = bc_vec_init(&f->labels, sizeof(size_t), NULL))) goto label_err;

  f->nparams = 0;

  return BC_STATUS_SUCCESS;

label_err:
  bc_vec_free(&f->autos);
err:
  bc_vec_free(&f->code);
  return status;
}

void bc_func_free(void *func) {

  BcFunc *f = (BcFunc*) func;

  if (!f) return;

  bc_vec_free(&f->code);
  bc_vec_free(&f->autos);
  bc_vec_free(&f->labels);
}

BcStatus bc_array_copy(BcVec *d, BcVec *s) {

  BcStatus status;
  size_t i;
  BcNum *dnum, *snum;

  assert(d && s && d != s && d->size == s->size && d->dtor == s->dtor);

  bc_vec_npop(d, d->len);

  if ((status = bc_vec_expand(d, s->cap))) return status;

  d->len = s->len;

  for (i = 0; !status && i < s->len; ++i) {

    dnum = bc_vec_item(d, i);
    snum = bc_vec_item(s, i);

    if ((status = bc_num_init(dnum, snum->len))) return status;
    if ((status = bc_num_copy(dnum, snum))) bc_num_free(dnum);
  }

  return status;
}

BcStatus bc_array_expand(BcVec *a, size_t len) {

  BcStatus status = BC_STATUS_SUCCESS;
  BcNum num;

  while (!status && len > a->len) {
    if ((status = bc_num_init(&num, BC_NUM_DEF_SIZE))) return status;
    bc_num_zero(&num);
    if ((status = bc_vec_push(a, &num))) bc_num_free(&num);
  }

  return status;
}

void bc_string_free(void *string) {
  char **s = string;
  if (s) free(*s);
}

int bc_entry_cmp(void *entry1, void *entry2) {
  return strcmp(((BcEntry*) entry1)->name, ((BcEntry*) entry2)->name);
}

void bc_entry_free(void *entry) {
  BcEntry *e = entry;
  if (e) free(e->name);
}

void bc_auto_free(void *auto1) {
  BcAuto *a = (BcAuto*) auto1;
  if (a && a->name) free(a->name);
}

void bc_result_free(void *result) {

  BcResult *r = (BcResult*) result;

  if (!r) return;

  switch (r->type) {

    case BC_RESULT_TEMP:
    case BC_RESULT_SCALE:
    case BC_RESULT_VAR_AUTO:
    {
      bc_num_free(&r->data.num);
      break;
    }

    case BC_RESULT_ARRAY_AUTO:
    {
      bc_vec_free(&r->data.array);
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
