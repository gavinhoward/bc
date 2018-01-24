#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <arbprec/arbprec.h>

#include <bc/program.h>
#include <bc/parse.h>

static BcStatus bc_program_execList(BcProgram* p, BcStmtList* list);
static BcStatus bc_program_printString(const char* str);
static BcStatus bc_program_execExpr(BcProgram* p, BcStmt* stmt,
                                    fxdpnt* num, bool print);

BcStatus bc_program_init(BcProgram* p, const char* file) {

	BcStatus st;

	if (p == NULL) {
		return BC_STATUS_INVALID_PARAM;
	}

	p->file = file;

	p->list = bc_list_create();

	if (!p->list) {
		return BC_STATUS_MALLOC_FAIL;
	}

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

	st = bc_stack_init(&p->ctx_stack, sizeof(BcStmtList*), NULL);

	if (st) {
		goto ctx_err;
	}

	st = bc_stack_init(&p->locals, sizeof(BcLocal), bc_local_free);

	if (st) {
		goto local_err;
	}

	st = bc_stack_init(&p->temps, sizeof(fxdpnt), (BcFreeFunc) arb_free);

	if (st) {
		goto temps_err;
	}

	return st;

temps_err:

	bc_stack_free(&p->locals);

local_err:

	bc_stack_free(&p->ctx_stack);

ctx_err:

	bc_segarray_free(&p->arrays);

array_err:

	bc_segarray_free(&p->vars);

var_err:

	bc_segarray_free(&p->funcs);

func_err:

	bc_list_free(p->list);
	p->list = NULL;

	return st;
}

BcStatus bc_program_func_add(BcProgram* p, BcFunc* func) {

	if (!p || !func) {
		return BC_STATUS_INVALID_PARAM;
	}

	return bc_segarray_add(&p->funcs, func);
}

BcStatus bc_program_var_add(BcProgram* p, BcVar* var) {

	if (!p || !var) {
		return BC_STATUS_INVALID_PARAM;
	}

	return bc_segarray_add(&p->vars, var);
}

BcStatus bc_program_array_add(BcProgram* p, BcArray* array) {

	if (!p || !array) {
		return BC_STATUS_INVALID_PARAM;
	}

	return bc_segarray_add(&p->arrays, array);
}

BcStatus bc_program_exec(BcProgram* p) {
	return bc_program_execList(p, p->list);
}

void bc_program_free(BcProgram* p) {

	if (p == NULL) {
		return;
	}

	bc_list_free(p->list);

	bc_segarray_free(&p->funcs);
	bc_segarray_free(&p->vars);
	bc_segarray_free(&p->arrays);

	bc_stack_free(&p->ctx_stack);
	bc_stack_free(&p->locals);
	bc_stack_free(&p->temps);
}

static BcStatus bc_program_execList(BcProgram* p, BcStmtList* list) {

	BcStatus status;
	BcStmtList* next;
	BcStmtList* cur;
	BcStmtType type;
	BcStmt* stmt;
	int pchars;
	fxdpnt result;

	assert(list);

	status = BC_STATUS_SUCCESS;

	cur = list;

	do {

		next = cur->next;

		while (cur->idx < cur->num_stmts) {

			stmt = cur->stmts + cur->idx;
			type = stmt->type;

			++cur->idx;

			switch (type) {

				case BC_STMT_EXPR:
				{
					status = bc_program_execExpr(p, stmt, &result, true);

					if (status) {
						break;
					}



					break;
				}

				case BC_STMT_STRING:
				{
					pchars = fprintf(stdout, "%s", stmt->data.string);
					status = pchars > 0 ? BC_STATUS_SUCCESS :
					                      BC_STATUS_VM_PRINT_ERR;
					break;
				}

				case BC_STMT_STRING_PRINT:
				{
					status = bc_program_printString(stmt->data.string);
					break;
				}

				case BC_STMT_BREAK:
				{
					status = BC_STATUS_VM_BREAK;
					break;
				}

				case BC_STMT_CONTINUE:
				{
					status = BC_STATUS_VM_CONTINUE;
					break;
				}

				case BC_STMT_HALT:
				{
					status = BC_STATUS_VM_HALT;
					break;
				}

				case BC_STMT_RETURN:
				{
					break;
				}

				case BC_STMT_IF:
				{
					break;
				}

				case BC_STMT_WHILE:
				{
					break;
				}

				case BC_STMT_FOR:
				{
					break;
				}

				case BC_STMT_LIST:
				{
					status = bc_program_execList(p, stmt->data.list);
					break;
				}

				default:
				{
					return BC_STATUS_VM_INVALID_STMT;
				}
			}
		}

		if (cur->idx == cur->num_stmts) {
			cur = next;
		}
		else {
			cur = NULL;
		}

	} while (!status && cur);

	return status;
}

static BcStatus bc_program_printString(const char* str) {

	char c;
	char c2;
	size_t len;
	int err;

	len = strlen(str);

	for (size_t i = 0; i < len; ++i) {

		c = str[i];

		if (c != '\\') {
			err = fputc(c, stdout);
		}
		else {

			++i;

			if (i >= len) {
				return BC_STATUS_VM_INVALID_STRING;
			}

			c2 = str[i];

			switch (c2) {

				case 'a':
				{
					err = fputc('\a', stdout);
					break;
				}

				case 'b':
				{
					err = fputc('\b', stdout);
					break;
				}

				case 'e':
				{
					err = fputc('\\', stdout);
					break;
				}

				case 'f':
				{
					err = fputc('\f', stdout);
					break;
				}

				case 'n':
				{
					err = fputc('\n', stdout);
					break;
				}

				case 'r':
				{
					err = fputc('\r', stdout);
					break;
				}

				case 'q':
				{
					fputc('"', stdout);
					break;
				}

				case 't':
				{
					err = fputc('\t', stdout);
					break;
				}

				default:
				{
					// Do nothing.
					err = 0;
					break;
				}
			}
		}

		if (err == EOF) {
			return BC_STATUS_VM_PRINT_ERR;
		}
	}

	return BC_STATUS_SUCCESS;
}

static BcStatus bc_program_execExpr(BcProgram* p, BcStmt* stmt,
                                    fxdpnt* num, bool print)
{

}
