#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "stack.h"

static BcStatus bc_stack_expand(BcStack* stack);

BcStatus bc_stack_init(BcStack* stack, size_t esize) {

	// Check for invalid params.
	if (stack == NULL || esize == 0) {
		return BC_STATUS_INVALID_PARAM;
	}

	// Set the fields.
	stack->size = esize;
	stack->cap = BC_STACK_START;
	stack->len = 0;

	// Allocate the array.
	stack->stack = malloc(esize * BC_STACK_START);

	// Check for error.
	if (stack->stack == NULL) {
		return BC_STATUS_MALLOC_FAIL;
	}

	return BC_STATUS_SUCCESS;
}

BcStatus bc_stack_push(BcStack* stack, void* data) {

	// Check for invalid params.
	if (stack == NULL || data == NULL) {
		return BC_STATUS_INVALID_PARAM;
	}

	// Check if we need to expand.
	if (stack->len == stack->cap) {

		// Expand the stack.
		BcStatus status = bc_stack_expand(stack);

		// Check for error.
		if (status != BC_STATUS_SUCCESS) {
			return status;
		}
	}

	// Copy the data.
	size_t size = stack->size;
	memmove(stack->stack + (size * stack->len), data, size);

	// Increment the length.
	++stack->len;

	return BC_STATUS_SUCCESS;
}

void* bc_stack_top(BcStack* stack) {

	// Check for invalid state.
	if (stack == NULL || stack->len == 0) {
		return NULL;
	}

	// Calculate the return pointer.
	return stack->stack + stack->size * (stack->len - 1);
}

void* bc_stack_item(BcStack* stack, uint32_t idx) {

	// Check for invalid state.
	if (stack == NULL || stack->len == 0 || idx >= stack->len) {
		return NULL;
	}

	// Calculate the return pointer.
	return stack->stack + stack->size * (stack->len - idx - 1);
}

BcStatus bc_stack_pop(BcStack* stack) {

	// Check for invalid params.
	if (stack == NULL) {
		return BC_STATUS_INVALID_PARAM;
	}

	// Decrement the length.
	--stack->len;

	return BC_STATUS_SUCCESS;
}

void bc_stack_free(void* stack) {

	BcStack* s;

	s = (BcStack*) stack;

	// Check for NULL.
	if (s == NULL) {
		return;
	}

	// Free the stack.
	free(s->stack);

	// Zero the fields.
	s->size = 0;
	s->stack = NULL;
	s->len = 0;
	s->cap = 0;
}

static BcStatus bc_stack_expand(BcStack* stack) {

	// Realloc.
	uint8_t* ptr = realloc(stack->stack, stack->size * (stack->cap + BC_STACK_START));

	// Check for error.
	if (ptr == NULL) {
		return BC_STATUS_MALLOC_FAIL;
	}

	// Assign the fields.
	stack->stack = ptr;
	stack->cap += BC_STACK_START;

	return BC_STATUS_SUCCESS;
}
