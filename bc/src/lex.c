#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bc.h"
#include "lex.h"

static const char* const token_type_strs[] = {
    BC_LEX_TOKEN_FOREACH(BC_LEX_GEN_STR)
};

static const char* const keywords[] = {

    "auto",
    "break",
    "continue",
    "define",
    "else",
    "for",
    "halt",
    "ibase",
    "if",
    "last",
    "length",
    "limits",
    "obase",
    "print",
    "quit",
    "read",
    "return",
    "scale",
    "sqrt",
    "while",

};

static const uint32_t keyword_lens[] = {

    4, // auto
    5, // break
    8, // continue
    6, // define
    4, // else
    3, // for
    4, // halt
    5, // ibase
    2, // if
    4, // last
    6, // length
    6, // limits
    5, // obase
    5, // print
    4, // quit
    4, // read
    6, // return
    5, // scale
    4, // sqrt
    5, // while

};

static BcStatus bc_lex_token(BcLex* lex, BcLexToken* token);
static BcStatus bc_lex_whitespace(BcLex* lex, BcLexToken* token);
static BcStatus bc_lex_string(BcLex* lex, BcLexToken* token);
static BcStatus bc_lex_comment(BcLex* lex, BcLexToken* token);
static BcStatus bc_lex_number(BcLex* lex, BcLexToken* token, char start);
static BcStatus bc_lex_name(BcLex* lex, BcLexToken* token);

BcStatus bc_lex_printToken(BcLexToken* token) {

	// Print the type.
	printf("<%s", token_type_strs[token->type]);

	// Figure out if more needs to be done.
	switch (token->type) {

		case BC_LEX_STRING:
		case BC_LEX_NAME:
		case BC_LEX_NUMBER:
			printf(":%s", token->string);
			break;

		default:
			break;
	}

	// Print the end.
	putchar('>');
	putchar('\n');

	return BC_STATUS_SUCCESS;
}

BcStatus bc_lex_init(BcLex* lex) {

	if (lex == NULL ) {
		return BC_STATUS_INVALID_PARAM;
	}

	lex->line = 1;
	lex->newline = false;

	return BC_STATUS_SUCCESS;
}

BcStatus bc_lex_text(BcLex* lex, const char* text) {

	if (lex == NULL || text == NULL) {
		return BC_STATUS_INVALID_PARAM;
	}

	lex->buffer = text;
	lex->idx = 0;
	lex->len = strlen(text);

	return BC_STATUS_SUCCESS;
}

BcStatus bc_lex_next(BcLex* lex, BcLexToken* token) {

	BcStatus status;

	if (lex == NULL || token == NULL) {
		return BC_STATUS_INVALID_PARAM;
	}

	if (lex->idx == lex->len) {
		token->type = BC_LEX_EOF;
		return BC_STATUS_LEX_EOF;
	}

	if (lex->newline) {
		++lex->line;
		lex->newline = false;
	}

	// Loop until failure or we don't have whitespace. This
	// is so the parser doesn't get inundated with whitespace.
	do {
		status = bc_lex_token(lex, token);
	} while (!status && token->type == BC_LEX_WHITESPACE);

	return status;
}

