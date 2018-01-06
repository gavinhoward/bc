#include "program.h"
#include "stack.h"
#include "parse.h"

typedef struct BcVm {

	BcProgram* program;
	BcParse parse;

	BcStack ctx_stack;

} BcVm;
