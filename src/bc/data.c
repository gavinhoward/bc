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
 * Code to manipulate data structures in programs.
 *
 */

#include <stdlib.h>

#include <bc/data.h>

BcStatus bc_func_init(BcFunc* func) {

  BcStatus status;

  status = BC_STATUS_SUCCESS;

  if (!func) {
    return BC_STATUS_INVALID_PARAM;
  }

  func->num_params = 0;
  func->num_autos = 0;

  status = bc_vec_init(&func->code, sizeof(uint8_t), NULL);

  if (status) {
    return status;
  }

  func->param_cap = BC_PROGRAM_DEF_SIZE;
  func->params = malloc(sizeof(BcAuto) * BC_PROGRAM_DEF_SIZE);

  if (!func->params) {
    goto param_err;
  }

  func->auto_cap = BC_PROGRAM_DEF_SIZE;
  func->autos = malloc(sizeof(BcAuto) * BC_PROGRAM_DEF_SIZE);

  if (!func->autos) {
    func->auto_cap = 0;
    goto auto_err;
  }

  status = bc_vec_init(&func->labels, sizeof(size_t), NULL);

  if (status) {
    goto label_err;
  }

  return BC_STATUS_SUCCESS;

label_err:

  free(func->autos);
  func->auto_cap = 0;

auto_err:

  free(func->params);
  func->param_cap = 0;

param_err:

  bc_vec_free(&func->code);

  return status;
}

BcStatus bc_func_insertParam(BcFunc* func, char* name, bool var) {

  BcAuto* params;
  size_t new_cap;

  if (!func || !name) {
    return BC_STATUS_INVALID_PARAM;
  }

  if (func->num_params == func->param_cap) {

    new_cap = func->param_cap * 2;
    params = realloc(func->params, new_cap * sizeof(BcAuto));

    if (!params) {
      return BC_STATUS_MALLOC_FAIL;
    }

    func->param_cap = new_cap;
  }

  func->params[func->num_params].name = name;
  func->params[func->num_params].var = var;

  ++func->num_params;

  return BC_STATUS_SUCCESS;
}

BcStatus bc_func_insertAuto(BcFunc* func, char* name, bool var) {

  BcAuto* autos;
  size_t new_cap;

  if (!func || !name) {
    return BC_STATUS_INVALID_PARAM;
  }

  if (func->num_autos == func->auto_cap) {

    new_cap = func->auto_cap * 2;
    autos = realloc(func->autos, new_cap * sizeof(BcAuto));

    if (!autos) {
      return BC_STATUS_MALLOC_FAIL;
    }

    func->auto_cap = new_cap;
  }

  func->autos[func->num_autos].name = name;
  func->autos[func->num_autos].var = var;

  ++func->num_autos;

  return BC_STATUS_SUCCESS;
}

void bc_func_free(void* func) {

  BcFunc* f;
  uint32_t num;
  BcAuto* vars;

  f = (BcFunc*) func;

  if (f == NULL) {
    return;
  }

  bc_vec_free(&f->code);

  num = f->num_params;
  vars = f->params;

  for (uint32_t i = 0; i < num; ++i) {
    free(vars[i].name);
  }

  num = f->num_autos;
  vars = f->autos;

  for (uint32_t i = 0; i < num; ++i) {
    free(vars[i].name);
  }

  free(f->params);
  free(f->autos);

  bc_vec_free(&f->labels);

  f->params = NULL;
  f->num_params = 0;
  f->param_cap = 0;
  f->autos = NULL;
  f->num_autos = 0;
  f->auto_cap = 0;
}

BcStatus bc_var_init(BcVar* var) {

  if (!var) {
    return BC_STATUS_INVALID_PARAM;
  }

  *var = arb_alloc(16);

  return BC_STATUS_SUCCESS;
}

void bc_var_free(void* var) {

  BcVar* v;

  v = (BcVar*) var;

  if (v == NULL) {
    return;
  }

  arb_free(*v);
}

