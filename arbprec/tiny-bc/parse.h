#ifndef BC_PARSE_H
#define BC_PARSE_H

#include <stdint.h>

#include "program.h"
#include "lex.h"

#define BC_PARSE_STACK_START (32)

#define BC_PARSE_FLAG_FUNC (0x01)

#define BC_PARSE_IN_FUNC(parse)  \
	((parse)->stack[(parse)->stack_len - 1] & BC_PARSE_FLAG_FUNC)

#define BC_PARSE_FLAG_LOOP (0x02)

#define BC_PARSE_IN_LOOP(parse)  \
	((parse)->stack[(parse)->stack_len - 1] & BC_PARSE_FLAG_LOOP)

typedef enum BcParseStatus {

	BC_PARSE_STATUS_SUCCESS,

} BcParseStatus;

typedef struct BcParse {

	BcProgram* program;
	BcLex lex;
	BcLexToken token;
	uint32_t stack_cap;
	uint32_t stack_len;
	uint8_t* stack;

} BcParse;

BcStatus bc_parse_init(BcParse* parse, BcProgram* program, const char* text);

BcStatus bc_parse_parse(BcParse* parse);

void bc_parse_free(BcParse* parse);

#endif // BC_PARSE_H