static BcStatus bc_lex_token(BcLex* lex, BcLexToken* token) {

	// We want to make sure this is cleared.
	BcStatus status = BC_STATUS_SUCCESS;

	// Get the character.
	char c = lex->buffer[lex->idx];

	// Increment the index.
	++lex->idx;

	char c2;

	// This is the workhorse of the lexer.
	switch (c) {

		case '\0':
		{
			token->type = BC_LEX_EOF;
			break;
		}

		case '\t':
		{
			status = bc_lex_whitespace(lex, token);
			break;
		}

		case '\n':
		{
			lex->newline = true;
			token->type = BC_LEX_NEWLINE;
			break;
		}

		case '\v':
		case '\f':
		case '\r':
		case ' ':
		{
			status = bc_lex_whitespace(lex, token);
			break;
		}

		case '!':
		{
			// Get the next character.
			c2 = lex->buffer[lex->idx];

			// The only valid way to have a bang is a NOT_EQ token.
			// If it's not valid, we do not want to increment twice.
			if (c2 == '=') {
				++lex->idx;
				token->type = BC_LEX_OP_REL_NOT_EQ;
			}
			else {
				token->type = BC_LEX_OP_BOOL_NOT;
			}

			break;
		}

		case '"':
		{
			status = bc_lex_string(lex, token);
			break;
		}

		case '%':
		{
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
		}

		case '&':
		{
			// Get the next character.
			c2 = lex->buffer[lex->idx];

			// This character can either be alone or as an assignment.
			// If it's an assignment, we need to increment the index.
			if (c2 == '&') {
				++lex->idx;
				token->type = BC_LEX_OP_BOOL_AND;
			}
			else {
				token->type = BC_LEX_INVALID;
				status = BC_STATUS_LEX_INVALID_TOKEN;
			}

			break;
		}

		case '(':
		{
			token->type = BC_LEX_LEFT_PAREN;
			break;
		}

		case ')':
		{
			token->type = BC_LEX_RIGHT_PAREN;
			break;
		}

		case '*':
		{
			// Get the next character.
			c2 = lex->buffer[lex->idx];

			// This character can either be alone or as an assignment.
			// If it's an assignment, we need to increment the index.
			if (c2 == '=') {
				++lex->idx;
				token->type = BC_LEX_OP_ASSIGN_MULTIPLY;
			}
			else {
				token->type = BC_LEX_OP_MULTIPLY;
			}

			break;
		}

		case '+':
		{
			// Get the next character.
			c2 = lex->buffer[lex->idx];

			// This character can either be alone or as an assignment.
			// If it's an assignment, we need to increment the index.
			if (c2 == '=') {
				++lex->idx;
				token->type = BC_LEX_OP_ASSIGN_PLUS;
			}
			else if (c2 == '+') {
				++lex->idx;
				token->type = BC_LEX_OP_INC;
			}
			else {
				token->type = BC_LEX_OP_PLUS;
			}

			break;
		}

		case ',':
		{
			token->type = BC_LEX_COMMA;
			break;
		}

		case '-':
		{
			// Get the next character.
			c2 = lex->buffer[lex->idx];

			// This character can either be alone or as an assignment.
			// If it's an assignment, we need to increment the index.
			// We also need to handle numbers.
			if (c2 == '=') {
				++lex->idx;
				token->type = BC_LEX_OP_ASSIGN_MINUS;
			}
			else if (c2 == '-') {
				++lex->idx;
				token->type = BC_LEX_OP_DEC;
			}
			else {
				token->type = BC_LEX_OP_MINUS;
			}

			break;
		}

		case '.':
		{
			status = bc_lex_number(lex, token, c);
			break;
		}

		case '/':
		{
			// Get the next character.
			c2 = lex->buffer[lex->idx];

			// This character can either be alone or as an assignment.
			// If it's an assignment, we need to increment the index.
			// We also need to handle comments.
			if (c2 == '=') {
				++lex->idx;
				token->type = BC_LEX_OP_ASSIGN_DIVIDE;
			}
			else if (c2 == '*') {
				status = bc_lex_comment(lex, token);
			}
			else {
				token->type = BC_LEX_OP_DIVIDE;
			}

			break;
		}

		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		{
			status = bc_lex_number(lex, token, c);
			break;
		}

		case ';':
		{
			token->type = BC_LEX_SEMICOLON;
			break;
		}

		case '<':
		{
			// Get the next character.
			c2 = lex->buffer[lex->idx];

			// This character can either be alone or with an equals.
			// If with an equals, we need to increment the index.
			if (c2 == '=') {
				++lex->idx;
				token->type = BC_LEX_OP_REL_LESS_EQ;
			}
			else {
				token->type = BC_LEX_OP_REL_LESS;
			}

			break;
		}

		case '=':
		{
			// Get the next character.
			c2 = lex->buffer[lex->idx];

			// This character can either be alone or with another equals.
			// If with another equals, we need to increment the index.
			if (c2 == '=') {
				++lex->idx;
				token->type = BC_LEX_OP_REL_EQUAL;
			}
			else {
				token->type = BC_LEX_OP_ASSIGN;
			}

			break;
		}

		case '>':
		{
			// Get the next character.
			c2 = lex->buffer[lex->idx];

			// This character can either be alone or with an equals.
			// If with an equals, we need to increment the index.
			if (c2 == '=') {
				++lex->idx;
				token->type = BC_LEX_OP_REL_GREATER_EQ;
			}
			else {
				token->type = BC_LEX_OP_REL_GREATER;
			}

			break;
		}

		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
		{
			status = bc_lex_number(lex, token, c);
			break;
		}

		case '[':
		{
			token->type = BC_LEX_LEFT_BRACKET;
			break;
		}

		case '\\':
		{
			status = bc_lex_whitespace(lex, token);
			break;
		}

		case ']':
		{
			token->type = BC_LEX_RIGHT_BRACKET;
			break;
		}

		case '^':
		{
			// Get the next character.
			c2 = lex->buffer[lex->idx];

			// This character can either be alone or as an assignment.
			// If it's an assignment, we need to increment the index.
			if (c2 == '=') {
				++lex->idx;
				token->type = BC_LEX_OP_ASSIGN_POWER;
			}
			else {
				token->type = BC_LEX_OP_POWER;
			}

			break;
		}

		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
		case 'g':
		case 'h':
		case 'i':
		case 'j':
		case 'k':
		case 'l':
		case 'm':
		case 'n':
		case 'o':
		case 'p':
		case 'q':
		case 'r':
		case 's':
		case 't':
		case 'u':
		case 'v':
		case 'w':
		case 'x':
		case 'y':
		case 'z':
		{
			status = bc_lex_name(lex, token);
			break;
		}

		case '{':
		{
			token->type = BC_LEX_LEFT_BRACE;
			break;
		}

		case '|':
		{
			// Get the next character.
			c2 = lex->buffer[lex->idx];

			// This character can either be alone or as an assignment.
			// If it's an assignment, we need to increment the index.
			if (c2 == '|') {
				++lex->idx;
				token->type = BC_LEX_OP_BOOL_OR;
			}
			else {
				token->type = BC_LEX_INVALID;
				status = BC_STATUS_LEX_INVALID_TOKEN;
			}

			break;
		}

		case '}':
		{
			token->type = BC_LEX_RIGHT_BRACE;
			break;
		}

		default:
		{
			// All other characters are invalid.
			token->type = BC_LEX_INVALID;
			status = BC_STATUS_LEX_INVALID_TOKEN;
			break;
		}
	}

	return status;
}

