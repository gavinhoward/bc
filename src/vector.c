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
#include <stdlib.h>
#include <string.h>

#include <status.h>
#include <vector.h>
#include <lang.h>

static BcStatus bc_vec_grow(BcVec *v, size_t n) {

	char *ptr;
	size_t cap = v->cap * 2;

	while (cap < v->len + n) cap *= 2;
	if (!(ptr = realloc(v->v, v->size * cap))) return BC_STATUS_ALLOC_ERR;

	v->v = ptr;
	v->cap = cap;

	return BC_STATUS_SUCCESS;
}

BcStatus bc_vec_init(BcVec *v, size_t esize, BcVecFree dtor) {

	assert(v && esize);

	v->size = esize;
	v->cap = BC_VEC_START_CAP;
	v->len = 0;
	v->dtor = dtor;

	if (!(v->v = malloc(esize * BC_VEC_START_CAP))) return BC_STATUS_ALLOC_ERR;

	return BC_STATUS_SUCCESS;
}

BcStatus bc_vec_expand(BcVec *v, size_t req) {

	char *ptr;

	assert(v);

	if (v->cap >= req) return BC_STATUS_SUCCESS;
	if (!(ptr = realloc(v->v, v->size * req))) return BC_STATUS_ALLOC_ERR;

	v->v = ptr;
	v->cap = req;

	return BC_STATUS_SUCCESS;
}

void bc_vec_npop(BcVec *v, size_t n) {
	assert(v && n <= v->len);
	if (!v->dtor) v->len -= n;
	else {
		size_t len = v->len - n;
		while (v->len > len) v->dtor(v->v + (v->size * --v->len));
	}
}

BcStatus bc_vec_push(BcVec *v, const void *data) {

	BcStatus s;

	assert(v && data);

	if (v->len + 1 > v->cap && (s = bc_vec_grow(v, 1))) return s;

	memmove(v->v + (v->size * v->len), data, v->size);
	v->len += 1;

	return BC_STATUS_SUCCESS;
}

BcStatus bc_vec_pushByte(BcVec *v, uint8_t data) {
	assert(v && v->size == sizeof(uint8_t));
	return bc_vec_push(v, &data);
}

static BcStatus bc_vec_pushAt(BcVec *v, const void *data, size_t idx) {

	BcStatus s;
	char *ptr;

	assert(v && data && idx <= v->len);

	if (idx == v->len) return bc_vec_push(v, data);
	if (v->len == v->cap && (s = bc_vec_grow(v, 1))) return s;

	ptr = v->v + v->size * idx;

	memmove(ptr + v->size, ptr, v->size * (v->len++ - idx));
	memmove(ptr, data, v->size);

	return BC_STATUS_SUCCESS;
}

BcStatus bc_vec_string(BcVec *v, size_t len, const char *str) {

	BcStatus s;

	assert(v && v->size == sizeof(char));
	assert(!v->len || !v->v[v->len - 1]);

	bc_vec_npop(v, v->len);
	if ((s = bc_vec_expand(v, len + 1))) return s;
	memcpy(v->v, str, len);

	v->len = len;

	return bc_vec_pushByte(v, '\0');
}

BcStatus bc_vec_concat(BcVec *v, const char *str) {

	BcStatus s;
	size_t len;

	assert(v && v->size == sizeof(char));
	assert(!v->len || !v->v[v->len - 1]);

	if (!v->len && (s = bc_vec_pushByte(v, '\0'))) return s;

	len = v->len + strlen(str);

	if (v->cap < len && (s = bc_vec_grow(v, len - v->len))) return s;
	strcat(v->v, str);

	v->len = len;

	return BC_STATUS_SUCCESS;
}

void* bc_vec_item(const BcVec *v, size_t idx) {
	assert(v && v->len && idx < v->len);
	return v->v + v->size * idx;
}

void* bc_vec_item_rev(const BcVec *v, size_t idx) {
	assert(v && v->len && idx < v->len);
	return v->v + v->size * (v->len - idx - 1);
}

void bc_vec_free(void *vec) {
	BcVec *v = (BcVec*) vec;
	bc_vec_npop(v, v->len);
	free(v->v);
}

static size_t bc_map_find(const BcVec *v, const void *ptr) {

	size_t low = 0, high = v->len;

	while (low < high) {

		size_t mid = (low + high) / 2;
		uint8_t *item = bc_vec_item(v, mid);
		int result = bc_id_cmp(ptr, item);

		if (!result) return mid;
		else if (result < 0) high = mid;
		else low = mid + 1;
	}

	return low;
}

BcStatus bc_map_insert(BcVec *v, const void *ptr, size_t *i) {

	BcStatus s;

	assert(v && ptr && i);

	if ((*i = bc_map_find(v, ptr)) > v->len) s = BC_STATUS_VEC_OUT_OF_BOUNDS;
	else if (*i == v->len) s = bc_vec_push(v, ptr);
	else if (!bc_id_cmp(ptr, bc_vec_item(v, *i))) s = BC_STATUS_VEC_ITEM_EXISTS;
	else s = bc_vec_pushAt(v, ptr, *i);

	return s;
}

size_t bc_map_index(const BcVec* v, const void *ptr) {
	assert(v && ptr);
	size_t i = bc_map_find(v, ptr);
	if (i >= v->len) return BC_VEC_INVALID_IDX;
	return bc_id_cmp(ptr, bc_vec_item(v, i)) ? BC_VEC_INVALID_IDX : i;
}
