#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <signal.h>
#include <unistd.h>

#include <bc/bc.h>
#include <bc/io.h>
#include <bc/vm.h>

static BcStatus bc_vm_execFile(BcVm* vm, int idx);
static BcStatus bc_vm_execStdin(BcVm* vm);

static void bc_vm_sigint(int sig) {

	signal(sig, bc_vm_sigint);

	const char* buf = "\n\nSIGINT detected\ntype \"quit\" to exit\n\n";

	write(STDERR_FILENO, buf, strlen(buf));

	bc_had_sigint = 1;
}

BcStatus bc_vm_init(BcVm* vm, int filec, const char* filev[]) {

	vm->filec = filec;
	vm->filev = filev;

	return BC_STATUS_SUCCESS;
}

BcStatus bc_vm_exec(BcVm* vm) {

	BcStatus status;
	int num_files;

	if (signal(SIGINT, bc_vm_sigint) < 0) {
		return BC_STATUS_VM_SIGACTION_FAIL;
	}

	status = BC_STATUS_SUCCESS;

	num_files = vm->filec;

	for (int i = 0; !status && i < num_files; ++i) {
		status = bc_vm_execFile(vm, i);
	}

	if (status) {
		return status == BC_STATUS_PARSE_QUIT ||
		       status == BC_STATUS_VM_HALT ?
		            BC_STATUS_SUCCESS : status;
	}

	status = bc_vm_execStdin(vm);

	return status == BC_STATUS_PARSE_QUIT ||
	       status == BC_STATUS_VM_HALT ?
	            BC_STATUS_SUCCESS : status;
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

	status = bc_parse_text(&vm->parse, data);

	if (status) {
		goto read_err;
	}

	do {

		status = bc_parse_parse(&vm->parse, &vm->program);

		if (status) {

			if (bc_had_sigint && !bc_interactive) {
				goto read_err;
			}
			else {
				bc_had_sigint = 0;
			}

			if (status != BC_STATUS_LEX_EOF &&
			    status != BC_STATUS_PARSE_EOF &&
			    status != BC_STATUS_PARSE_QUIT)
			{
				bc_error_file(status, vm->program.file, vm->parse.lex.line);
			}
			else if (status == BC_STATUS_PARSE_QUIT) {
				break;
			}

			while (vm->parse.token.type != BC_LEX_NEWLINE &&
			       vm->parse.token.type != BC_LEX_SEMICOLON)
			{
				status = bc_lex_next(&vm->parse.lex, &vm->parse.token);

				if (status) {
					break;
				}
			}
		}

	} while (!status);

	if (status != BC_STATUS_PARSE_EOF &&
	    status != BC_STATUS_LEX_EOF &&
	    status != BC_STATUS_PARSE_QUIT)
	{
		goto read_err;
	}

	do {

		if (BC_PARSE_CAN_EXEC(&vm->parse)) {

			status = bc_program_exec(&vm->program);

			if (status) {
				goto read_err;
			}

			if (bc_interactive) {

				fflush(stdout);

				if (bc_had_sigint) {
					fprintf(stderr, "ready for more input\n");
					fflush(stderr);
					bc_had_sigint = 0;
				}
			}
			else if (bc_had_sigint) {
				bc_had_sigint = 0;
				goto read_err;
			}
		}
		else {
			status = BC_STATUS_VM_FILE_NOT_EXECUTABLE;
		}

	} while (!status);

read_err:

	bc_error(status);

	free(data);

data_err:

	if (f) {
		fclose(f);
	}

file_err:

	bc_parse_free(&vm->parse, status);

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

	status = bc_program_init(&vm->program, "<stdin>");

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

	if (!buffer) {
		status = BC_STATUS_MALLOC_FAIL;
		goto buffer_err;
	}

	buffer[0] = '\0';

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
	while ((!status || status != BC_STATUS_PARSE_QUIT) &&
	       bc_io_getline(&buf, &bufn) != (size_t) -1)
	{

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

		if (!bc_had_sigint) {

			if (status) {

				if (status == BC_STATUS_PARSE_QUIT ||
				    status == BC_STATUS_LEX_EOF ||
				      status == BC_STATUS_PARSE_EOF)
				{
					break;
				}

				bc_error(status);
				goto exit_err;
			}
		}
		else if (status == BC_STATUS_PARSE_QUIT) {
			break;
		}

		while (!status) {
			status = bc_parse_parse(&vm->parse, &vm->program);
		}

		if (status == BC_STATUS_PARSE_QUIT) {
			break;
		}
		if (status != BC_STATUS_LEX_EOF && status != BC_STATUS_PARSE_EOF) {

			bc_error_file(status, vm->program.file, vm->parse.lex.line);

			while (vm->parse.token.type != BC_LEX_NEWLINE &&
			       vm->parse.token.type != BC_LEX_SEMICOLON)
			{
				status = bc_lex_next(&vm->parse.lex, &vm->parse.token);

				if (status && status != BC_STATUS_LEX_EOF) {
					bc_error_file(status, vm->program.file, vm->parse.lex.line);
					break;
				}
				else if (status == BC_STATUS_LEX_EOF) {
					status = BC_STATUS_SUCCESS;
					break;
				}
			}
		}

		if (BC_PARSE_CAN_EXEC(&vm->parse)) {

			status = bc_program_exec(&vm->program);

			if (status) {
				bc_error(status);
				goto exit_err;
			}

			if (bc_interactive) {

				fflush(stdout);

				if (bc_had_sigint) {
					fprintf(stderr, "ready for more input\n");
					bc_had_sigint = 0;
				}
			}
			else if (bc_had_sigint) {
				bc_had_sigint = 0;
				goto exit_err;
			}
		}

		buffer[0] = '\0';
	}

	status = !status || status == BC_STATUS_PARSE_QUIT ||
	         status == BC_STATUS_VM_HALT ||
	         status == BC_STATUS_LEX_EOF ||
	         status == BC_STATUS_PARSE_EOF ?
	             BC_STATUS_SUCCESS : status;

exit_err:

	free(buf);

buf_err:

	free(buffer);

buffer_err:

	bc_parse_free(&vm->parse, status);

parse_err:

	bc_program_free(&vm->program);

	return status;
}
