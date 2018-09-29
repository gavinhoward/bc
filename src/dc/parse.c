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
#include <stdlib.h>
#include <string.h>

#include <status.h>
#include <parse.h>
#include <program.h>
#include <vm.h>

BcStatus dc_parse_push(BcParse *p, BcVec *code, BcInst inst) {
	BcStatus s;
	if (p->nbraces >= bc_inst_noperands[inst]) {
		p->nbraces -= bc_inst_noperands[inst] - bc_inst_nresults[inst];
		s = bc_vec_pushByte(code, inst);
	}
	else s = BC_STATUS_PARSE_BAD_EXP;
	return s;
}

BcStatus dc_parse_string(BcParse *p, BcVec *code) {

	BcStatus s;
	char *str;
	char buf[DC_PARSE_BUF_SIZE + 1];
	size_t idx, len = p->prog->strs.len;

	if (sprintf(buf, "%*zu", DC_PARSE_BUF_SIZE, len) < 0)
		return BC_STATUS_IO_ERR;

	if (!(str = strdup(p->lex.t.v.vec))) return BC_STATUS_ALLOC_ERR;
	if ((s = bc_vec_pushByte(code, BC_INST_STR))) goto err;
	if ((s = bc_parse_pushIndex(code, len))) goto err;
	if ((s = bc_vec_push(&p->prog->strs, 1, &str))) goto err;

	if ((s = bc_program_addFunc(p->prog, buf, &idx))) return s;

	assert(idx == len + 2);

	return s;

err:
	free(str);
	return s;
}

BcStatus dc_parse_mem(BcParse *p, BcVec *code, uint8_t inst,
                      bool name, bool store, bool push_pop)
{
	BcStatus s;
	char *str;

	if ((s = bc_vec_push(code, 1, &inst))) return s;

	if (name) {
		if ((s = bc_lex_next(&p->lex))) return s;
		if (p->lex.t.t != BC_LEX_NAME) return BC_STATUS_PARSE_BAD_TOKEN;
		if (!(str = strdup(p->lex.t.v.vec))) return BC_STATUS_ALLOC_ERR;
		if ((s = bc_parse_pushName(code, str))) goto err;
	}

	if (store) {

		if ((s = bc_vec_pushByte(code, BC_INST_SWAP))) return s;

		if (push_pop) {
			if ((s = bc_vec_pushByte(code, BC_INST_PUSH_VAR))) return s;
		}
		else {
			if ((s = bc_vec_pushByte(code, BC_INST_ASSIGN))) return s;
			s = bc_vec_pushByte(code, BC_INST_POP);
		}
	}
	else if (push_pop) s = bc_vec_pushByte(code, BC_INST_POP_VAR);

	return s;

err:
	free(str);
	return s;
}

BcStatus dc_parse_token(BcParse *p, BcVec *code, BcLexType t) {

	BcStatus s = BC_STATUS_SUCCESS;
	BcInst prev;
	char *name;

	switch (t) {

		case BC_LEX_SCOLON:
		{
			// TODO
			break;
		}

		case BC_LEX_STRING:
		{
			s = dc_parse_string(p, code);
			break;
		}

		case BC_LEX_NUMBER:
		{
			s = bc_parse_number(p, code, &prev, &p->nbraces);
			break;
		}

		case BC_LEX_COLON:
		{
			// TODO
			break;
		}

		case BC_LEX_LOAD:
		case BC_LEX_LOAD_POP:
		case BC_LEX_STORE:
		case BC_LEX_STORE_PUSH:
		{
			bool store = t == BC_LEX_STORE || t == BC_LEX_STORE_PUSH;
			bool push_pop = t == BC_LEX_LOAD_POP || t == BC_LEX_STORE_PUSH;
			s = dc_parse_mem(p, code, BC_INST_VAR, true, store, push_pop);
			break;
		}

		case BC_LEX_STORE_IBASE:
		{
			s = dc_parse_mem(p, code, BC_INST_IBASE, false, true, false);
			break;
		}

		case BC_LEX_STORE_SCALE:
		{
			s = dc_parse_mem(p, code, BC_INST_SCALE, false, true, false);
			break;
		}

		case BC_LEX_STORE_OBASE:
		{
			s = dc_parse_mem(p, code, BC_INST_OBASE, false, true, false);
			break;
		}

		default:
		{
			s = BC_STATUS_PARSE_BAD_TOKEN;
			break;
		}
	}

	return s;
}

BcStatus dc_parse_expr(BcParse *p, BcVec *code, uint8_t flags, BcParseNext next)
{
	BcStatus s;
	BcInst inst;
	BcLexType t;

	(void) flags;
	(void) next;

	while ((t = p->lex.t.t) != BC_LEX_EOF) {

		if ((inst = dc_parse_insts[t]) != BC_INST_INVALID)
			s = dc_parse_push(p, code, inst);
		else s = dc_parse_token(p, code, t);

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
