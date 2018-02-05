#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <bc/bc.h>
#include <bc/vector.h>

static BcStatus bc_vec_expand(BcVec* vec);

BcStatus bc_vec_init(BcVec* vec, size_t esize, BcFreeFunc dtor) {

  if (vec == NULL || esize == 0) {
    return BC_STATUS_INVALID_PARAM;
  }

  vec->size = esize;
  vec->cap = BC_VEC_INITIAL_CAP;
  vec->len = 0;
  vec->dtor = dtor;

  vec->array = malloc(esize * BC_VEC_INITIAL_CAP);

  if (vec->array == NULL) {
    return BC_STATUS_MALLOC_FAIL;
  }

  return BC_STATUS_SUCCESS;
}

BcStatus bc_vec_push(BcVec* vec, void* data) {

  size_t size;

  if (vec == NULL || data == NULL) {
    return BC_STATUS_INVALID_PARAM;
  }

  if (vec->len == vec->cap) {

    BcStatus status = bc_vec_expand(vec);

    if (status != BC_STATUS_SUCCESS) {
      return status;
    }
  }

  size = vec->size;
  memmove(vec->array + (size * vec->len), data, size);

  ++vec->len;

  return BC_STATUS_SUCCESS;
}

BcStatus bc_vec_pushByte(BcVec* vec, uint8_t data) {

  size_t size;

  if (vec == NULL || vec->size != sizeof(uint8_t)) {
    return BC_STATUS_INVALID_PARAM;
  }

  if (vec->len == vec->cap) {

    BcStatus status = bc_vec_expand(vec);

    if (status != BC_STATUS_SUCCESS) {
      return status;
    }
  }

  size = vec->size;
  vec->array[vec->len] = data;

  ++vec->len;

  return BC_STATUS_SUCCESS;
}

BcStatus bc_vec_pushAt(BcVec* vec, void* data, size_t idx) {

  uint8_t* ptr;
  size_t size;

  if (vec == NULL || data == NULL || idx > vec->len) {
    return BC_STATUS_INVALID_PARAM;
  }

  if (idx == vec->len) return bc_vec_push(vec, data);

  if (vec->len == vec->cap) {

    BcStatus status = bc_vec_expand(vec);

    if (status != BC_STATUS_SUCCESS) {
      return status;
    }
  }

  size = vec->size;
  ptr = vec->array + size * idx;

  memmove(ptr + size, ptr, size * (vec->len - idx));
  memmove(ptr, data, size);

  ++vec->len;
}

void* bc_vec_top(BcVec* vec) {

  if (vec == NULL || vec->len == 0) {
    return NULL;
  }

  return vec->array + vec->size * (vec->len - 1);
}

void* bc_vec_item(BcVec* vec, size_t idx) {

  if (vec == NULL || vec->len == 0 || idx >= vec->len) {
    return NULL;
  }

  return vec->array + vec->size * idx;
}

BcStatus bc_vec_pop(BcVec* vec) {

  if (vec == NULL) {
    return BC_STATUS_INVALID_PARAM;
  }

  --vec->len;

  if (vec->dtor) vec->dtor(vec->array + (vec->size * vec->len));

  return BC_STATUS_SUCCESS;
}

void bc_vec_free(void* vec) {

  BcVec* s;
  size_t len;
  size_t esize;
  BcFreeFunc sfree;
  uint8_t* array;

  s = (BcVec*) vec;

  if (s == NULL) {
    return;
  }

  sfree = s->dtor;

  if (sfree) {

    len = s->len;
    array = s->array;
    esize = s->size;

    for (size_t i = 0; i < len; ++i) {
      sfree(array + (i * esize));
    }
  }

  free(s->array);

  s->size = 0;
  s->array = NULL;
  s->len = 0;
  s->cap = 0;
}

static BcStatus bc_vec_expand(BcVec* vec) {

  uint8_t* ptr = realloc(vec->array, vec->size * (vec->cap + BC_VEC_INITIAL_CAP));

  if (ptr == NULL) {
    return BC_STATUS_MALLOC_FAIL;
  }

  vec->array = ptr;
  vec->cap += BC_VEC_INITIAL_CAP;

  return BC_STATUS_SUCCESS;
}
