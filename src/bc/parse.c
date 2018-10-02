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
 * The parser for bc.
 *
 */

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <lex.h>
#include <parse.h>
#include <vm.h>

BcStatus bc_parse_else(BcParse *p, BcVec *code);
BcStatus bc_parse_stmt(BcParse *p, BcVec *code);

BcStatus bc_parse_operator(BcParse *p, BcVec *code, BcVec *ops, BcLexType type,
                           size_t *nexprs, bool next)
{
	BcStatus s;
	BcLexType t;
	uint8_t l, r = bc_parse_ops[type - BC_LEX_OP_INC].prec;
	bool left = bc_parse_ops[type - BC_LEX_OP_INC].left;

	while (ops->len && (t = *((BcLexType*) bc_vec_top(ops))) != BC_LEX_LPAREN &&
	       ((l = bc_parse_ops[t - BC_LEX_OP_INC].prec) < r || (l == r && left)))
	{
		if ((s = bc_vec_pushByte(code, BC_PARSE_TOKEN_TO_INST(t)))) return s;
		bc_vec_pop(ops);
		*nexprs -= t != BC_LEX_OP_BOOL_NOT && t != BC_LEX_NEG;
	}

	if ((s = bc_vec_push(ops, &type))) return s;
	if (next) s = bc_lex_next(&p->lex);

	return s;
}

BcStatus bc_parse_rightParen(BcParse *p, BcVec *code, BcVec *ops, size_t *nexs)
{
	BcStatus s;
	BcLexType top;

	if (!ops->len) return BC_STATUS_PARSE_BAD_EXP;

	while ((top = *((BcLexType*) bc_vec_top(ops))) != BC_LEX_LPAREN) {

		if ((s = bc_vec_pushByte(code, BC_PARSE_TOKEN_TO_INST(top)))) return s;

		bc_vec_pop(ops);
		*nexs -= top != BC_LEX_OP_BOOL_NOT && top != BC_LEX_NEG;

		if (!ops->len) return BC_STATUS_PARSE_BAD_EXP;
	}

	bc_vec_pop(ops);

	return bc_lex_next(&p->lex);
}

BcStatus bc_parse_params(BcParse *p, BcVec *code, uint8_t flags) {

	BcStatus s;
	bool comma = false;
	size_t nparams;

	if ((s = bc_lex_next(&p->lex))) return s;

	for (nparams = 0; p->lex.t.t != BC_LEX_RPAREN; ++nparams) {

		flags = (flags & ~BC_PARSE_PRINT) | BC_PARSE_ARRAY;
		if ((s = bc_parse_expr(p, code, flags, bc_parse_next_param))) return s;

		comma = p->lex.t.t == BC_LEX_COMMA;
		if (comma && (s = bc_lex_next(&p->lex))) return s;
	}

	if (comma) return BC_STATUS_PARSE_BAD_TOKEN;
	if ((s = bc_vec_pushByte(code, BC_INST_CALL))) return s;

	return bc_parse_pushIndex(code, nparams);
}

BcStatus bc_parse_call(BcParse *p, BcVec *code, char *name, uint8_t flags) {

	BcStatus s;
	BcEntry entry, *entry_ptr;
	size_t idx;

	entry.name = name;

	 if ((s = bc_parse_params(p, code, flags))) goto err;

	if (p->lex.t.t != BC_LEX_RPAREN) {
		s = BC_STATUS_PARSE_BAD_TOKEN;
		goto err;
	}

	idx = bc_veco_index(&p->prog->fn_map, &entry);

	if (idx == BC_VEC_INVALID_IDX) {
		if ((s = bc_program_addFunc(p->prog, name, &idx))) return s;
		name = NULL;
		idx = bc_veco_index(&p->prog->fn_map, &entry);
		assert(idx != BC_VEC_INVALID_IDX);
	}
	else free(name);

	entry_ptr = bc_veco_item(&p->prog->fn_map, idx);
	assert(entry_ptr);
	if ((s = bc_parse_pushIndex(code, entry_ptr->idx))) return s;

	return bc_lex_next(&p->lex);

err:
	if (name) free(name);
	return s;
}

BcStatus bc_parse_name(BcParse *p, BcVec *code, BcInst *type, uint8_t flags)
{
	BcStatus s;
	char *name;

	if (!(name = strdup(p->lex.t.v.vec))) return BC_STATUS_ALLOC_ERR;
	if ((s = bc_lex_next(&p->lex))) goto err;

	if (p->lex.t.t == BC_LEX_LBRACKET) {

		if ((s = bc_lex_next(&p->lex))) goto err;

		if (p->lex.t.t == BC_LEX_RBRACKET) {

			if (!(flags & BC_PARSE_ARRAY)) {
				s = BC_STATUS_PARSE_BAD_EXP;
				goto err;
			}

			*type = BC_INST_ARRAY;
		}
		else {

			*type = BC_INST_ARRAY_ELEM;

			flags &= ~(BC_PARSE_PRINT);
			if ((s = bc_parse_expr(p, code, flags, bc_parse_next_elem)))
				goto err;

			if (p->lex.t.t != BC_LEX_RBRACKET) {
				s = BC_STATUS_PARSE_BAD_TOKEN;
				goto err;
			}
		}

		if ((s = bc_lex_next(&p->lex))) goto err;
		if ((s = bc_vec_pushByte(code, (char) *type))) goto err;
		s = bc_parse_pushName(code, name);
	}
	else if (p->lex.t.t == BC_LEX_LPAREN) {

		if (flags & BC_PARSE_NOCALL) {
			s = BC_STATUS_PARSE_BAD_TOKEN;
			goto err;
		}

		*type = BC_INST_CALL;
		s = bc_parse_call(p, code, name, flags);
	}
	else {
		*type = BC_INST_VAR;
		if ((s = bc_vec_pushByte(code, BC_INST_VAR))) goto err;
		s = bc_parse_pushName(code, name);
	}

	return s;

err:
	free(name);
	return s;
}

