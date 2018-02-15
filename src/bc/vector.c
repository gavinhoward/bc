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
 * A special license exemption is granted to the Toybox project to use this
 * source under the following BSD 0-Clause License:
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
 * Code to manipulate vectors (resizable arrays).
 *
 */

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


  if (vec == NULL || vec->size != sizeof(uint8_t)) {
    return BC_STATUS_INVALID_PARAM;
  }

  if (vec->len == vec->cap) {

    BcStatus status = bc_vec_expand(vec);

    if (status != BC_STATUS_SUCCESS) {
      return status;
    }
  }

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

  return BC_STATUS_SUCCESS;
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

  uint8_t* ptr = realloc(vec->array, vec->size * (vec->cap * 2));

  if (ptr == NULL) {
    return BC_STATUS_MALLOC_FAIL;
  }

  vec->array = ptr;
  vec->cap *= 2;

  return BC_STATUS_SUCCESS;
}

static size_t bc_veco_find(BcVecO* vec, void *data);

BcStatus bc_veco_init(BcVecO* vec, size_t esize,
                      BcFreeFunc dtor, BcVecCmpFunc cmp)
{
  if (!vec || esize == 0 || !cmp) {
    return BC_STATUS_INVALID_PARAM;
  }

  vec->cmp = cmp;

  return bc_vec_init(&vec->vec, esize, dtor);
}

BcStatus bc_veco_insert(BcVecO* vec, void* data, size_t* idx) {

  if (!vec || !data) {
    return BC_STATUS_INVALID_PARAM;
  }

  *idx = bc_veco_find(vec, data);

  if (*idx > vec->vec.len) {
    return BC_STATUS_VECO_OUT_OF_BOUNDS;
  }

  if (*idx != vec->vec.len && !vec->cmp(data, bc_vec_item(&vec->vec, *idx))) {
    return BC_STATUS_VECO_ITEM_EXISTS;
  }

  return bc_vec_pushAt(&vec->vec, data, *idx);
}

size_t bc_veco_index(BcVecO* vec, void* data) {

  size_t idx;

  if (!vec || !data) {
    return BC_STATUS_INVALID_PARAM;
  }

  idx = bc_veco_find(vec, data);

  if (idx >= vec->vec.len || vec->cmp(data, bc_vec_item(&vec->vec, idx))) {
    return BC_INVALID_IDX;
  }

  return idx;
}

void* bc_veco_item(BcVecO* vec, size_t idx) {
  return bc_vec_item(&vec->vec, idx);
}

void bc_veco_free(BcVecO* vec) {
  bc_vec_free(&vec->vec);
}

static size_t bc_veco_find(BcVecO* vec, void* data) {

  BcVecCmpFunc cmp = vec->cmp;

  size_t low = 0;
  size_t high = vec->vec.len;

  while (low < high) {

    size_t mid;
    int result;
    uint8_t* ptr;

    mid = (low + high) / 2;

    ptr = bc_vec_item(&vec->vec, mid);

    result = cmp(ptr, data);

    if (!result) {
      return mid;
    }

    if (result > 0) {
      high = mid;
    }
    else {
      low = mid + 1;
    }
  }

  return low;
}
