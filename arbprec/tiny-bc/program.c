#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "program.h"

static BcStatus bc_program_expand(BcProgram* program);
static void bc_program_list_init(BcStmtList* list);
static void bc_program_list_free(BcStmtList* list);
static void bc_program_func_free(BcFunc* func);
static void bc_program_var_free(BcVar* var);
static void bc_program_array_free(BcArray* array);

BcStatus bc_program_init(BcProgram* program) {

	// Check for invalid params.
	if (program == NULL) {
		return BC_STATUS_INVALID_PARAM;
	}

	// Malloc a new statement list.
	program->first = malloc(sizeof(BcStmtList));

	// Check for error.
	if (program->first == NULL) {
		return BC_STATUS_MALLOC_FAIL;
	}

	// Initialize the list.
	bc_program_list_init(program->first);

	// Set the current list.
	program->cur = program->first;

	// Init symbol table.
	program->num_funcs = 0;
	program->num_vars = 0;
	program->num_arrays = 0;

	return BC_STATUS_SUCCESS;
}

BcStatus bc_program_insert(BcProgram* program, BcStmt* stmt) {

	// Check for invalid parameters.
	if (program == NULL || stmt == NULL) {
		return BC_STATUS_INVALID_PARAM;
	}

	// If the list is full...
	if (program->cur->num_stmts == BC_PROGRAM_MAX_STMTS) {

		// Expand the list.
		BcStatus status = bc_program_expand(program);

		// Check for error.
		if (status != BC_STATUS_SUCCESS) {
			return status;
		}
	}

	// Copy the data.
	BcStmtList* cur = program->cur;
	memcpy(cur->stmts + cur->num_stmts, stmt, sizeof(BcStmt));

	return BC_STATUS_SUCCESS;
}

void bc_program_free(BcProgram* program) {

	// Check for NULL.
	if (program == NULL) {
		return;
	}

	// These are used during the loop.
	BcStmtList* temp;
	BcStmtList* cur = program->first;

	// Loop through the lists and free them all.
	while (cur != NULL) {
		temp = cur->next;
		bc_program_list_free(cur);
		cur = temp;
	}

	// Set these values to NULL.
	program->cur = NULL;
	program->first = NULL;

	// Free funcs.
	uint32_t num = program->num_funcs;
	for (uint32_t i = 0; i < num; ++i) {
		bc_program_func_free(program->funcs + i);
	}

	// Free vars.
	num = program->num_vars;
	for (uint32_t i = 0; i < num; ++i) {
		bc_program_var_free(program->vars + i);
	}

	// Free arrays.
	num = program->num_arrays;
	for (uint32_t i = 0; i < num; ++i) {
		bc_program_array_free(program->arrays + i);
	}

	// Zero these.
	program->num_funcs = 0;
	program->num_vars = 0;
	program->num_arrays = 0;
}

static BcStatus bc_program_expand(BcProgram* program) {

	// Check for NULL.
	if (program == NULL) {
		return BC_STATUS_INVALID_PARAM;
	}

	// Malloc a new one.
	BcStmtList* next = malloc(sizeof(BcStmtList));

	// Check for error.
	if (next == NULL) {
		return BC_STATUS_MALLOC_FAIL;
	}

	// Initialize the list.
	bc_program_list_init(next);

	// Set the pointers.
	program->cur->next = next;
	program->cur = next;

	return BC_STATUS_SUCCESS;
}

static void bc_program_list_init(BcStmtList* list) {

	// Check for NULL.
	if (list == NULL) {
		return;
	}

	// Zero these.
	list->next = NULL;
	list->num_stmts = 0;
}

static void bc_program_list_free(BcStmtList* list) {

	// Check for NULL.
	if (list == NULL) {
		return;
	}

	// TODO: Write this function.
}

static void bc_program_func_free(BcFunc* func) {

	// Check for NULL.
	if (func == NULL) {
		return;
	}

	// TODO: Write this function.
}

static void bc_program_var_free(BcVar* var) {

	// Check for NULL.
	if (var == NULL) {
		return;
	}

	// TODO: Write this function.
}

static void bc_program_array_free(BcArray* array) {

	// Check for NULL.
	if (array == NULL) {
		return;
	}

	// TODO: Write this function.
}
