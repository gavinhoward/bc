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
#include <bc.h>
#include <vm.h>

#if BC_ENABLED
BcStatus bc_parse_operator(BcParse *p, BcLexType type, size_t start,
                           size_t *nexprs, bool next)
{
	BcStatus s = BC_STATUS_SUCCESS;
	BcLexType t;
	char l, r = bc_parse_ops[type - BC_LEX_OP_INC].prec;
	bool left = bc_parse_ops[type - BC_LEX_OP_INC].left;

	while (p->ops.len > start) {

		t = BC_PARSE_TOP_OP(p);
		if (t == BC_LEX_LPAREN) break;

		l = bc_parse_ops[t - BC_LEX_OP_INC].prec;
		if (l >= r && (l != r || !left)) break;

		bc_parse_push(p, BC_PARSE_TOKEN_INST(t));
		bc_vec_pop(&p->ops);
		*nexprs -= t != BC_LEX_OP_BOOL_NOT && t != BC_LEX_NEG;
	}

	bc_vec_push(&p->ops, &type);
	if (next) s = bc_lex_next(&p->l);

	return s;
}

BcStatus bc_parse_rightParen(BcParse *p, size_t ops_bgn, size_t *nexs) {

	BcLexType top;

	if (p->ops.len <= ops_bgn)
		return bc_vm_error(BC_ERROR_PARSE_BAD_EXP, p->l.line);

	top = BC_PARSE_TOP_OP(p);

	while (top != BC_LEX_LPAREN) {

		bc_parse_push(p, BC_PARSE_TOKEN_INST(top));

		bc_vec_pop(&p->ops);
		*nexs -= top != BC_LEX_OP_BOOL_NOT && top != BC_LEX_NEG;

		if (p->ops.len <= ops_bgn)
			return bc_vm_error(BC_ERROR_PARSE_BAD_EXP, p->l.line);

		top = BC_PARSE_TOP_OP(p);
	}

	bc_vec_pop(&p->ops);

	return bc_lex_next(&p->l);
}

BcStatus bc_parse_params(BcParse *p, uint8_t flags) {

	BcStatus s;
	bool comma = false;
	size_t nparams;

	s = bc_lex_next(&p->l);
	if (s) return s;

	for (nparams = 0; p->l.t.t != BC_LEX_RPAREN; ++nparams) {

		flags = (flags & ~(BC_PARSE_PRINT | BC_PARSE_REL)) | BC_PARSE_ARRAY;
		s = bc_parse_expr_status(p, flags, bc_parse_next_param);
		if (s) return s;

		comma = p->l.t.t == BC_LEX_COMMA;
		if (comma) {
			s = bc_lex_next(&p->l);
			if (s) return s;
		}
	}

	if (comma) return bc_vm_error(BC_ERROR_PARSE_BAD_TOKEN, p->l.line);
	bc_parse_push(p, BC_INST_CALL);
	bc_parse_pushIndex(p, nparams);

	return BC_STATUS_SUCCESS;
}

BcStatus bc_parse_call(BcParse *p, char *name, uint8_t flags) {

	BcStatus s;
	BcId entry, *entry_ptr;
	size_t idx;

	entry.name = name;

	s = bc_parse_params(p, flags);
	if (s) goto err;

	if (p->l.t.t != BC_LEX_RPAREN) {
		s = bc_vm_error(BC_ERROR_PARSE_BAD_TOKEN, p->l.line);
		goto err;
	}

	idx = bc_map_index(&p->prog->fn_map, &entry);

	if (idx == BC_VEC_INVALID_IDX) {
		bc_parse_addFunc(p, name);
		idx = bc_map_index(&p->prog->fn_map, &entry);
		assert(idx != BC_VEC_INVALID_IDX);
	}
	else free(name);

	assert(idx != BC_VEC_INVALID_IDX);

	entry_ptr = bc_vec_item(&p->prog->fn_map, idx);
	assert(entry_ptr);
	bc_parse_pushIndex(p, entry_ptr->idx);

	return bc_lex_next(&p->l);

err:
	free(name);
	return s;
}

BcStatus bc_parse_name(BcParse *p, BcInst *type, uint8_t flags) {

	BcStatus s;
	char *name;

	name = bc_vm_strdup(p->l.t.v.v);
	s = bc_lex_next(&p->l);
	if (s) goto err;

	if (p->l.t.t == BC_LEX_LBRACKET) {

		s = bc_lex_next(&p->l);
		if (s) goto err;

		if (p->l.t.t == BC_LEX_RBRACKET) {

			if (!(flags & BC_PARSE_ARRAY)) {
				s = bc_vm_error(BC_ERROR_PARSE_BAD_EXP, p->l.line);
				goto err;
			}

			*type = BC_INST_ARRAY;
		}
		else {

			*type = BC_INST_ARRAY_ELEM;

			flags &= ~(BC_PARSE_PRINT | BC_PARSE_REL);
			s = bc_parse_expr_status(p, flags, bc_parse_next_elem);
			if (s) goto err;
		}

		s = bc_lex_next(&p->l);
		if (s) goto err;
		bc_parse_push(p, *type);
		bc_parse_pushName(p, name);
	}
	else if (p->l.t.t == BC_LEX_LPAREN) {

		if (flags & BC_PARSE_NOCALL) {
			s = bc_vm_error(BC_ERROR_PARSE_BAD_TOKEN, p->l.line);
			goto err;
		}

		*type = BC_INST_CALL;
		s = bc_parse_call(p, name, flags);
	}
	else {
		*type = BC_INST_VAR;
		bc_parse_push(p, BC_INST_VAR);
		bc_parse_pushName(p, name);
	}

	return s;

err:
	free(name);
	return s;
}

