#ifndef BC_PROGRAM_H
#define BC_PROGRAM_H

#include <stdint.h>

#include "segarray.h"
#include "bc.h"

/**
 * @def BC_PROGRAM_MAX_STMTS
 * The max number of statements in one statement list.
 */
#define BC_PROGRAM_MAX_STMTS (128)

/**
 * @def BC_PROGRAM_MAX_NAMES
 * The max number of names that the program can have.
 */
#define BC_PROGRAM_MAX_NAMES (32767)

/**
 * The types that any statement can be.
 */
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

/**
 * A union of data that statement types need.
 */
typedef union BcStmtData {

	char* string;

} BcStmtData;

/**
 * One statement in a program.
 */
typedef struct BcStmt {

	BcStmtType type;
	BcStmtData data;

} BcStmt;

/**
 * A list of statements. Lists can be chained together.
 */
typedef struct BcStmtList {

	/// The next list. NULL if none.
	struct BcStmtList* next;

	/// The number of statements that this list has.
	uint32_t num_stmts;

	/// The list of statements.
	BcStmt stmts[BC_PROGRAM_MAX_STMTS];

} BcStmtList;

/**
 * A function.
 */
typedef struct BcFunc {

	/// The name of the function.
	char* name;

	/// The first of the statement lists.
	BcStmtList* first;

	/// The current statement list.
	BcStmtList* cur;

	/// The number of auto variables the function
	/// has. This number includes parameters.
	uint32_t num_autos;

} BcFunc;

/**
 * A variable.
 */
typedef struct BcVar {

	/// The name of the variable.
	char* name;

	/// The data. This will a pointer to the value
	/// struct that will actually hold a value.
	void* data;

} BcVar;

/**
 * An array.
 */
typedef struct BcArray {

	/// The name of the array.
	char* name;

	/// An array of data. This will be an array of
	/// pointers to the structs that hold values.
	void** array;

} BcArray;

typedef struct BcContext {

	BcStmtList* list;
	uint32_t stmt_idx;

} BcContext;

/**
 * A program. This holds all of the data
 * that allows the program to be executed.
 */
typedef struct BcProgram {

	/// The first of the statement lists.
	BcStmtList* first;

	/// The pointer to the current statement list.
	BcStmtList* cur;

	BcSegArray funcs;

	BcSegArray vars;

	BcSegArray arrays;

} BcProgram;

/**
 * Initializes @a program.
 * @param program	The program to initialize.
 * @return			BC_STATUS_SUCCESS on success,
 *					an error code otherwise.
 */
BcStatus bc_program_init(BcProgram* p);

/**
 * Inserts @a stmt into @a program.
 * @param program	The program to insert into.
 * @param stmt		The statement to insert.
 * @return			BC_STATUS_SUCCESS on success,
 *					an error code otherwise.
 */
BcStatus bc_program_insert(BcProgram* p, BcStmt* stmt);

BcStatus bc_program_exec(BcProgram* p);

void bc_program_free(BcProgram* program);

#endif // BC_PROGRAM_H
