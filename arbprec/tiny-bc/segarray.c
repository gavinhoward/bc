#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "segarray.h"
#include "program.h"

static BcStatus bc_segarray_addArray(BcSegArray* sa, uint32_t idx);

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

	// If we need to expand...
	if (end2 == 0 && end1 > 0) {

		// Do the expand.
		status = bc_segarray_addArray(sa, end1);

		// Check for error.
		if (status != BC_STATUS_SUCCESS) {
			return status;
		}
	}

	size_t esize = sa->esize;
	uint8_t* ptr = sa->ptrs[end1];
	memcpy(ptr + esize * end2, data, esize);

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

	BcSegArrayCmpFunc cmp = sa->cmp;
	uint32_t num = sa->num;

	for (uint32_t i = 0; i < num; ++i) {

		uint32_t idx1 = BC_SEGARRAY_IDX1(i);
		uint32_t idx2 = BC_SEGARRAY_IDX2(i);

		if (!cmp(bc_segarray_item2(sa, idx1, idx2), data)) {
			return i;
		}
	}

	return (uint32_t) -1;
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