static BcStatus bc_lex_whitespace(BcLex* lex, BcLexToken* token) {

	// Set the token type.
	token->type = BC_LEX_WHITESPACE;

	// Get the character.
	char c = lex->buffer[lex->idx];

	// Eat all whitespace (and non-newline) characters.
	while ((isspace(c) && c != '\n') || c == '\\') {
		++lex->idx;
		c = lex->buffer[lex->idx];
	}

	return BC_STATUS_SUCCESS;
}

static BcStatus bc_lex_string(BcLex* lex, BcLexToken* token) {

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
		return BC_STATUS_LEX_NO_STRING_END;
	}

	// Calculate the length of the string.
	size_t len = i - lex->idx;

	// Figure out the number of backslash newlines in a string.
	size_t backslashes = 0;
	for (size_t j = lex->idx; j < i; ++j) {
		c = lex->buffer[j];
		backslashes += c == '\\' && lex->buffer[j + 1] == '\n' ? 1 : 0;
	}

	// Allocate the string.
	token->string = malloc(len - backslashes + 1);

	// Check for error.
	if (token->string == NULL) {
		return BC_STATUS_MALLOC_FAIL;
	}

	// The copy start and the number of backslash
	// hits. These are for the upcoming loop.
	const char* start = lex->buffer + lex->idx;
	size_t hits = 0;

	// Copy the string.
	for (size_t j = 0; j < len; ++j) {

		// Get the character.
		char c = start[j];

		// If we have hit a backslash, skip it.
		if (hits < backslashes && c == '\\' && start[j + 1] == '\n') {
			++hits;
			continue;
		}

		// Copy the character.
		token->string[j - hits] = c;
	}

	// Make sure to set the null character.
	token->string[len] = '\0';

	// Set the index. We need to go one
	// past because of the closing quote.
	lex->idx = i + 1;

	return BC_STATUS_SUCCESS;
}

