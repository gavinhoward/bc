#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <getopt.h>

#include <bc/vm.h>

int bc_mathlib = 0;
int bc_quiet = 0;
int bc_std = 0;
int bc_warn = 0;

static const struct option bc_opts[] = {

    { "help", no_argument, NULL, 'h' },
    { "mathlib", no_argument, &bc_mathlib, 'l' },
    { "quiet", no_argument, &bc_quiet, 'q' },
    { "standard", no_argument, &bc_std, 's' },
    { "version", no_argument, NULL, 'v' },
    { "warn", no_argument, &bc_warn, 'w' },
    { 0, 0, 0, 0 },

};

static const char* const bc_copyright = "Copyright (c) 2018 Gavin D. Howard";

static const char* const bc_help =
  "usage: bc [options] [file ...]\n"
  "\n"
  "  -h  --help         print this usage message and exit\n"
  "  -i  --interactive  force interactive mode (currently unused)\n"
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

int main(int argc, char* argv[]) {

	BcStatus status;
	BcVm vm;
	bool do_exit;

	do_exit = false;

	// Getopt needs this.
	int opt_idx = 0;

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
				printf(bc_version_fmt, BC_VERSION_STR, bc_copyright);
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
		printf(bc_version_fmt, BC_VERSION_STR, bc_copyright);
		putchar('\n');
	}

	uint32_t num_files = argc - optind;
	const char** file_names = (const char**) (argv + optind);

	status = bc_vm_init(&vm, num_files, file_names);

	if (status) {
		return status;
	}

	status = bc_vm_exec(&vm);

	bc_vm_free(&vm);

	return status;
}
