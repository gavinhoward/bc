#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <bc/segarray.h>
#include <bc/program.h>

static BcStatus bc_segarray_addUnsorted(BcSegArray* sa, void* data) ;
static BcStatus bc_segarray_addSorted(BcSegArray* sa, void* data);
static BcStatus bc_segarray_addArray(BcSegArray* sa, uint32_t idx);
static uint32_t bc_segarray_findIndex(BcSegArray* sa, void* data);
static inline void bc_segarray_moveLast(BcSegArray* sa, uint32_t end1);
static void bc_segarray_move(BcSegArray* sa, uint32_t idx1,
                             uint32_t start, uint32_t num_elems);

BcStatus bc_segarray_init(BcSegArray* sa, size_t esize,
                          BcFreeFunc sfree, BcCmpFunc cmp)
{
	if (sa == NULL || esize == 0) {
		return BC_STATUS_INVALID_PARAM;
	}

	sa->esize = esize;
	sa->cmp = cmp;
	sa->sfree = sfree;
	sa->num = 0;

	sa->ptrs = malloc(sizeof(uint8_t*) * BC_SEGARRAY_NUM_ARRAYS);
	if (!sa->ptrs) {
		return BC_STATUS_MALLOC_FAIL;
	}

	sa->num_ptrs = 0;
	sa->ptr_cap = BC_SEGARRAY_NUM_ARRAYS;

	return bc_segarray_addArray(sa, 0);
}

BcStatus bc_segarray_add(BcSegArray* sa, void* data) {

	BcStatus status;

	if (sa == NULL || data == NULL) {
		return BC_STATUS_INVALID_PARAM;
	}

	if (sa->cmp) {
		status = bc_segarray_addSorted(sa, data);
	}
	else {
		status = bc_segarray_addUnsorted(sa, data);
	}

	return status;
}

void* bc_segarray_item(BcSegArray* sa, uint32_t idx) {
	return bc_segarray_item2(sa, BC_SEGARRAY_IDX1(idx), BC_SEGARRAY_IDX2(idx));
}

void* bc_segarray_item2(BcSegArray* sa, uint32_t idx1, uint32_t idx2) {

	uint32_t num = sa->num;
	uint32_t num1 = BC_SEGARRAY_IDX1(num);
	uint32_t num2 = BC_SEGARRAY_IDX2(num);
	if (sa == NULL || idx1 > num1 || (idx1 == num1 && idx2 >= num2)) {
		return NULL;
	}

	uint8_t* ptr = sa->ptrs[idx1];

	if (ptr == NULL) {
		return NULL;
	}

	return ptr + sa->esize * idx2;
}

uint32_t bc_segarray_find(BcSegArray* sa, void* data) {

	if (!sa) {
		return (uint32_t) -1;
	}

	uint32_t idx = bc_segarray_findIndex(sa, data);

	if (!sa->cmp(bc_segarray_item(sa, idx), data)) {
		return idx;
	}

	return (uint32_t) -1;
}

void bc_segarray_free(BcSegArray* sa) {

	BcFreeFunc sfree;
	uint32_t num;

	if (sa == NULL) {
		return;
	}

	num = sa->num;

	if (num) {

		sfree = sa->sfree;

		if (sfree) {

			for (uint32_t i = 0; i < num; ++i) {
				sfree(bc_segarray_item(sa, i));
			}
		}

		num = BC_SEGARRAY_IDX1(sa->num - 1);

		for (uint32_t i = 0; i < num; ++i) {
			free(sa->ptrs[i]);
		}

		free(sa->ptrs[num]);
	}
	else {
		free(sa->ptrs[0]);
	}

	free(sa->ptrs);
	sa->ptrs = NULL;

	sa->esize = 0;
	sa->num = 0;
	sa->num_ptrs = 0;
	sa->cmp = NULL;
	sa->sfree = NULL;
}

