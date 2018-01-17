#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <bc/bc.h>
#include <bc/stack.h>

static BcStatus bc_stack_expand(BcStack* stack);

BcStatus bc_stack_init(BcStack* stack, size_t esize, BcFreeFunc sfree) {

	if (stack == NULL || esize == 0) {
		return BC_STATUS_INVALID_PARAM;
	}

	stack->size = esize;
	stack->cap = BC_STACK_START;
	stack->len = 0;
	stack->sfree = sfree;

	stack->stack = malloc(esize * BC_STACK_START);

	if (stack->stack == NULL) {
		return BC_STATUS_MALLOC_FAIL;
	}

	return BC_STATUS_SUCCESS;
}

BcStatus bc_stack_push(BcStack* stack, void* data) {

	if (stack == NULL || data == NULL) {
		return BC_STATUS_INVALID_PARAM;
	}

	if (stack->len == stack->cap) {

		BcStatus status = bc_stack_expand(stack);

		if (status != BC_STATUS_SUCCESS) {
			return status;
		}
	}

	size_t size = stack->size;
	memmove(stack->stack + (size * stack->len), data, size);

	++stack->len;

	return BC_STATUS_SUCCESS;
}

void* bc_stack_top(BcStack* stack) {

	if (stack == NULL || stack->len == 0) {
		return NULL;
	}

	return stack->stack + stack->size * (stack->len - 1);
}

void* bc_stack_item(BcStack* stack, uint32_t idx) {

	if (stack == NULL || stack->len == 0 || idx >= stack->len) {
		return NULL;
	}

	return stack->stack + stack->size * (stack->len - idx - 1);
}

BcStatus bc_stack_pop(BcStack* stack) {

	if (stack == NULL) {
		return BC_STATUS_INVALID_PARAM;
	}

	--stack->len;

	return BC_STATUS_SUCCESS;
}

void bc_stack_free(void* stack) {

	BcStack* s;
	size_t len;
	size_t esize;
	BcFreeFunc sfree;
	uint8_t* array;

	s = (BcStack*) stack;

	if (s == NULL) {
		return;
	}

	sfree = s->sfree;

	if (sfree) {

		len = s->len;
		array = s->stack;
		esize = s->size;

		for (size_t i = 0; i < len; ++i) {
			sfree(array + (i * esize));
		}
	}

	free(s->stack);

	s->size = 0;
	s->stack = NULL;
	s->len = 0;
	s->cap = 0;
}

static BcStatus bc_stack_expand(BcStack* stack) {

	uint8_t* ptr = realloc(stack->stack, stack->size * (stack->cap + BC_STACK_START));

	if (ptr == NULL) {
		return BC_STATUS_MALLOC_FAIL;
	}

	stack->stack = ptr;
	stack->cap += BC_STACK_START;

	return BC_STATUS_SUCCESS;
}
