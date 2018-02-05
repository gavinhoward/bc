#ifndef BC_DATA_H
#define BC_DATA_H

#include <stdbool.h>

#include <arbprec/arbprec.h>

#include <bc/bc.h>
#include <bc/vector.h>

#define BC_PROGRAM_MAX_STMTS (128)

#define BC_PROGRAM_DEF_SIZE (16)

typedef enum BcTempType {

  BC_TEMP_NUM,
  BC_TEMP_NAME,

  BC_TEMP_SCALE,
  BC_TEMP_IBASE,
  BC_TEMP_OBASE,
  BC_TEMP_LAST,

} BcTempType;

typedef enum BcExprType {

  BC_EXPR_INC_PRE,
  BC_EXPR_DEC_PRE,

  BC_EXPR_INC_POST,
  BC_EXPR_DEC_POST,

  BC_EXPR_NEGATE,

  BC_EXPR_POWER,

  BC_EXPR_MULTIPLY,
  BC_EXPR_DIVIDE,
  BC_EXPR_MODULUS,

  BC_EXPR_PLUS,
  BC_EXPR_MINUS,

  BC_EXPR_ASSIGN_POWER,
  BC_EXPR_ASSIGN_MULTIPLY,
  BC_EXPR_ASSIGN_DIVIDE,
  BC_EXPR_ASSIGN_MODULUS,
  BC_EXPR_ASSIGN_PLUS,
  BC_EXPR_ASSIGN_MINUS,
  BC_EXPR_ASSIGN,

  BC_EXPR_REL_EQUAL,
  BC_EXPR_REL_LESS_EQ,
  BC_EXPR_REL_GREATER_EQ,
  BC_EXPR_REL_NOT_EQ,
  BC_EXPR_REL_LESS,
  BC_EXPR_REL_GREATER,

  BC_EXPR_BOOL_NOT,

  BC_EXPR_BOOL_OR,
  BC_EXPR_BOOL_AND,

  BC_EXPR_NUMBER,
  BC_EXPR_VAR,
  BC_EXPR_ARRAY_ELEM,

  BC_EXPR_FUNC_CALL,

  BC_EXPR_SCALE_FUNC,
  BC_EXPR_SCALE,
  BC_EXPR_IBASE,
  BC_EXPR_OBASE,
  BC_EXPR_LAST,
  BC_EXPR_LENGTH,
  BC_EXPR_READ,
  BC_EXPR_SQRT,

  BC_EXPR_PRINT,

} BcExprType;

typedef struct BcAuto {

  char* name;
  bool var;

} BcAuto;

typedef struct BcLocal {

  const char* name;
  bool var;

  union {
    fxdpnt num;
    struct {
      fxdpnt* array;
      uint32_t num_elems;
    };
  };

} BcLocal;

typedef struct BcTemp {

  BcTempType type;

  union {

    fxdpnt* num;
    const char* name;

  };

} BcTemp;

typedef struct BcFunc {

  char* name;

  BcVec code;

  BcAuto* params;
  uint32_t num_params;
  uint32_t param_cap;

  BcAuto* autos;
  uint32_t num_autos;
  uint32_t auto_cap;

} BcFunc;

typedef struct BcVar {

  char* name;

  fxdpnt* data;

} BcVar;

typedef struct BcArray {

  char* name;

  BcVec array;

} BcArray;

typedef struct BcInstPtr {

  size_t func;
  size_t idx;

} BcInstPtr;

BcStatus bc_func_init(BcFunc* func, char* name);
BcStatus bc_func_insertParam(BcFunc* func, char* name, bool var);
BcStatus bc_func_insertAuto(BcFunc* func, char* name, bool var);
int bc_func_cmp(void* func1, void* func2);
void bc_func_free(void* func);

BcStatus bc_var_init(BcVar* var, char* name);
int bc_var_cmp(void* var1, void* var2);
void bc_var_free(void* var);

BcStatus bc_array_init(BcArray* array, char* name);
int bc_array_cmp(void* array1, void* array2);
void bc_array_free(void* array);

BcStatus bc_local_initVar(BcLocal* local, const char* name, const char* num);
BcStatus bc_local_initArray(BcLocal* local, const char* name, uint32_t nelems);
void bc_local_free(void* local);

BcStatus bc_temp_initNum(BcTemp* temp, const char* val);
BcStatus bc_temp_initName(BcTemp* temp, const char* name);
BcStatus bc_temp_init(BcTemp* temp, BcTempType type);
void bc_temp_free(void* temp);

void bc_string_free(void* string);

#endif // BC_DATA_H
