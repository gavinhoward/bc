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

	size_t i = 0, len = strlen(name);

	for (; i < len; ++i) bc_parse_push(p, name[i]);
	bc_parse_push(p, BC_PARSE_STREND);
}

void bc_parse_pushIndex(BcParse *p, size_t idx) {
	bc_vec_pushIndex(&p->func->code, idx);
}

void bc_parse_addId(BcParse *p, uchar inst) {

	BcFunc *f = BC_IS_BC ? p->func : bc_vec_item(&p->prog->fns, BC_PROG_MAIN);
	BcVec *v = inst == BC_INST_NUM ? &f->consts : &f->strs;
	size_t idx = v->len;
	char *str = bc_vm_strdup(p->l.str.v);

	bc_vec_push(v, &str);
	bc_parse_updateFunc(p, p->fidx);
	bc_parse_push(p, inst);
	bc_parse_pushIndex(p, idx);
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

	bc_vec_npop(&p->flags, p->flags.len - 1);
#if BC_ENABLED
	bc_vec_npop(&p->exits, p->exits.len);
	bc_vec_npop(&p->conds, p->conds.len);
	bc_vec_npop(&p->ops, p->ops.len);
#endif // BC_ENABLED

	return bc_program_reset(p->prog, s);
}

void bc_parse_free(BcParse *p) {
	assert(p);
	bc_vec_free(&p->flags);
#if BC_ENABLED
	bc_vec_free(&p->exits);
	bc_vec_free(&p->conds);
	bc_vec_free(&p->ops);
#endif // BC_ENABLED
	bc_lex_free(&p->l);
}

void bc_parse_create(BcParse *p, BcProgram *prog, size_t func,
                     BcParseParse parse, BcLexNext next)
{
	uint16_t flag = 0;

	assert(p && prog);

	memset(p, 0, sizeof(BcParse));

	bc_lex_init(&p->l, next);
	bc_vec_init(&p->flags, sizeof(uint16_t), NULL);
	bc_vec_push(&p->flags, &flag);
#if BC_ENABLED
	bc_vec_init(&p->exits, sizeof(BcInstPtr), NULL);
	bc_vec_init(&p->conds, sizeof(size_t), NULL);
	bc_vec_init(&p->ops, sizeof(BcLexType), NULL);
#endif // BC_ENABLED

	p->parse = parse;
	p->prog = prog;
	p->auto_part = false;
	bc_parse_updateFunc(p, func);
}