BcStatus bc_parse_read(BcParse *p) {

	BcStatus s;

	s = bc_lex_next(&p->l);
	if (s) return s;
	if (p->l.t.t != BC_LEX_LPAREN)
		return bc_vm_error(BC_ERROR_PARSE_BAD_TOKEN, p->l.line);

	s = bc_lex_next(&p->l);
	if (s) return s;
	if (p->l.t.t != BC_LEX_RPAREN)
		return bc_vm_error(BC_ERROR_PARSE_BAD_TOKEN, p->l.line);

	bc_parse_push(p, BC_INST_READ);

	return bc_lex_next(&p->l);
}

BcStatus bc_parse_builtin(BcParse *p, BcLexType type,
                          uint8_t flags, BcInst *prev)
{
	BcStatus s;

	s = bc_lex_next(&p->l);
	if (s) return s;
	if (p->l.t.t != BC_LEX_LPAREN)
		return bc_vm_error(BC_ERROR_PARSE_BAD_TOKEN, p->l.line);

	flags = (flags & ~(BC_PARSE_PRINT | BC_PARSE_REL)) | BC_PARSE_ARRAY;

	s = bc_lex_next(&p->l);
	if (s) return s;

	s = bc_parse_expr_status(p, flags, bc_parse_next_rel);
	if (s) return s;

	if (p->l.t.t != BC_LEX_RPAREN)
		return bc_vm_error(BC_ERROR_PARSE_BAD_TOKEN, p->l.line);

	*prev = (type == BC_LEX_KEY_LENGTH) ? BC_INST_LENGTH : BC_INST_SQRT;
	bc_parse_push(p, *prev);

	return bc_lex_next(&p->l);
}

BcStatus bc_parse_scale(BcParse *p, BcInst *type, uint8_t flags) {

	BcStatus s;

	s = bc_lex_next(&p->l);
	if (s) return s;

	if (p->l.t.t != BC_LEX_LPAREN) {
		*type = BC_INST_SCALE;
		bc_parse_push(p, BC_INST_SCALE);
		return BC_STATUS_SUCCESS;
	}

	*type = BC_INST_SCALE_FUNC;
	flags &= ~(BC_PARSE_PRINT | BC_PARSE_REL);

	s = bc_lex_next(&p->l);
	if (s) return s;

	s = bc_parse_expr_status(p, flags, bc_parse_next_rel);
	if (s) return s;
	if (p->l.t.t != BC_LEX_RPAREN)
		return bc_vm_error(BC_ERROR_PARSE_BAD_TOKEN, p->l.line);

	bc_parse_push(p, BC_INST_SCALE_FUNC);

	return bc_lex_next(&p->l);
}

BcStatus bc_parse_incdec(BcParse *p, BcInst *prev, bool *paren_expr,
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
		*prev = inst = BC_INST_INC_POST + (p->l.t.t != BC_LEX_OP_INC);
		bc_parse_push(p, inst);
		s = bc_lex_next(&p->l);
	}
	else {

		*prev = inst = BC_INST_INC_PRE + (p->l.t.t != BC_LEX_OP_INC);
		*paren_expr = true;

		s = bc_lex_next(&p->l);
		if (s) return s;
		type = p->l.t.t;

		// Because we parse the next part of the expression
		// right here, we need to increment this.
		*nexprs = *nexprs + 1;

		switch (type) {

			case BC_LEX_NAME:
			{
				s = bc_parse_name(p, prev, flags | BC_PARSE_NOCALL);
				break;
			}

			case BC_LEX_KEY_IBASE:
			case BC_LEX_KEY_LAST:
			case BC_LEX_KEY_OBASE:
			{
				bc_parse_push(p, type - BC_LEX_KEY_IBASE + BC_INST_IBASE);
				s = bc_lex_next(&p->l);
				break;
			}

			case BC_LEX_KEY_SCALE:
			{
				s = bc_lex_next(&p->l);
				if (s) return s;
				if (p->l.t.t == BC_LEX_LPAREN)
					s = bc_vm_error(BC_ERROR_PARSE_BAD_TOKEN, p->l.line);
				else bc_parse_push(p, BC_INST_SCALE);
				break;
			}

			default:
			{
				s = bc_vm_error(BC_ERROR_PARSE_BAD_TOKEN, p->l.line);
				break;
			}
		}

		if (!s) bc_parse_push(p, inst);
	}

	return s;
}

BcStatus bc_parse_minus(BcParse *p, BcInst *prev, size_t ops_bgn,
                        bool rparen, size_t *nexprs)
{
	BcStatus s;
	BcLexType type;

	s = bc_lex_next(&p->l);
	if (s) return s;

	type = BC_PARSE_LEAF(*prev, rparen) ? BC_LEX_OP_MINUS : BC_LEX_NEG;
	*prev = BC_PARSE_TOKEN_INST(type);

	// We can just push onto the op stack because this is the largest
	// precedence operator that gets pushed. Inc/dec does not.
	if (type != BC_LEX_OP_MINUS) bc_vec_push(&p->ops, &type);
	else s = bc_parse_operator(p, type, ops_bgn, nexprs, false);

	return s;
}

BcStatus bc_parse_str(BcParse *p, char inst) {
	bc_parse_string(p);
	bc_parse_push(p, inst);
	return bc_lex_next(&p->l);
}