BcStatus bc_parse_read(BcParse *p, BcVec *code) {

	BcStatus s;

	if ((s = bc_lex_next(&p->lex))) return s;
	if (p->lex.t.t != BC_LEX_LPAREN) return BC_STATUS_PARSE_BAD_TOKEN;

	if ((s = bc_lex_next(&p->lex))) return s;
	if (p->lex.t.t != BC_LEX_RPAREN) return BC_STATUS_PARSE_BAD_TOKEN;

	if ((s = bc_vec_pushByte(code, BC_INST_READ))) return s;

	return bc_lex_next(&p->lex);
}

BcStatus bc_parse_builtin(BcParse *p, BcVec *code, BcLexType type,
                          uint8_t flags, BcInst *prev)
{
	BcStatus s;

	if ((s = bc_lex_next(&p->lex))) return s;
	if (p->lex.t.t != BC_LEX_LPAREN) return BC_STATUS_PARSE_BAD_TOKEN;

	flags = (flags & ~BC_PARSE_PRINT) | BC_PARSE_ARRAY;

	if ((s = bc_lex_next(&p->lex))) return s;
	if ((s = bc_parse_expr(p, code, flags, bc_parse_next_cond))) return s;

	if (p->lex.t.t != BC_LEX_RPAREN) return BC_STATUS_PARSE_BAD_TOKEN;

	*prev = (type == BC_LEX_KEY_LENGTH) ? BC_INST_LENGTH : BC_INST_SQRT;
	if ((s = bc_vec_pushByte(code, (char) *prev))) return s;

	return bc_lex_next(&p->lex);
}

BcStatus bc_parse_scale(BcParse *p, BcVec *code, BcInst *type, uint8_t flags) {

	BcStatus s;

	if ((s = bc_lex_next(&p->lex))) return s;

	if (p->lex.t.t != BC_LEX_LPAREN) {
		*type = BC_INST_SCALE;
		return bc_vec_pushByte(code, BC_INST_SCALE);
	}

	*type = BC_INST_SCALE_FUNC;
	flags &= ~(BC_PARSE_PRINT);

	if ((s = bc_lex_next(&p->lex))) return s;
	if ((s = bc_parse_expr(p, code, flags, bc_parse_next_cond))) return s;
	if (p->lex.t.t != BC_LEX_RPAREN) return BC_STATUS_PARSE_BAD_TOKEN;
	if ((s = bc_vec_pushByte(code, BC_INST_SCALE_FUNC))) return s;

	return bc_lex_next(&p->lex);
}

BcStatus bc_parse_incdec(BcParse *p, BcVec *code, BcInst *prev,
                         size_t *nexprs, uint8_t flags)
{
	BcStatus s;
	BcLexType type;
	char inst;
	BcInst etype = *prev;

	if (etype == BC_INST_VAR || etype == BC_INST_ARRAY_ELEM ||
	    etype == BC_INST_SCALE || etype == BC_INST_LAST ||
	    etype == BC_INST_IBASE || etype == BC_INST_OBASE)
	{
		*prev = inst = BC_INST_INC_POST + (p->lex.t.t != BC_LEX_OP_INC);
		if ((s = bc_vec_pushByte(code, inst))) return s;
		s = bc_lex_next(&p->lex);
	}
	else {

		*prev = inst = BC_INST_INC_PRE + (p->lex.t.t != BC_LEX_OP_INC);

		if ((s = bc_lex_next(&p->lex))) return s;
		type = p->lex.t.t;

		// Because we parse the next part of the expression
		// right here, we need to increment this.
		*nexprs = *nexprs + 1;

		switch (type) {

			case BC_LEX_NAME:
			{
				s = bc_parse_name(p, code, prev, flags | BC_PARSE_NOCALL);
				break;
			}

			case BC_LEX_KEY_IBASE:
			{
				if ((s = bc_vec_pushByte(code, BC_INST_IBASE))) return s;
				s = bc_lex_next(&p->lex);
				break;
			}

			case BC_LEX_KEY_LAST:
			{
				if ((s = bc_vec_pushByte(code, BC_INST_LAST))) return s;
				s = bc_lex_next(&p->lex);
				break;
			}

			case BC_LEX_KEY_OBASE:
			{
				if ((s = bc_vec_pushByte(code, BC_INST_OBASE))) return s;
				s = bc_lex_next(&p->lex);
				break;
			}

			case BC_LEX_KEY_SCALE:
			{
				if ((s = bc_lex_next(&p->lex))) return s;
				if (p->lex.t.t == BC_LEX_LPAREN)
					return BC_STATUS_PARSE_BAD_TOKEN;

				s = bc_vec_pushByte(code, BC_INST_SCALE);

				break;
			}

			default:
			{
				return BC_STATUS_PARSE_BAD_TOKEN;
			}
		}

		if (s) return s;
		s = bc_vec_pushByte(code, inst);
	}

	return s;
}

BcStatus bc_parse_minus(BcParse *p, BcVec *exs, BcVec *ops, BcInst *prev,
                        bool rparen, size_t *nexprs)
{
	BcStatus s;
	BcLexType type;
	BcInst etype = *prev;

	if ((s = bc_lex_next(&p->lex))) return s;

	type = p->lex.t.t;

	if (type != BC_LEX_NAME && type != BC_LEX_NUMBER &&
	    type != BC_LEX_KEY_SCALE && type != BC_LEX_KEY_LAST &&
	    type != BC_LEX_KEY_IBASE && type != BC_LEX_KEY_OBASE &&
	    type != BC_LEX_LPAREN && type != BC_LEX_OP_MINUS &&
	    type != BC_LEX_OP_INC && type != BC_LEX_OP_DEC &&
	    type != BC_LEX_OP_BOOL_NOT)
	{
		return BC_STATUS_PARSE_BAD_TOKEN;
	}

	type = rparen || etype == BC_INST_INC_POST || etype == BC_INST_DEC_POST ||
	       (etype >= BC_INST_NUM && etype <= BC_INST_SQRT) ?
	                 BC_LEX_OP_MINUS : BC_LEX_NEG;
	*prev = BC_PARSE_TOKEN_TO_INST(type);

	if (type == BC_LEX_OP_MINUS)
		s = bc_parse_operator(p, exs, ops, type, nexprs, false);
	else
		// We can just push onto the op stack because this is the largest
		// precedence operator that gets pushed. Inc/dec does not.
		s = bc_vec_push(ops, &type);

	return s;
}

