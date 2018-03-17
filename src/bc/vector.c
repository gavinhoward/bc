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
 * Code to manipulate vectors (resizable arrays).
 *
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <bc.h>
#include <vector.h>

static BcStatus bc_vec_double(BcVec *vec) {

  uint8_t *ptr;

  ptr = realloc(vec->array, vec->size * (vec->cap * 2));

  if (!ptr) return BC_STATUS_MALLOC_FAIL;

  vec->array = ptr;
  vec->cap *= 2;

  return BC_STATUS_SUCCESS;
}

BcStatus bc_vec_init(BcVec *vec, size_t esize, BcFreeFunc dtor) {

  if (!vec || !esize) return BC_STATUS_INVALID_ARG;

  vec->size = esize;
  vec->cap = BC_VEC_INITIAL_CAP;
  vec->len = 0;
  vec->dtor = dtor;

  vec->array = malloc(esize * BC_VEC_INITIAL_CAP);

  if (!vec->array) return BC_STATUS_MALLOC_FAIL;

  return BC_STATUS_SUCCESS;
}

BcStatus bc_vec_expand(BcVec *vec, size_t request) {

  uint8_t *ptr;

  if (!vec) return BC_STATUS_INVALID_ARG;

  if (vec->cap >= request) return BC_STATUS_SUCCESS;

  ptr = realloc(vec->array, vec->size * request);

  if (!ptr) return BC_STATUS_MALLOC_FAIL;

  vec->array = ptr;
  vec->cap = request;

  return BC_STATUS_SUCCESS;
}

BcStatus bc_vec_push(BcVec *vec, void *data) {

  BcStatus status;
  size_t size;

  if (!vec || !data) return BC_STATUS_INVALID_ARG;

  if (vec->len == vec->cap) {
    status = bc_vec_double(vec);
    if (status) return status;
  }

  size = vec->size;
  memmove(vec->array + (size * vec->len), data, size);

  ++vec->len;

  return BC_STATUS_SUCCESS;
}

BcStatus bc_vec_pushByte(BcVec *vec, uint8_t data) {

  BcStatus status;

  if (!vec || vec->size != sizeof(uint8_t)) return BC_STATUS_INVALID_ARG;

  if (vec->len == vec->cap) {
    status = bc_vec_double(vec);
    if (status) return status;
  }

  vec->array[vec->len] = data;

  ++vec->len;

  return BC_STATUS_SUCCESS;
}

BcStatus bc_vec_pushAt(BcVec *vec, void *data, size_t idx) {

  BcStatus status;
  uint8_t *ptr;
  size_t size;

  if (!vec || !data || idx > vec->len) return BC_STATUS_INVALID_ARG;

  if (idx == vec->len) return bc_vec_push(vec, data);

  if (vec->len == vec->cap) {
    status = bc_vec_double(vec);
    if (status) return status;
  }

  size = vec->size;
  ptr = vec->array + size * idx;

  memmove(ptr + size, ptr, size * (vec->len - idx));
  memmove(ptr, data, size);

  ++vec->len;

  return BC_STATUS_SUCCESS;
}

void* bc_vec_top(const BcVec *vec) {
  if (!vec || !vec->len) return NULL;
  return vec->array + vec->size * (vec->len - 1);
}

void* bc_vec_item(const BcVec *vec, size_t idx) {
  if (!vec || !vec->len || idx >= vec->len) return NULL;
  return vec->array + vec->size * idx;
}

void* bc_vec_item_rev(const BcVec *vec, size_t idx) {
  if (!vec || !vec->len || idx >= vec->len) return NULL;
  return vec->array + vec->size * (vec->len - idx - 1);
}

BcStatus bc_vec_pop(BcVec *vec) {

  if (!vec) return BC_STATUS_INVALID_ARG;
  if (!vec->len) return BC_STATUS_VEC_OUT_OF_BOUNDS;

  --vec->len;

  if (vec->dtor) vec->dtor(vec->array + (vec->size * vec->len));

  return BC_STATUS_SUCCESS;
}

void bc_vec_free(void *vec) {

  BcVec *s;
  size_t esize, len, i;
  BcFreeFunc sfree;
  uint8_t *array;

  s = (BcVec*) vec;

  if (!s) return;

  sfree = s->dtor;

  if (sfree) {

    len = s->len;
    array = s->array;
    esize = s->size;

    for (i = 0; i < len; ++i) sfree(array + (i * esize));
  }

  free(s->array);

  s->size = 0;
  s->array = NULL;
  s->len = 0;
  s->cap = 0;
}

static size_t bc_veco_find(const BcVecO* vec, void *data) {

  BcVecCmpFunc cmp;
  size_t low, high;

  cmp = vec->cmp;

  low = 0;
  high = vec->vec.len;

  while (low < high) {

    size_t mid;
    int result;
    uint8_t *ptr;

    mid = (low + high) / 2;

    ptr = bc_vec_item(&vec->vec, mid);

    result = cmp(data, ptr);

    if (!result) return mid;

    if (result < 0) high = mid;
    else low = mid + 1;
  }

  return low;
}

BcStatus bc_veco_init(BcVecO* vec, size_t esize,
                      BcFreeFunc dtor, BcVecCmpFunc cmp)
{
  if (!vec || !esize || !cmp) return BC_STATUS_INVALID_ARG;
  vec->cmp = cmp;
  return bc_vec_init(&vec->vec, esize, dtor);
}

BcStatus bc_veco_insert(BcVecO* vec, void *data, size_t *idx) {

  BcStatus status;

  if (!vec || !data) return BC_STATUS_INVALID_ARG;

  *idx = bc_veco_find(vec, data);

  if (*idx > vec->vec.len) return BC_STATUS_VEC_OUT_OF_BOUNDS;

  if (*idx != vec->vec.len && !vec->cmp(data, bc_vec_item(&vec->vec, *idx)))
    return BC_STATUS_VEC_ITEM_EXISTS;

  if (*idx >= vec->vec.len) {
    *idx = vec->vec.len;
    status = bc_vec_push(&vec->vec, data);
  }
  else status = bc_vec_pushAt(&vec->vec, data, *idx);

  return status;
}

size_t bc_veco_index(const BcVecO* vec, void *data) {

  size_t idx;

  if (!vec || !data) return BC_STATUS_INVALID_ARG;

  idx = bc_veco_find(vec, data);

  if (idx >= vec->vec.len || vec->cmp(data, bc_vec_item(&vec->vec, idx)))
    return BC_INVALID_IDX;

  return idx;
}

void* bc_veco_item(const BcVecO* vec, size_t idx) {
  return bc_vec_item(&vec->vec, idx);
}

void bc_veco_free(BcVecO* vec) {
  bc_vec_free(&vec->vec);
}
