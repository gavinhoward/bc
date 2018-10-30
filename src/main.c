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
 * The entry point for bc.
 *
 */

#include <stdlib.h>
#include <string.h>

#include <locale.h>
#include <libgen.h>

#include <status.h>
#include <vm.h>
#include <bc.h>
#include <dc.h>

BcGlobals bcg;

int main(int argc, char *argv[]) {

	BcStatus s;
	char *name;

	setlocale(LC_ALL, "");
	memset(&bcg, 0, sizeof(BcGlobals));

	name = bc_vm_strdup(argv[0]);
	bcg.name = basename(name);

#if !defined(DC_ENABLED)
	s = bc_main(argc, argv);
#elif !defined(BC_ENABLED)
	s = dc_main(argc, argv);
#else
	if (!strncmp(bcg.name, dc_name, strlen(dc_name))) s = dc_main(argc, argv);
	else s = bc_main(argc, argv);
#endif

	free(name);

	return (int) s;
}
