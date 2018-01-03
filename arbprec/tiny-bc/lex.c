#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "lex.h"

static BcLexStatus bc_lex_whitespace(BcLex* lex, BcLexToken* token);
static BcLexStatus bc_lex_string(BcLex* lex, BcLexToken* token);

BcLexStatus bc_lex_init(BcLex* lex, const char* text) {

	// Check for error.
	if (lex == NULL) {
		return BC_LEX_STATUS_INVALID_PARAM;
	}

	// Set the data.
	lex->buffer = text;
	lex->idx = 0;

	return BC_LEX_STATUS_SUCCESS;
}

BcLexStatus bc_lex_next(BcLex* lex, BcLexToken* token) {

	BcLexStatus status = BC_LEX_STATUS_SUCCESS;

	// Get the character.
	char c = lex->buffer[lex->idx];

	// Increment the index.
	++lex->idx;

	char c2;

	// This is the workhorse of the lexer.
	switch (c) {

		case '\0':
			token->type = BC_LEX_EOF;
			break;

		case '\t':
			status = bc_lex_whitespace(lex, token);
			break;

		case '\n':
			token->type = BC_LEX_NEWLINE;
			break;

		case '\v':
		case '\f':
		case '\r':
		case ' ':
			status = bc_lex_whitespace(lex, token);
			break;

		case '!':
			// Get the next character.
			c2 = lex->buffer[lex->idx];

			// The only valid way to have a bang is a NOT_EQ token.
			// If it's not valid, we do not want to increment twice.
			if (c2 == '=') {
				++lex->idx;
				token->type = BC_LEX_OP_REL_NOT_EQ;
			}
			else {
				token->type = BC_LEX_INVALID;
				status = BC_LEX_STATUS_INVALID_TOKEN;
			}

			break;

		case '"':
			status = bc_lex_string(lex, token);
			break;

		case '%':
			// Get the next character.
			c2 = lex->buffer[lex->idx];

			// This character can either be alone or as an assignment.
			// If it's an assignment, we need to increment the index.
			if (c2 == '=') {
				++lex->idx;
				token->type = BC_LEX_OP_ASSIGN_MODULUS;
			}
			else {
				token->type = BC_LEX_OP_MODULUS;
			}

			break;

		default:

			// All other characters are invalid.
			token->type = BC_LEX_INVALID;
			status = BC_LEX_STATUS_INVALID_TOKEN;
			break;
	}

	return status;
}

static BcLexStatus bc_lex_whitespace(BcLex* lex, BcLexToken* token) {

	// Set the token type.
	token->type = BC_LEX_WHITESPACE;

	// Get the character.
	char c = lex->buffer[lex->idx];

	// Eat all whitespace (and non-newline) characters.
	while (isspace(c) && c != '\n') {
		++lex->idx;
		c = lex->buffer[lex->idx];
	}

	return BC_LEX_STATUS_SUCCESS;
}

static BcLexStatus bc_lex_string(BcLex* lex, BcLexToken* token) {

	// TODO: Handle backslashes and other string weirdness.

	// Set the token type.
	token->type = BC_LEX_STRING;

	// Get the starting index and character.
	size_t i = lex->idx;
	char c = lex->buffer[i];

	// Find the end of the string, one way or the other.
	while (c != '"' && c != '\0') {
		c = lex->buffer[++i];
	}

	// If we have reached the end of the buffer, complain.
	if (c == '\0') {
		lex->idx = i;
		return BC_LEX_STATUS_NO_STRING_END;
	}

	// Calculate the length of the string.
	size_t len = i - lex->idx;

	// Allocate the string.
	token->data.string = malloc(len + 1);

	// Check for error.
	if (token->data.string == NULL) {
		return BC_LEX_STATUS_MALLOC_FAIL;
	}

	// Copy the string.
	token->data.string[0] = '\0';
	strncpy(token->data.string, lex->buffer + lex->idx, len);

	// Set the index. We need to go one
	// past because of the closing quote.
	lex->idx = i + 1;

	return BC_LEX_STATUS_SUCCESS;
}
