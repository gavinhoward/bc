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

#if DC_ENABLED

#include <string.h>

#include <status.h>
#include <dc.h>
#include <vm.h>

int dc_main(int argc, char **argv) {

	vm->help = dc_help;
#if BC_ENABLE_SIGNALS
	vm->sig_msg = dc_sig_msg;
	vm->sig_len = strlen(vm->sig_msg);
#endif // BC_ENABLE_SIGNALS

	vm->parse_init = dc_parse_init;
	vm->parse_expr = dc_parse_expr;

	return (int) bc_vm_boot(argc, argv, "DC_LINE_LENGTH");
}
#endif // DC_ENABLED
