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
 * Code common to the parsers.
 *
 */

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <limits.h>

#include <status.h>
#include <vector.h>
#include <lex.h>
#include <parse.h>
#include <program.h>
#include <vm.h>

void bc_parse_updateFunc(BcParse *p, size_t fidx) {
	p->fidx = fidx;
	p->func = bc_vec_item(&p->prog->fns, fidx);
}

void bc_parse_pushName(const BcParse *p, char *name, bool var) {
	size_t idx = bc_program_search(p->prog, name, var);
	bc_parse_pushIndex(p, idx);
}

void bc_parse_addId(BcParse *p, const char *string, uchar inst) {

	BcFunc *f = BC_IS_BC ? p->func : bc_vec_item(&p->prog->fns, BC_PROG_MAIN);
	BcVec *v = inst == BC_INST_NUM ? &f->consts : &f->strs;
	size_t idx = v->len;

	if (inst == BC_INST_NUM) {

		if (bc_parse_one[0] == string[0] && bc_parse_one[1] == string[1]) {
			bc_parse_push(p, BC_INST_ONE);
			return;
		}
		else {

			BcConst c;
			char *str = bc_vm_strdup(string);

			c.val = str;
			c.base = BC_NUM_BIGDIG_MAX;

			memset(&c.num, 0, sizeof(BcNum));
			bc_vec_push(v, &c);
		}
	}
	else {
		char *str = bc_vm_strdup(string);
		bc_vec_push(v, &str);
	}

	bc_parse_updateFunc(p, p->fidx);
	bc_parse_push(p, inst);
	bc_parse_pushIndex(p, idx);
}

void bc_parse_number(BcParse *p) {

#if BC_ENABLE_EXTRA_MATH
	char *exp = strchr(p->l.str.v, 'e');
	size_t idx = SIZE_MAX;

	if (exp != NULL) {
		idx = ((size_t) (exp - p->l.str.v));
		*exp = 0;
	}
#endif // BC_ENABLE_EXTRA_MATH

	bc_parse_addId(p, p->l.str.v, BC_INST_NUM);

#if BC_ENABLE_EXTRA_MATH
	if (exp != NULL) {

		bool neg;

		neg = (*((char*) bc_vec_item(&p->l.str, idx + 1)) == BC_LEX_NEG_CHAR);

		bc_parse_addId(p, bc_vec_item(&p->l.str, idx + 1 + neg), BC_INST_NUM);
		bc_parse_push(p, BC_INST_LSHIFT + neg);
	}
#endif // BC_ENABLE_EXTRA_MATH
}

BcStatus bc_parse_text(BcParse *p, const char *text) {
	// Make sure the pointer isn't invalidated.
	p->func = bc_vec_item(&p->prog->fns, p->fidx);
	return bc_lex_text(&p->l, text);
}

BcStatus bc_parse_reset(BcParse *p, BcStatus s) {

	if (p->fidx != BC_PROG_MAIN) {
		bc_func_reset(p->func);
		bc_parse_updateFunc(p, BC_PROG_MAIN);
	}

	p->l.i = p->l.len;
	p->l.t = BC_LEX_EOF;
	p->auto_part = false;

#if BC_ENABLED
	if (BC_IS_BC) {
		bc_vec_npop(&p->flags, p->flags.len - 1);
		bc_vec_npop(&p->exits, p->exits.len);
		bc_vec_npop(&p->conds, p->conds.len);
		bc_vec_npop(&p->ops, p->ops.len);
	}
#endif // BC_ENABLED

	return bc_program_reset(p->prog, s);
}

void bc_parse_free(BcParse *p) {

	assert(p != NULL);

#if BC_ENABLED
	if (BC_IS_BC) {
		bc_vec_free(&p->flags);
		bc_vec_free(&p->exits);
		bc_vec_free(&p->conds);
		bc_vec_free(&p->ops);
		bc_vec_free(&p->buf);
	}
#endif // BC_ENABLED

	bc_lex_free(&p->l);
}

void bc_parse_init(BcParse *p, BcProgram *prog, size_t func) {

#if BC_ENABLED
	uint16_t flag = 0;
#endif // BC_ENABLED

	assert(p != NULL && prog != NULL);

#if BC_ENABLED
	if (BC_IS_BC) {
		bc_vec_init(&p->flags, sizeof(uint16_t), NULL);
		bc_vec_push(&p->flags, &flag);
		bc_vec_init(&p->exits, sizeof(BcInstPtr), NULL);
		bc_vec_init(&p->conds, sizeof(size_t), NULL);
		bc_vec_init(&p->ops, sizeof(BcLexType), NULL);
		bc_vec_init(&p->buf, sizeof(char), NULL);
	}
#endif // BC_ENABLED

	bc_lex_init(&p->l);

	p->prog = prog;
	p->auto_part = false;
	bc_parse_updateFunc(p, func);
}
