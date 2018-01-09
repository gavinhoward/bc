#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "program.h"

static int bc_program_func_cmp(void* func1, void* func2);
static void bc_program_func_free(BcFunc* func);
static int bc_program_var_cmp(void* var1, void* var2);
static void bc_program_var_free(BcVar* var);
static int bc_program_array_cmp(void* array1, void* array2);
static void bc_program_array_free(BcArray* array);
static BcStatus bc_program_stmt_init(BcStmt* stmt);
static void bc_program_stmt_free(BcStmt* stmt);

BcStatus bc_program_init(BcProgram* p) {

	BcStatus st;

	if (p == NULL) {
		return BC_STATUS_INVALID_PARAM;
	}

	p->first = malloc(sizeof(BcStmtList));

	if (p->first == NULL) {
		return BC_STATUS_MALLOC_FAIL;
	}

	p->first = bc_program_list_create();

	if (!p->first) {
		return BC_STATUS_MALLOC_FAIL;
	}

	p->cur = p->first;

	st = bc_segarray_init(&p->funcs, sizeof(BcFunc), bc_program_func_cmp);
	if (st) {
		return st;
	}

	st = bc_segarray_init(&p->vars, sizeof(BcVar), bc_program_var_cmp);
	if (st) {
		return st;
	}

	st = bc_segarray_init(&p->arrays, sizeof(BcArray), bc_program_array_cmp);

	return st;
}

BcStatus bc_program_insert(BcProgram* p, BcStmt* stmt) {

	BcStmtList* cur;

	if (p == NULL || stmt == NULL) {
		return BC_STATUS_INVALID_PARAM;
	}

	cur = p->cur;

	if (cur->num_stmts == BC_PROGRAM_MAX_STMTS) {

		BcStatus status = bc_program_list_expand(cur);

		if (status != BC_STATUS_SUCCESS) {
			return status;
		}

		cur = cur->next;
	}

	memcpy(cur->stmts + cur->num_stmts, stmt, sizeof(BcStmt));

	return BC_STATUS_SUCCESS;
}

BcStatus bc_program_func_add(BcProgram* p, BcFunc* func) {
	assert(p && func);
	return bc_segarrary_add(&p->funcs, func);
}

BcStatus bc_program_var_add(BcProgram* p, BcVar* var) {
	assert(p && var);
	return bc_segarrary_add(&p->vars, var);
}

BcStatus bc_program_array_add(BcProgram* p, BcArray* array) {
	assert(p && array);
	return bc_segarrary_add(&p->arrays, array);
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

BcStatus bc_program_list_expand(BcStmtList* list) {

	if (list == NULL) {
		return BC_STATUS_INVALID_PARAM;
	}

	BcStmtList* next = malloc(sizeof(BcStmtList));

	if (next == NULL) {
		return BC_STATUS_MALLOC_FAIL;
	}

	next->next = NULL;
	next->num_stmts = 0;

	list->next = next;

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

static int bc_program_func_cmp(void* func1, void* func2) {

	assert(func1 && func2);

	BcFunc* f1 = (BcFunc*) func1;
	char* f2name = (char*) func2;

	return strcmp(f1->name, f2name);
}

static void bc_program_func_free(BcFunc* func) {

	if (func == NULL) {
		return;
	}

	free(func->name);

	bc_program_list_free(func->first);

	func->name = NULL;
	func->first = NULL;
	func->cur = NULL;
	func->num_autos = 0;
}

static int bc_program_var_cmp(void* var1, void* var2) {

	assert(var1 && var2);

	BcFunc* v1 = (BcFunc*) var1;
	char* v2name = (char*) var2;

	return strcmp(v1->name, v2name);
}

static void bc_program_var_free(BcVar* var) {

	if (var == NULL) {
		return;
	}

	free(var->name);
	arb_free(var->data);

	var->name = NULL;
	var->data = NULL;
}

static int bc_program_array_cmp(void* array1, void* array2) {

	assert(array1 && array2);

	BcFunc* a1 = (BcFunc*) array1;
	char* a2name = (char*) array2;

	return strcmp(a1->name, a2name);
}

static void bc_program_array_free(BcArray* array) {

	if (array == NULL) {
		return;
	}

	free(array->name);

	uint32_t elems = array->elems;
	fxdpnt** nums = array->array;
	for (uint32_t i = 0; i < elems; ++i) {
		arb_free(nums[i]);
	}

	array->name = NULL;
	array->array = NULL;
}

static BcStatus bc_program_stmt_init(BcStmt* stmt) {
	// TODO: Write this function.
}

static void bc_program_stmt_free(BcStmt* stmt) {
	// TODO: Write this function.
}