static BcStatus bc_segarray_addUnsorted(BcSegArray* sa, void* data) {

	BcStatus status;

	uint32_t end = sa->num;
	uint32_t end1 = BC_SEGARRAY_IDX1(end);
	uint32_t end2 = BC_SEGARRAY_IDX2(end);

	if (end2 == 0 && end1 > 0) {

		status = bc_segarray_addArray(sa, end1);

		if (status != BC_STATUS_SUCCESS) {
			return status;
		}
	}

	uint8_t* ptr = sa->ptrs[end1];
	memcpy(ptr + sa->esize * end2, data, sa->esize);

	++sa->num;

	return BC_STATUS_SUCCESS;
}

static BcStatus bc_segarray_addSorted(BcSegArray* sa, void* data) {

	BcStatus status;

	uint32_t end = sa->num;
	uint32_t end1 = BC_SEGARRAY_IDX1(end);
	uint32_t end2 = BC_SEGARRAY_IDX2(end);

	uint32_t idx = bc_segarray_findIndex(sa, data);
	idx = idx == (uint32_t) -1 ? end : idx;
	uint32_t idx1 = BC_SEGARRAY_IDX1(idx);
	uint32_t idx2 = BC_SEGARRAY_IDX2(idx);

	if (end2 == 0 && end1 > 0) {

		status = bc_segarray_addArray(sa, end1);

		if (status != BC_STATUS_SUCCESS) {
			return status;
		}

		if ((end1 != 0 && end2 == 0) && idx != end) {
			bc_segarray_moveLast(sa, end1);
		}
	}

	if (end1 > 0) {

		uint32_t move_idx = end2 == 0 ? end1 - 1 : end1;
		while (move_idx > idx1) {
			bc_segarray_move(sa, move_idx, 0, BC_SEGARRAY_SEG_LAST);
			bc_segarray_moveLast(sa, move_idx);
			--move_idx;
		}
	}

	if (idx2 != BC_SEGARRAY_SEG_LAST) {
		bc_segarray_move(sa, idx1, idx2, BC_SEGARRAY_SEG_LAST - idx2);
	}

	uint8_t* ptr = sa->ptrs[idx1];
	memcpy(ptr + sa->esize * idx2, data, sa->esize);

	++sa->num;

	return BC_STATUS_SUCCESS;
}

static BcStatus bc_segarray_addArray(BcSegArray* sa, uint32_t idx) {

	if (sa->num_ptrs == sa->ptr_cap) {

		uint32_t new_cap = sa->ptr_cap * 2;
		uint8_t** temp = realloc(sa->ptrs, sizeof(uint8_t*) * new_cap);

		if (!temp) {
			return BC_STATUS_MALLOC_FAIL;
		}

		sa->ptrs = temp;
		sa->ptr_cap = new_cap;
	}

	uint8_t* ptr = malloc(sa->esize * BC_SEGARRAY_SEG_SIZE);

	if (ptr == NULL) {
		return BC_STATUS_MALLOC_FAIL;
	}

	sa->ptrs[idx] = ptr;
	++sa->num_ptrs;

	return BC_STATUS_SUCCESS;
}

static uint32_t bc_segarray_findIndex(BcSegArray* sa, void* data) {

	BcCmpFunc cmp = sa->cmp;

	uint32_t low = 0;
	uint32_t high = sa->num;

	while (low < high) {

		uint32_t mid = (low + high) / 2;

		uint8_t* ptr = bc_segarray_item(sa, mid);

		if (cmp(ptr, data) > 0) {
			high = mid;
		}
		else {
			low = mid + 1;
		}
	}

	return low;
}

static void bc_segarray_moveLast(BcSegArray* sa, uint32_t end1) {
	uint8_t* dest = sa->ptrs[end1];
	uint8_t* src = sa->ptrs[end1 - 1] + sa->esize * BC_SEGARRAY_SEG_LAST;
	memcpy(dest, src, sa->esize);
}

static void bc_segarray_move(BcSegArray* sa, uint32_t idx1, uint32_t start, uint32_t num_elems) {

	assert(num_elems < BC_SEGARRAY_SEG_SIZE &&
	       start + num_elems < BC_SEGARRAY_SEG_SIZE);

	uint8_t* ptr = sa->ptrs[idx1];

	size_t esize = sa->esize;

	size_t move_size = esize * num_elems;

	uint8_t* src = ptr + esize * start;
	uint8_t* dest = src + esize;

	memmove(dest, src, move_size);
}
