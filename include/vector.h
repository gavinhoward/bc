/*
 * Copyright 2018 Contributors
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.

 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *******************************************************************************
 *
 * A special license exemption is granted to the Toybox project to use this
 * source under the following BSD 0-Clause License:
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 *******************************************************************************
 *
 * Definitions for bc vectors (resizable arrays).
 *
 */

#ifndef BC_VECTOR_H
#define BC_VECTOR_H

#include <stdlib.h>
#include <stdint.h>

#include <bc.h>

#define BC_VEC_INITIAL_CAP (32)

typedef int (*BcVecCmpFunc)(void*, void*);

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

typedef struct BcVecO {

  BcVec vec;
  BcVecCmpFunc cmp;

} BcVecO;

BcStatus bc_veco_init(BcVecO* vec, size_t esize,
                      BcFreeFunc dtor, BcVecCmpFunc cmp);

BcStatus bc_veco_insert(BcVecO* vec, void* data, size_t* idx);

size_t bc_veco_index(BcVecO* vec, void* data);

void* bc_veco_item(BcVecO* vec, size_t idx);

void bc_veco_free(BcVecO* vec);

#endif // BC_VECTOR_H
