/*
 * *****************************************************************************
 *
 * Copyright 2018 Gavin D. Howard
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * *****************************************************************************
 *
 * Code common to all of bc and dc.
 *
 */

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

#include <status.h>
#include <args.h>
#include <vm.h>
#include <io.h>

void bc_vm_sig(int sig) {
	if (sig == SIGINT) {
		size_t len = strlen(bcg.sig_msg);
		if (write(2, bcg.sig_msg, len) == (ssize_t) len)
			bcg.sig += (bcg.signe = bcg.sig == bcg.sigc);
	}
	else bcg.sig_other = 1;
}

BcStatus bc_vm_info(const char* const help) {
	if (printf("%s %s\n", bcg.name, BC_VERSION) < 0) return BC_STATUS_IO_ERR;
	if (puts(bc_copyright) == EOF) return BC_STATUS_IO_ERR;
	if (help && printf(help, bcg.name) < 0) return BC_STATUS_IO_ERR;
	return BC_STATUS_SUCCESS;
}

BcStatus bc_vm_envArgs(BcVm *vm) {

	BcStatus s;
	BcVec args;
	char *env_args = NULL, *buf;

	if (!(env_args = getenv(bc_args_env_name))) return BC_STATUS_SUCCESS;
	if (!(buf = (vm->env_args = strdup(env_args)))) return BC_STATUS_ALLOC_ERR;

	if ((s = bc_vec_init(&args, sizeof(char*), NULL))) goto args_err;
	if ((s = bc_vec_push(&args, &bc_args_env_name))) goto err;

	while (*buf) {
		if (!isspace(*buf)) {
			if ((s = bc_vec_push(&args, &buf))) goto err;
			while (*buf && !isspace(*buf)) ++buf;
			if (*buf) (*(buf++)) = '\0';
		}
		else ++buf;
	}

	s = bc_args((int) args.len, (char**) args.v,
	            &vm->flags, &vm->exprs, &vm->files);
	if (s) goto err;

	bc_vec_free(&args);

	return s;

err:
	bc_vec_free(&args);
args_err:
	free(vm->env_args);
	vm->env_args = NULL;
	return s;
}

size_t bc_vm_envLen(const char *var) {

	char *lenv;
	size_t i, len = BC_NUM_PRINT_WIDTH;
	int num;

	if ((lenv = getenv(var))) {
		len = strlen(lenv);
		for (num = 1, i = 0; num && i < len; ++i) num = isdigit(lenv[i]);
		if (!num || ((len = (size_t) atoi(lenv) - 1) < 2 && len >= INT32_MAX))
			len = BC_NUM_PRINT_WIDTH;
	}

	return len;
}

BcStatus bc_vm_error(BcStatus s, const char *file, size_t line) {

	assert(file);

	if (!s || s > BC_STATUS_VEC_ITEM_EXISTS) return s;

	fprintf(stderr, bc_err_fmt, bc_errs[bc_err_indices[s]], bc_err_descs[s]);
	fprintf(stderr, "    %s", file);
	fprintf(stderr, bc_err_line + 4 * !line, line);

	return s * (!bcg.ttyin || !!strcmp(file, bc_program_stdin_name));
}

#ifdef BC_ENABLED
BcStatus bc_vm_posixError(BcStatus s, const char *file,
                           size_t line, const char *msg)
{
	int p = (int) bcg.posix, w = (int) bcg.warn;

	if (!(p || w) || s < BC_STATUS_POSIX_NAME_LEN) return BC_STATUS_SUCCESS;

	fprintf(stderr, "\n%s %s: %s\n", bc_errs[bc_err_indices[s]],
	        p ? "error" : "warning", bc_err_descs[s]);

	if (msg) fprintf(stderr, "    %s\n", msg);

	if (file) {
		fprintf(stderr, "    %s", file);
		fprintf(stderr, bc_err_line + 4 * !line, line);
	}

	return s * (!bcg.ttyin && !!p);
}
#endif // BC_ENABLED

