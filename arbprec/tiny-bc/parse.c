#include <stdint.h>
#include <stdlib.h>

#include "parse.h"

static BcStatus bc_parse_func(BcParse* parse, BcStmt* stmt);
static BcStatus bc_parse_stmt(BcParse* parse, BcStmt* stmt);
static BcStatus bc_parse_expr(BcParse* parse, BcStmt* stmt);
static BcStatus bc_parse_name(BcParse* parse, BcStmt* stmt);
static BcStatus bc_parse_call(BcParse* parse);
static BcStatus bc_parse_params(BcParse* parse);
static BcStatus bc_parse_expandStack(BcParse* parse);

BcStatus bc_parse_init(BcParse* parse, BcProgram* program) {

	// Check for error.
	if (parse == NULL || program == NULL) {
		return BC_STATUS_INVALID_PARAM;
	}

	// Set the data.
	parse->program = program;

	// Prepare the flag stack.
	BcStatus status = bc_stack_init(&parse->flag_stack, sizeof(uint8_t));

	// Check for error.
	if (status != BC_STATUS_SUCCESS) {
		return status;
	}

	// Prepare the context stack.
	status = bc_stack_init(&parse->ctx_stack, sizeof(BcContext));

	return status;
}

BcStatus bc_parse_text(BcParse* parse, const char* text) {

	// Check for error.
	if (parse == NULL || text == NULL) {
		return BC_STATUS_INVALID_PARAM;
	}

	// Init the lexer.
	return bc_lex_init(&parse->lex, text);
}

BcStatus bc_parse_parse(BcParse* parse) {

	// Check for error.
	if (parse == NULL) {
		return BC_STATUS_INVALID_PARAM;
	}

	BcStmt stmt;

	// Get the next token.
	BcStatus status = bc_lex_next(&parse->lex, &parse->token);

	// Check for error.
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
			// Check for error.
			if (!BC_PARSE_CAN_EXEC(parse)) {
				return BC_STATUS_PARSE_INVALID_TOKEN;
			}

			// Parse a function definition.
			status = bc_parse_func(parse, &stmt);

			break;
		}

		default:
		{
			status = bc_parse_stmt(parse, &stmt);
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

static BcStatus bc_parse_func(BcParse* parse, BcStmt* stmt) {

	// TODO: Add to the symbol table.


}

static BcStatus bc_parse_stmt(BcParse* parse, BcStmt* stmt) {

}

static BcStatus bc_parse_expr(BcParse* parse, BcStmt* stmt) {

}

static BcStatus bc_parse_name(BcParse* parse, BcStmt* stmt) {

	BcStatus status = bc_lex_next(&parse->lex, &parse->token);

	if (status != BC_STATUS_SUCCESS) {
		return status;
	}

	switch (parse->token.type) {

		case BC_LEX_NAME:
		{
			stmt->type = BC_STMT_EXPR_NAME;
			stmt->data.string = parse->token.data.string;
			break;
		}

		case BC_LEX_KEY_IBASE:
		{
			stmt->type = BC_STMT_EXPR_IBASE;
			break;
		}

		case BC_LEX_KEY_LAST:
		{
			stmt->type = BC_STMT_EXPR_LAST;
			break;
		}

		case BC_LEX_KEY_OBASE:
		{
			stmt->type = BC_STMT_EXPR_OBASE;
			break;
		}

		case BC_LEX_KEY_SCALE:
		{
			stmt->type = BC_STMT_EXPR_SCALE;
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
