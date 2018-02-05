#ifndef BC_VECTOR_H
#define BC_VECTOR_H

#include <stdlib.h>
#include <stdint.h>

#include <bc/bc.h>

#define BC_VEC_INITIAL_CAP (32)

typedef struct BcVec {

  uint8_t* array;
  uint32_t len;
  uint32_t cap;
  size_t size;

  BcFreeFunc sfree;

} BcVec;

BcStatus bc_vec_init(BcVec* stack, size_t esize, BcFreeFunc sfree);

BcStatus bc_vec_push(BcVec* stack, void* data);

BcStatus bc_vec_pushAt(BcVec* vec, void* data, uint32_t idx);

void* bc_vec_top(BcVec* stack);

void* bc_vec_item(BcVec* stack, uint32_t idx);

BcStatus bc_vec_pop(BcVec* stack);

void bc_vec_free(void* stack);

#endif // BC_VECTOR_H
