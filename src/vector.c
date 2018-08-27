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

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <status.h>
#include <vector.h>
#include <bc.h>

BcStatus bc_vec_grow(BcVec *vec) {

  size_t cap = vec->cap * 2;
  uint8_t *ptr = realloc(vec->array, vec->size * cap);
  if (!ptr) return BC_STATUS_MALLOC_FAIL;

  vec->array = ptr;
  vec->cap = cap;

  return BC_STATUS_SUCCESS;
}

BcStatus bc_vec_init(BcVec *vec, size_t esize, BcVecFree dtor) {

  assert(vec && esize);

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

  assert(vec);

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

  assert(vec && data);
  if (vec->len == vec->cap && (status = bc_vec_grow(vec))) return status;

  size = vec->size;
  memmove(vec->array + (size * vec->len++), data, size);

  return BC_STATUS_SUCCESS;
}

BcStatus bc_vec_pushByte(BcVec *vec, uint8_t data) {

  BcStatus status;

  assert(vec && vec->size == sizeof(uint8_t));

  if (vec->len == vec->cap && (status = bc_vec_grow(vec))) return status;
  vec->array[vec->len++] = data;

  return BC_STATUS_SUCCESS;
}

BcStatus bc_vec_pushAt(BcVec *vec, void *data, size_t idx) {

  BcStatus status;
  uint8_t *ptr;

  assert(vec && data && idx <= vec->len);

  if (idx == vec->len) return bc_vec_push(vec, data);
  if (vec->len == vec->cap && (status = bc_vec_grow(vec))) return status;

  ptr = vec->array + vec->size * idx;

  memmove(ptr + vec->size, ptr, vec->size * (vec->len++ - idx));
  memmove(ptr, data, vec->size);

  return BC_STATUS_SUCCESS;
}

void* bc_vec_top(const BcVec *vec) {
  assert(vec && vec->len);
  return vec->array + vec->size * (vec->len - 1);
}

void* bc_vec_item(const BcVec *vec, size_t idx) {
  assert(vec && vec->len && idx < vec->len);
  return vec->array + vec->size * idx;
}

void* bc_vec_item_rev(const BcVec *vec, size_t idx) {
  assert(vec && vec->len && idx < vec->len);
  return vec->array + vec->size * (vec->len - idx - 1);
}

void bc_vec_pop(BcVec *vec) {
  assert(vec && vec->len);
  vec->len -= 1;
  if (vec->dtor) vec->dtor(vec->array + (vec->size * vec->len));
}

void bc_vec_npop(BcVec *vec, size_t n) {
  assert(vec && n <= vec->len);
  if (!vec->dtor) vec->len -= n;
  else {
    size_t len = vec->len - n;
    while (vec->len > len) bc_vec_pop(vec);
  }
}

void bc_vec_free(void *vec) {

  BcVec *s = (BcVec*) vec;

  if (!s) return;

  if (s->dtor) {
    size_t i;
    for (i = 0; i < s->len; ++i) s->dtor(s->array + (i * s->size));
  }

  free(s->array);
  memset(s, 0, sizeof(BcVec));
}

size_t bc_veco_find(const BcVecO* vec, void *data) {

  size_t low = 0, high = vec->vec.len;

  while (low < high) {

    size_t mid = (low + high) / 2;
    uint8_t *ptr = bc_vec_item(&vec->vec, mid);
    int result = vec->cmp(data, ptr);

    if (!result) return mid;

    if (result < 0) high = mid;
    else low = mid + 1;
  }

  return low;
}

BcStatus bc_veco_init(BcVecO *vec, size_t esize, BcVecFree dtor, BcVecCmp cmp) {
  assert(vec && esize && cmp);
  vec->cmp = cmp;
  return bc_vec_init(&vec->vec, esize, dtor);
}

BcStatus bc_veco_insert(BcVecO *vec, void *data, size_t *idx) {

  BcStatus status;

  assert(vec && data && idx);

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

size_t bc_veco_index(const BcVecO* v, void *data) {
  assert(v && data);
  size_t i = bc_veco_find(v, data);
  if (i >= v->vec.len || v->cmp(data, bc_vec_item(&v->vec, i))) return BC_INVALID_IDX;
  return i;
}