BcStatus bc_parse_print(BcParse *p) {

	BcStatus s;
	BcLexType type;
	bool comma = false;

	s = bc_lex_next(&p->l);
	if (s) return s;

	type = p->l.t.t;

	if (type == BC_LEX_SCOLON || type == BC_LEX_NLINE)
		return bc_vm_error(BC_ERROR_PARSE_BAD_PRINT, p->l.line);

	while (!s && type != BC_LEX_SCOLON && type != BC_LEX_NLINE) {

		if (type == BC_LEX_STR) s = bc_parse_str(p, BC_INST_PRINT_POP);
		else {
			s = bc_parse_expr_status(p, 0, bc_parse_next_print);
			if (s) return s;
			bc_parse_push(p, BC_INST_PRINT_POP);
		}

		if (s) return s;

		comma = p->l.t.t == BC_LEX_COMMA;
		if (comma) s = bc_lex_next(&p->l);
		type = p->l.t.t;
	}

	if (s) return s;
	if (comma) return bc_vm_error(BC_ERROR_PARSE_BAD_TOKEN, p->l.line);

	return bc_lex_next(&p->l);
}

BcStatus bc_parse_return(BcParse *p) {

	BcStatus s;
	BcLexType t;
	bool paren;

	if (!BC_PARSE_FUNC(p))
		return bc_vm_error(BC_ERROR_PARSE_BAD_TOKEN, p->l.line);

	s = bc_lex_next(&p->l);
	if (s) return s;

	t = p->l.t.t;
	paren = t == BC_LEX_LPAREN;

	if (t == BC_LEX_NLINE || t == BC_LEX_SCOLON) bc_parse_push(p, BC_INST_RET0);
	else {

		s = bc_parse_expr_error(p, 0, bc_parse_next_expr);
		if (s && s != BC_STATUS_EMPTY_EXPR) return s;
		else if (s == BC_STATUS_EMPTY_EXPR) {
			bc_parse_push(p, BC_INST_RET0);
			s = bc_lex_next(&p->l);
			if (s) return s;
		}

		if (!paren || p->l.t.last != BC_LEX_RPAREN) {
			s = bc_vm_posixError(BC_ERROR_POSIX_RET, p->l.line);
			if (s) return s;
		}

		bc_parse_push(p, BC_INST_RET);
	}

	return s;
}

BcStatus bc_parse_endBody(BcParse *p, bool brace) {

	BcStatus s = BC_STATUS_SUCCESS;
	bool has_brace, new_else = false;

	if (p->flags.len <= 1)
		return bc_vm_error(BC_ERROR_PARSE_BAD_TOKEN, p->l.line);

	if (brace) {
		if (p->l.t.t == BC_LEX_RBRACE) {
			s = bc_lex_next(&p->l);
			if (s) return s;
		}
		else return bc_vm_error(BC_ERROR_PARSE_BAD_TOKEN, p->l.line);
	}

	has_brace = (BC_PARSE_BRACE(p) != 0);

	do {

		size_t len = p->flags.len;

		if (has_brace && !brace)
			return bc_vm_error(BC_ERROR_PARSE_BAD_TOKEN, p->l.line);

		if (BC_PARSE_ELSE(p)) {

			BcInstPtr *ip;
			size_t *label;

			bc_vec_pop(&p->flags);

			ip = bc_vec_top(&p->exits);
			label = bc_vec_item(&p->func->labels, ip->idx);
			*label = p->func->code.len;

			bc_vec_pop(&p->exits);
		}
		else if (BC_PARSE_FUNC_INNER(p)) {
			bc_parse_push(p, BC_INST_RET0);
			bc_parse_updateFunc(p, BC_PROG_MAIN);
			bc_vec_pop(&p->flags);
		}
		else if (!BC_PARSE_IF(p)) {

			BcInstPtr *ip = bc_vec_top(&p->exits);
			size_t *label = bc_vec_top(&p->conds);

			bc_parse_push(p, BC_INST_JUMP);
			bc_parse_pushIndex(p, *label);

			label = bc_vec_item(&p->func->labels, ip->idx);
			*label = p->func->code.len;

			bc_vec_pop(&p->flags);
			bc_vec_pop(&p->exits);
			bc_vec_pop(&p->conds);
		}

		// This needs to be last to parse nested if's properly.
		if (BC_PARSE_IF(p) && (len == p->flags.len || !BC_PARSE_BRACE(p))) {

			while (p->l.t.t == BC_LEX_NLINE) {
				s = bc_lex_next(&p->l);
				if (s) return s;
			}

			bc_vec_pop(&p->flags);
			*(BC_PARSE_TOP_FLAG_PTR(p)) |= BC_PARSE_FLAG_IF_END;

			new_else = (p->l.t.t == BC_LEX_KEY_ELSE);
			if (new_else) s = bc_parse_else(p);
		}

	} while (p->flags.len > 1 && !has_brace && !new_else &&
	         !(has_brace = (BC_PARSE_BRACE(p) != 0)));

	return s;
}

void bc_parse_startBody(BcParse *p, uint16_t flags) {
	flags |= (BC_PARSE_TOP_FLAG(p) & (BC_PARSE_FLAG_FUNC | BC_PARSE_FLAG_LOOP));
	flags |= BC_PARSE_FLAG_BODY;
	bc_vec_push(&p->flags, &flags);
}

