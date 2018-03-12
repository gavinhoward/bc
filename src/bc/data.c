/*
 * *****************************************************************************
 *
 * Copyright 2018 Gavin D. Howard
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * *****************************************************************************
 *
 * Constant data for bc.
 *
 */

#include <instructions.h>
#include <lex.h>
#include <parse.h>

const char *bc_version = "0.1";

const char *bc_copyright =
  "bc copyright (c) 2018 Gavin D. Howard and contributors\n"
  "Report bugs at: https://github.com/gavinhoward/bc";

const char *bc_warranty_short =
  "This is free software with ABSOLUTELY NO WARRANTY.";

const char *bc_version_fmt = "bc %s\n%s\n\n%s\n\n";

const char *bc_err_types[] = {

  NULL,

  "bc",
  "bc",

  "bc",

  "bc",

  "bc",
  "bc",

  "vector",

  "ordered vector",
  "ordered vector",

  "Lex",
  "Lex",
  "Lex",
  "Lex",

  "Parse",
  "Parse",
  "Parse",
  "Parse",
  "Parse",
  "Parse",
  "Parse",
  "Parse",
  "Parse",
  "Parse",
  "Parse",
  "Parse",

  "Math",
  "Math",
  "Math",
  "Math",
  "Math",
  "Math",
  "Math",

  "Runtime",
  "Runtime",
  "Runtime",
  "Runtime",
  "Runtime",
  "Runtime",
  "Runtime",
  "Runtime",
  "Runtime",
  "Runtime",
  "Runtime",
  "Runtime",
  "Runtime",
  "Runtime",
  "Runtime",
  "Runtime",
  "Runtime",
  "Runtime",
  "Runtime",
  "Runtime",
  "Runtime",
  "Runtime",
  "Runtime",
  "Runtime",
  "Runtime",
  "Runtime",

  "POSIX",
  "POSIX",
  "POSIX",
  "POSIX",
  "POSIX",
  "POSIX",
  "POSIX",
  "POSIX",
  "POSIX",
  "POSIX",
  "POSIX",
  "POSIX",

};

const char *bc_err_descs[] = {

  NULL,

  "memory allocation error",
  "I/O error",

  "invalid parameter",

  "invalid option",

  "one or more limits not specified",
  "invalid limit; this is a bug in bc",

  "index is out of bounds for the vector and error was not caught; "
    "this is probably a bug in bc",

  "index is out of bounds for the ordered vector and error was not caught; "
    "this is probably a bug in bc",
  "item already exists in ordered vector and error was not caught; "
    "this is probably a bug in bc",

  "invalid token",
  "string end could not be found",
  "comment end could not be found",
  "end of file",

  "invalid token",
  "invalid expression",
  "invalid print statement",
  "invalid function definition",
  "invalid assignment: must assign to scale, "
    "ibase, obase, last, a variable, or an array element",
  "no auto variable found",
  "limits statement in file not handled correctly; "
    "this is most likely a bug in bc",
  "quit statement in file not exited correctly; "
    "this is most likely a bug in bc",
  "number of functions does not match the number of entries "
    "in the function map; this is most likely a bug in bc",
  "function parameter or auto var has the same name as another",
  "end of file",
  "bug in parser",

  "negative number",
  "non integer number",
  "overflow",
  "divide by zero",
  "negative square root",
  "invalid number string",
  "cannot truncate more places than exist after the decimal point",

  "couldn't open file",
  "mismatched parameters",
  "undefined function",
  "undefined variable",
  "undefined array",
  "file is not executable",
  "could not install signal handler",
  "invalid value for scale; must be an integer in the range [0, BC_SCALE_MAX]",
  "invalid value for ibase; must be an integer in the range [2, 16]",
  "invalid value for obase; must be an integer in the range [2, BC_BASE_MAX]",
  "invalid statement; this is most likely a bug in bc",
  "invalid expression; this is most likely a bug in bc",
  "invalid string",
  "string too long: length must be in the range [0, BC_STRING_MAX]",
  "invalid name/identifier",
  "invalid array length; must be an integer in the range [1, BC_DIM_MAX]",
  "invalid read() expression",
  "read() call inside of a read() call",
  "print error",
  "invalid constant",
  "invalid lvalue; cannot assign to constants or intermediate values",
  "cannot return from function; no function to return from",
  "invalid label; this is probably a bug in bc",
  "variable is wrong type",
  "invalid stack; this is probably a bug in bc",
  "bc was not halted correctly; this is a bug in bc",

  "POSIX only allows one character names; the following is invalid:",
  "POSIX does not allow '#' script comments",
  "POSIX does not allow the following keyword:",
  "POSIX does not allow a period ('.') as a shortcut for the last result",
  "POSIX requires parentheses around return expressions",
  "POSIX does not allow boolean operators; the following is invalid:",
  "POSIX does not allow comparison operators outside if or loops",
  "POSIX does not allow more than one comparison operator per condition",
  "POSIX does not allow an empty init expression in a for loop",
  "POSIX does not allow an empty condition expression in a for loop",
  "POSIX does not allow an empty update expression in a for loop",
  "POSIX requires the left brace be on the same line as the function header",

};

