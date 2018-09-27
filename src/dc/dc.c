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
 * The main procedure of dc.
 *
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <status.h>
#include <vector.h>
#include <dc.h>
#include <vm.h>
#include <args.h>

BcStatus dc_main(int argc, char *argv[]) {

	BcStatus s;
	BcVec files, args;
	unsigned int flags;
	char *env_args, *buffer, *buf;

	flags = 0;
	buffer = NULL;

	if ((s = bc_vec_init(&files, sizeof(char*), NULL))) return (int) s;

	if ((env_args = getenv(bc_args_env_name))) {

		if ((s = bc_vec_init(&args, sizeof(char*), NULL))) goto err;
		if ((s = bc_vec_push(&args, 1, &bc_args_env_name))) goto args_err;

		if (!(buffer = strdup(env_args))) {
			s = BC_STATUS_ALLOC_ERR;
			goto args_err;
		}

		buf = buffer;

		while (*buf) {

			if (!isspace(*buf)) {

				if ((s = bc_vec_push(&args, 1, &buf))) goto buf_err;

				while (*buf && !isspace(*buf)) ++buf;

				if (*buf) (*(buf++)) = '\0';
			}
			else ++buf;
		}

		s = bc_args((int) args.len, (char**) args.vec, dc_help, &flags, &files);
		if(s) goto buf_err;
	}

	if((s = bc_args(argc, argv, dc_help, &flags, &files))) goto buf_err;

	flags |= BC_FLAG_S * (getenv("POSIXLY_CORRECT") != NULL);

	s = bc_vm_exec(flags, &files);

buf_err:

	if (env_args) free(buffer);

args_err:

	if (env_args) bc_vec_free(&args);

err:

	bc_vec_free(&files);

	return (int) s;
}
