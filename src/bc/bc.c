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
 * Code common to all of bc.
 *
 */

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <limits.h>

#include <bc.h>
#include <vm.h>

static const char *bc_err_types[] = {

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

};

static const char *bc_err_descs[] = {

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
  "invalid temp variable; this is most likely a bug in bc",
  "invalid read() expression",
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
  "POSIX requires parentheses around return expressions",
  "POSIX does not allow boolean operators; the following is invalid:",
  "POSIX does not allow comparison operators outside if or loops",
  "POSIX does not allow more than one comparison operator per condition",
  "POSIX does not allow an empty init expression in a for loop",
  "POSIX does not allow an empty condition expression in a for loop",
  "POSIX does not allow an empty update expression in a for loop",
  "POSIX requires the left brace be on the same line as the function header",

};

static const char *bc_version = "0.1";

static const char *bc_copyright =
  "bc copyright (c) 2018 Gavin D. Howard";

static const char *bc_warranty_short =
  "This is free software with ABSOLUTELY NO WARRANTY.";

static const char *bc_version_fmt = "bc %s\n%s\n\n%s\n\n";

BcStatus bc_exec(unsigned int flags, unsigned int filec, const char *filev[]) {

  BcStatus status;
  BcVm vm;

  if (flags & BC_FLAG_INTERACTIVE ||
      (isatty(STDIN_FILENO) && isatty(STDOUT_FILENO)))
  {
    bc_interactive = 1;
  }
  else bc_interactive = 0;

  bc_code = flags & BC_FLAG_CODE;
  bc_std = flags & BC_FLAG_STANDARD;
  bc_warn = flags & BC_FLAG_WARN;

  if (!(flags & BC_FLAG_QUIET)) {

    status = bc_print_version();

    if (status) return status;
  }

  status = bc_vm_init(&vm, filec, filev);

  if (status) return status;

  if (flags & BC_FLAG_MATHLIB) {

    status = bc_parse_file(&vm.parse, bc_lib_name);

    if (status) goto err;

    status = bc_parse_text(&vm.parse, (const char*) bc_lib);

    if (status) goto err;

    while (!status) status = bc_parse_parse(&vm.parse);

    if (status != BC_STATUS_LEX_EOF && status != BC_STATUS_PARSE_EOF) goto err;
  }

  status = bc_vm_exec(&vm);

err:

  bc_vm_free(&vm);

  return status;
}

BcStatus bc_print_version() {

  int err;

  err = printf(bc_version_fmt, bc_version, bc_copyright, bc_warranty_short);

  return err < 0 ? BC_STATUS_IO_ERR : BC_STATUS_SUCCESS;
}

void bc_error(BcStatus status) {

  if (!status || status == BC_STATUS_PARSE_QUIT ||
      status == BC_STATUS_EXEC_HALT ||
      status >= BC_STATUS_POSIX_NAME_LEN)
  {
    return;
  }

  fprintf(stderr, "\n%s Error: %s\n\n", bc_err_types[status], bc_err_descs[status]);
}

void bc_error_file(BcStatus status, const char *file, uint32_t line) {

  if (!status || status == BC_STATUS_PARSE_QUIT ||
      !file || status >= BC_STATUS_POSIX_NAME_LEN)
  {
    return;
  }

  fprintf(stderr, "\n%s Error: %s\n", bc_err_types[status],
          bc_err_descs[status]);

  fprintf(stderr, "    %s", file);

  if (line) fprintf(stderr, ":%d\n\n", line);
  else fprintf(stderr, "\n\n");
}

BcStatus bc_posix_error(BcStatus status, const char *file,
                        uint32_t line, const char *msg)
{
  if (!(bc_std || bc_warn) || status < BC_STATUS_POSIX_NAME_LEN || !file)
    return BC_STATUS_SUCCESS;

  fprintf(stderr, "\n%s %s: %s\n", bc_err_types[status],
          bc_std ? "Error" : "Warning", bc_err_descs[status]);

  if (msg) fprintf(stderr, "    %s\n", msg);

  fprintf(stderr, "    %s", file);

  if (line) fprintf(stderr, ":%d\n\n", line);
  else fprintf(stderr, "\n\n");

  return bc_std ? status : BC_STATUS_SUCCESS;
}
