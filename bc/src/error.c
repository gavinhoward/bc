#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <bc/bc.h>

static const char* const bc_err_types[] = {

    NULL,

    "bc",
    "bc",
    "bc",

    "bc",
    "bc",

    "runtime",
    "runtime",
    "runtime",
    "runtime",
    "runtime",
    "runtime",

    "lex",
    "lex",
    "lex",
    "lex",

    "parse",
    "parse",
    "parse",
    "parse",
    "parse",
    "parse",
    "parse",
    "parse",

};

static const char* const bc_err_descs[] = {

    NULL,

    "invalid option",
    "memory allocation error",
    "invalid parameter",

    "one or more limits not specified",
    "invalid limit; this is a bug in bc",

    "couldn't open file",
    "file read error",
    "divide by zero",
    "negative square root",
    "mismatched parameters",
    "undefined function",

    "invalid token",
    "string end could not be found",
    "comment end could not be found",
    "end of file",

    "invalid token",
    "invalid expression",
    "invalid print statement",
    "invalid function definition",
    "no auto variable found",
    "quit statement in file",
    "end of file",
    "bug in parser",

};

void bc_error(BcStatus status) {

	if (!status || status == BC_STATUS_PARSE_QUIT) {
		return;
	}

	fprintf(stderr, "%s error: %s\n", bc_err_types[status], bc_err_descs[status]);
}

void bc_error_file(BcStatus status, const char* file, uint32_t line) {

	if (!status || status == BC_STATUS_PARSE_QUIT || !file) {
		return;
	}

	fprintf(stderr, "%s error: %s\n", bc_err_types[status], bc_err_descs[status]);
	fprintf(stderr, "    %s", file);

	if (line) {
		fprintf(stderr, ":%d\n", line);
	}
	else {
		fputc('\n', stderr);
	}
}
