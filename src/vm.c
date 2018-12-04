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
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <signal.h>

#ifndef _WIN32

#include <sys/types.h>
#include <unistd.h>

#else // _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>

#endif // _WIN32

#include <status.h>
#include <args.h>
#include <vm.h>
#include <read.h>

#if BC_ENABLE_SIGNALS
#ifndef _WIN32
void bc_vm_sig(int sig) {
	int err = errno;
	size_t len = strlen(bcg.sig_msg);
	if (sig == SIGINT && write(2, bcg.sig_msg, len) == (ssize_t) len) {
		bcg.signe = bcg.sig == bcg.sigc;
		bcg.sig += bcg.signe;
	}
	errno = err;
}
#else // _WIN32
BOOL WINAPI bc_vm_sig(DWORD sig) {
	if (sig == CTRL_C_EVENT) {
		bc_vm_puts(bcg.sig_msg, stderr);
		bcg.signe = bcg.sig == bcg.sigc;
		bcg.sig += bcg.signe;
	}
	return TRUE;
}
#endif // _WIN32
#endif // BC_ENABLE_SIGNALS

void bc_vm_info(const char* const help) {
	bc_vm_printf("%s %s\n", bcg.name, BC_VERSION);
	bc_vm_puts(bc_copyright, stdout);
	if (help) bc_vm_printf(help, bcg.name);
}

BcStatus bc_vm_error(BcStatus s, const char *file, size_t line) {

	assert(file);

	if (!s || s > BC_STATUS_VEC_ITEM_EXISTS) return s;

	fprintf(stderr, bc_err_fmt, bc_errs[bc_err_ids[s]], bc_err_msgs[s]);
	fprintf(stderr, "    %s", file);
	fprintf(stderr, bc_err_line + 4 * !line, line);

	return s * (!bcg.ttyin || !!strcmp(file, bc_program_stdin_name));
}

#if BC_ENABLED
BcStatus bc_vm_posixError(BcStatus s, const char *file,
                          size_t line, const char *msg)
{
	int p = (int) bcg.s, w = (int) bcg.w;
	const char* const fmt = p ? bc_err_fmt : bc_warn_fmt;

	if (!(p || w) || s < BC_STATUS_POSIX_NAME_LEN) return BC_STATUS_SUCCESS;

	fprintf(stderr, fmt, bc_errs[bc_err_ids[s]], bc_err_msgs[s]);
	if (msg) fprintf(stderr, "    %s\n", msg);
	fprintf(stderr, "    %s", file);
	fprintf(stderr, bc_err_line + 4 * !line, line);

	return s * (!bcg.ttyin && !!p);
}

void bc_vm_envArgs(BcVm *vm) {

	BcVec v;
	char *env_args = getenv(bc_args_env_name), *buf;

	if (!env_args) return;

	vm->env_args = bc_vm_strdup(env_args);
	buf = vm->env_args;

	bc_vec_init(&v, sizeof(char*), NULL);
	bc_vec_push(&v, &bc_args_env_name);

	while (*buf != 0) {
		if (!isspace(*buf)) {
			bc_vec_push(&v, &buf);
			while (*buf != 0 && !isspace(*buf)) ++buf;
			if (*buf != 0) (*(buf++)) = '\0';
		}
		else ++buf;
	}

	// Make sure to push a NULL pointer at the end.
	buf = NULL;
	bc_vec_push(&v, &buf);

	bc_args((int) v.len - 1, (char**) v.v, &vm->flags, &vm->exprs, &vm->files);

	bc_vec_free(&v);
}
#endif // BC_ENABLED

size_t bc_vm_envLen(const char *var) {

	char *lenv = getenv(var);
	size_t i, len = BC_NUM_PRINT_WIDTH;
	int num;

	if (!lenv) return len;

	len = strlen(lenv);

	for (num = 1, i = 0; num && i < len; ++i) num = isdigit(lenv[i]);
	if (num) {
		len = (size_t) atoi(lenv) - 1;
		if (len < 2 || len >= INT32_MAX) len = BC_NUM_PRINT_WIDTH;
	}
	else len = BC_NUM_PRINT_WIDTH;

	return len;
}

void bc_vm_exit(BcStatus s) {
	bc_vm_error(s, NULL, 0);
	exit((int) s);
}

void* bc_vm_malloc(size_t n) {
	void* ptr = malloc(n);
	if (!ptr) bc_vm_exit(BC_STATUS_ALLOC_ERR);
	return ptr;
}

void* bc_vm_realloc(void *ptr, size_t n) {
	void* temp = realloc(ptr, n);
	if (!temp) bc_vm_exit(BC_STATUS_ALLOC_ERR);
	return temp;
}

char* bc_vm_strdup(const char *str) {
	char *s = strdup(str);
	if (!s) bc_vm_exit(BC_STATUS_ALLOC_ERR);
	return s;
}

