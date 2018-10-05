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

BcStatus bc_parse_pushName(BcVec *code, char *name) {

	BcStatus s = BC_STATUS_SUCCESS;
	size_t i, len = strlen(name);

	for (i = 0; !s && i < len; ++i) s = bc_vec_pushByte(code, (char) name[i]);
	if (s || (s = bc_vec_pushByte(code, BC_PARSE_STREND))) return s;

	free(name);

	return s;
}

BcStatus bc_parse_pushIndex(BcVec *code, size_t idx) {

	BcStatus s;
	unsigned char amt, i, nums[sizeof(size_t)];

	for (amt = 0; idx; ++amt) {
		nums[amt] = (char) idx;
		idx = (idx & ((unsigned long) ~(UINT8_MAX))) >> sizeof(char) * CHAR_BIT;
	}

	if ((s = bc_vec_pushByte(code, amt))) return s;
	for (i = 0; !s && i < amt; ++i) s = bc_vec_pushByte(code, nums[i]);

	return s;
}

BcStatus bc_parse_number(BcParse *p, BcVec *code, BcInst *prev, size_t *nexs) {

	BcStatus s;
	char *num;
	size_t idx = p->prog->consts.len;

	if (!(num = strdup(p->l.t.v.v))) return BC_STATUS_ALLOC_ERR;

	if ((s = bc_vec_push(&p->prog->consts, &num))) {
		free(num);
		return s;
	}

	if ((s = bc_vec_pushByte(code, BC_INST_NUM))) return s;
	if ((s = bc_parse_pushIndex(code, idx))) return s;

	++(*nexs);
	(*prev) = BC_INST_NUM;

	return s;
}

BcStatus bc_parse_reset(BcParse *p, BcStatus s) {

	if (p->func) {

		BcFunc *func = bc_vec_item(&p->prog->fns, p->func);

		func->nparams = 0;
		bc_vec_npop(&func->code, func->code.len);
		bc_vec_npop(&func->autos, func->autos.len);
		bc_vec_npop(&func->labels, func->labels.len);

		p->func = 0;
	}

	p->l.idx = p->l.len;
	p->l.t.t = BC_LEX_EOF;
	p->auto_part = (p->nbraces = 0);

	bc_vec_npop(&p->flags, p->flags.len - 1);
	bc_vec_npop(&p->exits, p->exits.len);
	bc_vec_npop(&p->conds, p->conds.len);
	bc_vec_npop(&p->ops, p->ops.len);

	return bc_program_reset(p->prog, s);
}

BcStatus bc_parse_create(BcParse *p, BcProgram *prog,
                         BcParseParse parse, BcLexNext next)
{
	BcStatus s;
	uint8_t flags = 0;

	assert(p && prog);

	if ((s = bc_lex_init(&p->l, next))) return s;
	if ((s = bc_vec_init(&p->flags, sizeof(uint8_t), NULL))) goto flags_err;
	if ((s = bc_vec_init(&p->exits, sizeof(BcInstPtr), NULL))) goto exit_err;
	if ((s = bc_vec_init(&p->conds, sizeof(size_t), NULL))) goto cond_err;
	if ((s = bc_vec_push(&p->flags, &flags))) goto push_err;
	if ((s = bc_vec_init(&p->ops, sizeof(BcLexType), NULL))) goto push_err;

	p->parse = parse;
	p->prog = prog;
	p->auto_part = (p->func = p->nbraces = 0);

	return s;

push_err:
	bc_vec_free(&p->conds);
cond_err:
	bc_vec_free(&p->exits);
exit_err:
	bc_vec_free(&p->flags);
flags_err:
	bc_lex_free(&p->l);
	return s;
}

void bc_parse_free(BcParse *p) {
	assert(p);
	bc_vec_free(&p->flags);
	bc_vec_free(&p->exits);
	bc_vec_free(&p->conds);
	bc_vec_free(&p->ops);
	bc_lex_free(&p->l);
}
