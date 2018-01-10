#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "lex.h"
#include "parse.h"

// This is an array that corresponds to token types. An entry is
// true if the token is valid in an expression, false otherwise.
static const bool bc_token_exprs[] = {

    true,
    true,

    true,

    true,
    true,
    true,

    true,
    true,

    true,
    true,
    true,
    true,
    true,
    true,
    true,

    true,
    true,
    true,
    true,
    true,
    true,

    true,

    true,
    true,

    false,

    false,

    true,
    true,

    true,
    true,

    false,
    false,

    false,
    false,

    false,
    true,
    true,

    false,
    false,
    false,
    false,
    false,
    false,
    false,
    true,
    false,
    true,
    true,
    true,
    true,
    false,
    false,
    true,
    false,
    true,
    true,
    false,

    false,

    false,
};

// This is an array of data for operators that correspond to token types.
// The last corresponds to BC_PARSE_OP_NEGATE_IDX since it doesn't have
// its own token type (it is the same token at the binary minus operator).
static const BcOp bc_ops[] = {

    { 0, false },
    { 0, false },

    { 2, false },

    { 3, true },
    { 3, true },
    { 3, true },

    { 4, true },
    { 4, true },

    { 5, false },
    { 5, false },
    { 5, false },
    { 5, false },
    { 5, false },
    { 5, false },
    { 5, false },

    { 6, true },
    { 6, true },
    { 6, true },
    { 6, true },
    { 6, true },
    { 6, true },

    { 7, false },

    { 8, true },
    { 8, true },

    { 1, false }

};

static BcStatus bc_parse_func(BcParse* parse, BcProgram* program);
static BcStatus bc_parse_semicolonList(BcParse* parse, BcStmtList* list);
static BcStatus bc_parse_semicolonListEnd(BcParse* parse, BcStmtList* list);
static BcStatus bc_parse_stmt(BcParse* parse, BcStmtList* list);
static BcStatus bc_parse_expr(BcParse* parse, BcStack* exprs);
static BcStatus bc_parse_operator(BcLexToken* token, BcExpr* expr);
static BcStatus bc_parse_call(BcParse* parse);
static BcStatus bc_parse_params(BcParse* parse);
static BcStatus bc_parse_string(BcParse* parse, BcStmtList* list);
static BcStatus bc_parse_return(BcParse* parse, BcStmtList* list);

BcStatus bc_parse_init(BcParse* parse, BcProgram* program) {

	if (parse == NULL || program == NULL) {
		return BC_STATUS_INVALID_PARAM;
	}

	parse->program = program;

	BcStatus status = bc_stack_init(&parse->flag_stack, sizeof(uint8_t));

	if (status != BC_STATUS_SUCCESS) {
		return status;
	}

	status = bc_stack_init(&parse->ctx_stack, sizeof(BcContext));

	if (status != BC_STATUS_SUCCESS) {
		return status;
	}

	uint8_t flags = 0;
	BcContext ctx;
	ctx.list = program->first;
	ctx.stmt_idx = 0;

	status = bc_stack_push(&parse->flag_stack, &flags);
	if (status != BC_STATUS_SUCCESS) {
		return status;
	}

	status = bc_stack_push(&parse->ctx_stack, &ctx);

	return status;
}

BcStatus bc_parse_text(BcParse* parse, const char* text) {

	if (parse == NULL || text == NULL) {
		return BC_STATUS_INVALID_PARAM;
	}

	return bc_lex_init(&parse->lex, text);
}

BcStatus bc_parse_parse(BcParse* parse, BcProgram* program) {

	if (parse == NULL) {
		return BC_STATUS_INVALID_PARAM;
	}

	BcStatus status = bc_lex_next(&parse->lex, &parse->token);

	if (status != BC_STATUS_SUCCESS) {
		return status;
	}

	switch (parse->token.type) {

		case BC_LEX_NEWLINE:
		{
			// We don't do anything if there is a newline.
			break;
		}

		case BC_LEX_KEY_DEFINE:
		{
			if (!BC_PARSE_CAN_EXEC(parse)) {
				return BC_STATUS_PARSE_INVALID_TOKEN;
			}

			status = bc_parse_func(parse, program);

			break;
		}

		default:
		{
			status = bc_parse_stmt(parse, program->cur);
			break;
		}
	}

	return status;
}

