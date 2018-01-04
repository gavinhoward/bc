#include <stdint.h>
#include <stdlib.h>

#include "program.h"

void bc_program_init(BcProgram* program) {
	program->next = NULL;
	program->num_stmts = 0;
}

BcProgram* bc_program_expand(BcProgram* program) {

	// Malloc a new one.
	BcProgram* next = malloc(sizeof(BcProgram));

	// Check for error.
	if (next == NULL) {
		return NULL;
	}

	// Init the program.
	bc_program_init(next);

	// If the parameter is not NULL, chain them.
	if (program != NULL) {
		program->next = next;
	}

	return next;
}

void bc_program_free(BcProgram* program) {
	// TODO: Write this function.
}
