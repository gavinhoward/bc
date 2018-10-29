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

#include <stddef.h>
#include <stdint.h>

#include <status.h>

#define BC_VEC_INVALID_IDX ((size_t) -1)
#define BC_VEC_START_CAP (1<<5)

typedef void (*BcVecFree)(void*);
typedef int (*BcVecCmp)(const void*, const void*);

typedef struct BcVec {

	char *v;
	size_t len;
	size_t cap;
	size_t size;

	BcVecFree dtor;

} BcVec;

// ** Exclude start. **

BcStatus bc_vec_init(BcVec *v, size_t esize, BcVecFree dtor);
BcStatus bc_vec_expand(BcVec *v, size_t req);

void bc_vec_npop(BcVec *v, size_t n);

BcStatus bc_vec_push(BcVec *v, const void *data);
BcStatus bc_vec_pushByte(BcVec *v, uint8_t data);
BcStatus bc_vec_string(BcVec *v, size_t len, const char *str);
BcStatus bc_vec_concat(BcVec *v, const char *str);

void* bc_vec_item(const BcVec *v, size_t idx);
void* bc_vec_item_rev(const BcVec *v, size_t idx);

void bc_vec_free(void *vec);
// ** Exclude end. **

#define bc_vec_pop(v) (bc_vec_npop((v), 1))
#define bc_vec_top(v) (bc_vec_item_rev((v), 0))

// ** Exclude start. **
BcStatus bc_map_insert(BcVec* v, const void *data, size_t *i);
size_t bc_map_index(const BcVec *v, const void *ptr);
// ** Exclude end. **

#define bc_map_init(v) (bc_vec_init((v), sizeof(BcId), bc_id_free))

#endif // BC_VECTOR_H