void bc_parse_noElse(BcParse *p) {

	BcInstPtr *ip;
	size_t *label;
	uint16_t *flag_ptr = BC_PARSE_TOP_FLAG_PTR(p);

	*flag_ptr = (*flag_ptr & ~(BC_PARSE_FLAG_IF_END));

	ip = bc_vec_top(&p->exits);
	assert(!ip->func && !ip->len);
	assert(p->func == bc_vec_item(&p->prog->fns, p->fidx));
	label = bc_vec_item(&p->func->labels, ip->idx);
	*label = p->func->code.len;

	bc_vec_pop(&p->exits);
}

BcStatus bc_parse_if(BcParse *p) {

	BcStatus s;
	BcInstPtr ip;

	s = bc_lex_next(&p->l);
	if (s) return s;
	if (p->l.t.t != BC_LEX_LPAREN)
		return bc_vm_error(BC_ERROR_PARSE_BAD_TOKEN, p->l.line);

	s = bc_lex_next(&p->l);
	if (s) return s;
	s = bc_parse_expr_status(p, BC_PARSE_REL, bc_parse_next_rel);
	if (s) return s;
	if (p->l.t.t != BC_LEX_RPAREN)
		return bc_vm_error(BC_ERROR_PARSE_BAD_TOKEN, p->l.line);

	s = bc_lex_next(&p->l);
	if (s) return s;
	bc_parse_push(p, BC_INST_JUMP_ZERO);

	ip.idx = p->func->labels.len;
	ip.func = ip.len = 0;

	bc_parse_pushIndex(p, ip.idx);
	bc_vec_push(&p->exits, &ip);
	bc_vec_push(&p->func->labels, &ip.idx);
	bc_parse_startBody(p, BC_PARSE_FLAG_IF);

	return BC_STATUS_SUCCESS;
}

BcStatus bc_parse_else(BcParse *p) {

	BcInstPtr ip;

	if (!BC_PARSE_IF_END(p))
		return bc_vm_error(BC_ERROR_PARSE_BAD_TOKEN, p->l.line);

	ip.idx = p->func->labels.len;
	ip.func = ip.len = 0;

	bc_parse_push(p, BC_INST_JUMP);
	bc_parse_pushIndex(p, ip.idx);

	bc_parse_noElse(p);

	bc_vec_push(&p->exits, &ip);
	bc_vec_push(&p->func->labels, &ip.idx);
	bc_parse_startBody(p, BC_PARSE_FLAG_ELSE);

	return bc_lex_next(&p->l);
}

BcStatus bc_parse_while(BcParse *p) {

	BcStatus s;
	BcInstPtr ip;

	s = bc_lex_next(&p->l);
	if (s) return s;
	if (p->l.t.t != BC_LEX_LPAREN)
		return bc_vm_error(BC_ERROR_PARSE_BAD_TOKEN, p->l.line);
	s = bc_lex_next(&p->l);
	if (s) return s;

	ip.idx = p->func->labels.len;

	bc_vec_push(&p->func->labels, &p->func->code.len);
	bc_vec_push(&p->conds, &ip.idx);

	ip.idx = p->func->labels.len;
	ip.func = 1;
	ip.len = 0;

	bc_vec_push(&p->exits, &ip);
	bc_vec_push(&p->func->labels, &ip.idx);

	s = bc_parse_expr_status(p, BC_PARSE_REL, bc_parse_next_rel);
	if (s) return s;
	if (p->l.t.t != BC_LEX_RPAREN)
		return bc_vm_error(BC_ERROR_PARSE_BAD_TOKEN, p->l.line);
	s = bc_lex_next(&p->l);
	if (s) return s;

	bc_parse_push(p, BC_INST_JUMP_ZERO);
	bc_parse_pushIndex(p, ip.idx);
	bc_parse_startBody(p, BC_PARSE_FLAG_LOOP | BC_PARSE_FLAG_LOOP_INNER);

	return BC_STATUS_SUCCESS;
}

