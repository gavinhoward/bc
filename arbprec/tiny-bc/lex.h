#ifndef BC_LEX_H
#define BC_LEX_H

#include <stdlib.h>

#include "bc.h"

#define BC_LEX_GEN_ENUM(ENUM) ENUM,
#define BC_LEX_GEN_STR(STRING) #STRING,

#define BC_LEX_TOKEN_FOREACH(TOKEN) \
	TOKEN(BC_LEX_OP_INC)  \
	TOKEN(BC_LEX_OP_DEC)  \
	                      \
	TOKEN(BC_LEX_OP_POWER)  \
	                        \
	TOKEN(BC_LEX_OP_MULTIPLY)  \
	TOKEN(BC_LEX_OP_DIVIDE)    \
	TOKEN(BC_LEX_OP_MODULUS)   \
	                           \
	TOKEN(BC_LEX_OP_PLUS)   \
	TOKEN(BC_LEX_OP_MINUS)  \
	                        \
	TOKEN(BC_LEX_OP_ASSIGN)           \
	TOKEN(BC_LEX_OP_ASSIGN_PLUS)      \
	TOKEN(BC_LEX_OP_ASSIGN_MINUS)     \
	TOKEN(BC_LEX_OP_ASSIGN_MULTIPLY)  \
	TOKEN(BC_LEX_OP_ASSIGN_DIVIDE)    \
	TOKEN(BC_LEX_OP_ASSIGN_MODULUS)   \
	TOKEN(BC_LEX_OP_ASSIGN_POWER)     \
	                                  \
	TOKEN(BC_LEX_OP_REL_EQUAL)       \
	TOKEN(BC_LEX_OP_REL_LESS_EQ)     \
	TOKEN(BC_LEX_OP_REL_GREATER_EQ)  \
	TOKEN(BC_LEX_OP_REL_NOT_EQ)      \
	TOKEN(BC_LEX_OP_REL_LESS)        \
	TOKEN(BC_LEX_OP_REL_GREATER)     \
	                                 \
	TOKEN(BC_LEX_OP_BOOL_NOT)  \
	                           \
	TOKEN(BC_LEX_OP_BOOL_OR)   \
	TOKEN(BC_LEX_OP_BOOL_AND)  \
	                           \
	TOKEN(BC_LEX_NEWLINE)  \
	                       \
	TOKEN(BC_LEX_WHITESPACE)  \
	                          \
	TOKEN(BC_LEX_LEFT_PAREN)   \
	TOKEN(BC_LEX_RIGHT_PAREN)  \
	                           \
	TOKEN(BC_LEX_LEFT_BRACKET)   \
	TOKEN(BC_LEX_RIGHT_BRACKET)  \
	                             \
	TOKEN(BC_LEX_LEFT_BRACE)   \
	TOKEN(BC_LEX_RIGHT_BRACE)  \
	                           \
	TOKEN(BC_LEX_COMMA)      \
	TOKEN(BC_LEX_SEMICOLON)  \
	                         \
	TOKEN(BC_LEX_STRING)  \
	TOKEN(BC_LEX_NAME)    \
	TOKEN(BC_LEX_NUMBER)  \
	                      \
	TOKEN(BC_LEX_KEY_AUTO)      \
	TOKEN(BC_LEX_KEY_BREAK)     \
	TOKEN(BC_LEX_KEY_CONTINUE)  \
	TOKEN(BC_LEX_KEY_DEFINE)    \
	TOKEN(BC_LEX_KEY_ELSE)      \
	TOKEN(BC_LEX_KEY_FOR)       \
	TOKEN(BC_LEX_KEY_HALT)      \
	TOKEN(BC_LEX_KEY_IBASE)     \
	TOKEN(BC_LEX_KEY_IF)        \
	TOKEN(BC_LEX_KEY_LAST)      \
	TOKEN(BC_LEX_KEY_LENGTH)    \
	TOKEN(BC_LEX_KEY_LIMITS)    \
	TOKEN(BC_LEX_KEY_OBASE)     \
	TOKEN(BC_LEX_KEY_PRINT)     \
	TOKEN(BC_LEX_KEY_QUIT)      \
	TOKEN(BC_LEX_KEY_READ)      \
	TOKEN(BC_LEX_KEY_RETURN)    \
	TOKEN(BC_LEX_KEY_SCALE)     \
	TOKEN(BC_LEX_KEY_SQRT)      \
	TOKEN(BC_LEX_KEY_WHILE)     \
	                            \
	TOKEN(BC_LEX_EOF)           \
	                            \
	TOKEN(BC_LEX_INVALID)       \

typedef enum BcLexTokenType {
	BC_LEX_TOKEN_FOREACH(BC_LEX_GEN_ENUM)
} BcLexTokenType;

typedef union BcLexData {

	char* string;

} BcLexData;

typedef struct BcLexToken {

	BcLexTokenType type;
	BcLexData data;

} BcLexToken;

typedef struct BcLex {

	const char* buffer;
	size_t idx;

} BcLex;

BcStatus bc_lex_init(BcLex* lex, const char* text);

BcStatus bc_lex_next(BcLex* lex, BcLexToken* token);

BcStatus bc_lex_printToken(BcLexToken* token);

#endif // BC_LEX_H
