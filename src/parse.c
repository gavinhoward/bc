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

void bc_parse_pushName(BcParse *p, char *name) {
	bc_vec_npush(&p->func->code, strlen(name), name);
	bc_parse_push(p, BC_PARSE_STREND);
}

void bc_parse_pushIndex(BcParse *p, size_t idx) {
	bc_vec_pushIndex(&p->func->code, idx);
}

void bc_parse_addId(BcParse *p, const char *string, uchar inst) {

	BcFunc *f = BC_IS_BC ? p->func : bc_vec_item(&p->prog->fns, BC_PROG_MAIN);
	BcVec *v = inst == BC_INST_NUM ? &f->consts : &f->strs;
	size_t idx = v->len;
	char *str = bc_vm_strdup(string);

	bc_vec_push(v, &str);
	bc_parse_updateFunc(p, p->fidx);
	bc_parse_push(p, inst);
	bc_parse_pushIndex(p, idx);
}

void bc_parse_number(BcParse *p) {

#if BC_ENABLE_EXTRA_MATH
	char *exp = strchr(p->l.str.v, 'e');
	size_t idx = SIZE_MAX;

	if (exp) {
		idx = ((uintptr_t) exp) - ((uintptr_t) p->l.str.v);
		*exp = 0;
	}
#endif // BC_ENABLE_EXTRA_MATH

	bc_parse_addId(p, p->l.str.v, BC_INST_NUM);

#if BC_ENABLE_EXTRA_MATH
	if (exp) {

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
	bc_vec_npop(&p->flags, p->flags.len - 1);
	bc_vec_npop(&p->exits, p->exits.len);
	bc_vec_npop(&p->conds, p->conds.len);
	bc_vec_npop(&p->ops, p->ops.len);
#endif // BC_ENABLED

	return bc_program_reset(p->prog, s);
}

void bc_parse_free(BcParse *p) {
	assert(p);
#if BC_ENABLED
	bc_vec_free(&p->flags);
	bc_vec_free(&p->exits);
	bc_vec_free(&p->conds);
	bc_vec_free(&p->ops);
#endif // BC_ENABLED
	bc_lex_free(&p->l);
}

void bc_parse_init(BcParse *p, BcProgram *prog, size_t func)
{
	assert(p && prog);
#if BC_ENABLED
	uint16_t flag = 0;
	bc_vec_init(&p->flags, sizeof(uint16_t), NULL);
	bc_vec_push(&p->flags, &flag);
	bc_vec_init(&p->exits, sizeof(BcInstPtr), NULL);
	bc_vec_init(&p->conds, sizeof(size_t), NULL);
	bc_vec_init(&p->ops, sizeof(BcLexType), NULL);
#endif // BC_ENABLED

	bc_lex_init(&p->l);

	p->prog = prog;
	p->auto_part = false;
	bc_parse_updateFunc(p, func);
}
