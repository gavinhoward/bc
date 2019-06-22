/*
 * *****************************************************************************
 *
 * Copyright (c) 2018-2019 Gavin D. Howard and contributors.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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

static BcStatus dc_parse_register(BcParse *p, bool var) {

	BcStatus s;

	s = bc_lex_next(&p->l);
	if (BC_ERR(s)) return s;
	if (p->l.t != BC_LEX_NAME) return bc_parse_err(p, BC_ERROR_PARSE_TOKEN);

	bc_parse_pushName(p, p->l.str.v, var);

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
		s = dc_parse_register(p, inst != BC_INST_ARRAY_ELEM);
		if (BC_ERR(s)) return s;
	}

	if (store) {
		bc_parse_push(p, BC_INST_SWAP);
		bc_parse_push(p, BC_INST_ASSIGN_NO_VAL);
	}

	return bc_lex_next(&p->l);
}

static BcStatus dc_parse_cond(BcParse *p, uchar inst) {

	BcStatus s;

	bc_parse_push(p, inst);
	bc_parse_push(p, BC_INST_EXEC_COND);

	s = dc_parse_register(p, true);
	if (BC_ERR(s)) return s;

	s = bc_lex_next(&p->l);
	if (BC_ERR(s)) return s;

	if (p->l.t == BC_LEX_KW_ELSE) {
		s = dc_parse_register(p, true);
		if (BC_ERR(s)) return s;
		s = bc_lex_next(&p->l);
	}
	else bc_parse_pushIndex(p, SIZE_MAX);

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
			inst = (uchar) (t - BC_LEX_OP_REL_EQ + BC_INST_REL_EQ);
			s = dc_parse_cond(p, inst);
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
		{
			if (dc_lex_negCommand(&p->l)) {
				bc_parse_push(p, BC_INST_NEG);
				get_token = true;
				break;
			}

			s = bc_lex_next(&p->l);
			if (BC_ERR(s)) return s;
		}
		// Fallthrough.
		case BC_LEX_NUMBER:
		{
			bc_parse_number(p);

			if (t == BC_LEX_NEG) bc_parse_push(p, BC_INST_NEG);
			get_token = true;

			break;
		}

		case BC_LEX_KW_READ:
		{
			if (BC_ERR(flags & BC_PARSE_NOREAD))
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

	if (BC_NO_ERR(!s) && get_token) s = bc_lex_next(&p->l);

	return s;
}

BcStatus dc_parse_expr(BcParse *p, uint8_t flags) {

	BcStatus s = BC_STATUS_SUCCESS;
	BcInst inst;
	BcLexType t;
	bool have_expr = false, need_expr = (flags & BC_PARSE_NOREAD) != 0;

	while (BC_NO_SIG && BC_NO_ERR(!s) && (t = p->l.t) != BC_LEX_EOF) {

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

		have_expr = true;
	}

	if (BC_NO_ERR(!s)) {
		if (BC_SIG) s = BC_STATUS_SIGNAL;
		else if (BC_ERR(need_expr && !have_expr))
			s = bc_vm_err(BC_ERROR_EXEC_READ_EXPR);
		else if (p->l.t == BC_LEX_EOF && (flags & BC_PARSE_NOCALL))
			bc_parse_push(p, BC_INST_POP_EXEC);
	}

	return s;
}

BcStatus dc_parse_parse(BcParse *p) {

	BcStatus s;

	assert(p != NULL);

	if (BC_ERR(p->l.t == BC_LEX_EOF)) s = bc_parse_err(p, BC_ERROR_PARSE_EOF);
	else s = dc_parse_expr(p, 0);

	if (BC_ERR(s) || BC_SIG) s = bc_parse_reset(p, s);

	return s;
}
#endif // DC_ENABLED
