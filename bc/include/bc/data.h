#ifndef BC_DATA_H
#define BC_DATA_H

#include <stdbool.h>

#include <arbprec/arbprec.h>

#include <bc/segarray.h>
#include <bc/bc.h>

#define BC_PROGRAM_MAX_STMTS (128)

#define BC_PROGRAM_DEF_SIZE (16)

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

	BC_EXPR_NUMBER,
	BC_EXPR_VAR,
	BC_EXPR_ARRAY_ELEM,

	BC_EXPR_FUNC_CALL,

	BC_EXPR_SCALE,
	BC_EXPR_SCALE_FUNC,
	BC_EXPR_IBASE,
	BC_EXPR_OBASE,
	BC_EXPR_LAST,
	BC_EXPR_LENGTH,
	BC_EXPR_READ,
	BC_EXPR_SQRT,

	BC_EXPR_PRINT,

} BcExprType;

typedef enum BcStmtType {

	BC_STMT_EXPR,

	BC_STMT_STRING,
	BC_STMT_STRING_PRINT,

	BC_STMT_BREAK,
	BC_STMT_CONTINUE,

	BC_STMT_HALT,

	BC_STMT_RETURN,

	BC_STMT_IF,
	BC_STMT_WHILE,
	BC_STMT_FOR,

	BC_STMT_LIST,

} BcStmtType;

typedef struct BcCall {

	char* name;
	BcSegArray params;

} BcCall;

typedef struct BcArrayElem {

	char* name;
	BcStack expr_stack;

} BcArrayElem;

typedef struct BcExpr {

	BcExprType type;
	union {
		char* string;
		BcStack* expr_stack;
		BcCall* call;
		BcArrayElem* elem;
	};

} BcExpr;

typedef struct BcIf {

	BcStack cond;
	struct BcStmtList* then_list;
	struct BcStmtList* else_list;

} BcIf;

typedef struct BcWhile {

	BcStack cond;
	struct BcStmtList* body;

} BcWhile;

typedef struct BcFor {

	BcStack cond;
	struct BcStmtList* body;
	BcStack update;
	BcStack init;

} BcFor;

typedef union BcStmtData {

	char* string;
	struct BcStmtList* list;
	BcStack* expr_stack;
	BcIf* if_stmt;
	BcWhile* while_stmt;
	BcFor* for_stmt;

} BcStmtData;

typedef struct BcStmt {

	BcStmtType type;
	BcStmtData data;

} BcStmt;

typedef struct BcStmtList {

	struct BcStmtList* next;

	uint32_t idx;
	uint32_t num_stmts;

	BcStmt stmts[BC_PROGRAM_MAX_STMTS];

} BcStmtList;

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

typedef struct BcFunc {

	char* name;

	BcStmtList* first;

	BcStmtList* cur;

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

	BcSegArray array;

} BcArray;

BcStmtList* bc_list_create();
BcStatus bc_list_insert(BcStmtList* list, BcStmt* stmt);
BcStmt* bc_list_last(BcStmtList* list);
void bc_list_destruct(BcStmtList* list);
void bc_list_free(BcStmtList* list);

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

BcStatus bc_stmt_init(BcStmt* stmt, BcStmtType type);
void bc_stmt_free(BcStmt* stmt);

BcIf* bc_if_create();
BcWhile* bc_while_create();
BcFor* bc_for_create();

BcStatus bc_expr_init(BcExpr* expr, BcExprType type);
void bc_expr_free(void* expr);

BcCall* bc_call_create();

BcStatus bc_local_initVar(BcLocal* local, const char* name, const char* num);
BcStatus bc_local_initArray(BcLocal* local, const char* name, uint32_t nelems);
void bc_local_free(void* local);

#endif // BC_DATA_H