BcStatus bc_parse_for(BcParse *p) {

	BcStatus s;
	BcInstPtr ip;
	size_t cond_idx, exit_idx, body_idx, update_idx;

	s = bc_lex_next(&p->l);
	if (s) return s;
	if (p->l.t.t != BC_LEX_LPAREN)
		return bc_vm_error(BC_ERROR_PARSE_BAD_TOKEN, p->l.line);
	s = bc_lex_next(&p->l);
	if (s) return s;

	if (p->l.t.t != BC_LEX_SCOLON)
		s = bc_parse_expr_status(p, 0, bc_parse_next_for);
	else s = bc_vm_posixError(BC_ERROR_POSIX_FOR1, p->l.line);

	if (s) return s;
	if (p->l.t.t != BC_LEX_SCOLON)
		return bc_vm_error(BC_ERROR_PARSE_BAD_TOKEN, p->l.line);
	s = bc_lex_next(&p->l);
	if (s) return s;

	cond_idx = p->func->labels.len;
	update_idx = cond_idx + 1;
	body_idx = update_idx + 1;
	exit_idx = body_idx + 1;

	bc_vec_push(&p->func->labels, &p->func->code.len);

	if (p->l.t.t != BC_LEX_SCOLON)
		s = bc_parse_expr_status(p, BC_PARSE_REL, bc_parse_next_for);
	else {

		// Set this for the next call to bc_parse_number.
		// This is safe to set because the current token
		// is a semicolon, which has no string requirement.
		bc_vec_string(&p->l.t.v, strlen(bc_parse_const1), bc_parse_const1);
		bc_parse_number(p);

		s = bc_vm_posixError(BC_ERROR_POSIX_FOR2, p->l.line);
	}

	if (s) return s;
	if (p->l.t.t != BC_LEX_SCOLON)
		return bc_vm_error(BC_ERROR_PARSE_BAD_TOKEN, p->l.line);

	s = bc_lex_next(&p->l);
	if (s) return s;

	bc_parse_push(p, BC_INST_JUMP_ZERO);
	bc_parse_pushIndex(p, exit_idx);
	bc_parse_push(p, BC_INST_JUMP);
	bc_parse_pushIndex(p, body_idx);

	ip.idx = p->func->labels.len;

	bc_vec_push(&p->conds, &update_idx);
	bc_vec_push(&p->func->labels, &p->func->code.len);

	if (p->l.t.t != BC_LEX_RPAREN)
		s = bc_parse_expr_status(p, 0, bc_parse_next_rel);
	else s = bc_vm_posixError(BC_ERROR_POSIX_FOR3, p->l.line);

	if (s) return s;

	if (p->l.t.t != BC_LEX_RPAREN)
		return bc_vm_error(BC_ERROR_PARSE_BAD_TOKEN, p->l.line);
	bc_parse_push(p, BC_INST_JUMP);
	bc_parse_pushIndex(p, cond_idx);
	bc_vec_push(&p->func->labels, &p->func->code.len);

	ip.idx = exit_idx;
	ip.func = 1;
	ip.len = 0;

	bc_vec_push(&p->exits, &ip);
	bc_vec_push(&p->func->labels, &ip.idx);
	s = bc_lex_next(&p->l);
	if (!s) bc_parse_startBody(p, BC_PARSE_FLAG_LOOP | BC_PARSE_FLAG_LOOP_INNER);

	return s;
}

BcStatus bc_parse_loopExit(BcParse *p, BcLexType type) {

	BcStatus s;
	size_t i;
	BcInstPtr *ip;

	if (!BC_PARSE_LOOP(p))
		return bc_vm_error(BC_ERROR_PARSE_BAD_TOKEN, p->l.line);

	if (type == BC_LEX_KEY_BREAK) {

		if (p->exits.len == 0)
			return bc_vm_error(BC_ERROR_PARSE_BAD_TOKEN, p->l.line);

		i = p->exits.len - 1;
		ip = bc_vec_item(&p->exits, i);

		while (!ip->func && i < p->exits.len) ip = bc_vec_item(&p->exits, i--);
		assert(ip);
		if (i >= p->exits.len && !ip->func)
			return bc_vm_error(BC_ERROR_PARSE_BAD_TOKEN, p->l.line);

		i = ip->idx;
	}
	else i = *((size_t*) bc_vec_top(&p->conds));

	bc_parse_push(p, BC_INST_JUMP);
	bc_parse_pushIndex(p, i);

	s = bc_lex_next(&p->l);
	if (s) return s;

	if (p->l.t.t != BC_LEX_SCOLON && p->l.t.t != BC_LEX_NLINE)
	{
		bool good = false;

		if (p->l.t.t == BC_LEX_RBRACE) {
			for (i = 0; !good && i < p->flags.len; ++i) {
				uint16_t *fptr = bc_vec_item_rev(&p->flags, i);
				good = ((*fptr) & BC_PARSE_FLAG_BRACE) != 0;
			}
		}
		else if (!BC_PARSE_BRACE(p)) {
			good = p->l.t.t == BC_LEX_KEY_IF || p->l.t.t == BC_LEX_KEY_ELSE ||
			       p->l.t.t == BC_LEX_KEY_FOR || p->l.t.t == BC_LEX_KEY_WHILE;
		}

		if (!good) return bc_vm_error(BC_ERROR_PARSE_BAD_TOKEN, p->l.line);
	}
	else s = bc_lex_next(&p->l);

	return s;
}

BcStatus bc_parse_func(BcParse *p) {

	BcStatus s;
	bool var, comma = false;
	uint16_t flags;
	char *name;
	size_t idx;

	s = bc_lex_next(&p->l);
	if (s) return s;
	if (p->l.t.t != BC_LEX_NAME)
		return bc_vm_error(BC_ERROR_PARSE_BAD_FUNC, p->l.line);

	assert(p->prog->fns.len == p->prog->fn_map.len);

	name = bc_vm_strdup(p->l.t.v.v);
	idx = bc_program_addFunc(p->prog, name);
	assert(idx);
	bc_parse_updateFunc(p, idx);

	s = bc_lex_next(&p->l);
	if (s) return s;
	if (p->l.t.t != BC_LEX_LPAREN)
		return bc_vm_error(BC_ERROR_PARSE_BAD_FUNC, p->l.line);
	s = bc_lex_next(&p->l);
	if (s) return s;

	while (p->l.t.t != BC_LEX_RPAREN) {

		if (p->l.t.t != BC_LEX_NAME)
			return bc_vm_error(BC_ERROR_PARSE_BAD_FUNC, p->l.line);

		++p->func->nparams;

		name = bc_vm_strdup(p->l.t.v.v);
		s = bc_lex_next(&p->l);
		if (s) goto err;

		var = p->l.t.t != BC_LEX_LBRACKET;

		if (!var) {

			s = bc_lex_next(&p->l);
			if (s) goto err;

			if (p->l.t.t != BC_LEX_RBRACKET) {
				s = bc_vm_error(BC_ERROR_PARSE_BAD_FUNC, p->l.line);
				goto err;
			}

			s = bc_lex_next(&p->l);
			if (s) goto err;
		}

		comma = p->l.t.t == BC_LEX_COMMA;
		if (comma) {
			s = bc_lex_next(&p->l);
			if (s) goto err;
		}

		s = bc_func_insert(p->func, name, var, p->l.line);
		if (s) goto err;
	}

	if (comma) return bc_vm_error(BC_ERROR_PARSE_BAD_FUNC, p->l.line);

	flags = BC_PARSE_FLAG_FUNC | BC_PARSE_FLAG_FUNC_INNER | BC_PARSE_FLAG_BODY;
	bc_parse_startBody(p, flags);

	s = bc_lex_next(&p->l);
	if (s) return s;

	if (p->l.t.t != BC_LEX_LBRACE)
		s = bc_vm_posixError(BC_ERROR_POSIX_BRACE, p->l.line);

	return s;

err:
	free(name);
	return s;
}