static BcStatus bc_lex_comment(BcLex* lex, BcLexToken* token) {

	// Set the token type.
	token->type = BC_LEX_WHITESPACE;

	// Increment the index.
	++lex->idx;

	// Get the starting index and character.
	size_t i = lex->idx;
	const char* buffer = lex->buffer;
	char c = buffer[i];

	// The end condition.
	int end = 0;

	// Loop until we have found the end.
	while (!end) {

		// Find the end of the string, one way or the other.
		while (c != '*' && c != '\0') {
			c = buffer[++i];
		}

		// If we've reached the end of the string,
		// but not the comment, complain.
		if (c == '\0' || buffer[i + 1] == '\0') {
			lex->idx = i;
			return BC_STATUS_LEX_NO_COMMENT_END;
		}

		// If we've reached the end, set the end.
		end = buffer[i + 1] == '/';
		i += end ? 0 : 1;
	}

	// Set the index. Plus 2 is to get past the comment end.
	lex->idx = i + 2;

	return BC_STATUS_SUCCESS;
}

static BcStatus bc_lex_number(BcLex* lex, BcLexToken* token, char start) {

	// Set the token type.
	token->type = BC_LEX_NUMBER;

	// Whether or not we already have passed a decimal point.
	int point = start == '.';

	// Get a pointer to the place in the buffer.
	const char* buffer = lex->buffer + lex->idx;

	// Cache these for the upcoming loop.
	size_t backslashes = 0;
	size_t i = 0;
	char c = buffer[i];

	// Find the end of the number.
	while (c && (isdigit(c) || (c >= 'A' && c <= 'F') || (c == '.' && !point) ||
	             (c == '\\' && buffer[i + 1] == '\n')))
	{
		// If we ran into a backslash, handle it.
		if (c == '\\') {
			++i;
			backslashes += 1;
		}

		// Increment and get the character.
		c = buffer[++i];
	}

	// Calculate the length of the string.
	size_t len = i + 1;

	// Allocate the string.
	token->string = malloc(len - backslashes + 1);

	// Check for error.
	if (token->string == NULL) {
		return BC_STATUS_MALLOC_FAIL;
	}

	// Set the starting character.
	token->string[0] = start;

	// The copy start and the number of backslash
	// hits. These are for the upcoming loop.
	const char* buf = buffer - 1;
	size_t hits = 0;

	// Copy the string.
	for (size_t j = 1; j < len; ++j) {

		// Get the character.
		char c = buf[j];

		// If we have hit a backslash, skip it.
		// We don't have to check for a newline
		// because it's guaranteed.
		if (hits < backslashes && c == '\\') {
			++hits;
			++j;
			continue;
		}

		// Copy the character.
		token->string[j - (hits * 2)] = c;
	}

	// Make sure to set the null character.
	token->string[len] = '\0';

	// Set the index. We need to go one
	// past because of the closing quote.
	lex->idx += i;

	return BC_STATUS_SUCCESS;
}

static BcStatus bc_lex_name(BcLex* lex, BcLexToken* token) {

	// Get a pointer to the place in the buffer. We subtract
	// one because the index is already incremented.
	const char* buffer = lex->buffer + lex->idx - 1;

	// Loop through the keywords.
	for (uint32_t i = 0; i < sizeof(keywords) / sizeof(char*); ++i) {

		// If a keyword matches, set it, increment, and return.
		if (!strncmp(buffer, keywords[i], keyword_lens[i])) {

			// We just need to add the starting
			// index of keyword token types.
			token->type = BC_LEX_KEY_AUTO + i;

			// We need to minus one because the
			// index has already been incremented.
			lex->idx += keyword_lens[i] - 1;

			return BC_STATUS_SUCCESS;
		}
	}

	// Set the type.
	token->type = BC_LEX_NAME;

	// These are for the next loop.
	size_t i = 0;
	char c = buffer[i];

	// Find the end of the name.
	while (islower(c) || isdigit(c) || c == '_') {
		++i;
		c = buffer[i];
	}

	// Malloc the name.
	token->string = malloc(i + 1);

	// Check for error.
	if (token->string == NULL) {
		return BC_STATUS_MALLOC_FAIL;
	}

	// Copy the string.
	strncpy(token->string, buffer, i);
	token->string[i] = '\0';

	// Increment the index. It is minus one
	// because it has already been incremented.
	lex->idx += i - 1;

	return BC_STATUS_SUCCESS;
}