BcStatus bc_parse_string(BcParse *p, BcVec *code, char inst) {

	BcStatus s;
	char *str;

	if (!(str = strdup(p->lex.t.v.vec))) return BC_STATUS_ALLOC_ERR;
	if ((s = bc_vec_pushByte(code, BC_INST_STR))) goto err;
	if ((s = bc_parse_pushIndex(code, p->prog->strs.len))) goto err;
	if ((s = bc_vec_push(&p->prog->strs, &str))) goto err;
	if ((s = bc_vec_push(code, &inst))) return s;
	if ((s = bc_vec_pushByte(code, BC_INST_POP))) return s;

	return bc_lex_next(&p->lex);

err:
	free(str);
	return s;
}

BcStatus bc_parse_print(BcParse *p, BcVec *code) {

	BcStatus s;
	BcLexType type;
	bool comma = false;

	if ((s = bc_lex_next(&p->lex))) return s;

	type = p->lex.t.t;

	if (type == BC_LEX_SCOLON || type == BC_LEX_NLINE)
		return BC_STATUS_PARSE_BAD_PRINT;

	while (!s && type != BC_LEX_SCOLON && type != BC_LEX_NLINE) {

		if (type == BC_LEX_STRING) {
			s = bc_parse_string(p, code, BC_INST_PRINT);
		}
		else {
			if ((s = bc_parse_expr(p, code, 0, bc_parse_next_print))) return s;
			s = bc_vec_pushByte(code, BC_INST_PRINT_POP);
		}

		if (s) return s;

		comma = p->lex.t.t == BC_LEX_COMMA;
		if (comma) s = bc_lex_next(&p->lex);

		type = p->lex.t.t;
	}

	if (s) return s;
	if (comma) return BC_STATUS_PARSE_BAD_TOKEN;

	return bc_lex_next(&p->lex);
}

BcStatus bc_parse_return(BcParse *p, BcVec *code) {

	BcStatus s;
	BcLexType t;
	bool paren;

	if (!BC_PARSE_FUNC(p)) return BC_STATUS_PARSE_BAD_TOKEN;

	if ((s = bc_lex_next(&p->lex))) return s;

	t = p->lex.t.t;
	paren = t == BC_LEX_LPAREN;

	if (t == BC_LEX_NLINE || t == BC_LEX_SCOLON) {
		s = bc_vec_pushByte(code, BC_INST_RET0);
	}
	else {

		paren = paren && p->lex.t.last == BC_LEX_RPAREN;
		s = bc_parse_expr(p, code, 0, bc_parse_next_expr);

		if (paren)
		{
			if (!s) s = bc_vec_pushByte(code, BC_INST_RET);
			else if (s == BC_STATUS_PARSE_EMPTY_EXP)
				s = bc_vec_pushByte(code, BC_INST_RET0);
		}
		else s = bc_vec_pushByte(code, BC_INST_RET);
	}

	if (!s &&t != BC_LEX_NLINE && t != BC_LEX_SCOLON && !paren &&
	    (s = bc_vm_posix_error(BC_STATUS_POSIX_RETURN_PARENS,
	                           p->lex.file, p->lex.line, NULL)))
	{
		return s;
	}

	return s;
}

BcStatus bc_parse_endBody(BcParse *p, BcVec *code, bool brace) {

	BcStatus s = BC_STATUS_SUCCESS;

	if (p->flags.len <= 1 || p->nbraces == 0) return BC_STATUS_PARSE_BAD_TOKEN;

	if (brace) {

		if (p->lex.t.t == BC_LEX_RBRACE) {

			if (!p->nbraces) return BC_STATUS_PARSE_BAD_TOKEN;

			--p->nbraces;

			if ((s = bc_lex_next(&p->lex))) return s;
		}
		else return BC_STATUS_PARSE_BAD_TOKEN;
	}

	if (BC_PARSE_IF(p)) {

		uint8_t *flag_ptr;

		while (p->lex.t.t == BC_LEX_NLINE) {
			if ((s = bc_lex_next(&p->lex))) return s;
		}

		bc_vec_pop(&p->flags);

		flag_ptr = BC_PARSE_TOP_FLAG_PTR(p);
		*flag_ptr = (*flag_ptr | BC_PARSE_FLAG_IF_END);

		if (p->lex.t.t == BC_LEX_KEY_ELSE) s = bc_parse_else(p, code);
	}
	else if (BC_PARSE_ELSE(p)) {

		BcInstPtr *ip;
		BcFunc *func;
		size_t *label;

		bc_vec_pop(&p->flags);

		ip = bc_vec_top(&p->exits);
		func = bc_vec_item(&p->prog->fns, p->func);
		label = bc_vec_item(&func->labels, ip->idx);
		*label = code->len;

		bc_vec_pop(&p->exits);
	}
	else if (BC_PARSE_FUNC_INNER(p)) {
		p->func = 0;
		if ((s = bc_vec_pushByte(code, BC_INST_RET0))) return s;
		bc_vec_pop(&p->flags);
	}
	else {

		BcInstPtr *ip;
		BcFunc *func;
		size_t *label;

		if ((s = bc_vec_pushByte(code, BC_INST_JUMP))) return s;

		ip = bc_vec_top(&p->exits);
		label = bc_vec_top(&p->conds);

		if ((s = bc_parse_pushIndex(code, *label))) return s;

		func = bc_vec_item(&p->prog->fns, p->func);
		label = bc_vec_item(&func->labels, ip->idx);
		*label = code->len;

		bc_vec_pop(&p->flags);
		bc_vec_pop(&p->exits);
		bc_vec_pop(&p->conds);
	}

	return s;
}