BcStatus bc_parse_auto(BcParse *p) {

	BcStatus s;
	bool comma, var, one;
	char *name;

	if (!p->auto_part) return bc_vm_error(BC_ERROR_PARSE_BAD_TOKEN, p->l.line);
	s = bc_lex_next(&p->l);
	if (s) return s;

	p->auto_part = comma = false;
	one = p->l.t.t == BC_LEX_NAME;

	while (p->l.t.t == BC_LEX_NAME) {

		name = bc_vm_strdup(p->l.t.v.v);
		s = bc_lex_next(&p->l);
		if (s) goto err;

		var = p->l.t.t != BC_LEX_LBRACKET;
		if (!var) {

			s = bc_lex_next(&p->l);
			if (s) goto err;

			if (p->l.t.t != BC_LEX_RBRACKET) {
				s = bc_vm_error(BC_ERROR_PARSE_BAD_FUNC, p->l.line);
				goto err;
			}

			s = bc_lex_next(&p->l);
			if (s) goto err;
		}

		comma = p->l.t.t == BC_LEX_COMMA;
		if (comma) {
			s = bc_lex_next(&p->l);
			if (s) goto err;
		}

		s = bc_func_insert(p->func, name, var, p->l.line);
		if (s) goto err;
	}

	if (comma) return bc_vm_error(BC_ERROR_PARSE_BAD_FUNC, p->l.line);
	if (!one) return bc_vm_error(BC_ERROR_PARSE_NO_AUTO, p->l.line);

	if (p->l.t.t != BC_LEX_NLINE && p->l.t.t != BC_LEX_SCOLON)
		return bc_vm_error(BC_ERROR_PARSE_BAD_TOKEN, p->l.line);

	return bc_lex_next(&p->l);

err:
	free(name);
	return s;
}

BcStatus bc_parse_body(BcParse *p, bool brace) {

	BcStatus s = BC_STATUS_SUCCESS;
	uint16_t *flag_ptr = BC_PARSE_TOP_FLAG_PTR(p);

	assert(p->flags.len >= 2);

	*flag_ptr &= ~(BC_PARSE_FLAG_BODY);

	if (*flag_ptr & BC_PARSE_FLAG_FUNC_INNER) {

		if (!brace) return bc_vm_error(BC_ERROR_PARSE_BAD_TOKEN, p->l.line);

		p->auto_part = p->l.t.t != BC_LEX_KEY_AUTO;

		if (!p->auto_part) {

			// Make sure this is true to not get a parse error.
			p->auto_part = true;

			s = bc_parse_auto(p);
			if (s) return s;
		}

		if (p->l.t.t == BC_LEX_NLINE) s = bc_lex_next(&p->l);
	}
	else {
		assert(*flag_ptr);
		s = bc_parse_stmt(p);
		if (!s && !brace && !BC_PARSE_BODY(p)) s = bc_parse_endBody(p, false);
	}

	return s;
}

