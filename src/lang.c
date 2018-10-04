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
 * Code to manipulate data structures in programs.
 *
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <lang.h>

void bc_auto_free(void *auto1) {
	BcAuto *a = (BcAuto*) auto1;
	assert(a && a->name);
	free(a->name);
}

BcStatus bc_func_insert(BcFunc *f, char *name, bool var) {

	BcAuto a;
	size_t i;

	assert(f && name);

	for (i = 0; i < f->autos.len; ++i) {
		if (!strcmp(name, ((BcAuto*) bc_vec_item(&f->autos, i))->name))
			return BC_STATUS_PARSE_DUPLICATE_LOCAL;
	}

	a.var = var;
	a.name = name;

	return bc_vec_push(&f->autos, &a);
}

BcStatus bc_func_init(BcFunc *f) {

	BcStatus s;

	assert(f);

	if ((s = bc_vec_init(&f->code, sizeof(char), NULL))) return s;
	if ((s = bc_vec_init(&f->autos, sizeof(BcAuto), bc_auto_free))) goto err;
	if ((s = bc_vec_init(&f->labels, sizeof(size_t), NULL))) goto label_err;

	f->nparams = 0;

	return BC_STATUS_SUCCESS;

label_err:
	bc_vec_free(&f->autos);
err:
	bc_vec_free(&f->code);
	return s;
}

void bc_func_free(void *func) {
	BcFunc *f = (BcFunc*) func;
	assert(f);
	bc_vec_free(&f->code);
	bc_vec_free(&f->autos);
	bc_vec_free(&f->labels);
}

BcStatus bc_array_init(BcVec *a, bool nums) {

	BcStatus s;

	if (nums) {
		if ((s = bc_vec_init(a, sizeof(BcNum), bc_num_free))) return s;
	}
	else if ((s = bc_vec_init(a, sizeof(BcVec), bc_vec_free))) return s;

	if ((s = bc_array_expand(a, 1))) goto err;

	return s;

err:
	bc_vec_free(a);
	return s;
}

BcStatus bc_array_copy(BcVec *d, const BcVec *s) {

	BcStatus status;
	size_t i;

	assert(d && s && d != s && d->size == s->size && d->dtor == s->dtor);

	bc_vec_npop(d, d->len);
	if ((status = bc_vec_expand(d, s->cap))) return status;
	d->len = s->len;

	for (i = 0; !status && i < s->len; ++i) {
		BcNum *dnum = bc_vec_item(d, i), *snum = bc_vec_item(s, i);
		if ((status = bc_num_init(dnum, snum->len))) return status;
		if ((status = bc_num_copy(dnum, snum))) bc_num_free(dnum);
	}

	return status;
}

BcStatus bc_array_expand(BcVec *a, size_t len) {

	BcStatus s = BC_STATUS_SUCCESS;
	BcNum n;
	BcVec v;

	assert(a);

	if (a->size == sizeof(BcNum) && a->dtor == bc_num_free) {
		while (!s && len > a->len) {
			if ((s = bc_num_init(&n, BC_NUM_DEF_SIZE))) return s;
			bc_num_zero(&n);
			if ((s = bc_vec_push(a, &n))) bc_num_free(&n);
		}
	}
	else {
		assert(a->size == sizeof(BcVec) && a->dtor == bc_vec_free);
		while (!s && len > a->len) {
			if ((s = bc_array_init(&v, true))) return s;
			if ((s = bc_vec_push(a, &v))) bc_vec_free(&v);
		}
	}

	return s;
}

void bc_string_free(void *string) {
	char **s = string;
	assert(s && *s);
	free(*s);
}

int bc_entry_cmp(const void *e1, const void *e2) {
	return strcmp(((const BcEntry*) e1)->name, ((const BcEntry*) e2)->name);
}

void bc_entry_free(void *entry) {
	BcEntry *e = entry;
	assert(e && e->name);
	free(e->name);
}

BcStatus bc_result_copy(BcResult *d, BcResult *s) {

	BcStatus status = BC_STATUS_SUCCESS;

	assert(d && s);

	switch (s->t) {

		case BC_RESULT_TEMP:
		case BC_RESULT_SCALE:
		{
			status = bc_num_copy(&d->data.n, &s->data.n);
			break;
		}

		case BC_RESULT_VAR:
		case BC_RESULT_ARRAY:
		case BC_RESULT_ARRAY_ELEM:
		{
			assert(s->data.id.name);
			if (!(d->data.id.name = strdup(s->data.id.name)))
				status = BC_STATUS_ALLOC_ERR;
			else status = BC_STATUS_SUCCESS;
			break;
		}

		default:
		{
			// Do nothing.
			break;
		}
	}

	return status;
}

void bc_result_free(void *result) {

	BcResult *r = (BcResult*) result;

	assert(r);

	switch (r->t) {

		case BC_RESULT_TEMP:
		case BC_RESULT_SCALE:
		{
			bc_num_free(&r->data.n);
			break;
		}

		case BC_RESULT_VAR:
		case BC_RESULT_ARRAY:
		case BC_RESULT_ARRAY_ELEM:
		{
			assert(r->data.id.name);
			free(r->data.id.name);
			break;
		}

		default:
		{
			// Do nothing.
			break;
		}
	}
}
