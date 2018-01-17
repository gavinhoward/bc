#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <bc/bc.h>

static const char* const bc_err_types[] = {

    NULL,

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

};

static const char* const bc_err_descs[] = {

    NULL,

    "invalid option",
    "memory allocation error",
    "invalid parameter",

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
    "end of file",
    "bug in parser",

};

void bc_error(BcStatus status) {

	if (!status) {
		return;
	}

	fprintf(stderr, "%s error: %s\n", bc_err_types[status], bc_err_descs[status]);
}

void bc_error_file(const char* file, uint32_t line, BcStatus status) {

	if (!status || !file) {
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
