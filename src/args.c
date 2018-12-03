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
	{ "version", no_argument, NULL, 'v' },
	{ "warn", no_argument, NULL, 'w' },
	{ "extended-register", no_argument, NULL, 'x' },
	{ 0, 0, 0, 0 },

};

static const char* const bc_args_opt = "e:f:hilqsvVwx";

void bc_args_exprs(BcVec *exprs, const char *str) {
	bc_vec_concat(exprs, str);
	bc_vec_concat(exprs, "\n");
}

void bc_args_file(BcVec *exprs, const char *file) {

	BcStatus s;
	char *buf;

	s = bc_read_file(file, &buf);

	if (s) bc_vm_error(s, file, 0);
	else {
		bc_args_exprs(exprs, buf);
		free(buf);
	}
}

void bc_args(int argc, char *argv[], uint32_t *flags, BcVec *exs, BcVec *files)
{
	BcStatus s = BC_STATUS_SUCCESS;
	int c, i;
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
				bc_args_exprs(exs, optarg);
				break;
			}

			case 'f':
			{
				bc_args_file(exs, optarg);
				break;
			}

			case 'h':
			{
				bc_vm_info(bcg.help);
				do_exit = true;
				break;
			}

#if BC_ENABLED
			case 'i':
			{
				if (!bcg.bc) s = BC_STATUS_INVALID_OPTION;
				(*flags) |= BC_FLAG_I;
				break;
			}

			case 'l':
			{
				if (!bcg.bc) s = BC_STATUS_INVALID_OPTION;
				(*flags) |= BC_FLAG_L;
				break;
			}

			case 'q':
			{
				if (!bcg.bc) s = BC_STATUS_INVALID_OPTION;
				(*flags) |= BC_FLAG_Q;
				break;
			}

			case 's':
			{
				if (!bcg.bc) s = BC_STATUS_INVALID_OPTION;
				(*flags) |= BC_FLAG_S;
				break;
			}

			case 'w':
			{
				if (!bcg.bc) s = BC_STATUS_INVALID_OPTION;
				(*flags) |= BC_FLAG_W;
				break;
			}
#endif // BC_ENABLED

			case 'V':
			case 'v':
			{
				(*flags) |= BC_FLAG_V;
				do_exit = true;
				break;
			}

#if DC_ENABLED
			case 'x':
			{
				if (bcg.bc) s = BC_STATUS_INVALID_OPTION;
				(*flags) |= BC_FLAG_X;
				break;
			}
#endif // DC_ENABLED

			// Getopt printed an error message, but we should exit.
			case '?':
			default:
			{
				exit(BC_STATUS_INVALID_OPTION);
				break;
			}
		}

		if (s) bc_vm_exit(s);
	}

	if ((*flags) & BC_FLAG_V) bc_vm_info(NULL);
	if (do_exit) exit((int) s);
	if (exs->len > 1 || !bcg.bc) (*flags) |= BC_FLAG_Q;
	if (argv[optind] && !strcmp(argv[optind], "--")) ++optind;

	for (i = optind; i < argc; ++i) bc_vec_push(files, argv + i);
}