void bc_vm_printf(const char *fmt, ...) {

	va_list args;
	bool bad;

	va_start(args, fmt);
	bad = vprintf(fmt, args) < 0;
	va_end(args);

	if (bad) bc_vm_exit(BC_STATUS_IO_ERR);
}

void bc_vm_puts(const char *str, FILE *restrict f) {
	if (fputs(str, f) == EOF) bc_vm_exit(BC_STATUS_IO_ERR);
}

void bc_vm_putchar(int c) {
	if (putchar(c) == EOF) bc_vm_exit(BC_STATUS_IO_ERR);
}

void bc_vm_fflush(FILE *restrict f) {
	if (fflush(f) == EOF) bc_vm_exit(BC_STATUS_IO_ERR);
}

BcStatus bc_vm_process(BcVm *vm, const char *text) {

	BcStatus s = bc_parse_text(&vm->prs, text);

	s = bc_vm_error(s, vm->prs.l.f, vm->prs.l.line);
	if (s) return s;

	while (vm->prs.l.t.t != BC_LEX_EOF) {

		s = vm->prs.parse(&vm->prs);

		if (s == BC_STATUS_LIMITS) {

			bc_vm_putchar('\n');
			bc_vm_printf("BC_BASE_MAX     = %lu\n", BC_MAX_OBASE);
			bc_vm_printf("BC_DIM_MAX      = %lu\n", BC_MAX_DIM);
			bc_vm_printf("BC_SCALE_MAX    = %lu\n", BC_MAX_SCALE);
			bc_vm_printf("BC_STRING_MAX   = %lu\n", BC_MAX_STRING);
			bc_vm_printf("BC_NAME_MAX     = %lu\n", BC_MAX_NAME);
			bc_vm_printf("BC_NUM_MAX      = %lu\n", BC_MAX_NUM);
			bc_vm_printf("Max Exponent    = %lu\n", BC_MAX_EXP);
			bc_vm_printf("Number of Vars  = %lu\n", BC_MAX_VARS);
			bc_vm_putchar('\n');

			s = BC_STATUS_SUCCESS;
		}
		else {
			if (s == BC_STATUS_QUIT) return s;
			s = bc_vm_error(s, vm->prs.l.f, vm->prs.l.line);
			if (s) return s;
		}
	}

	if (BC_PARSE_CAN_EXEC(&vm->prs)) {
		s = bc_program_exec(&vm->prog);
		if (!s && bcg.i) bc_vm_fflush(stdout);
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
	s = bc_read_file(file, &data);
	if (s) return bc_vm_error(s, file, 0);

	bc_lex_file(&vm->prs.l, file);
	s = bc_vm_process(vm, data);
	if (s) goto err;

	main_func = bc_vec_item(&vm->prog.fns, BC_PROG_MAIN);
	ip = bc_vec_item(&vm->prog.stack, 0);

	if (main_func->code.len < ip->idx)
		s = BC_STATUS_EXEC_FILE_NOT_EXECUTABLE;

err:
	free(data);
	return s;
}

BcStatus bc_vm_stdin(BcVm *vm) {

	BcStatus s = BC_STATUS_SUCCESS;
	BcVec buf, buffer;
	char c;
	size_t len, i, str = 0;
	bool comment = false, notend;

	vm->prog.file = bc_program_stdin_name;
	bc_lex_file(&vm->prs.l, bc_program_stdin_name);

	bc_vec_init(&buffer, sizeof(char), NULL);
	bc_vec_init(&buf, sizeof(char), NULL);
	bc_vec_pushByte(&buffer, '\0');

	// This loop is complex because the vm tries not to send any lines that end
	// with a backslash to the parser. The reason for that is because the parser
	// treats a backslash+newline combo as whitespace, per the bc spec. In that
	// case, and for strings and comments, the parser will expect more stuff.
	for (s = bc_read_line(&buf, ">>> "); !s; s = bc_read_line(&buf, ">>> ")) {

		char *string = buf.v;

		len = buf.len - 1;

		if (len == 1) {
			if (str && buf.v[0] == bcg.send) str -= 1;
			else if (buf.v[0] == bcg.sbgn) str += 1;
		}
		else if (len > 1 || comment) {

			for (i = 0; i < len; ++i) {

				notend = len > i + 1;
				c = string[i];

				if (i - 1 > len || string[i - 1] != '\\') {
					if (bcg.sbgn == bcg.send) str ^= c == bcg.sbgn;
					else if (c == bcg.send) str -= 1;
					else if (c == bcg.sbgn) str += 1;
				}

				if (c == '/' && notend && !comment && string[i + 1] == '*') {
					comment = true;
					break;
				}
				else if (c == '*' && notend && comment && string[i + 1] == '/')
					comment = false;
			}

			if (str || comment || string[len - 2] == '\\') {
				bc_vec_concat(&buffer, buf.v);
				continue;
			}
		}

		bc_vec_concat(&buffer, buf.v);
		s = bc_vm_process(vm, buffer.v);
		if (s) goto err;

		bc_vec_npop(&buffer, buffer.len);
	}

	if (s == BC_STATUS_BIN_FILE) s = bc_vm_error(s, vm->prs.l.f, 0);

	// I/O error will always happen when stdin is
	// closed. It's not a problem in that case.
	s = s == BC_STATUS_IO_ERR || s == BC_STATUS_QUIT ? BC_STATUS_SUCCESS : s;

	if (str) s = bc_vm_error(BC_STATUS_LEX_NO_STRING_END,
	                         vm->prs.l.f, vm->prs.l.line);
	else if (comment) s = bc_vm_error(BC_STATUS_LEX_NO_COMMENT_END,
	                                  vm->prs.l.f, vm->prs.l.line);

err:
	bc_vec_free(&buf);
	bc_vec_free(&buffer);
	return s;
}

BcStatus bc_vm_exec(BcVm *vm) {

	BcStatus s = BC_STATUS_SUCCESS;
	size_t i;

#if BC_ENABLED
	if (vm->flags & BC_FLAG_L) {

		bc_lex_file(&vm->prs.l, bc_lib_name);
		s = bc_parse_text(&vm->prs, bc_lib);

		while (!s && vm->prs.l.t.t != BC_LEX_EOF) s = vm->prs.parse(&vm->prs);

		if (s) return s;
		s = bc_program_exec(&vm->prog);
		if (s) return s;
	}
#endif // BC_ENABLED

	if (vm->exprs.len) {
		bc_lex_file(&vm->prs.l, bc_program_exprs_name);
		s = bc_vm_process(vm, vm->exprs.v);
		if (s) return s;
	}

	for (i = 0; !s && i < vm->files.len; ++i)
		s = bc_vm_file(vm, *((char**) bc_vec_item(&vm->files, i)));
	if (s && s != BC_STATUS_QUIT) return s;

	if ((BC_IS_BC || !vm->files.len) && !vm->exprs.len) s = bc_vm_stdin(vm);
	if (!s && !BC_PARSE_CAN_EXEC(&vm->prs)) s = bc_vm_process(vm, "");

	return s == BC_STATUS_QUIT ? BC_STATUS_SUCCESS : s;
}

void bc_vm_free(BcVm *vm) {
	bc_vec_free(&vm->files);
	bc_vec_free(&vm->exprs);
	bc_program_free(&vm->prog);
	bc_parse_free(&vm->prs);
	free(vm->env_args);
}

void bc_vm_init(BcVm *vm, const char *env_len) {

	size_t len = bc_vm_envLen(env_len);

	memset(vm, 0, sizeof(BcVm));

	bc_vec_init(&vm->files, sizeof(char*), NULL);
	bc_vec_init(&vm->exprs, sizeof(char), NULL);

#if BC_ENABLED
	if (BC_IS_BC) {
		vm->flags |= BC_FLAG_S * (getenv("POSIXLY_CORRECT") != NULL);
		bc_vm_envArgs(vm);
	}
#endif // BC_ENABLED

	bc_program_init(&vm->prog, len, bcg.init, bcg.exp);
	bcg.init(&vm->prs, &vm->prog, BC_PROG_MAIN);
}

BcStatus bc_vm_run(int argc, char *argv[], const char *env_len) {

	BcStatus st;
	BcVm vm;
#if BC_ENABLE_SIGNALS
#ifndef _WIN32
	struct sigaction sa;

	sigemptyset(&sa.sa_mask);
	sa.sa_handler = bc_vm_sig;
	sa.sa_flags = 0;
	sigaction(SIGINT, &sa, NULL);
#else // _WIN32
	SetConsoleCtrlHandler(bc_vm_sig, TRUE);
#endif // _WIN32
#endif // BC_ENABLE_SIGNALS

	bc_vm_init(&vm, env_len);
	bc_args(argc, argv, &vm.flags, &vm.exprs, &vm.files);

	bcg.ttyin = isatty(0);
	bcg.i = bcg.ttyin || (vm.flags & BC_FLAG_I) || isatty(1);

#if BC_ENABLED
	bcg.s = vm.flags & BC_FLAG_S;
	bcg.w = vm.flags & BC_FLAG_W;
#endif // BC_ENABLED
#if DC_ENABLED
	bcg.x = vm.flags & BC_FLAG_X;
#endif // DC_ENABLED

	if (bcg.ttyin && !(vm.flags & BC_FLAG_Q)) bc_vm_info(NULL);
	st = bc_vm_exec(&vm);

	bc_vm_free(&vm);
	return st;
}
