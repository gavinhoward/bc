#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <arbprec/arbprec.h>

#include <bc/program.h>

BcStatus bc_program_init(BcProgram* p, const char* file) {

	BcStatus st;

	if (p == NULL) {
		return BC_STATUS_INVALID_PARAM;
	}

	p->file = file;

	p->first = bc_list_create();

	if (!p->first) {
		return BC_STATUS_MALLOC_FAIL;
	}

	p->cur = p->first;

	st = bc_segarray_init(&p->funcs, sizeof(BcFunc), bc_func_free, bc_func_cmp);

	if (st) {
		goto func_err;
	}

	st = bc_segarray_init(&p->vars, sizeof(BcVar), bc_var_free, bc_var_cmp);

	if (st) {
		goto var_err;
	}

	st = bc_segarray_init(&p->arrays, sizeof(BcArray),
	                      bc_array_free, bc_array_cmp);

	if (st) {
		goto array_err;
	}

	return st;

array_err:

	bc_segarray_free(&p->vars);

var_err:

	bc_segarray_free(&p->funcs);

func_err:

	bc_list_free(p->first);
	p->first = NULL;
	p->cur = NULL;

	return st;
}

BcStatus bc_program_func_add(BcProgram* p, BcFunc* func) {
	assert(p && func);
	return bc_segarray_add(&p->funcs, func);
}

BcStatus bc_program_var_add(BcProgram* p, BcVar* var) {
	assert(p && var);
	return bc_segarray_add(&p->vars, var);
}

BcStatus bc_program_array_add(BcProgram* p, BcArray* array) {
	assert(p && array);
	return bc_segarray_add(&p->arrays, array);
}

BcStatus bc_program_exec(BcProgram* p) {
	// TODO: Write this function.
	return BC_STATUS_SUCCESS;
}

void bc_program_free(BcProgram* p) {

	if (p == NULL) {
		return;
	}

	bc_list_free(p->first);

	bc_segarray_free(&p->funcs);
	bc_segarray_free(&p->vars);
	bc_segarray_free(&p->arrays);
}
