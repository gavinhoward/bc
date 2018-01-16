#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <bc/vm.h>

static BcStatus bc_vm_execFile(BcVm* vm, int idx);
static BcStatus bc_vm_execStdin(BcVm* vm);

BcStatus bc_vm_init(BcVm* vm, int filec, const char* filev[]) {

	vm->filec = filec;
	vm->filev = filev;

	return bc_stack_init(&vm->ctx_stack, sizeof(BcStmtList*));
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

	return bc_vm_execStdin(vm);
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

	status = bc_program_init(&vm->program, vm->filev[idx]);

	if (status) {
		return status;
	}

	status = bc_parse_init(&vm->parse, &vm->program);

	if (status) {
		goto parse_err;
	}

	f = fopen(vm->filev[idx], "r");

	if (!f) {
		status = BC_STATUS_VM_FILE_ERR;
		goto file_err;
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
	f = NULL;

	bc_parse_text(&vm->parse, data);

	status = bc_parse_parse(&vm->parse, &vm->program);

	while (!status) {

		status = bc_program_exec(&vm->program);

		if (status) {
			bc_error(status);
			break;
		}

		status = bc_parse_parse(&vm->parse, &vm->program);

		if (status && (status != BC_STATUS_LEX_EOF &&
		               status != BC_STATUS_PARSE_EOF))
		{
			bc_error_file(vm->program.file, vm->parse.lex.line, status);
			goto read_err;
		}
	}

	bc_program_free(&vm->program);
	bc_parse_free(&vm->parse);

	free(data);

	return BC_STATUS_SUCCESS;

read_err:

	free(data);

data_err:

	if (f) {
		fclose(f);
	}

file_err:

	bc_parse_free(&vm->parse);

parse_err:

	bc_program_free(&vm->program);

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

	status = bc_program_init(&vm->program, "-");

	if (status) {
		return status;
	}

	status = bc_parse_init(&vm->parse, &vm->program);

	if (status) {
		goto parse_err;
	}

	n = BC_VM_BUF_SIZE;
	bufn = BC_VM_BUF_SIZE;
	buffer = malloc(BC_VM_BUF_SIZE + 1);
	buffer[0] = '\0';

	if (!buffer) {
		status = BC_STATUS_MALLOC_FAIL;
		goto buffer_err;
	}

	buf = malloc(BC_VM_BUF_SIZE + 1);

	if (!buf) {
		status = BC_STATUS_MALLOC_FAIL;
		goto buf_err;
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

		while (!status) {
			status = bc_parse_parse(&vm->parse, &vm->program);
		}

		if (status != BC_STATUS_LEX_EOF && status != BC_STATUS_PARSE_EOF) {
			goto exit_err;
		}

		status = bc_program_exec(&vm->program);

		buffer[0] = '\0';
	}

exit_err:

	free(buf);

buf_err:

	free(buffer);

buffer_err:

	bc_parse_free(&vm->parse);

parse_err:

	bc_program_free(&vm->program);

	return status;
}
