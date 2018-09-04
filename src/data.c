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

#include <bc.h>
#include <status.h>
#include <lex.h>
#include <parse.h>

const char bc_header[] =
  "bc 1.0\n"
  "bc copyright (c) 2018 Gavin D. Howard and contributors\n"
  "Report bugs at: https://github.com/gavinhoward/bc\n\n"
  "This is free software with ABSOLUTELY NO WARRANTY.\n\n";

const char bc_err_fmt[] = "\n%s error: %s\n\n";

const char *bc_errs[] = {
  "bc",
  "Lex",
  "Parse",
  "Math",
  "Runtime",
  "POSIX",
};

const uint8_t bc_err_indices[] = {
  BC_ERR_IDX_BC, BC_ERR_IDX_BC, BC_ERR_IDX_BC, BC_ERR_IDX_BC,
  BC_ERR_IDX_LEX, BC_ERR_IDX_LEX, BC_ERR_IDX_LEX, BC_ERR_IDX_LEX,
  BC_ERR_IDX_PARSE, BC_ERR_IDX_PARSE, BC_ERR_IDX_PARSE, BC_ERR_IDX_PARSE,
  BC_ERR_IDX_PARSE, BC_ERR_IDX_PARSE, BC_ERR_IDX_PARSE,
  BC_ERR_IDX_MATH, BC_ERR_IDX_MATH, BC_ERR_IDX_MATH, BC_ERR_IDX_MATH,
  BC_ERR_IDX_MATH, BC_ERR_IDX_MATH,
  BC_ERR_IDX_EXEC, BC_ERR_IDX_EXEC, BC_ERR_IDX_EXEC, BC_ERR_IDX_EXEC,
  BC_ERR_IDX_EXEC, BC_ERR_IDX_EXEC, BC_ERR_IDX_EXEC, BC_ERR_IDX_EXEC,
  BC_ERR_IDX_EXEC, BC_ERR_IDX_EXEC, BC_ERR_IDX_EXEC, BC_ERR_IDX_EXEC,
  BC_ERR_IDX_EXEC, BC_ERR_IDX_EXEC,
  BC_ERR_IDX_POSIX, BC_ERR_IDX_POSIX, BC_ERR_IDX_POSIX, BC_ERR_IDX_POSIX,
  BC_ERR_IDX_POSIX, BC_ERR_IDX_POSIX, BC_ERR_IDX_POSIX, BC_ERR_IDX_POSIX,
  BC_ERR_IDX_POSIX, BC_ERR_IDX_POSIX, BC_ERR_IDX_POSIX, BC_ERR_IDX_POSIX,
  BC_ERR_IDX_VEC, BC_ERR_IDX_VEC,
  BC_ERR_IDX_BC, BC_ERR_IDX_BC, BC_ERR_IDX_BC,
};

const char *bc_err_descs[] = {
  NULL,
  "memory allocation error",
  "I/O error",
  "file is not text",

  "bad character",
  "string end could not be found",
  "comment end could not be found",
  "end of file",

  "bad token",
  "bad expression",
  "bad print statement",
  "bad function definition",
  "bad assignment: left must be scale, ibase, "
    "obase, last, var, or array element",
  "no auto variable found",
  "function parameter or auto var has the same name as another",

  "negative number",
  "non integer number",
  "overflow",
  "divide by zero",
  "negative square root",
  "bad number string",

  "could not open file",
  "mismatched parameters",
  "undefined function",
  "file is not executable",
  "could not install signal handler",
  "bad scale; must be [0, BC_SCALE_MAX]",
  "bad ibase; must be [2, 16]",
  "bad obase; must be [2, BC_BASE_MAX]",
  "string too long: must be [1, BC_STRING_MAX]",
  "array too long; must be [1, BC_DIM_MAX]",
  "bad read() expression",
  "read() call inside of a read() call",
  "variable is wrong type",
  "signal caught",

  "POSIX only allows one character names; the following is bad:",
  "POSIX does not allow '#' script comments",
  "POSIX does not allow the following keyword:",
  "POSIX does not allow a period ('.') as a shortcut for the last result",
  "POSIX requires parentheses around return expressions",
  "POSIX does not allow boolean operators; the following is bad:",
  "POSIX does not allow comparison operators outside if or loops",
  "POSIX requires exactly one comparison operator per condition",
  "POSIX does not allow an empty init expression in a for loop",
  "POSIX does not allow an empty condition expression in a for loop",
  "POSIX does not allow an empty update expression in a for loop",
  "POSIX requires the left brace be on the same line as the function header",

  "index is out of bounds",
  "item already exists",
#ifndef NDEBUG
  "quit request not honored",
  "limits request not honored",
#endif // NDEBUG
};

