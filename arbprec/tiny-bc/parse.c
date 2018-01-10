#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "lex.h"
#include "parse.h"

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
static BcStatus bc_parse_semicolonList(BcParse* parse, BcProgram* program);
static BcStatus bc_parse_semicolonListEnd(BcParse* parse, BcProgram* program);
static BcStatus bc_parse_stmt(BcParse* parse, BcProgram* program);
static BcStatus bc_parse_expr(BcParse* parse, BcProgram* program);
static BcStatus bc_parse_operator(BcLexToken* token, BcExpr* expr);
static BcStatus bc_parse_call(BcParse* parse);
static BcStatus bc_parse_params(BcParse* parse);
static BcStatus bc_parse_string(BcParse* parse, BcProgram* program);

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
			status = bc_parse_stmt(parse, program);
			break;
		}
	}

	return status;
}

void bc_parse_free(BcParse* parse) {

	// Don't do anything if the parameter doesn't exist.
	if (parse) {

		// Free the stacks.
		bc_stack_free(&parse->flag_stack);
		bc_stack_free(&parse->ctx_stack);
	}
}

static BcStatus bc_parse_func(BcParse* parse, BcProgram* program) {

	// TODO: Add to the symbol table.


}

static BcStatus bc_parse_semicolonList(BcParse* parse, BcProgram* program) {

	BcStatus status = BC_STATUS_SUCCESS;

	switch (parse->token.type) {

		case BC_LEX_OP_INC:
		case BC_LEX_OP_DEC:
		case BC_LEX_OP_MINUS:
		case BC_LEX_OP_BOOL_NOT:
		case BC_LEX_NEWLINE:
		case BC_LEX_LEFT_PAREN:
		{
			status = bc_parse_stmt(parse, program);
			break;
		}

		case BC_LEX_SEMICOLON:
		{
			status = bc_parse_semicolonListEnd(parse, program);
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
			status = bc_parse_stmt(parse, program);
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
			status = bc_parse_stmt(parse, program);
			break;
		}

		case BC_LEX_EOF:
		{
			if (parse->ctx_stack.len > 0 || parse->ctx_stack.stack[0]) {
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

static BcStatus bc_parse_semicolonListEnd(BcParse* parse, BcProgram* program) {

	BcStatus status;

	if (parse->token.type == BC_LEX_SEMICOLON) {

		status = bc_lex_next(&parse->lex, &parse->token);

		if (status) {
			return status;
		}

		status = bc_parse_semicolonList(parse, program);
	}
	else if (parse->token.type == BC_LEX_NEWLINE) {
		status = BC_STATUS_SUCCESS;
	}
	else {
		status = BC_STATUS_PARSE_INVALID_TOKEN;
	}

	return status;
}

static BcStatus bc_parse_stmt(BcParse* parse, BcProgram* program) {

	BcStatus status = BC_STATUS_SUCCESS;

	switch (parse->token.type) {

		case BC_LEX_OP_INC:
		case BC_LEX_OP_DEC:
		case BC_LEX_OP_MINUS:
		case BC_LEX_OP_BOOL_NOT:
		{
			status = bc_parse_expr(parse, program);
			break;
		}

		case BC_LEX_NEWLINE:
		{
			// Do nothing.
			break;
		}

		case BC_LEX_LEFT_PAREN:
		{
			status = bc_parse_expr(parse, program);
			break;
		}

		case BC_LEX_STRING:
		{
			status = bc_parse_string(parse, program);
			break;
		}

		case BC_LEX_NAME:
		case BC_LEX_NUMBER:
		{
			status = bc_parse_expr(parse, program);
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
			bc_program_insert(program, &stmt);

			status = bc_lex_next(&parse->lex, &parse->token);

			if (status) {
				return status;
			}

			status = bc_parse_semicolonListEnd(parse, program);

			break;
		}

		case BC_LEX_KEY_IBASE:
		{
			status = bc_parse_expr(parse, program);
			break;
		}

		case BC_LEX_KEY_IF:
		{
			// TODO: If statement.
			break;
		}

		case BC_LEX_KEY_LAST:
		case BC_LEX_KEY_LENGTH:
		case BC_LEX_KEY_LIMITS:
		case BC_LEX_KEY_OBASE:
		{
			status = bc_parse_expr(parse, program);
			break;
		}

		case BC_LEX_KEY_PRINT:
		{
			// TODO: Parse print.
			break;
		}

		case BC_LEX_KEY_READ:
		{
			status = bc_parse_expr(parse, program);
			break;
		}

		case BC_LEX_KEY_RETURN:
		{
			// TODO: Check for in function and parse.
			break;
		}

		case BC_LEX_KEY_SCALE:
		case BC_LEX_KEY_SQRT:
		{
			status = bc_parse_expr(parse, program);
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

static BcStatus bc_parse_expr(BcParse* parse, BcProgram* program) {

	BcStatus status;
	BcStack exprs;
	BcStack ops;
	BcExpr expr;
	BcStmt stmt;
	uint32_t num_exprs;

	status = bc_stack_init(&exprs, sizeof(BcExpr));

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

			case BC_LEX_NUMBER:
			{
				expr.type = BC_EXPR_NUMBER;
				expr.string = parse->token.data.string;
				status = bc_stack_push(&exprs, &expr);

				++num_exprs;

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

	stmt.type = BC_STMT_EXPR;
	stmt.data.expr_stack = exprs;

	return bc_program_insert(program, &stmt);
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
	char* name = parse->token.data.string;

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

static BcStatus bc_parse_string(BcParse* parse, BcProgram* program) {

	BcStatus status;
	BcStmt stmt;

	stmt.type = BC_STMT_STRING;
	stmt.data.string = parse->token.data.string;

	status = bc_program_insert(program, &stmt);

	if (status) {
		return status;
	}

	status = bc_lex_next(&parse->lex, &parse->token);

	if (status) {
		return status;
	}

	return bc_parse_semicolonListEnd(parse, program);
}