void bc_parse_free(BcParse* parse) {

	if (!parse) {
		return;
	}

	bc_stack_free(&parse->flag_stack);
	bc_stack_free(&parse->ctx_stack);

	switch (parse->token.type) {

		case BC_LEX_STRING:
		case BC_LEX_NAME:
		case BC_LEX_NUMBER:
		{
			if (parse->token.string) {
				free(parse->token.string);
			}

			break;
		}

		default:
		{
			// We don't have have to free anything.
			break;
		}
	}
}

static BcStatus bc_parse_func(BcParse* parse, BcProgram* program) {

	// TODO: Add to the symbol table.


}

static BcStatus bc_parse_semicolonList(BcParse* parse, BcStmtList* list) {

	BcStatus status = BC_STATUS_SUCCESS;

	switch (parse->token.type) {

		case BC_LEX_OP_INC:
		case BC_LEX_OP_DEC:
		case BC_LEX_OP_MINUS:
		case BC_LEX_OP_BOOL_NOT:
		case BC_LEX_NEWLINE:
		case BC_LEX_LEFT_PAREN:
		{
			status = bc_parse_stmt(parse, list);
			break;
		}

		case BC_LEX_SEMICOLON:
		{
			status = bc_parse_semicolonListEnd(parse, list);
			break;
		}

		case BC_LEX_STRING:
		case BC_LEX_NAME:
		case BC_LEX_NUMBER:
		case BC_LEX_KEY_AUTO:
		case BC_LEX_KEY_BREAK:
		case BC_LEX_KEY_CONTINUE:
		case BC_LEX_KEY_FOR:
		case BC_LEX_KEY_HALT:
		case BC_LEX_KEY_IBASE:
		case BC_LEX_KEY_IF:
		case BC_LEX_KEY_LAST:
		case BC_LEX_KEY_LENGTH:
		case BC_LEX_KEY_LIMITS:
		case BC_LEX_KEY_OBASE:
		case BC_LEX_KEY_PRINT:
		{
			status = bc_parse_stmt(parse, list);
			break;
		}

		case BC_LEX_KEY_QUIT:
		{
			// Quit is a compile-time command,
			// so we just do it.
			exit(status);
			break;
		}

		case BC_LEX_KEY_READ:
		case BC_LEX_KEY_RETURN:
		case BC_LEX_KEY_SCALE:
		case BC_LEX_KEY_SQRT:
		case BC_LEX_KEY_WHILE:
		{
			status = bc_parse_stmt(parse, list);
			break;
		}

		case BC_LEX_EOF:
		{
			if (parse->ctx_stack.len > 0) {
				status = BC_STATUS_PARSE_INVALID_TOKEN;
			}

			break;
		}

		default:
		{
			status = BC_STATUS_PARSE_INVALID_TOKEN;
			break;
		}
	}

	return status;
}

static BcStatus bc_parse_semicolonListEnd(BcParse* parse, BcStmtList* list) {

	BcStatus status;

	if (parse->token.type == BC_LEX_SEMICOLON) {

		status = bc_lex_next(&parse->lex, &parse->token);

		if (status) {
			return status;
		}

		status = bc_parse_semicolonList(parse, list);
	}
	else if (parse->token.type == BC_LEX_NEWLINE) {
		status = BC_STATUS_SUCCESS;
	}
	else {
		status = BC_STATUS_PARSE_INVALID_TOKEN;
	}

	return status;
}