BcStatus bc_parse_startBody(BcParse *p, uint8_t flags) {
	uint8_t *flag_ptr = BC_PARSE_TOP_FLAG_PTR(p);
	flags |= (*flag_ptr & (BC_PARSE_FLAG_FUNC | BC_PARSE_FLAG_LOOP));
	flags |= BC_PARSE_FLAG_BODY;
	return bc_vec_push(&p->flags, &flags);
}

void bc_parse_noElse(BcParse *p, BcVec *code) {

	BcInstPtr *ip;
	BcFunc *func;
	size_t *label;
	uint8_t *flag_ptr = BC_PARSE_TOP_FLAG_PTR(p);

	*flag_ptr = (*flag_ptr & ~(BC_PARSE_FLAG_IF_END));

	ip = bc_vec_top(&p->exits);
	assert(!ip->func && !ip->len);
	func = bc_vec_item(&p->prog->fns, p->func);
	assert(code == &func->code);
	label = bc_vec_item(&func->labels, ip->idx);
	*label = code->len;

	bc_vec_pop(&p->exits);
}

BcStatus bc_parse_if(BcParse *p, BcVec *code) {

	BcStatus s;
	BcInstPtr ip;
	BcFunc *func;

	if ((s = bc_lex_next(&p->lex))) return s;
	if (p->lex.t.t != BC_LEX_LPAREN) return BC_STATUS_PARSE_BAD_TOKEN;

	if ((s = bc_lex_next(&p->lex))) return s;
	if ((s = bc_parse_expr(p, code, BC_PARSE_POSIX_REL, bc_parse_next_cond)))
		return s;
	if (p->lex.t.t != BC_LEX_RPAREN) return BC_STATUS_PARSE_BAD_TOKEN;

	if ((s = bc_lex_next(&p->lex))) return s;
	if ((s = bc_vec_pushByte(code, BC_INST_JUMP_ZERO))) return s;

	func = bc_vec_item(&p->prog->fns, p->func);

	ip.idx = func->labels.len;
	ip.func = 0;
	ip.len = 0;

	if ((s = bc_parse_pushIndex(code, ip.idx))) return s;
	if ((s = bc_vec_push(&p->exits, &ip))) return s;
	if ((s = bc_vec_push(&func->labels, &ip.idx))) return s;

	return bc_parse_startBody(p, BC_PARSE_FLAG_IF);
}

BcStatus bc_parse_else(BcParse *p, BcVec *code) {

	BcStatus s;
	BcInstPtr ip;
	BcFunc *func;

	if (!BC_PARSE_IF_END(p)) return BC_STATUS_PARSE_BAD_TOKEN;

	func = bc_vec_item(&p->prog->fns, p->func);

	ip.idx = func->labels.len;
	ip.func = 0;
	ip.len = 0;

	if ((s = bc_vec_pushByte(code, BC_INST_JUMP))) return s;
	if ((s = bc_parse_pushIndex(code, ip.idx))) return s;

	bc_parse_noElse(p, code);

	if ((s = bc_vec_push(&p->exits, &ip))) return s;
	if ((s = bc_vec_push(&func->labels, &ip.idx))) return s;
	if ((s = bc_lex_next(&p->lex))) return s;

	return bc_parse_startBody(p, BC_PARSE_FLAG_ELSE);
}

BcStatus bc_parse_while(BcParse *p, BcVec *code) {

	BcStatus s;
	BcFunc *func;
	BcInstPtr ip;

	if ((s = bc_lex_next(&p->lex))) return s;
	if (p->lex.t.t != BC_LEX_LPAREN) return BC_STATUS_PARSE_BAD_TOKEN;
	if ((s = bc_lex_next(&p->lex))) return s;

	func = bc_vec_item(&p->prog->fns, p->func);
	ip.idx = func->labels.len;

	if ((s = bc_vec_push(&func->labels, &code->len))) return s;
	if ((s = bc_vec_push(&p->conds, &ip.idx))) return s;

	ip.idx = func->labels.len;
	ip.func = 1;
	ip.len = 0;

	if ((s = bc_vec_push(&p->exits, &ip))) return s;
	if ((s = bc_vec_push(&func->labels, &ip.idx))) return s;

	if ((s = bc_parse_expr(p, code, BC_PARSE_POSIX_REL, bc_parse_next_cond)))
		return s;
	if (p->lex.t.t != BC_LEX_RPAREN) return BC_STATUS_PARSE_BAD_TOKEN;

	if ((s = bc_lex_next(&p->lex))) return s;
	if ((s = bc_vec_pushByte(code, BC_INST_JUMP_ZERO))) return s;
	if ((s = bc_parse_pushIndex(code, ip.idx))) return s;

	return bc_parse_startBody(p, BC_PARSE_FLAG_LOOP | BC_PARSE_FLAG_LOOP_INNER);
}

