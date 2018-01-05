#ifndef BC_PARSE_H
#define BC_PARSE_H

#include <stdint.h>

#include "program.h"
#include "lex.h"

#define BC_PARSE_STACK_START (32)

#define BC_PARSE_FLAG_FUNC_INNER (0x01)

#define BC_PARSE_FUNC_INNER(parse)  \
	((parse)->stack[(parse)->stack_len - 1] & BC_PARSE_FLAG_FUNC_INNER)

#define BC_PARSE_FLAG_FUNC (0x02)

#define BC_PARSE_FUNC(parse)  \
	((parse)->stack[(parse)->stack_len - 1] & BC_PARSE_FLAG_FUNC)

#define BC_PARSE_FLAG_HEADER (0x04)

#define BC_PARSE_HEADER(parse)  \
	((parse)->stack[(parse)->stack_len - 1] & BC_PARSE_FLAG_HEADER)

#define BC_PARSE_FLAG_LINE_END (0x08)

#define BC_PARSE_LINE_END(parse)  \
	((parse)->stack[(parse)->stack_len - 1] & BC_PARSE_FLAG_LINE_END)

#define BC_PARSE_FLAG_LOOP_INNER (0x10)

#define BC_PARSE_LOOP_INNER(parse)  \
	((parse)->stack[(parse)->stack_len - 1] & BC_PARSE_FLAG_LOOP_INNER)

#define BC_PARSE_CAN_EXEC(parse)  \
	(!((parse)->stack[(parse)->stack_len - 1] & (BC_PARSE_FLAG_FUNC_INNER |  \
	                                             BC_PARSE_FLAG_FUNC |        \
	                                             BC_PARSE_FLAG_HEADER |      \
	                                             BC_PARSE_FLAG_LOOP_INNER)))

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

	BcStmtList* inner;

} BcParse;

BcStatus bc_parse_init(BcParse* parse, BcProgram* program);
BcStatus bc_parse_text(BcParse* parse, const char* text);

BcStatus bc_parse_parse(BcParse* parse);

void bc_parse_free(BcParse* parse);

#endif // BC_PARSE_H
