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

BcVm *vm;

int main(int argc, char *argv[]) {

	int s;
	char *name;

	setlocale(LC_ALL, "");

	vm = calloc(1, sizeof(BcVm));
	if (!vm) return (int) bc_vm_err(BC_ERROR_VM_ALLOC_ERR);

	name = strrchr(argv[0], '/');
	vm->name = !name ? argv[0] : name + 1;

#if !DC_ENABLED
	s = bc_main(argc, argv);
#elif !BC_ENABLED
	s = dc_main(argc, argv);
#else
	if (BC_IS_BC) s = bc_main(argc, argv);
	else s = dc_main(argc, argv);
#endif

	return s;
}
