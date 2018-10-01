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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <getopt.h>

#include <status.h>
#include <vector.h>
#include <io.h>
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

static const char* const bc_args_opt = "e:f:hilqsvwx";
const char* const bc_args_env_name = "BC_ENV_ARGS";

static BcStatus bc_args_exprs(BcVec *exprs, const char *str) {

	BcStatus s;

	if ((s = bc_vec_concat(exprs, str))) return s;

	return bc_vec_concat(exprs, "\n");
}

static BcStatus bc_args_file(BcVec *exprs, const char *file) {

	BcStatus s;
	char *buf;

	if ((s = bc_io_fread(file, &buf))) return s;

	s = bc_args_exprs(exprs, buf);

	free(buf);

	return s;
}

BcStatus bc_args(int argc, char *argv[], const char* const help,
                 unsigned int *flags, BcVec *exprs, BcVec *files) {

	BcStatus s = BC_STATUS_SUCCESS;
	int c, i, idx;
	bool do_exit = false, bc = !strcmp(bcg.name, bc_name);

	idx = c = optind = 0;

	while ((c = getopt_long(argc, argv, bc_args_opt, bc_args_lopt, &idx)) != -1)
	{
		switch (c) {

			case 0:
			{
				// This is the case when a long option is found.
				if (bc_args_lopt[idx].val == 'e')
					s = bc_args_exprs(exprs, optarg);
				else if (bc_args_lopt[idx].val == 'f')
					s = bc_args_file(exprs, optarg);
				break;
			}

			case 'e':
			{
				if ((s = bc_args_exprs(exprs, optarg))) return s;
				break;
			}

			case 'f':
			{
				if ((s = bc_args_file(exprs, optarg))) return s;
				break;
			}

			case 'h':
			{
				if (printf(help, argv[0]) < 0) return BC_STATUS_IO_ERR;
				do_exit = true;
				break;
			}

#ifdef BC_CONFIG
			case 'i':
			{
				if (!bc) return BC_STATUS_INVALID_OPTION;
				(*flags) |= BC_FLAG_I;
				break;
			}

			case 'l':
			{
				if (!bc) return BC_STATUS_INVALID_OPTION;
				(*flags) |= BC_FLAG_L;
				break;
			}

			case 'q':
			{
				if (!bc) return BC_STATUS_INVALID_OPTION;
				(*flags) |= BC_FLAG_Q;
				break;
			}

			case 's':
			{
				if (!bc) return BC_STATUS_INVALID_OPTION;
				(*flags) |= BC_FLAG_S;
				break;
			}

			case 'w':
			{
				if (!bc) return BC_STATUS_INVALID_OPTION;
				(*flags) |= BC_FLAG_W;
				break;
			}
#endif // BC_CONFIG

			case 'V':
			case 'v':
			{
				if (puts(bc_header) < 0) return BC_STATUS_IO_ERR;
				do_exit = true;
				break;
			}

#ifdef DC_CONFIG
			case 'x':
			{
				if (bc) return BC_STATUS_INVALID_OPTION;
				(*flags) |= BC_FLAG_X;
				break;
			}
#endif // DC_CONFIG

			// Getopt printed an error message, but we should exit.
			case '?':
			default:
			{
				return BC_STATUS_INVALID_OPTION;
			}
		}
	}

	if (do_exit) exit((int) s);

	if (exprs->len > 1) (*flags) |= BC_FLAG_Q;

	if (argv[optind] && strcmp(argv[optind], "--") == 0) ++optind;

	for (i = optind; !s && i < argc; ++i) s = bc_vec_push(files, argv + i);

	return s;
}
