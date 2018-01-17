#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <arbprec/arbprec.h>

#include <bc/program.h>

static const uint32_t bc_stmt_sizes[] = {

    sizeof(BcStack),

    0,

    0,
    0,

    0,

    sizeof(BcStack),

    sizeof(BcIf),
    sizeof(BcWhile),
    sizeof(BcFor),

    0,

};

static const uint32_t bc_expr_sizes[] = {

    0,
    0,

    0,
    0,

    0,

    0,

    0,
    0,
    0,

    0,
    0,

    0,
    0,
    0,
    0,
    0,
    0,

    0,
    0,
    0,
    0,
    0,
    0,

    0,

    0,
    0,

    0,
    0,
    0,

    sizeof(BcCall),

    0,
    sizeof(BcStack),
    0,
    0,
    0,
    sizeof(BcStack),
    0,
    sizeof(BcStack),

    0

};

static BcStatus bc_program_list_expand(BcStmtList* list);

static int bc_program_func_cmp(void* func1, void* func2);
static void bc_program_func_free(void* func);
static int bc_program_var_cmp(void* var1, void* var2);
static void bc_program_var_free(void* var);
static int bc_program_array_cmp(void* array1, void* array2);
static void bc_program_array_free(void* array);
static void bc_program_stmt_free(BcStmt* stmt);

static void bc_program_num_init(fxdpnt* num);