BcStatus bc_parse_for(BcParse *p, BcVec *code) {

	BcStatus s;
	BcFunc *func;
	BcInstPtr ip;
	size_t cond_idx, exit_idx, body_idx, update_idx;

	if ((s = bc_lex_next(&p->lex))) return s;
	if (p->lex.t.t != BC_LEX_LPAREN) return BC_STATUS_PARSE_BAD_TOKEN;
	if ((s = bc_lex_next(&p->lex))) return s;

	if (p->lex.t.t != BC_LEX_SCOLON)
		s = bc_parse_expr(p, code, 0, bc_parse_next_for);
	else
		s = bc_vm_posix_error(BC_STATUS_POSIX_NO_FOR_INIT,
		                      p->lex.file, p->lex.line, NULL);

	if (s) return s;
	if (p->lex.t.t != BC_LEX_SCOLON) return BC_STATUS_PARSE_BAD_TOKEN;
	if ((s = bc_lex_next(&p->lex))) return s;

	func = bc_vec_item(&p->prog->fns, p->func);

	cond_idx = func->labels.len;
	update_idx = cond_idx + 1;
	body_idx = update_idx + 1;
	exit_idx = body_idx + 1;

	if ((s = bc_vec_push(&func->labels, &code->len))) return s;

	if (p->lex.t.t != BC_LEX_SCOLON)
		s = bc_parse_expr(p, code, BC_PARSE_POSIX_REL, bc_parse_next_for);
	else s = bc_vm_posix_error(BC_STATUS_POSIX_NO_FOR_COND,
	                           p->lex.file, p->lex.line, NULL);

	if (s) return s;
	if (p->lex.t.t != BC_LEX_SCOLON) return BC_STATUS_PARSE_BAD_TOKEN;

	if ((s = bc_lex_next(&p->lex))) return s;
	if ((s = bc_vec_pushByte(code, BC_INST_JUMP_ZERO))) return s;
	if ((s = bc_parse_pushIndex(code, exit_idx))) return s;
	if ((s = bc_vec_pushByte(code, BC_INST_JUMP))) return s;
	if ((s = bc_parse_pushIndex(code, body_idx))) return s;

	ip.idx = func->labels.len;

	if ((s = bc_vec_push(&p->conds, &update_idx))) return s;
	if ((s = bc_vec_push(&func->labels, &code->len))) return s;

	if (p->lex.t.t != BC_LEX_RPAREN)
		s = bc_parse_expr(p, code, 0, bc_parse_next_cond);
	else
		s = bc_vm_posix_error(BC_STATUS_POSIX_NO_FOR_UPDATE,
		                      p->lex.file, p->lex.line, NULL);

	if (s) return s;

	if (p->lex.t.t != BC_LEX_RPAREN) {
		s = bc_parse_expr(p, code, BC_PARSE_POSIX_REL, bc_parse_next_cond);
		if (s) return s;
	}

	if (p->lex.t.t != BC_LEX_RPAREN) return BC_STATUS_PARSE_BAD_TOKEN;
	if ((s = bc_vec_pushByte(code, BC_INST_JUMP))) return s;
	if ((s = bc_parse_pushIndex(code, cond_idx))) return s;
	if ((s = bc_vec_push(&func->labels, &code->len))) return s;

	ip.idx = exit_idx;
	ip.func = 1;
	ip.len = 0;

	if ((s = bc_vec_push(&p->exits, &ip))) return s;
	if ((s = bc_vec_push(&func->labels, &ip.idx))) return s;
	if ((s = bc_lex_next(&p->lex))) return s;

	return bc_parse_startBody(p, BC_PARSE_FLAG_LOOP | BC_PARSE_FLAG_LOOP_INNER);
}

BcStatus bc_parse_loopExit(BcParse *p, BcVec *code, BcLexType type) {

	BcStatus s;
	size_t idx;
	BcInstPtr *ip;

	if (!BC_PARSE_LOOP(p)) return BC_STATUS_PARSE_BAD_TOKEN;

	if (type == BC_LEX_KEY_BREAK) {

		size_t top;

		if (!p->exits.len) return BC_STATUS_PARSE_BAD_TOKEN;

		top = p->exits.len - 1;
		ip = bc_vec_item(&p->exits, top);

		while (top < p->exits.len && ip && !ip->func)
			ip = bc_vec_item(&p->exits, top--);

		if (top >= p->exits.len || !ip) return BC_STATUS_PARSE_BAD_TOKEN;

		idx = ip->idx;
	}
	else idx = *((size_t*) bc_vec_top(&p->conds));

	if ((s = bc_vec_pushByte(code, BC_INST_JUMP))) return s;
	if ((s = bc_parse_pushIndex(code, idx))) return s;
	if ((s = bc_lex_next(&p->lex))) return s;

	if (p->lex.t.t != BC_LEX_SCOLON && p->lex.t.t != BC_LEX_NLINE)
		return BC_STATUS_PARSE_BAD_TOKEN;

	return bc_lex_next(&p->lex);
}