BcStatus bc_parse_stmt(BcParse *p) {

	BcStatus s = BC_STATUS_SUCCESS;

	switch (p->l.t.t) {

		case BC_LEX_NLINE:
		{
			return bc_lex_next(&p->l);
		}

		case BC_LEX_KEY_ELSE:
		{
			p->auto_part = false;
			break;
		}

		case BC_LEX_LBRACE:
		{
			if (!BC_PARSE_BODY(p))
				return bc_vm_error(BC_ERROR_PARSE_BAD_TOKEN, p->l.line);

			*(BC_PARSE_TOP_FLAG_PTR(p)) |= BC_PARSE_FLAG_BRACE;
			s = bc_lex_next(&p->l);
			if (s) return s;

			return bc_parse_body(p, true);
		}

		case BC_LEX_KEY_AUTO:
		{
			return bc_parse_auto(p);
		}

		default:
		{
			p->auto_part = false;

			if (BC_PARSE_IF_END(p)) {
				bc_parse_noElse(p);
				return BC_STATUS_SUCCESS;
			}
			else if (BC_PARSE_BODY(p)) return bc_parse_body(p, false);

			break;
		}
	}

	switch (p->l.t.t) {

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
			s = bc_parse_expr_status(p, BC_PARSE_PRINT, bc_parse_next_expr);
			break;
		}

		case BC_LEX_KEY_ELSE:
		{
			s = bc_parse_else(p);
			break;
		}

		case BC_LEX_SCOLON:
		{
			while (!s && p->l.t.t == BC_LEX_SCOLON) s = bc_lex_next(&p->l);
			break;
		}

		case BC_LEX_RBRACE:
		{
			s = bc_parse_endBody(p, true);
			break;
		}

		case BC_LEX_STR:
		{
			s = bc_parse_str(p, BC_INST_PRINT_STR);
			break;
		}

		case BC_LEX_KEY_BREAK:
		case BC_LEX_KEY_CONTINUE:
		{
			s = bc_parse_loopExit(p, p->l.t.t);
			break;
		}

		case BC_LEX_KEY_FOR:
		{
			s = bc_parse_for(p);
			break;
		}

		case BC_LEX_KEY_HALT:
		{
			bc_parse_push(p, BC_INST_HALT);
			s = bc_lex_next(&p->l);
			break;
		}

		case BC_LEX_KEY_IF:
		{
			s = bc_parse_if(p);
			break;
		}

		case BC_LEX_KEY_LIMITS:
		{
			bc_vm_printf("BC_BASE_MAX     = %lu\n", BC_MAX_OBASE);
			bc_vm_printf("BC_DIM_MAX      = %lu\n", BC_MAX_DIM);
			bc_vm_printf("BC_SCALE_MAX    = %lu\n", BC_MAX_SCALE);
			bc_vm_printf("BC_STRING_MAX   = %lu\n", BC_MAX_STRING);
			bc_vm_printf("BC_NAME_MAX     = %lu\n", BC_MAX_NAME);
			bc_vm_printf("BC_NUM_MAX      = %lu\n", BC_MAX_NUM);
			bc_vm_printf("MAX Exponent    = %lu\n", BC_MAX_EXP);
			bc_vm_printf("Number of vars  = %lu\n", BC_MAX_VARS);

			s = bc_lex_next(&p->l);
			if (s) return s;

			break;
		}

		case BC_LEX_KEY_PRINT:
		{
			s = bc_parse_print(p);
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
			s = bc_parse_return(p);
			break;
		}

		case BC_LEX_KEY_WHILE:
		{
			s = bc_parse_while(p);
			break;
		}

		default:
		{
			s = bc_vm_error(BC_ERROR_PARSE_BAD_TOKEN, p->l.line);
			break;
		}
	}

	return s;
}

BcStatus bc_parse_parse(BcParse *p) {

	BcStatus s;

	assert(p);

	if (p->l.t.t == BC_LEX_EOF) s = bc_vm_error(BC_ERROR_PARSE_EOF, p->l.line);
	else if (p->l.t.t == BC_LEX_KEY_DEFINE) {
		if (!BC_PARSE_CAN_EXEC(p))
			return bc_vm_error(BC_ERROR_PARSE_BAD_TOKEN, p->l.line);
		s = bc_parse_func(p);
	}
	else s = bc_parse_stmt(p);

	if ((s && s != BC_STATUS_QUIT) || BC_SIGINT) s = bc_parse_reset(p, s);

	return s;
}