BcStatus bc_array_init(BcArray* array) {

  if (!array) {
    return BC_STATUS_INVALID_PARAM;
  }

  return bc_vec_init(array, sizeof(fxdpnt),
                          (BcFreeFunc) arb_free);
}

void bc_array_free(void* array) {

  BcArray* a;

  a = (BcArray*) array;

  if (a == NULL) {
    return;
  }

  bc_vec_free(a);
}

BcStatus bc_local_initVar(BcLocal* local, const char* name, const char* num) {

  local->name = name;
  local->var = true;

  // TODO: Don't malloc.
  arb_str2fxdpnt(num);

  return BC_STATUS_SUCCESS;
}

BcStatus bc_local_initArray(BcLocal* local, const char* name, uint32_t nelems) {

  fxdpnt* array;

  assert(nelems);

  local->name = name;
  local->var = false;

  array = malloc(nelems * sizeof(fxdpnt));

  if (!array) {
    return BC_STATUS_MALLOC_FAIL;
  }

  for (uint32_t i = 0; i < nelems; ++i) {
    arb_construct(array + i, BC_PROGRAM_DEF_SIZE);
  }

  return BC_STATUS_SUCCESS;
}

void bc_local_free(void* local) {

  BcLocal* l;
  fxdpnt* array;
  uint32_t nelems;

  l = (BcLocal*) local;

  free((void*) l->name);

  if (l->var) {
    arb_destruct(&l->num);
  }
  else {

    nelems = l->num_elems;
    array = l->array;

    for (uint32_t i = 0; i < nelems; ++i) {
      arb_destruct(array + i);
    }

    free(array);

    l->array = NULL;
    l->num_elems = 0;
  }
}

BcStatus bc_temp_initNum(BcTemp* temp, const char* val) {

  temp->type = BC_TEMP_NUM;

  if (val) {
    temp->num = arb_str2fxdpnt(val);
  }
  else {
    temp->num = arb_alloc(BC_PROGRAM_DEF_SIZE);
  }

  return BC_STATUS_SUCCESS;
}

BcStatus bc_temp_initName(BcTemp* temp, const char* name) {

  temp->type = BC_TEMP_NAME;

  if (!name) {
    return BC_STATUS_VM_INVALID_NAME;
  }

  temp->name = name;

  return BC_STATUS_SUCCESS;
}

BcStatus bc_temp_init(BcTemp* temp, BcTempType type) {

  if (type > BC_TEMP_LAST || type == BC_TEMP_NUM || type == BC_TEMP_NAME) {
    return BC_STATUS_VM_INVALID_TEMP;
  }

  temp->type = type;

  return BC_STATUS_SUCCESS;
}

void bc_temp_free(void* temp) {

  BcTemp* t;

  t = (BcTemp*) temp;

  if (t->type == BC_TEMP_NUM) {
    arb_free(t->num);
  }
}

void bc_string_free(void* string) {

  char* s;

  s = *((char**) string);

  free(s);
}

int bc_entry_cmp(void* entry1, void*entry2) {

  BcEntry* e1;
  BcEntry* e2;

  e1 = (BcEntry*) entry1;
  e2 = (BcEntry*) entry2;

  return strcmp(e1->name, e2->name);
}

void bc_entry_free(void* entry) {

  BcEntry* e;

  e = (BcEntry*) entry;

  free(e->name);
}

void bc_arb_free(void* num) {

  fxdpnt* n;

  n = *((fxdpnt**) num);

  arb_free(n);
}

void bc_num_free(void* num) {

  BcNum* n;

  n = (BcNum*) num;

  switch (n->type) {

    case BC_NUM_RESULT:
    case BC_NUM_SCALE:
    case BC_NUM_LAST:
    case BC_NUM_IBASE:
    case BC_NUM_OBASE:
    {
      arb_free(n->num);
      break;
    }

    default:
    {
      // We don't have to anything.
      break;
    }
  }
}