static BcStatus bc_parse_stmt(BcParse* parse, BcStmtList* list) {

	BcStatus status;
	BcStmt stmt;
	BcExpr expr;

	status = BC_STATUS_SUCCESS;

	switch (parse->token.type) {

		case BC_LEX_OP_INC:
		case BC_LEX_OP_DEC:
		case BC_LEX_OP_MINUS:
		case BC_LEX_OP_BOOL_NOT:
		case BC_LEX_LEFT_PAREN:
		case BC_LEX_NAME:
		case BC_LEX_NUMBER:
		case BC_LEX_KEY_IBASE:
		case BC_LEX_KEY_LAST:
		case BC_LEX_KEY_LENGTH:
		case BC_LEX_KEY_LIMITS:
		case BC_LEX_KEY_OBASE:
		case BC_LEX_KEY_READ:
		case BC_LEX_KEY_SCALE:
		case BC_LEX_KEY_SQRT:
		{
			status = bc_parse_expr(parse, &stmt.data.expr_stack);
			break;
		}

		case BC_LEX_NEWLINE:
		{
			// Do nothing.
			break;
		}

		case BC_LEX_STRING:
		{
			status = bc_parse_string(parse, list);
			break;
		}

		case BC_LEX_KEY_AUTO:
		{
			// TODO: Make sure we're in auto place.
			break;
		}

		case BC_LEX_KEY_BREAK:
		{
			// TODO: Make sure we're in a loop.
			break;
		}

		case BC_LEX_KEY_CONTINUE:
		{
			// TODO: Same as above. Combine?
			break;
		}

		case BC_LEX_KEY_FOR:
		{
			// TODO: Set up context for loop.
			break;
		}

		case BC_LEX_KEY_HALT:
		{
			BcStmt stmt;

			stmt.type = BC_STMT_HALT;
			bc_program_list_insert(list, &stmt);

			status = bc_lex_next(&parse->lex, &parse->token);

			if (status) {
				return status;
			}

			status = bc_parse_semicolonListEnd(parse, list);

			break;
		}

		case BC_LEX_KEY_IF:
		{
			// TODO: If statement.
			break;
		}

		case BC_LEX_KEY_PRINT:
		{
			// TODO: Parse print.
			break;
		}

		case BC_LEX_KEY_RETURN:
		{
			status = bc_parse_return(parse, list);
			break;
		}

		case BC_LEX_KEY_WHILE:
		{
			// TODO: Set up context for loop.
			break;
		}

		default:
		{
			status = BC_STATUS_PARSE_INVALID_TOKEN;
			break;
		}
	}

	return status;
}

static BcStatus bc_parse_expr(BcParse* parse, BcStack* exprs) {

	BcStatus status;
	BcStack ops;
	BcExpr expr;
	uint32_t num_exprs;
	bool paren_first;

	paren_first = parse->token.type == BC_LEX_LEFT_PAREN;

	status = bc_stack_init(exprs, sizeof(BcExpr));

	if (status) {
		return status;
	}

	status = bc_stack_init(&ops, sizeof(BcLexToken));

	if (status) {
		return status;
	}

	num_exprs = 0;

	while (!status && bc_token_exprs[parse->token.type]) {

		switch (parse->token.type) {

			case BC_LEX_OP_INC:
			case BC_LEX_OP_DEC:
			case BC_LEX_OP_BOOL_NOT:
			{
				// TODO: Handle these specially.
				break;
			}

			case BC_LEX_OP_POWER:
			case BC_LEX_OP_MULTIPLY:
			case BC_LEX_OP_DIVIDE:
			case BC_LEX_OP_MODULUS:
			case BC_LEX_OP_PLUS:
			case BC_LEX_OP_MINUS:
			case BC_LEX_OP_ASSIGN:
			case BC_LEX_OP_ASSIGN_PLUS:
			case BC_LEX_OP_ASSIGN_MINUS:
			case BC_LEX_OP_ASSIGN_MULTIPLY:
			case BC_LEX_OP_ASSIGN_DIVIDE:
			case BC_LEX_OP_ASSIGN_MODULUS:
			case BC_LEX_OP_ASSIGN_POWER:
			case BC_LEX_OP_REL_EQUAL:
			case BC_LEX_OP_REL_LESS_EQ:
			case BC_LEX_OP_REL_GREATER_EQ:
			case BC_LEX_OP_REL_NOT_EQ:
			case BC_LEX_OP_REL_LESS:
			case BC_LEX_OP_REL_GREATER:
			{
				// TODO: Do this.
				break;
			}

			case BC_LEX_NAME:
			{
				// TODO: Do this properly for function calls, arrays, and vars.

				break;
			}

			case BC_LEX_NUMBER:
			{
				expr.type = BC_EXPR_NUMBER;
				expr.string = parse->token.string;
				status = bc_stack_push(exprs, &expr);

				++num_exprs;

				break;
			}

			default:
			{
				status = BC_STATUS_PARSE_INVALID_TOKEN;
				break;
			}
		}

		if (status) {
			return status;
		}

		status = bc_lex_next(&parse->lex, &parse->token);
	}

	if (status) {
		return status;
	}

	if (num_exprs != 1) {
		return BC_STATUS_PARSE_INVALID_EXPR;
	}

	while (!status && ops.len > 0) {


	}

	return BC_STATUS_SUCCESS;
}

