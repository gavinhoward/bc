#ifndef BC_VM_H
#define BC_VM_H

#include <bc/program.h>
#include <bc/stack.h>
#include <bc/parse.h>

#define BC_VM_BUF_SIZE (1024)

typedef struct BcVm {

	BcProgram program;
	BcParse parse;

	int filec;
	const char** filev;

} BcVm;

BcStatus bc_vm_init(BcVm* vm, int filec, const char* filev[]);

BcStatus bc_vm_exec(BcVm* vm);

#endif // BC_VM_H
