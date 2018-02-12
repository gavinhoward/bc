/*
 * Copyright 2018 Contributors
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.

 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *******************************************************************************
 *
 * A special license exemption is granted to the Toybox project and to the
 * Android Open Source Project to use this source under the following BSD
 * 0-Clause License:
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
 *******************************************************************************
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

#include <bc/vm.h>

#include <bc/bc.h>

static const char* const bc_err_types[] = {

  NULL,

  "bc",
  "bc",

  "bc",

  "bc",
  "bc",

  "vector",

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

static const char* const bc_err_descs[] = {

  NULL,

  "memory allocation error",
  "invalid parameter",

  "invalid option",

  "one or more limits not specified",
  "invalid limit; this is a bug in bc",

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

  "couldn't open file",
  "file read error",
  "divide by zero",
  "negative square root",
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
  "bc was not halted correctly; this is a bug in bc",

  "POSIX bc only allows one character names; the following is invalid:",
  "POSIX bc does not allow '#' script comments",
  "POSIX bc does not allow the following keyword:",
  "POSIX bc requires parentheses around return expressions",
  "POSIX bc does not allow boolean operators; the following is invalid:",
  "POSIX bc does not allow comparison operators outside if or loops",
  "POSIX bc does not allow more than one comparison operator per condition",
  "POSIX bc does not allow an empty init expression in a for loop",
  "POSIX bc does not allow an empty condition expression in a for loop",
  "POSIX bc does not allow an empty update expression in a for loop",
  "POSIX bc requires the left brace be on the same line as the function header",

};

static const char* const bc_copyright =
  "bc copyright (c) 2018 Gavin D. Howard and arbprec contributors.\n"
  "arbprec copyright (c) 2016-2018 CM Graff,\n"
  "        copyright (c) 2018 Bao Hexing.";

static const char* const bc_version_fmt = "bc %s\n%s\n";

BcStatus bc_exec(unsigned int flags, unsigned int filec, const char* filev[]) {

  BcStatus status;
  BcVm vm;
  int do_exit;
  const char** files;

  status = BC_STATUS_SUCCESS;
  do_exit = 0;
  files = NULL;


  if (flags & BC_FLAG_HELP) do_exit = 1;

  if (flags & BC_FLAG_VERSION) {
    printf(bc_version_fmt, bc_version, bc_copyright);
    do_exit = 1;
  }

  if (flags & BC_FLAG_INTERACTIVE ||
      (isatty(STDIN_FILENO) && isatty(STDOUT_FILENO)))
  {
    bc_interactive = 1;
  }
  else bc_interactive = 0;

  bc_code = flags & BC_FLAG_CODE;
  bc_std = flags & BC_FLAG_STANDARD;
  bc_warn = flags & BC_FLAG_WARN;

  if (do_exit) return status;

  if (!(flags & BC_FLAG_QUIET)) {
    printf(bc_version_fmt, bc_version, bc_copyright);
    putchar('\n');
  }

  if (flags & BC_FLAG_MATHLIB) {
    // TODO: Load the math functions.
  }

  status = bc_vm_init(&vm, filec, filev);

  if (status) {
    return status;
  }

  status = bc_vm_exec(&vm);

  if (files) {
    free(files);
  }

  return status;
}

void bc_error(BcStatus status) {

  if (!status || status == BC_STATUS_PARSE_QUIT ||
      status == BC_STATUS_VM_HALT ||
      status >= BC_STATUS_POSIX_NAME_LEN)
  {
    return;
  }

  fprintf(stderr, "\n%s Error: %s\n\n", bc_err_types[status], bc_err_descs[status]);
}

void bc_error_file(BcStatus status, const char* file, uint32_t line) {

  if (!status || status == BC_STATUS_PARSE_QUIT ||
      !file || status >= BC_STATUS_POSIX_NAME_LEN)
  {
    return;
  }

  fprintf(stderr, "\n%s Error: %s\n", bc_err_types[status], bc_err_descs[status]);
  fprintf(stderr, "    %s", file);

  if (line) {
    fprintf(stderr, ":%d\n\n", line);
  }
  else {
    fputc('\n', stderr);
    fputc('\n', stderr);
  }
}

BcStatus bc_posix_error(BcStatus status, const char* file,
                        uint32_t line, const char* msg)
{
  if (!(bc_std || bc_warn) || status < BC_STATUS_POSIX_NAME_LEN || !file) {
    return BC_STATUS_SUCCESS;
  }

  fprintf(stderr, "\n%s %s: %s\n", bc_err_types[status],
          bc_std ? "Error" : "Warning", bc_err_descs[status]);

  if (msg) {
    fprintf(stderr, "    %s\n", msg);
  }

  fprintf(stderr, "    %s", file);

  if (line) {
    fprintf(stderr, ":%d\n\n", line);
  }
  else {
    fputc('\n', stderr);
    fputc('\n', stderr);
  }

  return bc_std ? status : BC_STATUS_SUCCESS;
}
