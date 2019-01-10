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

	BcStatus s;

	vm->read_ret = BC_INST_POP_EXEC;
	vm->help = dc_help;
#if BC_ENABLE_SIGNALS
	vm->sig_msg = dc_sig_msg;
	vm->sig_len = (uchar) strlen(vm->sig_msg);
#endif // BC_ENABLE_SIGNALS

	vm->next = dc_lex_token;
	vm->parse = dc_parse_parse;
	vm->expr = dc_parse_expr;

	s = bc_vm_boot(argc, argv, "DC_LINE_LENGTH");

	return (int) s;
}
#endif // DC_ENABLED
