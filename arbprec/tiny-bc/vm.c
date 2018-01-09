#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "vm.h"

static BcStatus bc_vm_execFile(BcVm* vm, int idx);
static BcStatus bc_vm_execStdin(BcVm* vm);

BcStatus bc_vm_init(BcVm* vm, int filec, const char* filev[]) {

	BcStatus status;

	vm->filec = filec;
	vm->filev = filev;

	status = bc_program_init(&vm->program);

	if (status) {
		goto program_err;
	}

	status = bc_parse_init(&vm->parse, &vm->program);

	if (status) {
		goto parse_err;
	}

	status = bc_stack_init(&vm->ctx_stack, sizeof(BcContext));

	if (status) {
		goto stack_err;
	}

	return BC_STATUS_SUCCESS;

stack_err:

	bc_parse_free(&vm->parse);

parse_err:

	bc_program_free(&vm->program);

program_err:

	return status;
}

BcStatus bc_vm_exec(BcVm* vm) {

	BcStatus status;
	int num_files;

	status = BC_STATUS_SUCCESS;

	num_files = vm->filec;

	for (int i = 0; !status && i < num_files; ++i) {
		status = bc_vm_execFile(vm, i);
	}

	if (status) {
		return status;
	}

	status = bc_vm_execStdin(vm);

	return status;
}

void bc_vm_free(BcVm* vm) {
	bc_stack_free(&vm->ctx_stack);
	bc_parse_free(&vm->parse);
	bc_program_free(&vm->program);
}

static BcStatus bc_vm_execFile(BcVm* vm, int idx) {

	BcStatus status;
	FILE* f;
	size_t size;
	size_t read_size;

	f = fopen(vm->filev[idx], "r");

	if (!f) {
		return BC_STATUS_VM_FILE_ERR;
	}

	fseek(f, 0, SEEK_END);
	size = ftell(f);

	fseek(f, 0, SEEK_SET);

	char* data = malloc(size + 1);

	if (!data) {
		status = BC_STATUS_MALLOC_FAIL;
		goto data_err;
	}

	read_size = fread(data, 1, size, f);

	if (read_size != size) {
		status = BC_STATUS_VM_FILE_READ_ERR;
		goto read_err;
	}

	data[size] = '\0';

	fclose(f);

	bc_parse_text(&vm->parse, data);

	status = bc_parse_parse(&vm->parse);

	while (status == BC_STATUS_SUCCESS) {

		status = bc_program_exec(&vm->program);

		if (status) {
			break;
		}

		status = bc_parse_parse(&vm->parse);
	}

	free(data);

	return status;

read_err:

	free(data);

data_err:

	fclose(f);

	return status;
}

static BcStatus bc_vm_execStdin(BcVm* vm) {

	BcStatus status;
	char* buf;
	char* buffer;
	char* temp;
	size_t n;
	size_t bufn;
	size_t slen;
	size_t total_len;

	status = BC_STATUS_SUCCESS;

	n = BC_VM_BUF_SIZE;
	bufn = BC_VM_BUF_SIZE;
	buffer = malloc(BC_VM_BUF_SIZE + 1);

	if (!buffer) {
		return BC_STATUS_MALLOC_FAIL;
	}

	buf = malloc(BC_VM_BUF_SIZE + 1);

	if (!buf) {
		free(buffer);
		return BC_STATUS_MALLOC_FAIL;
	}

	// The following loop is complicated because the vm tries
	// not to send any lines that end with a backslash to the
	// parser. The reason for that is because the parser treats
	// a backslash newline combo as whitespace, per the bc
	// spec. Thus, the parser will expect more stuff.
	while (!status && getline(&buf, &bufn, stdin) != -1) {

		size_t len;

		len = strlen(buf);
		slen = strlen(buffer);
		total_len = slen + len;

		if (len > 1) {

			if (buf[len - 2] == '\\') {

				if (total_len > n) {

					temp = realloc(buffer, total_len + 1);

					if (!temp) {
						status = BC_STATUS_MALLOC_FAIL;
						goto exit_err;
					}

					buffer = temp;
					n = slen + len;
				}

				strcat(buffer, buf);

				continue;
			}
		}

		if (total_len > n) {

			temp = realloc(buffer, total_len + 1);

			if (!temp) {
				status = BC_STATUS_MALLOC_FAIL;
				goto exit_err;
			}

			buffer = temp;
			n = slen + len;
		}

		strcat(buffer, buf);

		status = bc_parse_text(&vm->parse, buffer);

		if (status) {
			goto exit_err;
		}

		status = bc_parse_parse(&vm->parse);

		if (status) {
			goto exit_err;
		}

		status = bc_program_exec(&vm->program);

		buffer[0] = '\0';
	}

exit_err:

	free(buf);
	free(buffer);

	return status;
}
