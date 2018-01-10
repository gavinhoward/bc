#ifndef BC_PROGRAM_H
#define BC_PROGRAM_H

#include <stdint.h>

#include <arbprec/arbprec.h>

#include "segarray.h"
#include "bc.h"

#define BC_PROGRAM_MAX_STMTS (128)

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

	BC_EXPR_ASSIGN,
	BC_EXPR_ASSIGN_PLUS,
	BC_EXPR_ASSIGN_MINUS,
	BC_EXPR_ASSIGN_MULTIPLY,
	BC_EXPR_ASSIGN_DIVIDE,
	BC_EXPR_ASSIGN_MODULUS,
	BC_EXPR_ASSIGN_POWER,

	BC_EXPR_REL_EQUAL,
	BC_EXPR_REL_LESS_EQ,
	BC_EXPR_REL_GREATER_EQ,
	BC_EXPR_REL_NOT_EQ,
	BC_EXPR_REL_LESS,
	BC_EXPR_REL_GREATER,

	BC_EXPR_BOOL_NOT,

	BC_EXPR_BOOL_OR,
	BC_EXPR_BOOL_AND,

	BC_EXPR_VAR,
	BC_EXPR_ARRAY,
	BC_EXPR_NUMBER,

	BC_EXPR_FUNC_CALL,

	BC_EXPR_SCALE,
	BC_EXPR_SCALE_FUNC,
	BC_EXPR_IBASE,
	BC_EXPR_OBASE,
	BC_EXPR_LAST,

	BC_EXPR_PRINT,

} BcExprType;

typedef enum BcStmtType {

	BC_STMT_EXPR,

	BC_STMT_STRING,

	BC_STMT_BREAK,
	BC_STMT_CONTINUE,

	BC_STMT_HALT,

	BC_STMT_RETURN,

	BC_STMT_IF,
	BC_STMT_WHILE,
	BC_STMT_FOR,

} BcStmtType;

typedef struct BcExpr {

	BcExprType type;
	char* string;

} BcExpr;

typedef struct BcStmtList BcStmtList;

typedef union BcStmtData {

	char* string;
	BcStmtList* list;
	BcStack expr_stack;

} BcStmtData;

typedef struct BcStmt {

	BcStmtType type;
	BcStmtData data;

} BcStmt;

typedef struct BcStmtList {

	struct BcStmtList* next;

	uint32_t num_stmts;

	BcStmt stmts[BC_PROGRAM_MAX_STMTS];

} BcStmtList;

/**
 * A function.
 */
typedef struct BcFunc {

	char* name;

	BcStmtList* first;

	BcStmtList* cur;

	uint32_t num_autos;

} BcFunc;

typedef struct BcVar {

	char* name;

	fxdpnt* data;

} BcVar;

/**
 * An array.
 */
typedef struct BcArray {

	char* name;

	BcSegArray array;

} BcArray;

typedef struct BcContext {

	BcStmtList* list;
	uint32_t stmt_idx;

} BcContext;

typedef struct BcProgram {

	BcStmtList* first;

	BcStmtList* cur;

	BcSegArray funcs;

	BcSegArray vars;

	BcSegArray arrays;

} BcProgram;

BcStatus bc_program_init(BcProgram* p);
BcStatus bc_program_list_insert(BcStmtList* list, BcStmt* stmt);
BcStatus bc_program_func_add(BcProgram* p, BcFunc* func);
BcStatus bc_program_var_add(BcProgram* p, BcVar* var);
BcStatus bc_program_array_add(BcProgram* p, BcArray* array);
BcStatus bc_program_exec(BcProgram* p);
void bc_program_free(BcProgram* program);

BcStmtList* bc_program_list_create();
void bc_program_list_free(BcStmtList* list);

BcStatus bc_program_func_init(BcFunc* func, char* name);
BcStatus bc_program_var_init(BcVar* var, char* name);
BcStatus bc_program_array_init(BcArray* array, char* name);

BcStatus bc_program_stmt_init(BcStmt* stmt);

#endif // BC_PROGRAM_H
