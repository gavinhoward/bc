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

    false,
    false,

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
static BcStatus bc_parse_expr_name(BcParse* parse, BcStack* exprs,
                                   BcExprType* type);
static BcStatus bc_parse_operator(BcStack* exs, BcStack* ops,BcLexTokenType t,
                                  uint32_t* num_exprs);
static BcStatus bc_parse_call(BcParse* parse, BcExpr* expr);
static BcStatus bc_parse_params(BcParse* parse, BcExpr* expr);
static BcStatus bc_parse_read(BcParse* parse, BcStack* exprs);
static BcStatus bc_parse_builtin(BcParse* parse, BcStack* exs, BcExprType type);
static BcStatus bc_parse_scale(BcParse* parse, BcStack* exs, BcExprType* type);
static BcStatus bc_parse_incdec(BcParse* parse, BcStack* exs, BcExprType* prev);
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
		case BC_LEX_KEY_OBASE:
		case BC_LEX_KEY_PRINT:
		{
			status = bc_parse_stmt(parse, list);
			break;
		}

		case BC_LEX_KEY_LIMITS:
		{
			// TODO: Print limits and get next token.
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
	uint32_t num_parens;
	bool paren_first;
	BcExprType prev;
	BcLexTokenType type;

	prev = BC_EXPR_PRINT;

	paren_first = parse->token.type == BC_LEX_LEFT_PAREN;
	num_parens = paren_first ? 1 : 0;

	status = bc_stack_init(exprs, sizeof(BcExpr));

	if (status) {
		return status;
	}

	status = bc_stack_init(&ops, sizeof(BcLexTokenType));

	if (status) {
		return status;
	}

	num_exprs = 0;

	type = parse->token.type;

	while (!status && bc_token_exprs[type]) {

		switch (type) {

			case BC_LEX_OP_INC:
			case BC_LEX_OP_DEC:
			{
				status = bc_parse_incdec(parse, exprs, &prev);
				break;
			}

			case BC_LEX_OP_MINUS:
			{
				// Handle this one even more carefully.
				break;
			}

			case BC_LEX_OP_POWER:
			{
				prev = BC_EXPR_POWER;
				status = bc_parse_operator(exprs, &ops, type, &num_exprs);
				break;
			}

			case BC_LEX_OP_MULTIPLY:
			{
				prev = BC_EXPR_MULTIPLY;
				status = bc_parse_operator(exprs, &ops, type, &num_exprs);
				break;
			}

			case BC_LEX_OP_DIVIDE:
			{
				prev = BC_EXPR_DIVIDE;
				status = bc_parse_operator(exprs, &ops, type, &num_exprs);
				break;
			}

			case BC_LEX_OP_MODULUS:
			{
				prev = BC_EXPR_MODULUS;
				status = bc_parse_operator(exprs, &ops, type, &num_exprs);
				break;
			}

			case BC_LEX_OP_PLUS:
			{
				prev = BC_EXPR_PLUS;
				status = bc_parse_operator(exprs, &ops, type, &num_exprs);
				break;
			}

			case BC_LEX_OP_ASSIGN:
			{
				prev = BC_EXPR_ASSIGN;
				status = bc_parse_operator(exprs, &ops, type, &num_exprs);
				break;
			}

			case BC_LEX_OP_ASSIGN_PLUS:
			{
				prev = BC_EXPR_ASSIGN_PLUS;
				status = bc_parse_operator(exprs, &ops, type, &num_exprs);
				break;
			}

			case BC_LEX_OP_ASSIGN_MINUS:
			{
				prev = BC_EXPR_ASSIGN_MINUS;
				status = bc_parse_operator(exprs, &ops, type, &num_exprs);
				break;
			}

			case BC_LEX_OP_ASSIGN_MULTIPLY:
			{
				prev = BC_EXPR_ASSIGN_MULTIPLY;
				status = bc_parse_operator(exprs, &ops, type, &num_exprs);
				break;
			}

			case BC_LEX_OP_ASSIGN_DIVIDE:
			{
				prev = BC_EXPR_ASSIGN_DIVIDE;
				status = bc_parse_operator(exprs, &ops, type, &num_exprs);
				break;
			}

			case BC_LEX_OP_ASSIGN_MODULUS:
			{
				prev = BC_EXPR_ASSIGN_MODULUS;
				status = bc_parse_operator(exprs, &ops, type, &num_exprs);
				break;
			}

			case BC_LEX_OP_ASSIGN_POWER:
			{
				prev = BC_EXPR_ASSIGN_POWER;
				status = bc_parse_operator(exprs, &ops, type, &num_exprs);
				break;
			}

			case BC_LEX_OP_REL_EQUAL:
			{
				prev = BC_EXPR_REL_EQUAL;
				status = bc_parse_operator(exprs, &ops, type, &num_exprs);
				break;
			}

			case BC_LEX_OP_REL_LESS_EQ:
			{
				prev = BC_EXPR_REL_LESS_EQ;
				status = bc_parse_operator(exprs, &ops, type, &num_exprs);
				break;
			}

			case BC_LEX_OP_REL_GREATER_EQ:
			{
				prev = BC_EXPR_REL_GREATER_EQ;
				status = bc_parse_operator(exprs, &ops, type, &num_exprs);
				break;
			}

			case BC_LEX_OP_REL_NOT_EQ:
			{
				prev = BC_EXPR_REL_NOT_EQ;
				status = bc_parse_operator(exprs, &ops, type, &num_exprs);
				break;
			}

			case BC_LEX_OP_REL_LESS:
			{
				prev = BC_EXPR_REL_LESS;
				status = bc_parse_operator(exprs, &ops, type, &num_exprs);
				break;
			}

			case BC_LEX_OP_REL_GREATER:
			{
				prev = BC_EXPR_REL_GREATER;
				status = bc_parse_operator(exprs, &ops, type, &num_exprs);
				break;
			}

			case BC_LEX_OP_BOOL_NOT:
			{
				// TODO: Handle these specially.
				// We may not have to though...

				prev = BC_EXPR_REL_EQUAL;
				status = bc_parse_operator(exprs, &ops, type, &num_exprs);
				break;
			}

			case BC_LEX_OP_BOOL_OR:
			{
				prev = BC_EXPR_REL_GREATER;
				status = bc_parse_operator(exprs, &ops, type, &num_exprs);
				break;
			}

			case BC_LEX_OP_BOOL_AND:
			{
				prev = BC_EXPR_REL_GREATER;
				status = bc_parse_operator(exprs, &ops, type, &num_exprs);
				break;
			}

			case BC_LEX_LEFT_PAREN:
			{
				// TODO: Handle this.
				break;
			}

			case BC_LEX_RIGHT_PAREN:
			{
				// TODO: Handle this.
				break;
			}

			case BC_LEX_NAME:
			{
				status = bc_parse_expr_name(parse, exprs, &prev);
				++num_exprs;
				break;
			}

			case BC_LEX_NUMBER:
			{
				expr.type = BC_EXPR_NUMBER;
				expr.string = parse->token.string;
				status = bc_stack_push(exprs, &expr);

				++num_exprs;
				prev = BC_EXPR_NUMBER;

				break;
			}

			case BC_LEX_KEY_IBASE:
			{
				expr.type = BC_EXPR_IBASE;
				status = bc_stack_push(exprs, &expr);

				++num_exprs;
				prev = BC_EXPR_IBASE;

				break;
			}

			case BC_LEX_KEY_LENGTH:
			{
				status = bc_parse_builtin(parse, exprs, BC_EXPR_LENGTH);
				++num_exprs;
				prev = BC_EXPR_LENGTH;
				break;
			}

			case BC_LEX_KEY_OBASE:
			{
				expr.type = BC_EXPR_OBASE;
				status = bc_stack_push(exprs, &expr);

				++num_exprs;
				prev = BC_EXPR_OBASE;

				break;
			}

			case BC_LEX_KEY_READ:
			{
				status = bc_parse_read(parse, exprs);
				++num_exprs;
				prev = BC_EXPR_READ;
				break;
			}

			case BC_LEX_KEY_SCALE:
			{
				status = bc_parse_scale(parse, exprs, &prev);
				++num_exprs;
				prev = BC_EXPR_SCALE;
				break;
			}

			case BC_LEX_KEY_SQRT:
			{
				status = bc_parse_builtin(parse, exprs, BC_EXPR_SQRT);
				++num_exprs;
				prev = BC_EXPR_SQRT;
				break;
			}

			default:
			{
				status = BC_STATUS_PARSE_INVALID_TOKEN;
				break;
			}
		}

		type = parse->token.type;
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

static BcStatus bc_parse_expr_name(BcParse* parse, BcStack* exprs,
                                   BcExprType* type)
{
	BcStatus status;
	BcStack stack;
	BcExpr expr;

	status = bc_lex_next(&parse->lex, &parse->token);

	if (status) {
		return status;
	}

	if (parse->token.type == BC_LEX_LEFT_BRACKET) {

		expr.type = BC_EXPR_ARRAY_ELEM;
		*type = BC_EXPR_ARRAY_ELEM;
		expr.string = parse->token.string;

		status = bc_lex_next(&parse->lex, &parse->token);

		if (status) {
			return status;
		}

		status = bc_parse_expr(parse, &stack);

		if (status) {
			return status;
		}

		if (parse->token.type != BC_LEX_RIGHT_BRACKET) {
			return BC_STATUS_PARSE_INVALID_TOKEN;
		}
	}
	else if (parse->token.type == BC_LEX_LEFT_PAREN) {
		*type = BC_EXPR_FUNC_CALL;
		status = bc_parse_call(parse, &expr);
	}
	else {
		expr.type = BC_EXPR_VAR;
		*type = BC_EXPR_VAR;
		expr.string = parse->token.string;
	}

	return bc_stack_push(exprs, &expr);
}

static BcStatus bc_parse_call(BcParse* parse, BcExpr* expr) {

	BcStatus status;

	expr->type = BC_EXPR_FUNC_CALL;
	expr->call.name = parse->token.string;

	status = bc_parse_params(parse, expr);

	if (status) {
		return status;
	}

	if (parse->token.type != BC_LEX_RIGHT_PAREN) {
		return BC_STATUS_PARSE_INVALID_TOKEN;
	}

	return bc_lex_next(&parse->lex, &parse->token);
}

static BcStatus bc_parse_params(BcParse* parse, BcExpr* expr) {

	BcStack exprs;
	BcStatus status;

	status = bc_segarray_init(&expr->call.params, sizeof(BcStack),
	                          bc_stack_free, NULL);

	if (status) {
		return status;
	}

	if (parse->token.type == BC_LEX_RIGHT_PAREN) {
		return BC_STATUS_SUCCESS;
	}

	do {

		status = bc_parse_expr(parse, &exprs);

		if (status) {
			return status;
		}

		status = bc_segarray_add(&expr->call.params, &exprs);

	} while (!status && parse->token.type == BC_LEX_COMMA);

	return status;
}

static BcStatus bc_parse_read(BcParse* parse, BcStack* exprs) {

	BcStatus status;
	BcExpr expr;

	status = bc_lex_next(&parse->lex, &parse->token);

	if (status) {
		return status;
	}

	if (parse->token.type != BC_LEX_LEFT_PAREN) {
		return BC_STATUS_PARSE_INVALID_TOKEN;
	}

	status = bc_lex_next(&parse->lex, &parse->token);

	if (status) {
		return status;
	}

	if (parse->token.type != BC_LEX_LEFT_PAREN) {
		return BC_STATUS_PARSE_INVALID_TOKEN;
	}

	expr.type = BC_EXPR_READ;

	status = bc_stack_push(exprs, &expr);

	if (status) {
		return status;
	}

	return bc_lex_next(&parse->lex, &parse->token);
}

static BcStatus bc_parse_builtin(BcParse* parse, BcStack* exs, BcExprType type)
{
	BcStatus status;
	BcExpr expr;

	expr.type = type;

	status = bc_stack_init(&expr.expr_stack, sizeof(BcExpr));

	if (status) {
		return status;
	}

	status = bc_lex_next(&parse->lex, &parse->token);

	if (status) {
		return status;
	}

	if (parse->token.type != BC_LEX_LEFT_PAREN) {
		return BC_STATUS_PARSE_INVALID_TOKEN;
	}

	status = bc_lex_next(&parse->lex, &parse->token);

	if (status) {
		return status;
	}

	status = bc_parse_expr(parse, &expr.expr_stack);

	if (status) {
		return status;
	}

	if (parse->token.type != BC_LEX_RIGHT_PAREN) {
		return BC_STATUS_PARSE_INVALID_TOKEN;
	}

	status = bc_stack_push(exs, &expr);

	if (status) {
		return status;
	}

	return bc_lex_next(&parse->lex, &parse->token);
}

static BcStatus bc_parse_scale(BcParse* parse, BcStack* exs, BcExprType* type) {

	BcStatus status;
	BcExpr expr;

	status = bc_lex_next(&parse->lex, &parse->token);

	if (status) {
		return status;
	}

	if (parse->token.type != BC_LEX_LEFT_PAREN) {

		expr.type = BC_EXPR_SCALE;
		*type = BC_EXPR_SCALE;

		return bc_stack_push(exs, &expr);
	}

	expr.type = BC_EXPR_SCALE_FUNC;
	*type = BC_EXPR_SCALE_FUNC;

	status = bc_stack_init(&expr.expr_stack, sizeof(BcExpr));

	if (status) {
		return status;
	}

	status = bc_parse_expr(parse, &expr.expr_stack);

	if (status) {
		return status;
	}

	status = bc_lex_next(&parse->lex, &parse->token);

	if (status) {
		return status;
	}

	if (parse->token.type != BC_LEX_RIGHT_PAREN) {
		return BC_STATUS_PARSE_INVALID_TOKEN;
	}

	status = bc_stack_push(exs, &expr);

	if (status) {
		return status;
	}

	return bc_lex_next(&parse->lex, &parse->token);
}

static BcStatus bc_parse_incdec(BcParse* parse, BcStack* exs, BcExprType* prev)
{
	BcStatus status;
	BcLexTokenType type;
	BcExpr expr;
	BcExprType etype;

	etype = *prev;

	if (etype == BC_EXPR_VAR || etype == BC_EXPR_ARRAY_ELEM ||
	    etype == BC_EXPR_SCALE || etype == BC_EXPR_LAST ||
	    etype == BC_EXPR_IBASE || etype == BC_EXPR_OBASE)
	{
		expr.type = parse->token.type == BC_LEX_OP_INC ?
		                BC_EXPR_INC_POST : BC_EXPR_DEC_POST;
		*prev = expr.type;

		status = bc_stack_push(exs, &expr);
	}
	else {

		expr.type = parse->token.type == BC_LEX_OP_INC ?
		                BC_EXPR_INC_PRE : BC_EXPR_DEC_PRE;
		*prev = expr.type;

		status = bc_lex_next(&parse->lex, &parse->token);

		if (status) {
			return status;
		}

		type = parse->token.type;

		if (type != BC_LEX_NAME && type != BC_LEX_KEY_SCALE &&
		    type != BC_LEX_KEY_LAST && type != BC_LEX_KEY_IBASE &&
		    type != BC_LEX_KEY_OBASE)
		{
			return BC_STATUS_PARSE_INVALID_TOKEN;
		}

		status = bc_stack_push(exs, &expr);
	}

	if (status) {
		return status;
	}

	return bc_lex_next(&parse->lex, &parse->token);
}

static BcStatus bc_parse_operator(BcStack* exs, BcStack* ops, BcLexTokenType t,
                                  uint32_t* num_exprs)
{
	BcExpr expr;
	BcStatus status;
	BcLexTokenType top;
	BcLexTokenType* ptr;
	uint8_t lp;
	uint8_t rp;
	bool rleft;

	rp = bc_ops[t].prec;
	rleft = bc_ops[t].left;

	ptr = bc_stack_top(ops);

	if (ptr) {

		top = *ptr;
		lp = bc_ops[top].prec;

		while (top != BC_LEX_LEFT_PAREN && (lp < rp || (lp == rp && rleft))) {

			// We can calculate this by subtracting the position of the
			// first operator in the lex enum and adding the position of
			// the first in the expression enum.
			expr.type = top - BC_LEX_OP_POWER + BC_EXPR_POWER;

			status = bc_stack_push(exs, &expr);

			if (status) {
				return status;
			}

			status = bc_stack_pop(ops);

			if (status) {
				return status;
			}

			ptr = bc_stack_top(ops);

			if (!ptr) {
				break;
			}
		}
	}

	// TODO: Handle num_exprs.

	return bc_stack_push(ops, &t);
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
