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

static const char* const bc_short_opts = "hlqsvw";

int main(int argc, char* argv[]) {

	BcStatus status;
	BcVm vm;

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
				// TODO: Print help.
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
				// TODO: Print version.
				break;

			case 'w':
				bc_warn = 'w';
				break;

			case '?':
				// Getopt printed an error message, but we should exit.
			default:
				exit(BC_STATUS_INVALID_OPTION);
				break;
		}

		// Get the next option.
		c = getopt_long(argc, argv, bc_short_opts, bc_opts, &opt_idx);
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
