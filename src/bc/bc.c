#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <limits.h>

#include <bc/vm.h>

static const char* const bc_err_types[] = {

  NULL,

  "bc",
  "bc",
  "bc",

  "bc",
  "bc",

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
  "parse",

  "runtime",
  "runtime",
  "runtime",
  "runtime",
  "runtime",
  "runtime",
  "runtime",
  "runtime",
  "runtime",
  "runtime",
  "runtime",
  "runtime",
  "runtime",
  "runtime",
  "runtime",
  "runtime",
  "runtime",
  "runtime",
  "runtime",
  "runtime",
  "runtime",
  "runtime",

};

static const char* const bc_err_descs[] = {

  NULL,

  "invalid option",
  "memory allocation error",
  "invalid parameter",

  "one or more limits not specified",
  "invalid limit; this is a bug in bc",

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
  "quit statement in file",
  "end of file",
  "bug in parser",

  "couldn't open file",
  "file read error",
  "divide by zero",
  "negative square root",
  "mismatched parameters",
  "undefined function",
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
  "print error",
  "break statement outside loop; "
    "this is a bug in bc (parser should have caught it)",
  "continue statement outside loop; "
    "this is a bug in bc (parser should have caught it)",
  "bc was not halted correctly; this is a bug in bc",

};

int bc_interactive = 0;
int bc_std = 0;
int bc_warn = 0;

int bc_had_sigint = 0;

static const char* const bc_copyright =
  "bc copyright (c) 2018 Gavin D. Howard and arbprec contributors.\n"
  "arbprec copyright (c) 2016-2018 CM Graff,\n"
  "        copyright (c) 2018 Bao Hexing.";

static const char* const bc_help =
  "usage: bc [options] [file ...]\n"
  "\n"
  "  -h  --help         print this usage message and exit\n"
  "  -i  --interactive  force interactive mode\n"
  "  -l  --mathlib      use predefined math routines:\n\n"
  "                       s(expr)  =  sine of expr in radians\n"
  "                       c(expr)  =  cosine of expr in radians\n"
  "                       a(expr)  =  arctangent of expr, returning radians\n"
  "                       l(expr)  =  natural log of expr\n"
  "                       e(expr)  =  raises e to the power of expr\n"
  "                       j(n, x)  =  Bessel function of integer order n of x\n"
  "\n"
  "  -q  --quiet        don't print version and copyright\n"
  "  -s  --standard     error if any non-POSIX extensions are used\n"
  "  -w  --warn         warn if any non-POSIX extensions are used\n"
  "  -v  --version      print version information and copyright and exit\n\n";

static const char* const bc_version_fmt = "bc %s\n%s\n";

static const char* const bc_version = "0.1";

BcStatus bc_exec(unsigned int flags, unsigned int filec, const char* filev[]) {

  BcStatus status;
  BcVm vm;
  int do_exit;

  status = BC_STATUS_SUCCESS;
  do_exit = 0;

  if (flags & BC_FLAG_HELP) {
    printf(bc_help);
    do_exit = 1;
  }

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

  bc_std = flags & BC_FLAG_STANDARD;
  bc_warn = flags & BC_FLAG_WARN;

  if (do_exit) return status;

  if (!(flags & BC_FLAG_QUIET)) {
    printf(bc_version_fmt, bc_version, bc_copyright);
    putchar('\n');
  }

  status = bc_vm_init(&vm, filec, filev);

  if (status) {
    return status;
  }

  if (flags & BC_FLAG_MATHLIB) {
    // TODO: Load the math functions.
  }

  status = bc_vm_exec(&vm);

  return status;
}

void bc_error(BcStatus status) {

  if (!status || status == BC_STATUS_PARSE_QUIT || status == BC_STATUS_VM_HALT)
  {
    return;
  }

  fprintf(stderr, "\n%s error: %s\n\n", bc_err_types[status], bc_err_descs[status]);
}

void bc_error_file(BcStatus status, const char* file, uint32_t line) {

  if (!status || status == BC_STATUS_PARSE_QUIT || !file) {
    return;
  }

  fprintf(stderr, "\n%s error: %s\n", bc_err_types[status], bc_err_descs[status]);
  fprintf(stderr, "    %s", file);

  if (line) {
    fprintf(stderr, ":%d\n\n", line);
  }
  else {
    fputc('\n', stderr);
    fputc('\n', stderr);
  }
}
