/*
 * *****************************************************************************
 *
 * Copyright (c) 2018-2019 Gavin D. Howard and contributors.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
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
	{ "no-prompt", no_argument, NULL, 'P' },
#if BC_ENABLED
	{ "global-stacks", no_argument, NULL, 'g' },
	{ "mathlib", no_argument, NULL, 'l' },
	{ "quiet", no_argument, NULL, 'q' },
	{ "standard", no_argument, NULL, 's' },
	{ "warn", no_argument, NULL, 'w' },
#endif // BC_ENABLED
	{ "version", no_argument, NULL, 'v' },
#if DC_ENABLED
	{ "extended-register", no_argument, NULL, 'x' },
#endif // DC_ENABLED
	{ 0, 0, 0, 0 },

};

#if !BC_ENABLED
static const char* const bc_args_opt = "e:f:hiPvVx";
#elif !DC_ENABLED
static const char* const bc_args_opt = "e:f:ghilPqsvVw";
#else // BC_ENABLED && DC_ENABLED
static const char* const bc_args_opt = "e:f:ghilPqsvVwx";
#endif // BC_ENABLED && DC_ENABLED

static void bc_args_exprs(BcVec *exprs, const char *str) {
	bc_vec_concat(exprs, str);
	bc_vec_concat(exprs, "\n");
}

static BcStatus bc_args_file(BcVec *exprs, const char *file) {

	BcStatus s;
	char *buf;

	vm->file = file;

	s = bc_read_file(file, &buf);
	if (BC_ERR(s)) return s;

	bc_args_exprs(exprs, buf);
	free(buf);

	return s;
}

BcStatus bc_args(int argc, char *argv[]) {

	BcStatus s = BC_STATUS_SUCCESS;
	int c, i, err = 0;
	bool do_exit = false, version = false;

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
				if (BC_ERR(s)) return s;
				break;
			}

			case 'h':
			{
				bc_vm_info(vm->help);
				do_exit = true;
				break;
			}

			case 'i':
			{
				vm->flags |= BC_FLAG_I;
				break;
			}

			case 'P':
			{
				vm->flags |= BC_FLAG_P;
				break;
			}

#if BC_ENABLED
			case 'g':
			{
				if (BC_ERR(!BC_IS_BC)) err = c;
				vm->flags |= BC_FLAG_G;
				break;
			}

			case 'l':
			{
				if (BC_ERR(!BC_IS_BC)) err = c;
				vm->flags |= BC_FLAG_L;
				break;
			}

			case 'q':
			{
				if (BC_ERR(!BC_IS_BC)) err = c;
				vm->flags |= BC_FLAG_Q;
				break;
			}

			case 's':
			{
				if (BC_ERR(!BC_IS_BC)) err = c;
				vm->flags |= BC_FLAG_S;
				break;
			}

			case 'w':
			{
				if (BC_ERR(!BC_IS_BC)) err = c;
				vm->flags |= BC_FLAG_W;
				break;
			}
#endif // BC_ENABLED

			case 'V':
			case 'v':
			{
				do_exit = version = true;
				break;
			}

#if DC_ENABLED
			case 'x':
			{
				if (BC_ERR(BC_IS_BC)) err = c;
				vm->flags |= DC_FLAG_X;
				break;
			}
#endif // DC_ENABLED

			// Getopt printed an error message, but we should exit.
			case '?':
			default:
			{
				return BC_STATUS_ERROR_FATAL;
			}
		}

		if (BC_ERR(err)) {

			for (i = 0; bc_args_lopt[i].name != NULL; ++i) {
				if (bc_args_lopt[i].val == err) break;
			}

			return bc_vm_verr(BC_ERROR_FATAL_OPTION, err, bc_args_lopt[i].name);
		}
	}

	if (version) bc_vm_info(NULL);
	if (do_exit) exit((int) s);
	if (vm->exprs.len > 1 || !BC_IS_BC) vm->flags |= BC_FLAG_Q;
	if (argv[optind] != NULL && !strcmp(argv[optind], "--")) ++optind;

	for (i = optind; i < argc; ++i) bc_vec_push(&vm->files, argv + i);

	return s;
}
