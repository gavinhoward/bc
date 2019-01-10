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
 * Code for processing command-line arguments.
 *
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <getopt.h>

#include <status.h>
#include <vector.h>
#include <read.h>
#include <vm.h>
#include <args.h>

static const struct option bc_args_lopt[] = {

	{ "expression", required_argument, NULL, 'e' },
	{ "file", required_argument, NULL, 'f' },
	{ "help", no_argument, NULL, 'h' },
	{ "interactive", no_argument, NULL, 'i' },
	{ "mathlib", no_argument, NULL, 'l' },
	{ "quiet", no_argument, NULL, 'q' },
	{ "standard", no_argument, NULL, 's' },
	{ "warn", no_argument, NULL, 'w' },
	{ "version", no_argument, NULL, 'v' },
	{ "extended-register", no_argument, NULL, 'x' },
	{ 0, 0, 0, 0 },

};

static const char* const bc_args_opt = "e:f:hilqsvVwx";

static void bc_args_exprs(BcVec *exprs, const char *str) {
	bc_vec_concat(exprs, str);
	bc_vec_concat(exprs, "\n");
}

static BcStatus bc_args_file(BcVec *exprs, const char *file) {

	BcStatus s;
	char *buf;

	s = bc_read_file(file, &buf);
	if (s) return s;

	bc_args_exprs(exprs, buf);
	free(buf);

	return s;
}

BcStatus bc_args(int argc, char *argv[]) {

	BcStatus s = BC_STATUS_SUCCESS;
	int c, i, err = 0;
	bool do_exit = false;

	i = optind = 0;

	while ((c = getopt_long(argc, argv, bc_args_opt, bc_args_lopt, &i)) != -1) {

		switch (c) {

			case 0:
			{
				// This is the case when a long option is found.
				break;
			}

			case 'e':
			{
				bc_args_exprs(&vm->exprs, optarg);
				break;
			}

			case 'f':
			{
				s = bc_args_file(&vm->exprs, optarg);
				break;
			}

			case 'h':
			{
				bc_vm_info(vm->help);
				do_exit = true;
				break;
			}

#if BC_ENABLED
			case 'i':
			{
#if DC_ENABLED
				if (!BC_IS_BC) err = c;
#endif // DC_ENABLED
				vm->flags |= BC_FLAG_I;
				break;
			}

			case 'l':
			{
#if DC_ENABLED
				if (!BC_IS_BC) err = c;
#endif // DC_ENABLED
				vm->flags |= BC_FLAG_L;
				break;
			}

			case 'q':
			{
#if DC_ENABLED
				if (!BC_IS_BC) err = c;
#endif // DC_ENABLED
				vm->flags |= BC_FLAG_Q;
				break;
			}

			case 's':
			{
#if DC_ENABLED
				if (!BC_IS_BC) err = c;
#endif // DC_ENABLED
				vm->flags |= BC_FLAG_S;
				break;
			}

			case 'w':
			{
#if DC_ENABLED
				if (!BC_IS_BC) err = c;
#endif // DC_ENABLED
				vm->flags |= BC_FLAG_W;
				break;
			}
#endif // BC_ENABLED

			case 'V':
			case 'v':
			{
				vm->flags |= BC_FLAG_V;
				do_exit = true;
				break;
			}

#if DC_ENABLED
			case 'x':
			{
#if BC_ENABLED
				if (BC_IS_BC) err = c;
#endif // BC_ENABLED
				vm->flags |= DC_FLAG_X;
				break;
			}
#endif // DC_ENABLED

			// Getopt printed an error message, but we should exit.
			case '?':
			default:
			{
				// We don't set err, so we won't print a message.
				s = BC_STATUS_ERROR;
				break;
			}
		}

		if (err && !s) {

			for (i = 0; bc_args_lopt[i].name; ++i) {
				if (bc_args_lopt[i].val == err) break;
			}

			s = bc_vm_verr(BC_ERROR_VM_OPTION, err, bc_args_lopt[i].name);
		}

		if (s) return s;
	}

	if (vm->flags & BC_FLAG_V) bc_vm_info(NULL);
	if (do_exit) exit((int) s);
	if (vm->exprs.len > 1 || !BC_IS_BC) vm->flags |= BC_FLAG_Q;
	if (argv[optind] && !strcmp(argv[optind], "--")) ++optind;

	for (i = optind; i < argc; ++i) bc_vec_push(&vm->files, argv + i);

	return s;
}