BcStatus bc_program_init(BcProgram* p, const char* file) {

	BcStatus st;

	if (p == NULL) {
		return BC_STATUS_INVALID_PARAM;
	}

	p->file = file;

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
	return BC_STATUS_SUCCESS;
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

BcStmt* bc_program_list_last(BcStmtList* list) {

	if (!list) {
		return NULL;
	}

	while (list->next) {
		list = list->next;
	}

	return list->stmts + (list->num_stmts - 1);
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

	BcStatus status;

	status = BC_STATUS_SUCCESS;

	if (!func || !name) {
		return BC_STATUS_INVALID_PARAM;
	}

	func->num_params = 0;
	func->num_autos = 0;

	func->first = bc_program_list_create();

	if (!func->first) {
		return BC_STATUS_MALLOC_FAIL;
	}

	func->name = name;
	func->cur = func->first;

	func->param_cap = BC_PROGRAM_DEF_SIZE;
	func->params = malloc(sizeof(BcAuto) * BC_PROGRAM_DEF_SIZE);

	if (!func->params) {
		goto param_err;
	}

	func->auto_cap = BC_PROGRAM_DEF_SIZE;
	func->autos = malloc(sizeof(BcAuto) * BC_PROGRAM_DEF_SIZE);

	if (!func->autos) {
		func->auto_cap = 0;
		goto auto_err;
	}

	return BC_STATUS_SUCCESS;

auto_err:

	free(func->params);
	func->param_cap = 0;

param_err:

	bc_program_list_free(func->first);
	func->first = func->cur = NULL;

	return status;
}

BcStatus bc_program_func_insertParam(BcFunc* func, char* name, bool var) {

	BcAuto* params;
	size_t new_cap;

	if (!func || !name) {
		return BC_STATUS_INVALID_PARAM;
	}

	if (func->num_params == func->param_cap) {

		new_cap = func->param_cap * 2;
		params = realloc(func->params, new_cap * sizeof(BcAuto));

		if (!params) {
			return BC_STATUS_MALLOC_FAIL;
		}

		func->param_cap = new_cap;
	}

	func->params[func->num_params].name = name;
	func->params[func->num_params].var = var;

	++func->num_params;

	return BC_STATUS_SUCCESS;
}

BcStatus bc_program_func_insertAuto(BcFunc* func, char* name, bool var) {

	BcAuto* autos;
	size_t new_cap;

	if (!func || !name) {
		return BC_STATUS_INVALID_PARAM;
	}

	if (func->num_autos == func->auto_cap) {

		new_cap = func->auto_cap * 2;
		autos = realloc(func->autos, new_cap * sizeof(BcAuto));

		if (!autos) {
			return BC_STATUS_MALLOC_FAIL;
		}

		func->auto_cap = new_cap;
	}

	func->autos[func->num_autos].name = name;
	func->autos[func->num_autos].var = var;

	++func->num_autos;

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

	free(f->params);
	free(f->autos);

	f->params = NULL;
	f->num_params = 0;
	f->param_cap = 0;
	f->autos = NULL;
	f->num_autos = 0;
	f->auto_cap = 0;
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

BcStatus bc_program_stmt_init(BcStmt* stmt, BcStmtType type) {

	stmt->type = type;

	if (bc_stmt_sizes[type]) {

		stmt->data.expr_stack = malloc(bc_stmt_sizes[type]);

		if (!stmt->data.expr_stack) {
			return BC_STATUS_MALLOC_FAIL;
		}
	}
	else if (type == BC_STMT_LIST) {

		stmt->data.list = bc_program_list_create();

		if (!stmt->data.list) {
			return BC_STATUS_MALLOC_FAIL;
		}
	}

	return BC_STATUS_SUCCESS;
}

static void bc_program_stmt_free(BcStmt* stmt) {

	switch (stmt->type) {

		case BC_STMT_EXPR:
		case BC_STMT_RETURN:
		{
			bc_stack_free(stmt->data.expr_stack);
			break;
		}

		case BC_STMT_STRING:
		{
			free(stmt->data.string);
			break;
		}

		case BC_STMT_IF:
		{
			bc_stack_free(&stmt->data.if_stmt->cond);
			bc_program_list_free(stmt->data.if_stmt->then_list);
			bc_program_list_free(stmt->data.if_stmt->else_list);

			stmt->data.if_stmt->then_list = NULL;
			stmt->data.if_stmt->else_list = NULL;

			break;
		}

		case BC_STMT_WHILE:
		{
			bc_stack_free(&stmt->data.while_stmt->cond);
			bc_program_list_free(stmt->data.while_stmt->body);

			stmt->data.while_stmt->body = NULL;

			break;
		}

		case BC_STMT_FOR:
		{
			bc_stack_free(&stmt->data.for_stmt->cond);
			bc_stack_free(&stmt->data.for_stmt->update);
			bc_stack_free(&stmt->data.for_stmt->init);
			bc_program_list_free(stmt->data.for_stmt->body);

			stmt->data.for_stmt->body = NULL;

			break;
		}

		case BC_STMT_LIST:
		{
			bc_program_list_free(stmt->data.list);
			break;
		}

		default:
		{
			// Do nothing.
			break;
		}
	}

	stmt->data.expr_stack = NULL;
}

BcIf* bc_program_if_create() {

	BcIf* if_stmt;

	if_stmt = malloc(sizeof(BcIf));

	if (!if_stmt) {
		return NULL;
	}

	if_stmt->then_list = NULL;
	if_stmt->else_list = NULL;

	return if_stmt;
}

BcWhile* bc_program_while_create() {

	BcWhile* while_stmt;

	while_stmt = malloc(sizeof(BcWhile));

	return while_stmt;
}

BcFor* bc_program_for_create() {

	BcFor* for_stmt;

	for_stmt = malloc(sizeof(BcFor));

	return for_stmt;
}

BcStatus bc_program_expr_init(BcExpr* expr, BcExprType type) {

	expr->type = type;

	if (bc_expr_sizes[type]) {

		expr->string = malloc(bc_expr_sizes[type]);

		if (!expr->string) {
			return BC_STATUS_MALLOC_FAIL;
		}
	}

	return BC_STATUS_SUCCESS;
}

static void bc_program_expr_free(BcExpr* expr) {

	switch (expr->type) {

		case BC_EXPR_NUMBER:
		case BC_EXPR_VAR:
		{
			free(expr->string);
			break;
		}

		case BC_EXPR_ARRAY_ELEM:
		{
			free(expr->elem->name);
			bc_stack_free(&expr->elem->expr_stack);
			free(expr->elem);
			break;
		}

		case BC_EXPR_FUNC_CALL:
		{
			free(expr->call->name);
			bc_segarray_free(&expr->call->params);
			free(expr->call);
			break;
		}

		case BC_EXPR_SCALE_FUNC:
		case BC_EXPR_LENGTH:
		case BC_EXPR_SQRT:
		{
			bc_stack_free(&expr->expr_stack);
			break;
		}

		default:
		{
			// Do nothing.
			break;
		}
	}

	expr->string = NULL;
}

BcCall* bc_program_call_create() {

	BcCall* call;

	call = malloc(sizeof(BcCall));

	if (!call) {
		return NULL;
	}

	call->name = NULL;

	return call;
}

static void bc_program_num_init(fxdpnt* num) {

	num->number = arb_calloc(1, sizeof(ARBT) * BC_PROGRAM_DEF_SIZE);
	num->sign = '+';

	// FIXME: this should likely be "len"
	// Look at arb_alloc().
	num->lp = 0;

	num->rp = 0;
	num->allocated = BC_PROGRAM_DEF_SIZE;
	//num->len = len;
	num->len = 0;
	num->chunk = 4;
}
