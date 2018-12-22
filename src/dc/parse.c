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
#include <dc.h>
#include <program.h>
#include <vm.h>

#if DC_ENABLED
BcStatus dc_parse_register(BcParse *p) {

	BcStatus s;
	char *name;

	s = bc_lex_next(&p->l);
	if (s) return s;
	if (p->l.t.t != BC_LEX_NAME)
		return bc_vm_error(BC_ERROR_PARSE_TOKEN, p->l.line);

	name = bc_vm_strdup(p->l.t.v.v);
	bc_parse_pushName(p, name);

	return s;
}

BcStatus dc_parse_string(BcParse *p) {

	BcFunc f;

	bc_program_addFunc(p->prog, &f);
	bc_parse_string(p);

	return bc_lex_next(&p->l);
}

BcStatus dc_parse_mem(BcParse *p, uchar inst, bool name, bool store) {

	BcStatus s;

	bc_parse_push(p, inst);
	if (name) {
		s = dc_parse_register(p);
		if (s) return s;
	}

	if (store) {
		bc_parse_push(p, BC_INST_SWAP);
		bc_parse_push(p, BC_INST_ASSIGN);
		bc_parse_push(p, BC_INST_POP);
	}

	return bc_lex_next(&p->l);
}

BcStatus dc_parse_cond(BcParse *p, uchar inst) {

	BcStatus s;

	bc_parse_push(p, inst);
	bc_parse_push(p, BC_INST_EXEC_COND);

	s = dc_parse_register(p);
	if (s) return s;

	s = bc_lex_next(&p->l);
	if (s) return s;

	if (p->l.t.t == BC_LEX_ELSE) {
		s = dc_parse_register(p);
		if (s) return s;
		s = bc_lex_next(&p->l);
	}
	else bc_parse_push(p, BC_PARSE_STREND);

	return s;
}

BcStatus dc_parse_token(BcParse *p, BcLexType t, uint8_t flags) {

	BcStatus s = BC_STATUS_SUCCESS;
	uchar inst;
	bool assign, get_token = false;

	switch (t) {

		case BC_LEX_OP_REL_EQ:
		case BC_LEX_OP_REL_LE:
		case BC_LEX_OP_REL_GE:
		case BC_LEX_OP_REL_NE:
		case BC_LEX_OP_REL_LT:
		case BC_LEX_OP_REL_GT:
		{
			s = dc_parse_cond(p, t - BC_LEX_OP_REL_EQ + BC_INST_REL_EQ);
			break;
		}

		case BC_LEX_SCOLON:
		case BC_LEX_COLON:
		{
			s = dc_parse_mem(p, BC_INST_ARRAY_ELEM, true, t == BC_LEX_COLON);
			break;
		}

		case BC_LEX_STR:
		{
			s = dc_parse_string(p);
			break;
		}

		case BC_LEX_NEG:
		case BC_LEX_NUMBER:
		{
			if (t == BC_LEX_NEG) {
				s = bc_lex_next(&p->l);
				if (s) return s;
				if (p->l.t.t != BC_LEX_NUMBER)
					return bc_vm_error(BC_ERROR_PARSE_TOKEN, p->l.line);
			}

			bc_parse_number(p);

			if (t == BC_LEX_NEG) bc_parse_push(p, BC_INST_NEG);
			get_token = true;

			break;
		}

		case BC_LEX_KEY_READ:
		{
			if (flags & BC_PARSE_NOREAD)
				s = bc_vm_error(BC_ERROR_EXEC_REC_READ, p->l.line);
			else bc_parse_push(p, BC_INST_READ);
			get_token = true;
			break;
		}

		case BC_LEX_OP_ASSIGN:
		case BC_LEX_STORE_PUSH:
		{
			assign = t == BC_LEX_OP_ASSIGN;
			inst = assign ? BC_INST_VAR : BC_INST_PUSH_TO_VAR;
			s = dc_parse_mem(p, inst, true, assign);
			break;
		}

		case BC_LEX_LOAD:
		case BC_LEX_LOAD_POP:
		{
			inst = t == BC_LEX_LOAD_POP ? BC_INST_PUSH_VAR : BC_INST_LOAD;
			s = dc_parse_mem(p, inst, true, false);
			break;
		}

		case BC_LEX_STORE_IBASE:
		case BC_LEX_STORE_SCALE:
		case BC_LEX_STORE_OBASE:
		{
			inst = t - BC_LEX_STORE_IBASE + BC_INST_IBASE;
			s = dc_parse_mem(p, inst, false, true);
			break;
		}

		default:
		{
			s = bc_vm_error(BC_ERROR_PARSE_TOKEN, p->l.line);
			get_token = true;
			break;
		}
	}

	if (!s && get_token) s = bc_lex_next(&p->l);

	return s;
}

BcStatus dc_parse_expr(BcParse *p, uint8_t flags) {

	BcStatus s = BC_STATUS_SUCCESS;
	BcInst inst;
	BcLexType t;

	for (t = p->l.t.t; !BC_SIGINT && !s && t != BC_LEX_EOF; t = p->l.t.t) {

		inst = dc_parse_insts[t];

		if (inst != BC_INST_INVALID) {
			bc_parse_push(p, inst);
			s = bc_lex_next(&p->l);
		}
		else s = dc_parse_token(p, t, flags);
	}

	if (!s) {
		if (BC_SIGINT) s = BC_STATUS_SIGNAL;
		else if (p->l.t.t == BC_LEX_EOF && (flags & BC_PARSE_NOCALL))
			bc_parse_push(p, BC_INST_POP_EXEC);
	}

	return s;
}

BcStatus dc_parse_parse(BcParse *p) {

	BcStatus s;

	assert(p);

	if (p->l.t.t == BC_LEX_EOF) s = bc_vm_error(BC_ERROR_PARSE_EOF, p->l.line);
	else s = dc_parse_expr(p, 0);

	if (s || BC_SIGINT) s = bc_parse_reset(p, s);

	return s;
}

void dc_parse_init(BcParse *p, BcProgram *prog, size_t func) {
	assert(p && prog);
	bc_parse_create(p, prog, func, dc_parse_parse, dc_lex_token);
}
#endif // DC_ENABLED
