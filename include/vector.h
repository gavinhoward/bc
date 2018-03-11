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
 * Definitions for bc vectors (resizable arrays).
 *
 */

#ifndef BC_VECTOR_H
#define BC_VECTOR_H

#include <stdlib.h>
#include <stdint.h>

#include <bc.h>

#define BC_VEC_INITIAL_CAP (32)

typedef int (*BcVecCmpFunc)(void*, void*);

typedef struct BcVec {

  uint8_t *array;
  size_t len;
  size_t cap;
  size_t size;

  BcFreeFunc dtor;

} BcVec;

// ** Exclude start. **
BcStatus bc_vec_init(BcVec *vec, size_t esize, BcFreeFunc dtor);

BcStatus bc_vec_expand(BcVec *vec, size_t request);

BcStatus bc_vec_push(BcVec *vec, void *data);

BcStatus bc_vec_pushByte(BcVec *vec, uint8_t data);

BcStatus bc_vec_pushAt(BcVec *vec, void *data, size_t idx);

void* bc_vec_top(const BcVec *vec);

void* bc_vec_item(const BcVec *vec, size_t idx);

void* bc_vec_item_rev(const BcVec *vec, size_t idx);

BcStatus bc_vec_pop(BcVec *vec);

void bc_vec_free(void *vec);
// ** Exclude end. **

typedef struct BcVecO {

  BcVec vec;
  BcVecCmpFunc cmp;

} BcVecO;

BcStatus bc_veco_init(BcVecO* vec, size_t esize,
                      BcFreeFunc dtor, BcVecCmpFunc cmp);

BcStatus bc_veco_insert(BcVecO* vec, void *data, size_t *idx);

size_t bc_veco_index(const BcVecO *vec, void *data);

void* bc_veco_item(const BcVecO* vec, size_t idx);

void bc_veco_free(BcVecO* vec);

#endif // BC_VECTOR_H
