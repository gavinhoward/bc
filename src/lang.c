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
#include <vm.h>

int bc_id_cmp(const void *e1, const void *e2) {
	return strcmp(((const BcId*) e1)->name, ((const BcId*) e2)->name);
}

void bc_id_free(void *id) {
	assert(id && ((BcId*) id)->name);
	free(((BcId*) id)->name);
}

BcStatus bc_func_insert(BcFunc *f, char *name, bool var) {

	BcId a;
	size_t i;

	assert(f && name);

	for (i = 0; i < f->autos.len; ++i) {
		if (!strcmp(name, ((BcId*) bc_vec_item(&f->autos, i))->name))
			return BC_STATUS_PARSE_DUPLICATE_LOCAL;
	}

	a.idx = var;
	a.name = name;

	bc_vec_push(&f->autos, &a);

	return BC_STATUS_SUCCESS;
}

void bc_func_init(BcFunc *f) {
	assert(f);
	bc_vec_init(&f->code, sizeof(char), NULL);
	bc_vec_init(&f->autos, sizeof(BcId), bc_id_free);
	bc_vec_init(&f->labels, sizeof(size_t), NULL);
	f->nparams = 0;
}

void bc_func_free(void *func) {
	BcFunc *f = (BcFunc*) func;
	assert(f);
	bc_vec_free(&f->code);
	bc_vec_free(&f->autos);
	bc_vec_free(&f->labels);
}

void bc_array_init(BcVec *a, bool nums) {
	if (nums) bc_vec_init(a, sizeof(BcNum), bc_num_free);
	else bc_vec_init(a, sizeof(BcVec), bc_vec_free);
	bc_array_expand(a, 1);
}

void bc_array_copy(BcVec *d, const BcVec *s) {

	size_t i;

	assert(d && s && d != s && d->size == s->size && d->dtor == s->dtor);

	bc_vec_npop(d, d->len);
	bc_vec_expand(d, s->cap);
	d->len = s->len;

	for (i = 0; i < s->len; ++i) {
		BcNum *dnum = bc_vec_item(d, i), *snum = bc_vec_item(s, i);
		bc_num_init(dnum, snum->len);
		bc_num_copy(dnum, snum);
	}
}

void bc_array_expand(BcVec *a, size_t len) {

	BcResultData data;

	assert(a);

	if (a->size == sizeof(BcNum) && a->dtor == bc_num_free) {
		while (len > a->len) {
			bc_num_init(&data.n, BC_NUM_DEF_SIZE);
			bc_vec_push(a, &data.n);
		}
	}
	else {
		assert(a->size == sizeof(BcVec) && a->dtor == bc_vec_free);
		while (len > a->len) {
			bc_array_init(&data.v, true);
			bc_vec_push(a, &data.v);
		}
	}
}

void bc_string_free(void *string) {
	assert(string && *((char**) string));
	free(*((char**) string));
}

#ifdef DC_ENABLED
void bc_result_copy(BcResult *d, BcResult *src) {

	assert(d && src);

	switch ((d->t = src->t)) {

		case BC_RESULT_TEMP:
		case BC_RESULT_IBASE:
		case BC_RESULT_SCALE:
		case BC_RESULT_OBASE:
		{
			bc_num_init(&d->d.n, src->d.n.len);
			bc_num_copy(&d->d.n, &src->d.n);
			break;
		}

		case BC_RESULT_VAR:
		case BC_RESULT_ARRAY:
		case BC_RESULT_ARRAY_ELEM:
		{
			assert(src->d.id.name);
			d->d.id.name = bc_vm_strdup(src->d.id.name);
			break;
		}

		case BC_RESULT_CONSTANT:
		case BC_RESULT_LAST:
		case BC_RESULT_ONE:
		case BC_RESULT_STR:
		{
			memcpy(&d->d.n, &src->d.n, sizeof(BcNum));
			break;
		}
	}
}
#endif // DC_ENABLED

void bc_result_free(void *result) {

	BcResult *r = (BcResult*) result;

	assert(r);

	switch (r->t) {

		case BC_RESULT_TEMP:
		case BC_RESULT_IBASE:
		case BC_RESULT_SCALE:
		case BC_RESULT_OBASE:
		{
			bc_num_free(&r->d.n);
			break;
		}

		case BC_RESULT_VAR:
		case BC_RESULT_ARRAY:
		case BC_RESULT_ARRAY_ELEM:
		{
			assert(r->d.id.name);
			free(r->d.id.name);
			break;
		}

		default:
		{
			// Do nothing.
			break;
		}
	}
}