BcStatus bc_parse_expr_error(BcParse *p, uint8_t flags, BcParseNext next) {

	BcStatus s = BC_STATUS_SUCCESS;
	BcInst prev = BC_INST_PRINT;
	BcLexType top, t = p->l.t.t;
	size_t nexprs = 0, ops_bgn = p->ops.len;
	uint32_t i, nparens, nrelops;
	bool paren_first, paren_expr, rprn, done, get_token, assign, bin_last;

	paren_first = p->l.t.t == BC_LEX_LPAREN;
	nparens = nrelops = 0;
	paren_expr = rprn = done = get_token = assign = false;
	bin_last = true;

	for (; !BC_SIGINT && !s && !done && bc_parse_exprs[t]; t = p->l.t.t) {

		switch (t) {

			case BC_LEX_OP_INC:
			case BC_LEX_OP_DEC:
			{
				s = bc_parse_incdec(p, &prev, &paren_expr, &nexprs, flags);
				rprn = get_token = bin_last = false;
				break;
			}

			case BC_LEX_OP_MINUS:
			{
				s = bc_parse_minus(p, &prev, ops_bgn, rprn, &nexprs);
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
					s = bc_vm_error(BC_ERROR_PARSE_ASSIGN, p->l.line);
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
					return bc_vm_error(BC_ERROR_PARSE_BAD_EXP, p->l.line);
				}

				nrelops += t >= BC_LEX_OP_REL_EQ && t <= BC_LEX_OP_REL_GT;
				prev = BC_PARSE_TOKEN_INST(t);
				s = bc_parse_operator(p, t, ops_bgn, &nexprs, true);
				rprn = get_token = false;
				bin_last = t != BC_LEX_OP_BOOL_NOT;

				break;
			}

			case BC_LEX_LPAREN:
			{
				if (BC_PARSE_LEAF(prev, rprn))
					return bc_vm_error(BC_ERROR_PARSE_BAD_EXP, p->l.line);

				++nparens;
				paren_expr = rprn = bin_last = false;
				get_token = true;
				bc_vec_push(&p->ops, &t);

				break;
			}

			case BC_LEX_RPAREN:
			{
				if (bin_last || prev == BC_INST_BOOL_NOT)
					return bc_vm_error(BC_ERROR_PARSE_BAD_EXP, p->l.line);

				if (nparens == 0) {
					s = BC_STATUS_SUCCESS;
					done = true;
					get_token = false;
					break;
				}
				// This needs to be a status. The error
				// is handled in bc_parse_expr_status().
				else if (!paren_expr) return BC_STATUS_EMPTY_EXPR;

				--nparens;
				paren_expr = rprn = true;
				get_token = bin_last = false;

				s = bc_parse_rightParen(p, ops_bgn, &nexprs);

				break;
			}

			case BC_LEX_NAME:
			{
				if (BC_PARSE_LEAF(prev, rprn))
					return bc_vm_error(BC_ERROR_PARSE_BAD_EXP, p->l.line);

				paren_expr = true;
				rprn = get_token = bin_last = false;
				s = bc_parse_name(p, &prev, flags & ~BC_PARSE_NOCALL);
				++nexprs;

				break;
			}

			case BC_LEX_NUMBER:
			{
				if (BC_PARSE_LEAF(prev, rprn))
					return bc_vm_error(BC_ERROR_PARSE_BAD_EXP, p->l.line);

				bc_parse_number(p);
				nexprs += 1;
				prev = BC_INST_NUM;
				paren_expr = get_token = true;
				rprn = bin_last = false;

				break;
			}

			case BC_LEX_KEY_IBASE:
			case BC_LEX_KEY_LAST:
			case BC_LEX_KEY_OBASE:
			{
				if (BC_PARSE_LEAF(prev, rprn))
					return bc_vm_error(BC_ERROR_PARSE_BAD_EXP, p->l.line);

				prev = (char) (t - BC_LEX_KEY_IBASE + BC_INST_IBASE);
				bc_parse_push(p, (char) prev);

				paren_expr = get_token = true;
				rprn = bin_last = false;
				++nexprs;

				break;
			}

			case BC_LEX_KEY_LENGTH:
			case BC_LEX_KEY_SQRT:
			{
				if (BC_PARSE_LEAF(prev, rprn))
					return bc_vm_error(BC_ERROR_PARSE_BAD_EXP, p->l.line);

				s = bc_parse_builtin(p, t, flags, &prev);
				paren_expr = true;
				rprn = get_token = bin_last = false;
				++nexprs;

				break;
			}

			case BC_LEX_KEY_READ:
			{
				if (BC_PARSE_LEAF(prev, rprn))
					return bc_vm_error(BC_ERROR_PARSE_BAD_EXP, p->l.line);
				else if (flags & BC_PARSE_NOREAD)
					s = bc_vm_error(BC_ERROR_EXEC_REC_READ, p->l.line);
				else s = bc_parse_read(p);

				paren_expr = true;
				rprn = get_token = bin_last = false;
				++nexprs;
				prev = BC_INST_READ;

				break;
			}

			case BC_LEX_KEY_SCALE:
			{
				if (BC_PARSE_LEAF(prev, rprn))
					return bc_vm_error(BC_ERROR_PARSE_BAD_EXP, p->l.line);

				s = bc_parse_scale(p, &prev, flags);
				paren_expr = true;
				rprn = get_token = bin_last = false;
				++nexprs;
				prev = BC_INST_SCALE;

				break;
			}

			default:
			{
				s = bc_vm_error(BC_ERROR_PARSE_BAD_TOKEN, p->l.line);
				break;
			}
		}

		if (!s && get_token) s = bc_lex_next(&p->l);
	}

	if (s) return s;
	if (BC_SIGINT) return BC_STATUS_EXEC_SIGNAL;

	while (p->ops.len > ops_bgn) {

		top = BC_PARSE_TOP_OP(p);
		assign = top >= BC_LEX_OP_ASSIGN_POWER && top <= BC_LEX_OP_ASSIGN;

		if (top == BC_LEX_LPAREN || top == BC_LEX_RPAREN)
			return bc_vm_error(BC_ERROR_PARSE_BAD_EXP, p->l.line);

		bc_parse_push(p, BC_PARSE_TOKEN_INST(top));

		nexprs -= top != BC_LEX_OP_BOOL_NOT && top != BC_LEX_NEG;
		bc_vec_pop(&p->ops);
	}

	if (prev == BC_INST_BOOL_NOT || nexprs != 1)
		return bc_vm_error(BC_ERROR_PARSE_BAD_EXP, p->l.line);

	for (i = 0; i < next.len && t != next.tokens[i]; ++i);
	if (i == next.len && t != BC_LEX_KEY_ELSE && t != BC_LEX_EOF)
		return bc_vm_error(BC_ERROR_PARSE_BAD_EXP, p->l.line);

	if (!(flags & BC_PARSE_REL) && nrelops) {
		s = bc_vm_posixError(BC_ERROR_POSIX_REL_POS, p->l.line);
		if (s) return s;
	}
	else if ((flags & BC_PARSE_REL) && nrelops > 1) {
		s = bc_vm_posixError(BC_ERROR_POSIX_MULTIREL, p->l.line);
		if (s) return s;
	}

	if (flags & BC_PARSE_PRINT) {
		if (paren_first || !assign) bc_parse_push(p, BC_INST_PRINT);
		bc_parse_push(p, BC_INST_POP);
	}

	return s;
}

BcStatus bc_parse_expr_status(BcParse *p, uint8_t flags, BcParseNext next) {

	BcStatus s = bc_parse_expr_error(p, flags, next);

	if (s == BC_STATUS_EMPTY_EXPR)
		s = bc_vm_error(BC_ERROR_PARSE_EMPTY_EXPR, p->l.line);

	return s;
}

void bc_parse_init(BcParse *p, BcProgram *prog, size_t func) {
	bc_parse_create(p, prog, func, bc_parse_parse, bc_lex_token);
}

BcStatus bc_parse_expr(BcParse *p, uint8_t flags) {
	assert(p);
	return bc_parse_expr_status(p, flags, bc_parse_next_read);
}
#endif // BC_ENABLED
