#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <arbprec/arbprec.h>

#include "program.h"

static BcStatus bc_program_list_expand(BcStmtList* list);

static int bc_program_func_cmp(void* func1, void* func2);
static void bc_program_func_free(void* func);
static int bc_program_var_cmp(void* var1, void* var2);
static void bc_program_var_free(void* var);
static int bc_program_array_cmp(void* array1, void* array2);
static void bc_program_array_free(void* array);
static void bc_program_stmt_free(BcStmt* stmt);

BcStatus bc_program_init(BcProgram* p) {

	BcStatus st;

	if (p == NULL) {
		return BC_STATUS_INVALID_PARAM;
	}

	p->first = bc_program_list_create();

	if (!p->first) {
		return BC_STATUS_MALLOC_FAIL;
	}

	p->cur = p->first;

	st = bc_segarray_init(&p->funcs, sizeof(BcFunc), bc_program_func_free,
	                      bc_program_func_cmp);

	if (st) {
		goto func_err;
	}

	st = bc_segarray_init(&p->vars, sizeof(BcVar), bc_program_var_free,
	                      bc_program_var_cmp);

	if (st) {
		goto var_err;
	}

	st = bc_segarray_init(&p->arrays, sizeof(BcArray), bc_program_array_free,
	                      bc_program_array_cmp);

	if (st) {
		goto array_err;
	}

	return st;

array_err:

	bc_segarray_free(&p->vars);

var_err:

	bc_segarray_free(&p->funcs);

func_err:

	bc_program_list_free(p->first);
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
}

void bc_program_free(BcProgram* p) {

	if (p == NULL) {
		return;
	}

	BcStmtList* temp;
	BcStmtList* cur = p->first;

	while (cur != NULL) {
		temp = cur->next;
		bc_program_list_free(cur);
		cur = temp;
	}

	p->cur = NULL;
	p->first = NULL;

	uint32_t num = p->funcs.num;
	for (uint32_t i = 0; i < num; ++i) {
		bc_program_func_free(bc_segarray_item(&p->funcs, i));
	}

	num = p->vars.num;
	for (uint32_t i = 0; i < num; ++i) {
		bc_program_var_free(bc_segarray_item(&p->vars, i));
	}

	num = p->arrays.num;
	for (uint32_t i = 0; i < num; ++i) {
		bc_program_array_free(bc_segarray_item(&p->arrays, i));
	}
}

BcStmtList* bc_program_list_create() {

	BcStmtList* list;

	list = malloc(sizeof(BcStmtList));

	if (list == NULL) {
		return NULL;
	}

	list->next = NULL;
	list->num_stmts = 0;

	return list;
}

BcStatus bc_program_list_insert(BcStmtList* list, BcStmt* stmt) {

	if (list == NULL || stmt == NULL) {
		return BC_STATUS_INVALID_PARAM;
	}

	// Find the end list.
	while (list->num_stmts == BC_PROGRAM_MAX_STMTS && list->next) {
		list = list->next;
	}

	if (list->num_stmts == BC_PROGRAM_MAX_STMTS) {

		BcStatus status = bc_program_list_expand(list);

		if (status != BC_STATUS_SUCCESS) {
			return status;
		}

		list = list->next;
	}

	memcpy(list->stmts + list->num_stmts, stmt, sizeof(BcStmt));
	++list->num_stmts;

	return BC_STATUS_SUCCESS;
}

void bc_program_list_free(BcStmtList* list) {

	BcStmtList* temp;
	uint32_t num;
	BcStmt* stmts;

	if (list == NULL) {
		return;
	}

	do {

		temp = list->next;

		num = list->num_stmts;
		stmts = list->stmts;

		for (uint32_t i = 0; i < num; ++i) {
			bc_program_stmt_free(stmts + i);
		}

		free(list);

		list = temp;

	} while (list);
}

static BcStatus bc_program_list_expand(BcStmtList* list) {

	if (list == NULL) {
		return BC_STATUS_INVALID_PARAM;
	}

	BcStmtList* next = bc_program_list_create();

	if (next == NULL) {
		return BC_STATUS_MALLOC_FAIL;
	}

	list->next = next;

	return BC_STATUS_SUCCESS;
}

BcStatus bc_program_func_init(BcFunc* func, char* name) {

	if (!func || !name) {
		return BC_STATUS_INVALID_PARAM;
	}

	func->first = bc_program_list_create();

	if (!func->first) {
		return BC_STATUS_MALLOC_FAIL;
	}

	func->name = name;
	func->cur = func->first;
	func->num_autos = 0;

	return BC_STATUS_SUCCESS;
}

static int bc_program_func_cmp(void* func1, void* func2) {

	assert(func1 && func2);

	BcFunc* f1 = (BcFunc*) func1;
	char* f2name = (char*) func2;

	return strcmp(f1->name, f2name);
}

static void bc_program_func_free(void* func) {

	BcFunc* f;

	f = (BcFunc*) func;

	if (f == NULL) {
		return;
	}

	free(f->name);

	bc_program_list_free(f->first);

	f->name = NULL;
	f->first = NULL;
	f->cur = NULL;
	f->num_autos = 0;
}

BcStatus bc_program_var_init(BcVar* var, char* name) {

	if (!var || !name) {
		return BC_STATUS_INVALID_PARAM;
	}

	var->name = name;

	var->data = arb_alloc(16);

	return BC_STATUS_SUCCESS;
}

static int bc_program_var_cmp(void* var1, void* var2) {

	assert(var1 && var2);

	BcFunc* v1 = (BcFunc*) var1;
	char* v2name = (char*) var2;

	return strcmp(v1->name, v2name);
}

static void bc_program_var_free(void* var) {

	BcVar* v;

	v = (BcVar*) var;

	if (v == NULL) {
		return;
	}

	free(v->name);
	arb_free(v->data);

	v->name = NULL;
	v->data = NULL;
}

BcStatus bc_program_array_init(BcArray* array, char* name) {

	if (!array || !name) {
		return BC_STATUS_INVALID_PARAM;
	}

	array->name = name;

	return bc_segarray_init(&array->array, sizeof(fxdpnt),
	                        (BcSegArrayFreeFunc) arb_free, NULL);
}

static int bc_program_array_cmp(void* array1, void* array2) {

	assert(array1 && array2);

	BcFunc* a1 = (BcFunc*) array1;
	char* a2name = (char*) array2;

	return strcmp(a1->name, a2name);
}

static void bc_program_array_free(void* array) {

	BcArray* a;

	a = (BcArray*) array;

	if (a == NULL) {
		return;
	}

	free(a->name);
	a->name = NULL;

	bc_segarray_free(&a->array);
}

BcStatus bc_program_stmt_init(BcStmt* stmt) {
	// TODO: Write this function.
}

static void bc_program_stmt_free(BcStmt* stmt) {
	// TODO: Write this function.
}