static BcStatus bc_parse_call(BcParse* parse) {

	BcStatus status = bc_lex_next(&parse->lex, &parse->token);
	if (status != BC_STATUS_SUCCESS) {
		return status;
	}

	if (parse->token.type != BC_LEX_NAME) {
		return BC_STATUS_PARSE_INVALID_TOKEN;
	}

	// Get the name of the function.
	char* name = parse->token.string;

	status = bc_lex_next(&parse->lex, &parse->token);
	if (status != BC_STATUS_SUCCESS) {
		return status;
	}

	if (parse->token.type != BC_LEX_LEFT_PAREN) {
		return BC_STATUS_PARSE_INVALID_TOKEN;
	}

	status = bc_parse_params(parse);
	if (status != BC_STATUS_SUCCESS) {
		return status;
	}

	if (parse->token.type != BC_LEX_RIGHT_PAREN) {
		return BC_STATUS_PARSE_INVALID_TOKEN;
	}
}

static BcStatus bc_parse_params(BcParse* parse) {

}

static BcStatus bc_parse_string(BcParse* parse, BcStmtList* list) {

	BcStatus status;
	BcStmt stmt;

	stmt.type = BC_STMT_STRING;
	stmt.data.string = parse->token.string;

	status = bc_program_list_insert(list, &stmt);

	if (status) {
		return status;
	}

	status = bc_lex_next(&parse->lex, &parse->token);

	if (status) {
		return status;
	}

	return bc_parse_semicolonListEnd(parse, list);
}

static BcStatus bc_parse_return(BcParse* parse, BcStmtList* list) {

	BcStatus status;
	BcStmt stmt;
	BcExpr expr;

	if (BC_PARSE_FUNC(parse)) {

		status = bc_lex_next(&parse->lex, &parse->token);

		if (status) {
			return status;
		}

		stmt.type = BC_STMT_RETURN;

		if (parse->token.type == BC_LEX_NEWLINE ||
		    parse->token.type == BC_LEX_SEMICOLON)
		{
			status = bc_stack_init(&stmt.data.expr_stack, sizeof(BcExpr));

			if (status) {
				return status;
			}

			expr.type = BC_EXPR_NUMBER;
			expr.string = NULL;

			status = bc_stack_push(&stmt.data.expr_stack, &expr);

			if (status) {
				return status;
			}

			bc_program_list_insert(list, &stmt);
		}
		else {
			status = bc_parse_expr(parse, &stmt.data.expr_stack);
		}
	}
	else {
		status = BC_STATUS_PARSE_INVALID_TOKEN;
	}

	return status;
}
