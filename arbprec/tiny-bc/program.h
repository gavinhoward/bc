#ifndef BC_PROGRAM_H
#define BC_PROGRAM_H

#include <stdint.h>

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

	BC_STMT_EXPR_VAR,
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
	BC_STMT_EXPR_HALT,

} BcStmtType;

typedef struct BcStmt {

	BcStmtType type;

} BcStmt;

typedef struct BcProgram {

	struct BcProgram* next;
	uint32_t num_stmts;
	BcStmt stmts[BC_PROGRAM_MAX_STMTS];

} BcProgram;

void bc_program_init(BcProgram* program);

BcProgram* bc_program_expand(BcProgram* program);

void bc_program_free(BcProgram* program);

#endif // BC_PROGRAM_H
