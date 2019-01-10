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
#include <vm.h>

static void bc_vec_grow(BcVec *restrict v, size_t n) {
	size_t cap = v->cap * 2;
	while (cap < v->len + n) cap *= 2;
	v->v = bc_vm_realloc(v->v, v->size * cap);
	v->cap = cap;
}

void bc_vec_init(BcVec *restrict v, size_t esize, BcVecFree dtor) {
	assert(v && esize);
	v->size = esize;
	v->cap = BC_VEC_START_CAP;
	v->len = 0;
	v->dtor = dtor;
	v->v = bc_vm_malloc(esize * BC_VEC_START_CAP);
}

void bc_vec_expand(BcVec *restrict v, size_t req) {
	assert(v);
	if (v->cap < req) {
		v->v = bc_vm_realloc(v->v, v->size * req);
		v->cap = req;
	}
}

void bc_vec_npop(BcVec *restrict v, size_t n) {
	assert(v && n <= v->len);
	if (!v->dtor) v->len -= n;
	else {
		size_t len = v->len - n;
		while (v->len > len) v->dtor(v->v + (v->size * --v->len));
	}
}

void bc_vec_npush(BcVec *restrict v, size_t n, const void *data) {
	assert(v && data);
	if (v->len + n > v->cap) bc_vec_grow(v, n);
	memcpy(v->v + (v->size * v->len), data, v->size * n);
	v->len += n;
}

void bc_vec_push(BcVec *restrict v, const void *data) {
	bc_vec_npush(v, 1, data);
}

void bc_vec_pushByte(BcVec *restrict v, uchar data) {
	assert(v && v->size == sizeof(uchar));
	bc_vec_push(v, &data);
}

void bc_vec_pushIndex(BcVec *restrict v, size_t idx) {

	uchar amt, nums[sizeof(size_t)];

	assert(v->size == sizeof(uchar));

	for (amt = 0; idx; ++amt) {
		nums[amt] = (uchar) idx;
		idx &= ((size_t) ~(UCHAR_MAX));
		idx >>= sizeof(uchar) * CHAR_BIT;
	}

	bc_vec_push(v, &amt);
	bc_vec_npush(v, amt, nums);
}

static void bc_vec_pushAt(BcVec *restrict v, const void *data, size_t idx) {

	assert(v && data && idx <= v->len);

	if (idx == v->len) bc_vec_push(v, data);
	else {

		char *ptr;

		if (v->len == v->cap) bc_vec_grow(v, 1);

		ptr = v->v + v->size * idx;

		memmove(ptr + v->size, ptr, v->size * (v->len++ - idx));
		memmove(ptr, data, v->size);
	}
}

void bc_vec_string(BcVec *restrict v, size_t len, const char *restrict str) {

	assert(v && v->size == sizeof(char));
	assert(!v->len || !v->v[v->len - 1]);

	bc_vec_npop(v, v->len);
	bc_vec_expand(v, len + 1);
	memcpy(v->v, str, len);
	v->len = len;

	bc_vec_pushByte(v, '\0');
}

void bc_vec_concat(BcVec *restrict v, const char *restrict str) {

	size_t len;

	assert(v && v->size == sizeof(char));
	assert(!v->len || !v->v[v->len - 1]);

	if (!v->len) bc_vec_pushByte(v, '\0');

	len = v->len + strlen(str);

	if (v->cap < len) bc_vec_grow(v, len - v->len);
	strcat(v->v, str);

	v->len = len;
}

void bc_vec_empty(BcVec *restrict v) {
	assert(v && v->size == sizeof(char));
	bc_vec_npop(v, v->len);
	bc_vec_pushByte(v, '\0');
}

#if BC_ENABLE_HISTORY
void bc_vec_popAt(BcVec *restrict v, size_t idx) {

	char* ptr, *data;

	ptr = bc_vec_item(v, idx);
	data = bc_vec_item(v, idx + 1);

	if (v->dtor) v->dtor(ptr);

	memmove(ptr, data, --v->len * v->size);
}

void bc_vec_replaceAt(BcVec *restrict v, size_t idx, const void *data) {
	char *ptr = bc_vec_item(v, idx);
	if (v->dtor) v->dtor(ptr);
	memcpy(ptr, data, v->size);
}
#endif // BC_ENABLE_HISTORY

void* bc_vec_item(const BcVec *restrict v, size_t idx) {
	assert(v && v->len && idx < v->len);
	return v->v + v->size * idx;
}

void* bc_vec_item_rev(const BcVec *restrict v, size_t idx) {
	assert(v && v->len && idx < v->len);
	return v->v + v->size * (v->len - idx - 1);
}

void bc_vec_free(void *vec) {
	BcVec *v = (BcVec*) vec;
	bc_vec_npop(v, v->len);
	free(v->v);
}

static size_t bc_map_find(const BcVec *restrict v, const BcId *restrict ptr) {

	size_t low = 0, high = v->len;

	while (low < high) {

		size_t mid = (low + high) / 2;
		BcId *id = bc_vec_item(v, mid);
		int result = bc_id_cmp(ptr, id);

		if (!result) return mid;
		else if (result < 0) high = mid;
		else low = mid + 1;
	}

	return low;
}

bool bc_map_insert(BcVec *restrict v, const BcId *restrict ptr, size_t *restrict i) {

	assert(v && ptr && i);

	*i = bc_map_find(v, ptr);

	assert(*i <= v->len);

	if (*i == v->len) bc_vec_push(v, ptr);
	else if (!bc_id_cmp(ptr, bc_vec_item(v, *i))) return false;
	else bc_vec_pushAt(v, ptr, *i);

	return true;
}

size_t bc_map_index(const BcVec *restrict v, const BcId *restrict ptr) {
	assert(v && ptr);
	size_t i = bc_map_find(v, ptr);
	if (i >= v->len) return BC_VEC_INVALID_IDX;
	return bc_id_cmp(ptr, bc_vec_item(v, i)) ? BC_VEC_INVALID_IDX : i;
}