BcStatus bc_parse_func(BcParse *p) {

	BcStatus s;
	BcFunc *fptr;
	bool var, comma = false;
	uint8_t flags;
	char *name;

	if ((s = bc_lex_next(&p->lex))) return s;
	if (p->lex.t.t != BC_LEX_NAME) return BC_STATUS_PARSE_BAD_FUNC;

	assert(p->prog->fns.len == p->prog->fn_map.vec.len);

	if (!(name = strdup(p->lex.t.v.vec))) return BC_STATUS_ALLOC_ERR;
	if ((s = bc_program_addFunc(p->prog, name, &p->func))) return s;
	assert(p->func);
	fptr = bc_vec_item(&p->prog->fns, p->func);

	if ((s = bc_lex_next(&p->lex))) return s;
	if (p->lex.t.t != BC_LEX_LPAREN) return BC_STATUS_PARSE_BAD_FUNC;
	if ((s = bc_lex_next(&p->lex))) return s;

	while (!s && p->lex.t.t != BC_LEX_RPAREN) {

		if (p->lex.t.t != BC_LEX_NAME) return BC_STATUS_PARSE_BAD_FUNC;

		++fptr->nparams;

		if (!(name = strdup(p->lex.t.v.vec))) return BC_STATUS_ALLOC_ERR;
		if ((s = bc_lex_next(&p->lex))) goto err;

		var = p->lex.t.t != BC_LEX_LBRACKET;

		if (!var) {

			if ((s = bc_lex_next(&p->lex))) goto err;

			if (p->lex.t.t != BC_LEX_RBRACKET) {
				s = BC_STATUS_PARSE_BAD_FUNC;
				goto err;
			}

			if ((s = bc_lex_next(&p->lex))) goto err;
		}

		comma = p->lex.t.t == BC_LEX_COMMA;
		if (comma && (s = bc_lex_next(&p->lex))) goto err;

		if ((s = bc_func_insert(fptr, name, var))) goto err;
	}

	if (comma) return BC_STATUS_PARSE_BAD_FUNC;

	flags = BC_PARSE_FLAG_FUNC | BC_PARSE_FLAG_FUNC_INNER | BC_PARSE_FLAG_BODY;

	if ((s = bc_parse_startBody(p, flags))) return s;
	if ((s = bc_lex_next(&p->lex))) return s;

	if (p->lex.t.t != BC_LEX_LBRACE)
		return bc_vm_posix_error(BC_STATUS_POSIX_HEADER_BRACE,
		                         p->lex.file, p->lex.line, NULL);

	return s;

err:
	free(name);
	return s;
}

BcStatus bc_parse_auto(BcParse *p) {

	BcStatus s;
	bool comma, var, one;
	char *name;
	BcFunc *func;

	if (!p->auto_part) return BC_STATUS_PARSE_BAD_TOKEN;
	if ((s = bc_lex_next(&p->lex))) return s;

	p->auto_part = comma = one = false;
	func = bc_vec_item(&p->prog->fns, p->func);

	while (!s && p->lex.t.t == BC_LEX_NAME) {

		if (!(name = strdup(p->lex.t.v.vec))) return BC_STATUS_ALLOC_ERR;
		if ((s = bc_lex_next(&p->lex))) goto err;

		one = true;
		var = p->lex.t.t != BC_LEX_LBRACKET;

		if (!var) {

			if ((s = bc_lex_next(&p->lex))) goto err;

			if (p->lex.t.t != BC_LEX_RBRACKET) {
				s = BC_STATUS_PARSE_BAD_FUNC;
				goto err;
			}

			if ((s = bc_lex_next(&p->lex))) goto err;
		}

		comma = p->lex.t.t == BC_LEX_COMMA;
		if (comma && (s = bc_lex_next(&p->lex))) goto err;

		if ((s = bc_func_insert(func, name, var))) goto err;
	}

	if (comma) return BC_STATUS_PARSE_BAD_FUNC;
	if (!one) return BC_STATUS_PARSE_NO_AUTO;

	if (p->lex.t.t != BC_LEX_NLINE && p->lex.t.t != BC_LEX_SCOLON)
		return BC_STATUS_PARSE_BAD_TOKEN;

	return bc_lex_next(&p->lex);

err:
	free(name);
	return s;
}

BcStatus bc_parse_body(BcParse *p, BcVec *code, bool brace) {

	BcStatus s;
	uint8_t *flag_ptr = bc_vec_top(&p->flags);

	assert(p->flags.len >= 2);

	*flag_ptr &= ~(BC_PARSE_FLAG_BODY);

	if (*flag_ptr & BC_PARSE_FLAG_FUNC_INNER) {

		if (!brace) return BC_STATUS_PARSE_BAD_TOKEN;
		p->auto_part = true;

		if (p->lex.t.t == BC_LEX_KEY_AUTO) {
			if ((s = bc_parse_auto(p))) return s;
			p->auto_part = true;
		}

		s = bc_lex_next(&p->lex);
	}
	else {
		assert(*flag_ptr);
		if ((s = bc_parse_stmt(p, code))) return s;
		if (!brace) s = bc_parse_endBody(p, code, false);
	}

	return s;
}

