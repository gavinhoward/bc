#include <stdint.h>
#include <stdlib.h>

#include "parse.h"

static BcStatus bc_parse_expandStack(BcParse* parse);

BcStatus bc_parse_init(BcParse* parse, BcProgram* program, const char* text) {

	// Check for error.
	if (parse == NULL || program == NULL || text == NULL) {
		return BC_STATUS_INVALID_PARAM;
	}

	// Set the data.
	bc_lex_init(&parse->lex, text);
	parse->program = program;

	// Malloc a stack.
	parse->stack = malloc(sizeof(uint8_t) * BC_PARSE_STACK_START);

	// Check for error.
	if (parse->stack == NULL) {
		return BC_STATUS_MALLOC_FAIL;
	}

	// Set the cap and length.
	parse->stack_cap = BC_PARSE_STACK_START;
	parse->stack_len = 0;

	return BC_STATUS_SUCCESS;
}

BcStatus bc_parse_parse(BcParse* parse) {

	// TODO: Finish this function.

	// Check for error.
	if (parse == NULL) {
		return BC_STATUS_INVALID_PARAM;
	}

	BcStatus status;

	// Get the next token.
	status = bc_lex_next(&parse->lex, &parse->token);

	while (status == BC_STATUS_SUCCESS) {

		switch (parse->token.type) {

			case BC_LEX_OP_INC:
			case BC_LEX_OP_DEC:
				break;

			case BC_LEX_OP_MINUS:
				break;

			case BC_LEX_OP_BOOL_NOT:
				break;

			case BC_LEX_NEWLINE:
				break;

			case BC_LEX_LEFT_PAREN:
				break;

			case BC_LEX_SEMICOLON:
				break;

			case BC_LEX_STRING:
				break;

			case BC_LEX_NAME:
				break;

			case BC_LEX_NUMBER:
				break;

			case BC_LEX_KEY_BREAK:
				break;

			case BC_LEX_KEY_CONTINUE:
				break;

			case BC_LEX_KEY_DEFINE:
				break;

			case BC_LEX_KEY_FOR:
				break;

			case BC_LEX_KEY_HALT:
				break;

			case BC_LEX_KEY_IBASE:
				break;

			case BC_LEX_KEY_IF:
				break;

			case BC_LEX_KEY_LAST:
				break;

			case BC_LEX_KEY_LENGTH:
				break;

			case BC_LEX_KEY_LIMITS:
				break;

			case BC_LEX_KEY_OBASE:
				break;

			case BC_LEX_KEY_PRINT:
				break;

			case BC_LEX_KEY_QUIT:
				exit(0);
				break;

			case BC_LEX_KEY_READ:
				break;

			case BC_LEX_KEY_RETURN:
				break;

			case BC_LEX_KEY_SCALE:
				break;

			case BC_LEX_KEY_SQRT:
				break;

			case BC_LEX_KEY_WHILE:
				break;

			case BC_LEX_EOF:
				return status;

			default:
				status = BC_STATUS_PARSE_INVALID_TOKEN;
				break;

		}

		// Break on error.
		if (status != BC_STATUS_SUCCESS) {
			break;
		}

		// Get the next token.
		status = bc_lex_next(&parse->lex, &parse->token);
	}

	return status;
}

void bc_parse_free(BcParse* parse) {

	// Don't do anything if the parameter doesn't exist.
	if (parse) {

		// Set all this to NULL.
		parse->stack_cap = 0;
		parse->stack_len = 0;
		parse->program = NULL;

		// Free the stack.
		if (parse->stack) {
			free(parse->stack);
			parse->stack = NULL;
		}
	}
}

static BcStatus bc_parse_expandStack(BcParse* parse) {

	// Calculate the size of the new allocation.
	size_t alloc = parse->stack_cap + BC_PARSE_STACK_START;

	// Reallocate.
	uint8_t* ptr = realloc(parse->stack, alloc);

	// Check for error.
	if (ptr == NULL) {
		return BC_STATUS_MALLOC_FAIL;
	}

	// Set the fields.
	parse->stack = ptr;
	parse->stack_cap = alloc;

	return BC_STATUS_SUCCESS;
}
