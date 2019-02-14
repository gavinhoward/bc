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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <status.h>

#define BC_VEC_INVALID_IDX (SIZE_MAX)
#define BC_VEC_START_CAP (1<<5)

typedef unsigned char uchar;

typedef void (*BcVecFree)(void*);

// Forward declaration.
struct BcId;

typedef struct BcVec {
	char *v;
	size_t len;
	size_t cap;
	size_t size;
	BcVecFree dtor;
} BcVec;

void bc_vec_init(BcVec *restrict v, size_t esize, BcVecFree dtor);
void bc_vec_expand(BcVec *restrict v, size_t req);

void bc_vec_npop(BcVec *restrict v, size_t n);

void bc_vec_push(BcVec *restrict v, const void *data);
void bc_vec_npush(BcVec *restrict v, size_t n, const void *data);
void bc_vec_pushByte(BcVec *restrict v, uchar data);
void bc_vec_pushIndex(BcVec *restrict v, size_t idx);
void bc_vec_string(BcVec *restrict v, size_t len, const char *restrict str);
void bc_vec_concat(BcVec *restrict v, const char *restrict str);
void bc_vec_empty(BcVec *restrict v);

#if BC_ENABLE_HISTORY
void bc_vec_popAt(BcVec *restrict v, size_t idx);
void bc_vec_replaceAt(BcVec *restrict v, size_t idx, const void *data);
#endif // BC_ENABLE_HISTORY

void* bc_vec_item(const BcVec *restrict v, size_t idx);
void* bc_vec_item_rev(const BcVec *restrict v, size_t idx);

void bc_vec_free(void *vec);

bool bc_map_insert(BcVec *restrict v, const struct BcId *restrict ptr,
                   size_t *restrict i);
size_t bc_map_index(const BcVec *restrict v, const struct BcId *restrict ptr);

#define bc_vec_pop(v) (bc_vec_npop((v), 1))
#define bc_vec_top(v) (bc_vec_item_rev((v), 0))

#define bc_map_init(v) (bc_vec_init((v), sizeof(BcId), bc_id_free))

#endif // BC_VECTOR_H
