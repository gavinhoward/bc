#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../bc.h"
#include "../segarray.h"
#include "../program.h"

int bc_var_cmp(void* var1, void* var2) {

	BcVar* v1 = (BcVar*) var1;
	BcVar* v2 = (BcVar*) var2;

	return strcmp(v1->name, v2->name);
}

int main(int argc, char* argv[]) {

	BcStatus status;
	BcSegArray sa;

	if (argc < 3) {
		fprintf(stderr, "Need an input file and an output file\n"
		                "    Usage: test <input_file> <output_file>");
		exit(BC_STATUS_INVALID_OPTION);
	}

	status = bc_segarray_init(&sa, sizeof(BcVar), bc_var_cmp);

	if (status) {
		return status;
	}

	FILE* in = fopen(argv[1], "r");

	if (!in) {
		BC_STATUS_VM_FILE_ERR;
	}

	char* buffer = NULL;
	size_t n = 0;

	while (getline(&buffer, &n, in) != -1) {

		BcVar var;

		var.name = buffer;
		var.data = NULL;

		status = bc_segarray_add(&sa, &var);

		if (status) {
			return status;
		}

		buffer = NULL;
		n = 0;
	}

	fclose(in);

	FILE* out = fopen(argv[2], "w");

	if (!out) {
		BC_STATUS_VM_FILE_ERR;
	}

	for (uint32_t i = 0; i < sa.num; ++i) {

		BcVar* v = bc_segarray_item(&sa, i);

		if (!v) {
			return 1;
		}

		fprintf(out, "%s", v->name);
	}

	fclose(out);

	return BC_STATUS_SUCCESS;
}