BcStatus bc_vm_process(BcVm *vm, const char *text) {

	BcStatus s = bc_lex_text(&vm->prs.l, text);

	if ((s = bc_vm_error(s, vm->prs.l.f, vm->prs.l.line))) return s;

	while (vm->prs.l.t.t != BC_LEX_EOF) {

		if ((s = vm->prs.parse(&vm->prs)) == BC_STATUS_LIMITS) {

			s = BC_STATUS_IO_ERR;

			if (putchar('\n') == EOF) return s;
			if (printf("BC_BASE_MAX     = %lu\n", BC_MAX_OBASE) < 0) return s;
			if (printf("BC_DIM_MAX      = %lu\n", BC_MAX_DIM) < 0) return s;
			if (printf("BC_SCALE_MAX    = %lu\n", BC_MAX_SCALE) < 0) return s;
			if (printf("BC_STRING_MAX   = %lu\n", BC_MAX_STRING) < 0) return s;
			if (printf("BC_NAME_MAX     = %lu\n", BC_MAX_NAME) < 0) return s;
			if (printf("BC_NUM_MAX      = %lu\n", BC_MAX_NUM) < 0) return s;
			if (printf("Max Exponent    = %lu\n", BC_MAX_EXP) < 0) return s;
			if (printf("Number of Vars  = %lu\n", BC_MAX_VARS) < 0) return s;
			if (putchar('\n') == EOF) return s;

			s = BC_STATUS_SUCCESS;
		}
		else if (s == BC_STATUS_QUIT || bcg.sig_other ||
		         (s = bc_vm_error(s, vm->prs.l.f, vm->prs.l.line)))
		{
			return s;
		}
	}

	if (BC_PARSE_CAN_EXEC(&vm->prs)) {
		s = bc_program_exec(&vm->prog);
		if (!s && bcg.tty && fflush(stdout) == EOF) s = BC_STATUS_IO_ERR;
		if (s && s != BC_STATUS_QUIT)
			s = bc_vm_error(bc_program_reset(&vm->prog, s), vm->prs.l.f, 0);
	}

	return s;
}

BcStatus bc_vm_file(BcVm *vm, const char *file) {

	BcStatus s;
	char *data;
	BcFunc *main_func;
	BcInstPtr *ip;

	vm->prog.file = file;
	if ((s = bc_io_fread(file, &data))) return bc_vm_error(s, file, 0);

	bc_lex_file(&vm->prs.l, file);
	if ((s = bc_vm_process(vm, data))) goto err;

	main_func = bc_vec_item(&vm->prog.fns, BC_PROG_MAIN);
	ip = bc_vec_item(&vm->prog.stack, 0);

	if (main_func->code.len < ip->idx)
		s = bc_vm_error(BC_STATUS_EXEC_FILE_NOT_EXECUTABLE, file, 0);

err:
	free(data);
	return s;
}

BcStatus bc_vm_stdin(BcVm *vm) {

	BcStatus s;
	BcVec buf, buffer;
	char c;
	size_t len, i, string = 0;
	bool comment = false, notend;

	vm->prog.file = bc_program_stdin_name;
	bc_lex_file(&vm->prs.l, bc_program_stdin_name);

	if ((s = bc_vec_init(&buffer, sizeof(char), NULL))) return s;
	if ((s = bc_vec_init(&buf, sizeof(char), NULL))) goto buf_err;
	if ((s = bc_vec_pushByte(&buffer, '\0'))) goto err;

	// This loop is complex because the vm tries not to send any lines that end
	// with a backslash to the parser. The reason for that is because the parser
	// treats a backslash+newline combo as whitespace, per the bc spec. In that
	// case, and for strings and comments, the parser will expect more stuff.
	while (!s && !(s = bc_io_getline(&buf, ">>> "))) {

		char *str = buf.v;

		len = buf.len - 1;

		if (len == 1) {
			if (string && buf.v[0] == vm->exe.strend) string -= 1;
			else if (buf.v[0] == vm->exe.strbgn) string += 1;
		}
		else if (len > 1 || comment) {

			for (i = 0; i < len; ++i) {

				notend = len > i + 1;
				c = str[i];

				if (i - 1 > len || str[i - 1] != '\\') {
					if (c == vm->exe.strend) string -= 1;
					else if (c == vm->exe.strbgn) string += 1;
				}
				else if (c == '/' && notend && !comment && str[i + 1] == '*') {
					comment = true;
					break;
				}
				else if (c == '*' && notend && comment && str[i + 1] == '/')
					comment = false;
			}

			if (string || comment || str[len - 2] == '\\') {
				if ((s = bc_vec_concat(&buffer, buf.v))) goto err;
				continue;
			}
		}

		if ((s = bc_vec_concat(&buffer, buf.v))) goto err;
		if ((s = bc_vm_process(vm, buffer.v))) goto err;

		bc_vec_npop(&buffer, buffer.len);
	}

	if (s == BC_STATUS_BIN_FILE) s = bc_vm_error(s, vm->prs.l.f, 0);

	// I/O error will always happen when stdin is
	// closed. It's not a problem in that case.
	s = s == BC_STATUS_IO_ERR || s == BC_STATUS_QUIT ? BC_STATUS_SUCCESS : s;

	if (string) s = bc_vm_error(BC_STATUS_LEX_NO_STRING_END,
	                            vm->prs.l.f, vm->prs.l.line);
	else if (comment) s = bc_vm_error(BC_STATUS_LEX_NO_COMMENT_END,
	                                  vm->prs.l.f, vm->prs.l.line);

err:
	bc_vec_free(&buf);
buf_err:
	bc_vec_free(&buffer);
	return s;
}

