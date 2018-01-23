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
    "invalid statement; this is most likely a bug in bc",
    "invalid string",
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

BcStatus bc_limits() {

	long base_max;
	long dim_max;
	long scale_max;
	long string_max;

#ifdef _POSIX_BC_BASE_MAX
	base_max = _POSIX_BC_BASE_MAX;
#elif defined(_BC_BASE_MAX)
	base_max = _BC_BASE_MAX;
#else
	errno = 0;
	base_max = sysconf(_SC_BC_BASE_MAX);

	if (base_max == -1) {

		if (errno) {
			return BC_STATUS_NO_LIMIT;
		}

		base_max = BC_BASE_MAX_DEF;
	}
	else if (base_max > BC_BASE_MAX_DEF) {
		return BC_STATUS_INVALID_LIMIT;
	}
#endif

#ifdef _POSIX_BC_DIM_MAX
	dim_max = _POSIX_BC_DIM_MAX;
#elif defined(_BC_DIM_MAX)
	dim_max = _BC_DIM_MAX;
#else
	errno = 0;
	dim_max = sysconf(_SC_BC_DIM_MAX);

	if (dim_max == -1) {

		if (errno) {
			return BC_STATUS_NO_LIMIT;
		}

		dim_max = BC_DIM_MAX_DEF;
	}
	else if (dim_max > BC_DIM_MAX_DEF) {
		return BC_STATUS_INVALID_LIMIT;
	}
#endif

#ifdef _POSIX_BC_SCALE_MAX
	scale_max = _POSIX_BC_SCALE_MAX;
#elif defined(_BC_SCALE_MAX)
	scale_max = _BC_SCALE_MAX;
#else
	errno = 0;
	scale_max = sysconf(_SC_BC_SCALE_MAX);

	if (scale_max == -1) {

		if (errno) {
			return BC_STATUS_NO_LIMIT;
		}

		scale_max = BC_SCALE_MAX_DEF;
	}
	else if (scale_max > BC_SCALE_MAX_DEF) {
		return BC_STATUS_INVALID_LIMIT;
	}
#endif

#ifdef _POSIX_BC_STRING_MAX
	string_max = _POSIX_BC_STRING_MAX;
#elif defined(_BC_STRING_MAX)
	string_max = _BC_STRING_MAX;
#else
	errno = 0;
	string_max = sysconf(_SC_BC_STRING_MAX);

	if (string_max == -1) {

		if (errno) {
			return BC_STATUS_NO_LIMIT;
		}

		string_max = BC_STRING_MAX_DEF;
	}
	else if (string_max > BC_STRING_MAX_DEF) {
		return BC_STATUS_INVALID_LIMIT;
	}
#endif

	putchar('\n');

	printf("BC_BASE_MAX     = %ld\n", base_max);
	printf("BC_DIM_MAX      = %ld\n", dim_max);
	printf("BC_SCALE_MAX    = %ld\n", scale_max);
	printf("BC_STRING_MAX   = %ld\n", string_max);
	printf("Max Exponent    = %ld\n", INT64_MAX);
	printf("Number of Vars  = %u\n", UINT32_MAX);

	putchar('\n');

	return BC_STATUS_SUCCESS;
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
