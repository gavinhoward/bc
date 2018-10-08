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

BcStatus bc_args_env(unsigned int *flags, BcVec *exprs, BcVec *files) {

	BcStatus s;
	BcVec args;
	char *env_args = NULL, *buffer = NULL, *buf;

	if (!(env_args = getenv(bc_args_env_name))) return BC_STATUS_SUCCESS;

	if ((s = bc_vec_init(&args, sizeof(char*), NULL))) return s;
	if ((s = bc_vec_push(&args, &bc_args_env_name))) goto push_err;

	if (!(buf = (buffer = strdup(env_args)))) {
		s = BC_STATUS_ALLOC_ERR;
		goto push_err;
	}

	while (*buf) {
		if (!isspace(*buf)) {
			if ((s = bc_vec_push(&args, &buf))) goto err;
			while (*buf && !isspace(*buf)) ++buf;
			if (*buf) (*(buf++)) = '\0';
		}
		else ++buf;
	}

	s = bc_args((int) args.len, (char**) args.v, flags, exprs, files);

err:
	free(buffer);
push_err:
	bc_vec_free(&args);
	return s;
}

BcStatus bc_args(int argc, char *argv[], unsigned int *flags,
                 BcVec *exprs, BcVec *files)
{
	BcStatus s = BC_STATUS_SUCCESS;
	int c, i, idx;
	bool do_exit = false;

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
				if ((s = bc_vm_info(bcg.help))) return s;
				do_exit = true;
				break;
			}

#ifdef BC_ENABLED
			case 'i':
			{
				if (!bcg.bc) return BC_STATUS_INVALID_OPTION;
				(*flags) |= BC_FLAG_I;
				break;
			}

			case 'l':
			{
				if (!bcg.bc) return BC_STATUS_INVALID_OPTION;
				(*flags) |= BC_FLAG_L;
				break;
			}

			case 'q':
			{
				if (!bcg.bc) return BC_STATUS_INVALID_OPTION;
				(*flags) |= BC_FLAG_Q;
				break;
			}

			case 's':
			{
				if (!bcg.bc) return BC_STATUS_INVALID_OPTION;
				(*flags) |= BC_FLAG_S;
				break;
			}

			case 'w':
			{
				if (!bcg.bc) return BC_STATUS_INVALID_OPTION;
				(*flags) |= BC_FLAG_W;
				break;
			}
#endif // BC_ENABLED

			case 'V':
			case 'v':
			{
				if ((s = bc_vm_info(NULL))) return s;
				do_exit = true;
				break;
			}

#ifdef DC_ENABLED
			case 'x':
			{
				if (bcg.bc) return BC_STATUS_INVALID_OPTION;
				(*flags) |= BC_FLAG_X;
				break;
			}
#endif // DC_ENABLED

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