const char *bc_lang_func_main = "(main)";
const char *bc_lang_func_read = "(read)";

const char *bc_lex_token_type_strs[] = {
  BC_LEX_TOKEN_FOREACH(BC_LEX_GEN_STR)
};

const BcLexKeyword bc_lex_keywords[20] = {
  KW_TABLE_ENTRY("auto", 4, true),
  KW_TABLE_ENTRY("break", 5, true),
  KW_TABLE_ENTRY("continue", 8, false),
  KW_TABLE_ENTRY("define", 6, true),
  KW_TABLE_ENTRY("else", 4, false),
  KW_TABLE_ENTRY("for", 3, true),
  KW_TABLE_ENTRY("halt", 4, false),
  KW_TABLE_ENTRY("ibase", 5, true),
  KW_TABLE_ENTRY("if", 2, true),
  KW_TABLE_ENTRY("last", 4, false),
  KW_TABLE_ENTRY("length", 6, true),
  KW_TABLE_ENTRY("limits", 6, false),
  KW_TABLE_ENTRY("obase", 5, true),
  KW_TABLE_ENTRY("print", 5, false),
  KW_TABLE_ENTRY("quit", 4, true),
  KW_TABLE_ENTRY("read", 4, false),
  KW_TABLE_ENTRY("return", 6, true),
  KW_TABLE_ENTRY("scale", 5, true),
  KW_TABLE_ENTRY("sqrt", 4, true),
  KW_TABLE_ENTRY("while", 5, true),
};

const char bc_num_hex_digits[] = "0123456789ABCDEF";

// This is an array that corresponds to token types. An entry is
// true if the token is valid in an expression, false otherwise.
const bool bc_parse_token_exprs[] = {

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
const BcOp bc_parse_ops[] = {

  { 0, false },
  { 0, false },

  { 1, false },

  { 2, false },

  { 3, true },
  { 3, true },
  { 3, true },

  { 4, true },
  { 4, true },

  { 6, true },
  { 6, true },
  { 6, true },
  { 6, true },
  { 6, true },
  { 6, true },

  { 7, false },

  { 8, true },
  { 8, true },

  { 5, false },
  { 5, false },
  { 5, false },
  { 5, false },
  { 5, false },
  { 5, false },
  { 5, false },

};

const uint8_t bc_parse_insts[] = {

  BC_INST_OP_NEGATE,

  BC_INST_OP_POWER,

  BC_INST_OP_MULTIPLY,
  BC_INST_OP_DIVIDE,
  BC_INST_OP_MODULUS,

  BC_INST_OP_PLUS,
  BC_INST_OP_MINUS,

  BC_INST_OP_REL_EQUAL,
  BC_INST_OP_REL_LESS_EQ,
  BC_INST_OP_REL_GREATER_EQ,
  BC_INST_OP_REL_NOT_EQ,
  BC_INST_OP_REL_LESS,
  BC_INST_OP_REL_GREATER,

  BC_INST_OP_BOOL_NOT,

  BC_INST_OP_BOOL_NOT,
  BC_INST_OP_BOOL_AND,

  BC_INST_OP_ASSIGN_POWER,
  BC_INST_OP_ASSIGN_MULTIPLY,
  BC_INST_OP_ASSIGN_DIVIDE,
  BC_INST_OP_ASSIGN_MODULUS,
  BC_INST_OP_ASSIGN_PLUS,
  BC_INST_OP_ASSIGN_MINUS,
  BC_INST_OP_ASSIGN,

};

const char *bc_program_byte_fmt = "%02x";

const BcNumBinaryFunc bc_program_math_ops[] = {

  bc_num_mod,
  NULL, // &
  NULL, // '
  NULL, // (
  NULL, // )
  bc_num_mul,
  bc_num_add,
  NULL, // ,
  bc_num_sub,
  NULL, // .
  bc_num_div,

};

const char *bc_program_stdin_name = "<stdin>";

const char *bc_program_ready_prompt = "ready for more input\n\n";

const char *bc_program_sigint_msg = "\n\ninterrupt (type \"quit\" to exit)\n\n";
