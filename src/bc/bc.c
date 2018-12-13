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
 * The main procedure of bc.
 *
 */

#include <status.h>
#include <bc.h>
#include <vm.h>

#if BC_ENABLED
int bc_main(int argc, char **argv) {

	vm->help = bc_help;
#if BC_ENABLE_SIGNALS
	vm->sig_msg = "\ninterrupt (type \"quit\" to exit)\n";
#endif // BC_ENABLE_SIGNALS

	vm->parse_init = bc_parse_init;
	vm->parse_expr = bc_parse_expression;

	return (int) bc_vm_boot(argc, argv, "BC_LINE_LENGTH");
}
#endif // BC_ENABLED