const char bc_sig_msg[34] = "\ninterrupt (type \"quit\" to exit)\n";

const char bc_lang_func_main[7] = "(main)";
const char bc_lang_func_read[7] = "(read)";

#ifndef NDEBUG
const char bc_lang_inst_chars[] =
  "edED_^*/%+-=;?~<>!|&`{}@[],NVMACaI.LlrOqpQsSJjPR$H";
#endif // NDEBUG

const BcLexKeyword bc_lex_keywords[20] = {
  BC_LEX_KW_ENTRY("auto", 4, true),
  BC_LEX_KW_ENTRY("break", 5, true),
  BC_LEX_KW_ENTRY("continue", 8, false),
  BC_LEX_KW_ENTRY("define", 6, true),
  BC_LEX_KW_ENTRY("else", 4, false),
  BC_LEX_KW_ENTRY("for", 3, true),
  BC_LEX_KW_ENTRY("halt", 4, false),
  BC_LEX_KW_ENTRY("ibase", 5, true),
  BC_LEX_KW_ENTRY("if", 2, true),
  BC_LEX_KW_ENTRY("last", 4, false),
  BC_LEX_KW_ENTRY("length", 6, true),
  BC_LEX_KW_ENTRY("limits", 6, false),
  BC_LEX_KW_ENTRY("obase", 5, true),
  BC_LEX_KW_ENTRY("print", 5, false),
  BC_LEX_KW_ENTRY("quit", 4, true),
  BC_LEX_KW_ENTRY("read", 4, false),
  BC_LEX_KW_ENTRY("return", 6, true),
  BC_LEX_KW_ENTRY("scale", 5, true),
  BC_LEX_KW_ENTRY("sqrt", 4, true),
  BC_LEX_KW_ENTRY("while", 5, true),
};

const char bc_num_hex_digits[] = "0123456789ABCDEF";

// This is an array that corresponds to token types. An entry is
// true if the token is valid in an expression, false otherwise.
const bool bc_parse_token_exprs[] = {
  true, true, true, true, true, true, true, true, true, true, true, true, true,
  true, true, true, true, true, true, true, true, true, true, true, true, false,
  false, true, true, false, false, false, false, false, false, false, true,
  true, false, false, false, false, false, false, false, true, false, true,
  true, true, true, false, false, true, false, true, true, false, false, false,
};

// This is an array of data for operators that correspond to token types.
const BcOp bc_parse_ops[] = {
  { 0, false }, { 0, false },
  { 1, false },
  { 2, false },
  { 3, true }, { 3, true }, { 3, true },
  { 4, true }, { 4, true },
  { 6, true }, { 6, true }, { 6, true }, { 6, true }, { 6, true }, { 6, true },
  { 7, false },
  { 8, true }, { 8, true },
  { 5, false }, { 5, false }, { 5, false }, { 5, false }, { 5, false },
  { 5, false }, { 5, false },
};

const BcNumBinaryOp bc_program_ops[] = {
  bc_num_pow, bc_num_mul, bc_num_div, bc_num_mod, bc_num_add, bc_num_sub,
};

const char bc_program_stdin_name[] = "<stdin>";
const char bc_program_ready_prompt[] = "ready for more input\n";
