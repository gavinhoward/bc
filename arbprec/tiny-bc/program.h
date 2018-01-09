#ifndef BC_PROGRAM_H
#define BC_PROGRAM_H

#include <stdint.h>

#include <arbprec/arbprec.h>

#include "segarray.h"
#include "bc.h"

#define BC_PROGRAM_MAX_STMTS (128)

typedef enum BcStmtType {

	BC_STMT_STRING,

	BC_STMT_BREAK,
	BC_STMT_CONTINUE,

	BC_STMT_HALT,

	BC_STMT_RETURN,

	BC_STMT_IF,
	BC_STMT_WHILE,
	BC_STMT_FOR,

	BC_STMT_EXPR_NAME,
	BC_STMT_EXPR_ARRAY,
	BC_STMT_EXPR_NUMBER,

	BC_STMT_EXPR_PARENS,

	BC_STMT_EXPR_BIN_OP,

	BC_STMT_EXPR_PRE_INC,
	BC_STMT_EXPR_PRE_DEC,

	BC_STMT_EXPR_POST_INC,
	BC_STMT_EXPR_POST_DEC,

	BC_STMT_EXPR_ASSIGN,

	BC_STMT_EXPR_FUNC_CALL,

	BC_STMT_EXPR_REL_OP,
	BC_STMT_EXPR_BOOL_OP,

	BC_STMT_EXPR_SCALE,
	BC_STMT_EXPR_SCALE_FUNC,
	BC_STMT_EXPR_IBASE,
	BC_STMT_EXPR_OBASE,
	BC_STMT_EXPR_LAST,
	BC_STMT_EXPR_HALT,

} BcStmtType;

typedef struct BcStmtList BcStmtList;

typedef union BcStmtData {

	char* string;
	BcStmtList* list;

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

	fxdpnt** array;

	uint32_t elems;

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
BcStatus bc_program_insert(BcProgram* p, BcStmt* stmt);
BcStatus bc_program_func_add(BcProgram* p, BcFunc* func);
BcStatus bc_program_var_add(BcProgram* p, BcVar* var);
BcStatus bc_program_array_add(BcProgram* p, BcArray* array);
BcStatus bc_program_exec(BcProgram* p);
void bc_program_free(BcProgram* program);

BcStmtList* bc_program_list_create();
BcStatus bc_program_list_expand(BcStmtList* list);
void bc_program_list_free(BcStmtList* list);

#endif // BC_PROGRAM_H
