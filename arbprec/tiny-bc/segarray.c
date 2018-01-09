#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "segarray.h"
#include "program.h"

static BcStatus bc_segarray_addArray(BcSegArray* sa, uint32_t idx);
static inline void bc_segarray_moveLast(BcSegArray* sa, uint32_t end1);
static void bc_segarray_move(BcSegArray* sa, uint32_t idx1, uint32_t start, uint32_t num_elems);

BcStatus bc_segarray_init(BcSegArray* sa, size_t esize, BcSegArrayCmpFunc cmp) {

	// Check for invalid params.
	if (sa == NULL || esize == 0 || cmp == NULL) {
		return BC_STATUS_INVALID_PARAM;
	}

	// Set the fields.
	sa->esize = esize;
	sa->cmp = cmp;
	sa->num = 0;

	// Init the stack.
	sa->ptrs = malloc(sizeof(uint8_t*) * BC_SEGARRAY_NUM_ARRAYS);
	if (!sa->ptrs) {
		return BC_STATUS_MALLOC_FAIL;
	}

	// Add an array.
	return bc_segarray_addArray(sa, 0);
}

BcStatus bc_segarrary_add(BcSegArray* sa, void* data) {

	BcStatus status;

	// Check for invalid params.
	if (sa == NULL || data == NULL) {
		return BC_STATUS_INVALID_PARAM;
	}

	if (sa->num >= BC_SEGARRAY_MAX) {
		return BC_STATUS_SEGARRAY_MAX;
	}

	// Get the indices of the end.
	uint32_t end = sa->num;
	uint32_t end1 = BC_SEGARRAY_IDX1(end);
	uint32_t end2 = BC_SEGARRAY_IDX2(end);

	// Get the insert index and split it.
	uint32_t idx = bc_segarray_find(sa, data);
	idx = idx == (uint32_t) -1 ? end : idx;
	uint32_t idx1 = BC_SEGARRAY_IDX1(idx);
	uint32_t idx2 = BC_SEGARRAY_IDX2(idx);

	// If we need to expand...
	if (end2 == 0 && end1 > 0) {

		// Do the expand.
		status = bc_segarray_addArray(sa, end1);

		// Check for error.
		if (status != BC_STATUS_SUCCESS) {
			return status;
		}

		// Move the last element of the previous array.
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

void* bc_segarray_item(BcSegArray* sa, uint32_t idx) {
	return bc_segarray_item2(sa, BC_SEGARRAY_IDX1(idx), BC_SEGARRAY_IDX2(idx));
}

void* bc_segarray_item2(BcSegArray* sa, uint32_t idx1, uint32_t idx2) {

	// Check for NULL or out of bounds.
	uint32_t num = sa->num;
	uint32_t num1 = BC_SEGARRAY_IDX1(num);
	uint32_t num2 = BC_SEGARRAY_IDX2(num);
	if (sa == NULL || idx1 > num1 || (idx1 == num1 && idx2 >= num2)) {
		return NULL;
	}

	// Get the pointer.
	uint8_t* ptr = sa->ptrs[idx1];

	// Check for error.
	if (ptr == NULL) {
		return NULL;
	}

	// Calculate the return pointer.
	return ptr + sa->esize * idx2;
}

uint32_t bc_segarray_find(BcSegArray* sa, void* data) {

	// Cache this.
	BcSegArrayCmpFunc cmp = sa->cmp;

	// Set the high and low.
	uint32_t low = 0;
	uint32_t high = sa->num;

	// Loop until the index is found.
	while (low < high) {

		// Calculate the mid point.
		uint32_t mid = (low + high) / 2;

		// Get the pointer.
		uint8_t* ptr = bc_segarray_item(sa, mid);

		// Figure out what to do.
		if (cmp(ptr, data) > 0) {
			high = mid;
		}
		else {
			low = mid + 1;
		}
	}

	return low;
}

void bc_segarray_free(BcSegArray* sa) {

	// Check for NULL.
	if (sa == NULL) {
		return;
	}

	// Loop through the arrays and free them all.
	uint32_t num_arrays = BC_SEGARRAY_IDX1(sa->num - 1);
	for (uint32_t i = num_arrays - 1; i < num_arrays; --i) {
		uint8_t* ptr = bc_segarray_item(sa, i);
		free(ptr);
	}

	// Free the stack.
	free(sa->ptrs);
	sa->ptrs = NULL;

	// Clear these.
	sa->esize = 0;
	sa->cmp = NULL;
}

static BcStatus bc_segarray_addArray(BcSegArray* sa, uint32_t idx) {

	// Malloc space.
	uint8_t* ptr = malloc(sa->esize * BC_SEGARRAY_SEG_SIZE);

	// Check for error.
	if (ptr == NULL) {
		return BC_STATUS_MALLOC_FAIL;
	}

	sa->ptrs[idx] = ptr;

	return BC_STATUS_SUCCESS;
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
