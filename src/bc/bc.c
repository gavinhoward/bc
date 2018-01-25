#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <limits.h>

#include <getopt.h>

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
int bc_mathlib = 0;
int bc_quiet = 0;
int bc_std = 0;
int bc_warn = 0;

int bc_had_sigint = 0;

static const struct option bc_opts[] = {

    { "help", no_argument, NULL, 'h' },
    { "interactive", no_argument, &bc_interactive, 'i' },
    { "mathlib", no_argument, &bc_mathlib, 'l' },
    { "quiet", no_argument, &bc_quiet, 'q' },
    { "standard", no_argument, &bc_std, 's' },
    { "version", no_argument, NULL, 'v' },
    { "warn", no_argument, &bc_warn, 'w' },
    { 0, 0, 0, 0 },

};

static const char* const bc_copyright =
  "bc copyright (c) 2018 Gavin D. Howard.\n"
  "arbprec copyright (c) 2016-2018 CM Graff,\n"
  "        copyright (c) 2018 Bao Hexing and Gavin D. Howard.";

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

static const char* const bc_short_opts = "hlqsvw";

static const char* const bc_version_fmt = "bc %s\n%s\n";

static const char* const bc_version = "0.1";

BcStatus bc_main(int argc, char* argv[]) {

	BcStatus status;
	BcVm vm;
	bool do_exit;

	do_exit = false;

	// Getopt needs this.
	int opt_idx = 0;

	if (isatty(STDIN_FILENO) && isatty(STDOUT_FILENO)) {
		bc_interactive = 'i';
	}
	else {
		bc_interactive = 0;
	}

	int c = getopt_long(argc, argv, bc_short_opts, bc_opts, &opt_idx);

	while (c != -1) {

		switch (c) {

			case 0:
				// This is the case when a long option is
				// found, so we don't need to do anything.
				break;

			case 'h':
				printf(bc_help);
				do_exit = true;
				break;

			case 'i':
				bc_interactive = 'i';
				break;

			case 'l':
				bc_mathlib = 'l';
				break;

			case 'q':
				bc_quiet = 'q';
				break;

			case 's':
				bc_std = 's';
				break;

			case 'v':
				printf(bc_version_fmt, bc_version, bc_copyright);
				do_exit = true;
				break;

			case 'w':
				bc_warn = 'w';
				break;

			case '?':
				// Getopt printed an error message, but we should exit.
			default:
				do_exit = true;
				status = BC_STATUS_INVALID_OPTION;
				break;
		}

		c = getopt_long(argc, argv, bc_short_opts, bc_opts, &opt_idx);
	}

	if (do_exit) {
		exit(status);
	}

	if (!bc_quiet) {
		printf(bc_version_fmt, bc_version, bc_copyright);
		putchar('\n');
	}

	uint32_t num_files = argc - optind;
	const char** file_names = (const char**) (argv + optind);

	status = bc_vm_init(&vm, num_files, file_names);

	if (status) {
		return status;
	}

	if (bc_mathlib) {
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
