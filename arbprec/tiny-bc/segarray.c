#include <stdint.h>
#include <stdlib.h>

#include "segarray.h"

static BcStatus bc_segarray_addArray(BcSegArray* sa);

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
	BcStatus status = bc_stack_init(&sa->stack, sizeof(uint8_t*));

	// Check for error.
	if (status != BC_STATUS_SUCCESS) {
		return status;
	}

	// Add an array.
	status = bc_segarray_addArray(sa);

	return status;
}

BcStatus bc_segarrary_add(BcSegArray* sa, void* data) {

}

void* bc_segarray_item(BcSegArray* sa, uint32_t idx) {
	return bc_segarray_item2(sa, BC_SEGARRAY_IDX1(idx), BC_SEGARRAY_IDX2(idx));
}

void* bc_segarray_item2(BcSegArray* sa, uint32_t idx1, uint32_t idx2) {

	// Check for NULL or out of bounds.
	uint32_t num = sa->num;
	if (sa == NULL || idx1 > BC_SEGARRAY_IDX1(num) || idx2 >= BC_SEGARRAY_IDX2(num)) {
		return NULL;
	}

	// Get the pointer.
	uint8_t* ptr = bc_stack_item(&sa->stack, idx1);

	// Check for error.
	if (ptr == NULL) {
		return NULL;
	}

	// Calculate the return pointer.
	return ptr + sa->esize * idx2;
}

void bc_segarray_free(BcSegArray* sa) {

	// Check for NULL.
	if (sa == NULL) {
		return;
	}

	// Clear these.
	sa->esize = 0;
	sa->cmp = NULL;

	// Loop through the arrays and free them all.
	uint32_t num_arrays = BC_SEGARRAY_IDX1(sa->num - 1);
	for (uint32_t i = num_arrays - 1; i < num_arrays; --i) {
		uint8_t** ptr = bc_stack_item(&sa->stack, i);
		free(*ptr);
	}

	// Free the stack.
	bc_stack_free(&sa->stack);
}

static BcStatus bc_segarray_addArray(BcSegArray* sa) {

	// Malloc space.
	uint8_t* ptr = malloc(sa->esize * BC_SEGARRAY_SEG_SIZE);

	// Check for error.
	if (ptr == NULL) {
		return BC_STATUS_MALLOC_FAIL;
	}

	// Push the array onto the stack.
	return bc_stack_push(&sa->stack, &ptr);
}
