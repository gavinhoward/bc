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
 * The parser for dc.
 *
 */

#include <assert.h>

#include <status.h>
#include <parse.h>
#include <program.h>
#include <vm.h>

BcStatus dc_parse_push(BcParse *p, BcVec *code, BcInst inst) {
	BcStatus s;
	if (p->nbraces >= bc_inst_operands[inst]) s = bc_vec_pushByte(code, inst);
	else s = BC_STATUS_PARSE_BAD_EXP;
	return s;
}

BcStatus dc_parse_expr(BcParse *p, BcVec *code, uint8_t flags, BcParseNext next)
{
	BcStatus s;
	BcLexType t = p->lex.t.t;
	BcInst prev;
	BcInst inst;

	(void) flags;
	(void) next;

	for (; t != BC_LEX_EOF; t = p->lex.t.t) {

		if ((inst = dc_parse_insts[t]) != BC_INST_INVALID) {
			s = dc_parse_push(p, code, inst);
		}
		else {

			switch (t) {

				case BC_LEX_NUMBER:
				{
					s = bc_parse_number(p, code, &prev, &p->nbraces);
					break;
				}

				default:
				{
					s = BC_STATUS_PARSE_BAD_TOKEN;
					break;
				}
			}
		}

		if (!s) s = bc_lex_next(&p->lex);
	}

	return s;
}

BcStatus dc_parse_parse(BcParse *p) {

	BcStatus s;
	BcFunc *func = bc_vec_item(&p->prog->fns, p->func);

	assert(p);

	if (p->lex.t.t == BC_LEX_EOF) s = BC_STATUS_LEX_EOF;
	else s = dc_parse_expr(p, &func->code, 0, bc_parse_next_read);

	if (s || bcg.signe) s = bc_parse_reset(p, s);

	return s;
}

BcStatus dc_parse_init(BcParse *p, BcProgram *prog) {
	return bc_parse_create(p, prog, dc_parse_parse, dc_lex_token);
}
