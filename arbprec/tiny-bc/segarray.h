#ifndef BC_SEGARRAY_H
#define BC_SEGARRAY_H

#include <stdint.h>
#include <stdlib.h>

#include "stack.h"

#define BC_SEGARRAY_SEG_POWER (10)

#define BC_SEGARRAY_SEG_SIZE (1 << BC_SEGARRAY_SEG_POWER)

#define BC_SEGARRAY_SEG_IDX2_MASK (BC_SEGARRAY_SEG_SIZE - 1)

#define BC_SEGARRAY_SEG_IDX1_MASK (~BC_SEGARRAY_SEG_IDX2_MASK)

#define BC_SEGARRAY_IDX1(idx)  \
	(((idx) & BC_SEGARRAY_SEG_IDX1_MASK) >> BC_SEGARRAY_SEG_POWER)

#define BC_SEGARRAY_IDX2(idx) ((idx) & BC_SEGARRAY_SEG_IDX2_MASK)

typedef int (*BcSegArrayCmpFunc)(void*, void*);

typedef struct BcSegArray {

	size_t esize;
	uint32_t num;
	BcStack stack;
	BcSegArrayCmpFunc cmp;

} BcSegArray;

BcStatus bc_segarray_init(BcSegArray* sa, size_t esize, BcSegArrayCmpFunc cmp);

BcStatus bc_segarrary_add(BcSegArray* sa, void*data);

void* bc_segarray_item(BcSegArray* sa, uint32_t idx);

void* bc_segarray_item2(BcSegArray* sa, uint32_t idx1, uint32_t idx2);

uint32_t bc_segarray_find(BcSegArray* sa, void* data);

void bc_segarray_free(BcSegArray* sa);

#endif // BC_SEGARRAY_H
