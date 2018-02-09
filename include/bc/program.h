#ifndef BC_PROGRAM_H
#define BC_PROGRAM_H

#include <stdbool.h>
#include <stdint.h>

#include <bc/bc.h>
#include <bc/data.h>

#include <arbprec/arbprec.h>

#define BC_PROGRAM_BUF_SIZE (1024)

typedef fxdpnt* (*BcMathOpFunc)(fxdpnt*, fxdpnt*, fxdpnt*, int, size_t);

typedef struct BcProgram {

  size_t idx;

  BcVec ip_stack;

  long scale;
  long ibase;
  long obase;

  long base_max;
  long dim_max;
  long scale_max;
  long string_max;

  BcVec stack;

  BcVec locals;

  BcVec temps;

  BcVec funcs;

  BcVecO func_map;

  BcVec vars;

  BcVecO var_map;

  BcVec arrays;

  BcVecO array_map;

  BcVec strings;

  BcVec constants;

  const char* file;

  fxdpnt* last;

  fxdpnt* zero;
  fxdpnt* one;

  char* num_buf;
  size_t buf_size;

} BcProgram;

BcStatus bc_program_init(BcProgram* p);
void bc_program_limits(BcProgram* p);
BcStatus bc_program_func_add(BcProgram* p, char* name, size_t* idx);
BcStatus bc_program_var_add(BcProgram* p, char *name, size_t *idx);
BcStatus bc_program_array_add(BcProgram* p, char *name, size_t *idx);
BcStatus bc_program_exec(BcProgram* p);
void bc_program_printCode(BcProgram* p);
void bc_program_free(BcProgram* program);

#endif // BC_PROGRAM_H
