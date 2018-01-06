#include <stdlib.h>
#include <stdint.h>

#include "bc.h"

#define BC_STACK_START (16)

typedef struct BcStack {

	size_t size;
	uint8_t* stack;
	uint32_t len;
	uint32_t cap;

} BcStack;

BcStatus bc_stack_init(BcStack* stack, size_t esize);

BcStatus bc_stack_push(BcStack* stack, void* data);

void* bc_stack_top(BcStack* stack);

BcStatus bc_stack_pop(BcStack* stack);

void bc_stack_free(BcStack* stack);
