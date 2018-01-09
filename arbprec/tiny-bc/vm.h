#ifndef BC_VM_H
#define BC_VM_H

#include "program.h"
#include "stack.h"
#include "parse.h"

#define BC_VM_BUF_SIZE (1024)

typedef struct BcVm {

	BcProgram program;
	BcParse parse;

	BcStack ctx_stack;

	int filec;
	const char** filev;

} BcVm;

BcStatus bc_vm_init(BcVm* vm, int filec, const char* filev[]);

BcStatus bc_vm_exec(BcVm* vm);

void bc_vm_free(BcVm* vm);

#endif // BC_VM_H
