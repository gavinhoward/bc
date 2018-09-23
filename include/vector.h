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

#include <status.h>

#define BC_VEC_INVALID_IDX ((size_t) -1)
#define BC_VEC_INITIAL_CAP (1<<5)

typedef void (*BcVecFree)(void*);
typedef int (*BcVecCmp)(const void*, const void*);

typedef struct BcVec {

  uint8_t *array;
  size_t len;
  size_t cap;
  size_t size;

  BcVecFree dtor;

} BcVec;

// ** Exclude start. **
BcStatus bc_vec_init(BcVec *vec, size_t esize, BcVecFree dtor);
BcStatus bc_vec_expand(BcVec *vec, size_t request);

void bc_vec_pop(BcVec *vec);
void bc_vec_npop(BcVec *vec, size_t n);

BcStatus bc_vec_push(BcVec *vec, size_t n, const void *data);
BcStatus bc_vec_pushByte(BcVec *vec, uint8_t data);
BcStatus bc_vec_pushAt(BcVec *vec, const void *data, size_t idx);
BcStatus bc_vec_setToString(BcVec *vec, size_t len, const char *str);

void* bc_vec_top(const BcVec *vec);
void* bc_vec_item(const BcVec *vec, size_t idx);
void* bc_vec_item_rev(const BcVec *vec, size_t idx);
BcStatus bc_vec_string(const BcVec *vec, char **d);

void bc_vec_free(void *vec);
// ** Exclude end. **

typedef struct BcVecO {

  BcVec vec;
  BcVecCmp cmp;

} BcVecO;

// ** Exclude start. **
BcStatus bc_veco_init(BcVecO* vec, size_t esize, BcVecFree dtor, BcVecCmp cmp);
BcStatus bc_veco_insert(BcVecO* vec, const void *data, size_t *idx);
size_t bc_veco_index(const BcVecO *v, const void *data);
// ** Exclude end. **

#define bc_veco_item(v, idx) (bc_vec_item(&(v)->vec, (idx)))
#define bc_veco_free(v) (bc_vec_free(&(v)->vec))

#endif // BC_VECTOR_H
