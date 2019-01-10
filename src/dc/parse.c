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

#if DC_ENABLED

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <status.h>
#include <parse.h>
#include <dc.h>
#include <program.h>
#include <vm.h>

static BcStatus dc_parse_register(BcParse *p) {

	BcStatus s;

	s = bc_lex_next(&p->l);
	if (s) return s;
	if (p->l.t != BC_LEX_NAME) return bc_parse_err(p, BC_ERROR_PARSE_TOKEN);

	bc_parse_pushName(p, p->l.str.v);

	return s;
}

static BcStatus dc_parse_string(BcParse *p) {

	BcFunc f;

	bc_program_addFunc(p->prog, &f, bc_func_main);
	bc_parse_string(p);

	return bc_lex_next(&p->l);
}

static BcStatus dc_parse_mem(BcParse *p, uchar inst, bool name, bool store) {

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

static BcStatus dc_parse_cond(BcParse *p, uchar inst) {

	BcStatus s;

	bc_parse_push(p, inst);
	bc_parse_push(p, BC_INST_EXEC_COND);

	s = dc_parse_register(p);
	if (s) return s;

	s = bc_lex_next(&p->l);
	if (s) return s;

	if (p->l.t == BC_LEX_KEY_ELSE) {
		s = dc_parse_register(p);
		if (s) return s;
		s = bc_lex_next(&p->l);
	}
	else bc_parse_push(p, BC_PARSE_STREND);

	return s;
}

static BcStatus dc_parse_token(BcParse *p, BcLexType t, uint8_t flags) {

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
			s = dc_parse_cond(p, (uchar) (t - BC_LEX_OP_REL_EQ + BC_INST_REL_EQ));
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
				if (p->l.t != BC_LEX_NUMBER)
					return bc_parse_err(p, BC_ERROR_PARSE_TOKEN);
			}

			bc_parse_number(p);

			if (t == BC_LEX_NEG) bc_parse_push(p, BC_INST_NEG);
			get_token = true;

			break;
		}

		case BC_LEX_KEY_READ:
		{
			if (flags & BC_PARSE_NOREAD)
				s = bc_parse_err(p, BC_ERROR_EXEC_REC_READ);
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
		case BC_LEX_STORE_OBASE:
		case BC_LEX_STORE_SCALE:
		{
			inst = (uchar) (t - BC_LEX_STORE_IBASE + BC_INST_IBASE);
			s = dc_parse_mem(p, inst, false, true);
			break;
		}

		default:
		{
			s = bc_parse_err(p, BC_ERROR_PARSE_TOKEN);
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

	while (!BC_SIGNAL && !s && (t = p->l.t) != BC_LEX_EOF) {

		if (t == BC_LEX_NLINE) {
			s = bc_lex_next(&p->l);
			continue;
		}

		inst = dc_parse_insts[t];

		if (inst != BC_INST_INVALID) {
			bc_parse_push(p, inst);
			s = bc_lex_next(&p->l);
		}
		else s = dc_parse_token(p, t, flags);
	}

	if (!s) {
		if (BC_SIGNAL) s = BC_STATUS_SIGNAL;
		else if (p->l.t == BC_LEX_EOF && (flags & BC_PARSE_NOCALL))
			bc_parse_push(p, BC_INST_POP_EXEC);
	}

	return s;
}

BcStatus dc_parse_parse(BcParse *p) {

	BcStatus s;

	assert(p);

	if (p->l.t == BC_LEX_EOF) s = bc_parse_err(p, BC_ERROR_PARSE_EOF);
	else s = dc_parse_expr(p, 0);

	if (s || BC_SIGNAL) s = bc_parse_reset(p, s);

	return s;
}
#endif // DC_ENABLED