BcStatus bc_vm_exec(BcVm *vm) {

	BcStatus s = BC_STATUS_SUCCESS;
	size_t i;

#ifdef BC_ENABLED
	if (vm->flags & BC_FLAG_L) {

		bc_lex_file(&vm->prs.l, bc_lib_name);
		if ((s = bc_lex_text(&vm->prs.l, bc_lib))) return s;

		while (!s && vm->prs.l.t.t != BC_LEX_EOF) s = vm->prs.parse(&vm->prs);

		if (s || (s = bc_program_exec(&vm->prog))) return s;
	}
#endif // BC_ENABLED

	if (vm->exprs.len > 1 && (s = bc_vm_process(vm, vm->exprs.v))) return s;

	for (i = 0; !bcg.sig_other && !s && i < vm->files.len; ++i)
		s = bc_vm_file(vm, *((char**) bc_vec_item(&vm->files, i)));
	if ((s && s != BC_STATUS_QUIT) || bcg.sig_other) return s;

	if ((bcg.bc || !vm->files.len) && vm->exprs.len <= 1) s = bc_vm_stdin(vm);

	return s == BC_STATUS_QUIT ? BC_STATUS_SUCCESS : s;
}

BcStatus bc_vm_init(BcVm *vm, BcVmExe exe, const char *env_len) {

	BcStatus s;
	struct sigaction sa;
	size_t len = bc_vm_envLen(env_len);

	sigemptyset(&sa.sa_mask);
	sa.sa_handler = bc_vm_sig;
	sa.sa_flags = 0;

	if (sigaction(SIGINT, &sa, NULL) < 0 || sigaction(SIGPIPE, &sa, NULL) < 0 ||
	    sigaction(SIGHUP, &sa, NULL) < 0 || sigaction(SIGTERM, &sa, NULL) < 0)
	{
		return BC_STATUS_EXEC_SIGACTION_FAIL;
	}

	vm->exe = exe;
	vm->flags = 0;
	vm->env_args = NULL;

	if ((s = bc_vec_init(&vm->files, sizeof(char*), NULL))) return s;
	if ((s = bc_vec_init(&vm->exprs, sizeof(char), NULL))) goto exprs_err;

#ifdef BC_ENABLED
	vm->flags |= BC_FLAG_S * bcg.bc * (getenv("POSIXLY_CORRECT") != NULL);
	if (bcg.bc && (s = bc_vm_envArgs(vm))) goto prog_err;
#endif // BC_ENABLED

	if ((s = bc_program_init(&vm->prog, len, exe.init, exe.exp))) goto prog_err;
	if ((s = exe.init(&vm->prs, &vm->prog, BC_PROG_MAIN))) goto parse_err;

	return s;

parse_err:
	bc_program_free(&vm->prog);
prog_err:
	bc_vec_free(&vm->exprs);
exprs_err:
	bc_vec_free(&vm->files);
	return s;
}

void bc_vm_free(BcVm *vm) {
	bc_vec_free(&vm->files);
	bc_vec_free(&vm->exprs);
	bc_program_free(&vm->prog);
	bc_parse_free(&vm->prs);
	free(vm->env_args);
}

BcStatus bc_vm_run(int argc, char *argv[], BcVmExe exe, const char *env_len) {

	BcStatus st;
	BcVm vm;

	if ((st = bc_vm_init(&vm, exe, env_len))) return st;
	if ((st = bc_args(argc, argv, &vm.flags, &vm.exprs, &vm.files))) goto err;

	bcg.tty = (bcg.ttyin = isatty(0)) || (vm.flags & BC_FLAG_I) || isatty(1);

#ifdef BC_ENABLED
	bcg.posix = vm.flags & BC_FLAG_S;
	bcg.warn = vm.flags & BC_FLAG_W;
#endif // BC_ENABLED
#ifdef DC_ENABLED
	bcg.exreg = vm.flags & BC_FLAG_X;
#endif // DC_ENABLED

	if (bcg.ttyin && !(vm.flags & BC_FLAG_Q)) st = bc_vm_info(NULL);
	if (!st) st = bc_vm_exec(&vm);

err:
	bc_vm_free(&vm);
	return st;
}
