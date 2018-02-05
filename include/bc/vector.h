#ifndef BC_VECTOR_H
#define BC_VECTOR_H

#include <stdlib.h>
#include <stdint.h>

#include <bc/bc.h>

#define BC_VEC_INITIAL_CAP (32)

typedef struct BcVec {

  uint8_t* array;
  size_t len;
  size_t cap;
  size_t size;

  BcFreeFunc dtor;

} BcVec;

BcStatus bc_vec_init(BcVec* vec, size_t esize, BcFreeFunc dtor);

BcStatus bc_vec_push(BcVec* vec, void* data);

BcStatus bc_vec_pushByte(BcVec* vec, uint8_t data);

BcStatus bc_vec_pushAt(BcVec* vec, void* data, size_t idx);

void* bc_vec_top(BcVec* vec);

void* bc_vec_item(BcVec* vec, size_t idx);

BcStatus bc_vec_pop(BcVec* vec);

void bc_vec_free(void* vec);

#endif // BC_VECTOR_H