BcStatus bc_parse_stmt(BcParse *p, BcVec *code) {

	BcStatus s;

	switch (p->lex.t.t) {

		case BC_LEX_NLINE:
		{
			return bc_lex_next(&p->lex);
		}

		case BC_LEX_KEY_ELSE:
		{
			p->auto_part = false;
			break;
		}

		case BC_LEX_LBRACE:
		{
			if (!BC_PARSE_BODY(p)) return BC_STATUS_PARSE_BAD_TOKEN;

			++p->nbraces;
			if ((s = bc_lex_next(&p->lex))) return s;

			return bc_parse_body(p, code, true);
		}

		case BC_LEX_KEY_AUTO:
		{
			return bc_parse_auto(p);
		}

		default:
		{
			p->auto_part = false;

			if (BC_PARSE_IF_END(p)) {
				bc_parse_noElse(p, code);
				return BC_STATUS_SUCCESS;
			}
			else if (BC_PARSE_BODY(p)) return bc_parse_body(p, code, false);

			break;
		}
	}

	switch (p->lex.t.t) {

		case BC_LEX_OP_INC:
		case BC_LEX_OP_DEC:
		case BC_LEX_OP_MINUS:
		case BC_LEX_OP_BOOL_NOT:
		case BC_LEX_LPAREN:
		case BC_LEX_NAME:
		case BC_LEX_NUMBER:
		case BC_LEX_KEY_IBASE:
		case BC_LEX_KEY_LAST:
		case BC_LEX_KEY_LENGTH:
		case BC_LEX_KEY_OBASE:
		case BC_LEX_KEY_READ:
		case BC_LEX_KEY_SCALE:
		case BC_LEX_KEY_SQRT:
		{
			s = bc_parse_expr(p, code, BC_PARSE_PRINT, bc_parse_next_expr);
			break;
		}

		case BC_LEX_KEY_ELSE:
		{
			s = bc_parse_else(p, code);
			break;
		}

		case BC_LEX_SCOLON:
		{
			s = BC_STATUS_SUCCESS;
			while (!s && p->lex.t.t == BC_LEX_SCOLON) s = bc_lex_next(&p->lex);
			break;
		}

		case BC_LEX_RBRACE:
		{
			s = bc_parse_endBody(p, code, true);
			break;
		}

		case BC_LEX_STRING:
		{
			s = bc_parse_string(p, code, BC_INST_PRINT_STR);
			break;
		}

		case BC_LEX_KEY_BREAK:
		case BC_LEX_KEY_CONTINUE:
		{
			s = bc_parse_loopExit(p, code, p->lex.t.t);
			break;
		}

		case BC_LEX_KEY_FOR:
		{
			s = bc_parse_for(p, code);
			break;
		}

		case BC_LEX_KEY_HALT:
		{
			if ((s = bc_vec_pushByte(code, BC_INST_HALT))) return s;
			s = bc_lex_next(&p->lex);
			break;
		}

		case BC_LEX_KEY_IF:
		{
			s = bc_parse_if(p, code);
			break;
		}

		case BC_LEX_KEY_LIMITS:
		{
			if ((s = bc_lex_next(&p->lex))) return s;
			s = BC_STATUS_LIMITS;
			break;
		}

		case BC_LEX_KEY_PRINT:
		{
			s = bc_parse_print(p, code);
			break;
		}

		case BC_LEX_KEY_QUIT:
		{
			// Quit is a compile-time command. We don't exit directly,
			// so the vm can clean up. Limits do the same thing.
			s = BC_STATUS_QUIT;
			break;
		}

		case BC_LEX_KEY_RETURN:
		{
			if ((s = bc_parse_return(p, code))) return s;
			break;
		}

		case BC_LEX_KEY_WHILE:
		{
			s = bc_parse_while(p, code);
			break;
		}

		case BC_LEX_EOF:
		{
			s = (p->flags.len > 0) * BC_STATUS_PARSE_NO_BLOCK_END;
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

BcStatus bc_parse_parse(BcParse *p) {

	BcStatus s;

	assert(p);

	if (p->lex.t.t == BC_LEX_EOF) s = BC_STATUS_LEX_EOF;
	else if (p->lex.t.t == BC_LEX_KEY_DEFINE) {
		if (!BC_PARSE_CAN_EXEC(p)) return BC_STATUS_PARSE_BAD_TOKEN;
		s = bc_parse_func(p);
	}
	else {
		BcFunc *func = bc_vec_item(&p->prog->fns, p->func);
		s = bc_parse_stmt(p, &func->code);
	}

	if (s || bcg.signe) s = bc_parse_reset(p, s);

	return s;
}

BcStatus bc_parse_expr(BcParse *p, BcVec *code, uint8_t flags, BcParseNext next)
{
	BcStatus s = BC_STATUS_SUCCESS;
	BcInst prev = BC_INST_PRINT;
	BcLexType top, t = p->lex.t.t;
	size_t nexprs = 0;
	uint32_t i, nparens, nrelops, ops_start = (uint32_t) p->ops.len;
	bool paren_first, paren_expr, rprn, done, get_token, assign, bin_last;

	paren_first = p->lex.t.t == BC_LEX_LPAREN;
	nparens = nrelops = 0;
	paren_expr = rprn = done = get_token = assign = false;
	bin_last = true;

	for (; !bcg.signe && !s && !done && bc_parse_exprs[t]; t = p->lex.t.t)
	{
		switch (t) {

			case BC_LEX_OP_INC:
			case BC_LEX_OP_DEC:
			{
				s = bc_parse_incdec(p, code, &prev, &nexprs, flags);
				rprn = get_token = bin_last = false;
				break;
			}

			case BC_LEX_OP_MINUS:
			{
				s = bc_parse_minus(p, code, &p->ops, &prev, rprn, &nexprs);
				rprn = get_token = false;
				bin_last = prev == BC_INST_MINUS;
				break;
			}

			case BC_LEX_OP_ASSIGN_POWER:
			case BC_LEX_OP_ASSIGN_MULTIPLY:
			case BC_LEX_OP_ASSIGN_DIVIDE:
			case BC_LEX_OP_ASSIGN_MODULUS:
			case BC_LEX_OP_ASSIGN_PLUS:
			case BC_LEX_OP_ASSIGN_MINUS:
			case BC_LEX_OP_ASSIGN:
			{
				if (prev != BC_INST_VAR && prev != BC_INST_ARRAY_ELEM &&
				    prev != BC_INST_SCALE && prev != BC_INST_IBASE &&
				    prev != BC_INST_OBASE && prev != BC_INST_LAST)
				{
					s = BC_STATUS_PARSE_BAD_ASSIGN;
					break;
				}
			}
			// Fallthrough.
			case BC_LEX_OP_POWER:
			case BC_LEX_OP_MULTIPLY:
			case BC_LEX_OP_DIVIDE:
			case BC_LEX_OP_MODULUS:
			case BC_LEX_OP_PLUS:
			case BC_LEX_OP_REL_EQ:
			case BC_LEX_OP_REL_LE:
			case BC_LEX_OP_REL_GE:
			case BC_LEX_OP_REL_NE:
			case BC_LEX_OP_REL_LT:
			case BC_LEX_OP_REL_GT:
			case BC_LEX_OP_BOOL_NOT:
			case BC_LEX_OP_BOOL_OR:
			case BC_LEX_OP_BOOL_AND:
			{
				if (((t == BC_LEX_OP_BOOL_NOT) != bin_last) ||
				    (t != BC_LEX_OP_BOOL_NOT && prev == BC_INST_BOOL_NOT))
				{
					return BC_STATUS_PARSE_BAD_EXP;
				}

				nrelops += t >= BC_LEX_OP_REL_EQ && t <= BC_LEX_OP_REL_GT;
				prev = BC_PARSE_TOKEN_TO_INST(t);
				s = bc_parse_operator(p, code, &p->ops, t, &nexprs, true);
				rprn = get_token = false;
				bin_last = t != BC_LEX_OP_BOOL_NOT;

				break;
			}

			case BC_LEX_LPAREN:
			{
				if (BC_PARSE_LEAF(prev, rprn)) return BC_STATUS_PARSE_BAD_EXP;

				++nparens;
				paren_expr = rprn = bin_last = false;
				get_token = true;
				s = bc_vec_push(&p->ops, &t);

				break;
			}

			case BC_LEX_RPAREN:
			{
				if (bin_last || prev == BC_INST_BOOL_NOT)
					return BC_STATUS_PARSE_BAD_EXP;

				if (nparens == 0) {
					s = BC_STATUS_SUCCESS;
					done = true;
					get_token = false;
					break;
				}
				else if (!paren_expr) return BC_STATUS_PARSE_EMPTY_EXP;

				--nparens;
				paren_expr = rprn = true;
				get_token = bin_last = false;

				s = bc_parse_rightParen(p, code, &p->ops, &nexprs);

				break;
			}

			case BC_LEX_NAME:
			{
				if (BC_PARSE_LEAF(prev, rprn)) return BC_STATUS_PARSE_BAD_EXP;

				paren_expr = true;
				rprn = get_token = bin_last = false;
				s = bc_parse_name(p, code, &prev, flags & ~BC_PARSE_NOCALL);
				++nexprs;

				break;
			}

			case BC_LEX_NUMBER:
			{
				if (BC_PARSE_LEAF(prev, rprn)) return BC_STATUS_PARSE_BAD_EXP;

				s = bc_parse_number(p, code, &prev, &nexprs);
				paren_expr = get_token = true;
				rprn = bin_last = false;

				break;
			}

			case BC_LEX_KEY_IBASE:
			case BC_LEX_KEY_LAST:
			case BC_LEX_KEY_OBASE:
			{
				if (BC_PARSE_LEAF(prev, rprn)) return BC_STATUS_PARSE_BAD_EXP;

				prev = (char) (t - BC_LEX_KEY_IBASE + BC_INST_IBASE);
				s = bc_vec_pushByte(code, (char) prev);

				paren_expr = get_token = true;
				rprn = bin_last = false;
				++nexprs;

				break;
			}

			case BC_LEX_KEY_LENGTH:
			case BC_LEX_KEY_SQRT:
			{
				if (BC_PARSE_LEAF(prev, rprn)) return BC_STATUS_PARSE_BAD_EXP;

				s = bc_parse_builtin(p, code, t, flags, &prev);
				paren_expr = true;
				rprn = get_token = bin_last = false;
				++nexprs;

				break;
			}

			case BC_LEX_KEY_READ:
			{
				if (BC_PARSE_LEAF(prev, rprn)) return BC_STATUS_PARSE_BAD_EXP;
				else if (flags & BC_PARSE_NOREAD) s = BC_STATUS_EXEC_REC_READ;
				else s = bc_parse_read(p, code);

				paren_expr = true;
				rprn = get_token = bin_last = false;
				++nexprs;
				prev = BC_INST_READ;

				break;
			}

			case BC_LEX_KEY_SCALE:
			{
				if (BC_PARSE_LEAF(prev, rprn)) return BC_STATUS_PARSE_BAD_EXP;

				s = bc_parse_scale(p, code, &prev, flags);
				paren_expr = true;
				rprn = get_token = bin_last = false;
				++nexprs;
				prev = BC_INST_SCALE;

				break;
			}

			default:
			{
				s = BC_STATUS_PARSE_BAD_TOKEN;
				break;
			}
		}

		if (!s && get_token) s = bc_lex_next(&p->lex);
	}

	if (s) return s;
	if (bcg.signe) return BC_STATUS_EXEC_SIGNAL;

	while (!s && p->ops.len > ops_start) {

		top = *((BcLexType*) bc_vec_top(&p->ops));
		assign = top >= BC_LEX_OP_ASSIGN_POWER && top <= BC_LEX_OP_ASSIGN;

		if (top == BC_LEX_LPAREN || top == BC_LEX_RPAREN)
			return BC_STATUS_PARSE_BAD_EXP;

		if ((s = bc_vec_pushByte(code, BC_PARSE_TOKEN_TO_INST(top)))) return s;

		nexprs -= top != BC_LEX_OP_BOOL_NOT && top != BC_LEX_NEG;
		bc_vec_pop(&p->ops);
	}

	s = BC_STATUS_PARSE_BAD_EXP;
	if (prev == BC_INST_BOOL_NOT || nexprs != 1) return s;

	for (i = 0; s && i < next.len; ++i) s *= t != next.tokens[i];
	if (s) return s;

	if (!(flags & BC_PARSE_POSIX_REL) && nrelops &&
	    (s = bc_vm_posix_error(BC_STATUS_POSIX_REL_OUTSIDE,
	                           p->lex.file, p->lex.line, NULL)))
	{
		return s;
	}
	else if ((flags & BC_PARSE_POSIX_REL) && nrelops != 1 &&
	         (s = bc_vm_posix_error(BC_STATUS_POSIX_MULTIPLE_REL,
	                                p->lex.file, p->lex.line, NULL)))
	{
		return s;
	}

	if (flags & BC_PARSE_PRINT) {
		if (paren_first || !assign) s = bc_vec_pushByte(code, BC_INST_PRINT);
		s = bc_vec_pushByte(code, BC_INST_POP);
	}

	return s;
}

BcStatus bc_parse_init(BcParse *p, BcProgram *prog) {
	return bc_parse_create(p, prog, bc_parse_parse, bc_lex_token);
}
