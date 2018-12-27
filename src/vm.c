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
	size_t len = strlen(vm->sig_msg);
	if (sig == SIGINT && write(2, vm->sig_msg, len) == (ssize_t) len)
		vm->sig = sig;
	errno = err;
}
#else // _WIN32
BOOL WINAPI bc_vm_sig(DWORD sig) {
	if (sig == CTRL_C_EVENT) {
		bc_vm_puts(vm->sig_msg, stderr);
		vm->sig = sig;
	}
	return TRUE;
}
#endif // _WIN32
#endif // BC_ENABLE_SIGNALS

void bc_vm_info(const char* const help) {
	bc_vm_printf("%s %s\n", vm->name, BC_VERSION);
	bc_vm_puts(bc_copyright, stdout);
	if (help) bc_vm_printf(help, vm->name);
}

void bc_vm_printError(BcError e, const char* const fmt,
                      size_t line, va_list args)
{
	fprintf(stderr, fmt, bc_errs[(size_t) bc_err_ids[e]]);
	vfprintf(stderr, bc_err_msgs[e], args);

	if (vm->file) {
		fprintf(stderr, "\n    %s", vm->file);
		if (line) fprintf(stderr, bc_err_line, line);
	}

	fputs("\n\n", stderr);
}

BcStatus bc_vm_error(BcError e, size_t line, ...) {

	va_list args;

	if (e > BC_ERROR_EXEC_STACK) return BC_STATUS_SUCCESS;

	va_start(args, line);
	bc_vm_printError(e, bc_err_fmt, line, args);
	va_end(args);

	return BC_STATUS_ERROR;
}

#if BC_ENABLED
BcStatus bc_vm_posixError(BcError e, size_t line, ...)
{
	va_list args;
	int p = (int) BC_S, w = (int) BC_W;

	if (!(p || w) || e < BC_ERROR_POSIX_NAME_LEN) return BC_STATUS_SUCCESS;

	va_start(args, line);
	bc_vm_printError(e, p ? bc_err_fmt : bc_warn_fmt, line, args);
	va_end(args);

	return (!!p) ? BC_STATUS_ERROR : BC_STATUS_SUCCESS;
}

BcStatus bc_vm_envArgs(BcVm *vm) {

	BcStatus s;
	BcVec v;
	char *env_args = getenv(bc_args_env_name), *buf;

	if (!env_args) return BC_STATUS_SUCCESS;

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

	s = bc_args((int) v.len - 1, (char**) v.v);

	bc_vec_free(&v);

	return s;
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

void bc_vm_exit(BcError e) {
	BcStatus s = bc_vm_err(e);
	bc_vm_shutdown();
	exit((int) s);
}

void* bc_vm_malloc(size_t n) {
	void* ptr = malloc(n);
	if (!ptr) bc_vm_exit(BC_ERROR_VM_ALLOC_ERR);
	return ptr;
}

void* bc_vm_realloc(void *ptr, size_t n) {
	void* temp = realloc(ptr, n);
	if (!temp) bc_vm_exit(BC_ERROR_VM_ALLOC_ERR);
	return temp;
}

char* bc_vm_strdup(const char *str) {
	char *s = strdup(str);
	if (!s) bc_vm_exit(BC_ERROR_VM_ALLOC_ERR);
	return s;
}

void bc_vm_printf(const char *fmt, ...) {

	va_list args;
	bool bad;

	va_start(args, fmt);
	bad = vprintf(fmt, args) < 0;
	va_end(args);

	if (bad) bc_vm_exit(BC_ERROR_VM_IO_ERR);
}

void bc_vm_puts(const char *str, FILE *restrict f) {
	if (fputs(str, f) == EOF) bc_vm_exit(BC_ERROR_VM_IO_ERR);
}

void bc_vm_putchar(int c) {
	if (putchar(c) == EOF) bc_vm_exit(BC_ERROR_VM_IO_ERR);
}

void bc_vm_fflush(FILE *restrict f) {
	if (fflush(f) == EOF) bc_vm_exit(BC_ERROR_VM_IO_ERR);
}

void bc_vm_garbageCollect() {

	BcProgram *prog = &vm->prog;
	BcVec *fns = &prog->fns;
	BcFunc *f = bc_vec_item(fns, BC_PROG_MAIN);
	BcInstPtr *ip = bc_vec_item(&prog->stack, 0);
	bool good = BC_IS_BC;

	if (!good) {

		size_t i;

		for (i = 0; i < vm->prog.vars.len; ++i) {
			BcVec *arr = bc_vec_item(&vm->prog.vars, i);
			if (arr->len != 1) break;
		}

		if (i == vm->prog.vars.len) {

			for (i = 0; i < vm->prog.arrs.len; ++i) {
				BcVec *arr = bc_vec_item(&vm->prog.arrs, i);
				if (arr->len != 1) break;
			}

			good = i == vm->prog.arrs.len;
		}
	}

	// If this condition is true, we can get rid of strings,
	// constants, and code. This is an idea from busybox.
	if (good && prog->stack.len == 1 && prog->results.len == 0 &&
	    vm->prs.flags.len == 1 && ip->idx == f->code.len)
	{
		bc_vec_npop(&f->strs, f->strs.len);
		bc_vec_npop(&f->consts, f->consts.len);
		bc_vec_npop(&f->code, f->code.len);
		ip->idx = 0;

		if (!BC_IS_BC) bc_vec_npop(fns, fns->len - BC_PROG_REQ_FUNCS);
	}
}

BcStatus bc_vm_process(BcVm *vm, const char *text, bool is_stdin) {

	BcStatus s;

	s = bc_parse_text(&vm->prs, text);
	if (s) goto err;

	while (vm->prs.l.t != BC_LEX_EOF) {
		s = vm->prs.parse(&vm->prs);
		if (s) goto err;
	}

	if (BC_PARSE_CAN_EXEC(&vm->prs)) {
		s = bc_program_exec(&vm->prog);
		if (BC_I) bc_vm_fflush(stdout);
		if (!s) bc_vm_garbageCollect();
	}

err:
	if (s || BC_SIGINT) s = bc_program_reset(&vm->prog, s);
	return s == BC_STATUS_QUIT || !BC_I || !is_stdin ? s : BC_STATUS_SUCCESS;
}

BcStatus bc_vm_file(BcVm *vm, const char *file) {

	BcStatus s;
	char *data;
	BcFunc *main_func;
	BcInstPtr *ip;

	bc_lex_file(&vm->prs.l, file);
	s = bc_read_file(file, &data);
	if (s) return s;

	s = bc_vm_process(vm, data, false);
	if (s) goto err;

	main_func = bc_vec_item(&vm->prog.fns, BC_PROG_MAIN);
	ip = bc_vec_item(&vm->prog.stack, 0);

	if (vm->prs.flags.len > 1)
		s = bc_vm_error(BC_ERROR_PARSE_BLOCK, vm->prs.l.line);
	else if (!BC_PARSE_CAN_EXEC(&vm->prs) || main_func->code.len < ip->idx)
		s = bc_vm_err(BC_ERROR_EXEC_FILE_NOT_EXECUTABLE);

err:
	free(data);
	return s;
}

BcStatus bc_vm_stdin(BcVm *vm) {

	BcStatus s = BC_STATUS_SUCCESS;
	BcVec buf, buffer;
	size_t string = 0;
	bool comment = false, done = false;

	bc_lex_file(&vm->prs.l, bc_program_stdin_name);

	bc_vec_init(&buffer, sizeof(uchar), NULL);
	bc_vec_init(&buf, sizeof(uchar), NULL);
	bc_vec_pushByte(&buffer, '\0');

	// This loop is complex because the vm tries not to send any lines that end
	// with a backslash to the parser. The reason for that is because the parser
	// treats a backslash+newline combo as whitespace, per the bc spec. In that
	// case, and for strings and comments, the parser will expect more stuff.
	while (!done && (!(s = bc_read_line(&buf, ">>> ")) || buf.len > 1) &&
	       !BC_SIGINT && s != BC_STATUS_SIGNAL)
	{
		char c2, *str = buf.v;
		size_t i, len = buf.len - 1;

		done = (s == BC_STATUS_EOF);

		for (i = 0; i < len; ++i) {

			bool notend = len > i + 1;
			uchar c = str[i];

			if (!comment && (i - 1 > len || str[i - 1] != '\\')) {
				if (BC_IS_BC) string ^= c == '"';
				else if (c == ']') string -= 1;
				else if (c == '[') string += 1;
			}

			if (!string && notend) {

				c2 = str[i + 1];

				if (c == '/' && !comment && c2 == '*') {
					comment = true;
					++i;
				}
				else if (c == '*' && comment && c2 == '/') {
					comment = false;
					++i;
				}
			}
		}

		bc_vec_concat(&buffer, buf.v);

		if (string || comment) continue;
		if (len >= 2 && str[len - 2] == '\\' && str[len - 1] == '\n') continue;

		s = bc_vm_process(vm, buffer.v, true);
		if (s) goto err;

		bc_vec_empty(&buffer);
	}

	if (s && s != BC_STATUS_EOF) goto err;
	else if (BC_SIGINT && !s) s = BC_STATUS_SIGNAL;
	else if (s != BC_STATUS_ERROR) {
		if (comment) s = bc_vm_error(BC_ERROR_PARSE_COMMENT, vm->prs.l.line);
		else if (string) s = bc_vm_error(BC_ERROR_PARSE_STRING, vm->prs.l.line);
		else if (vm->prs.flags.len > 1)
			s = bc_vm_error(BC_ERROR_PARSE_BLOCK, vm->prs.l.line);
	}

err:
	bc_vec_free(&buf);
	bc_vec_free(&buffer);
	return s;
}

BcStatus bc_vm_exec() {

	BcStatus s = BC_STATUS_SUCCESS;
	size_t i;

#if BC_ENABLED
	if (vm->flags & BC_FLAG_L) {

		bc_lex_file(&vm->prs.l, bc_lib_name);
		s = bc_parse_text(&vm->prs, bc_lib);

		while (!s && vm->prs.l.t != BC_LEX_EOF) s = vm->prs.parse(&vm->prs);

		if (s) return s;
	}
#endif // BC_ENABLED

	if (vm->exprs.len) {
		bc_lex_file(&vm->prs.l, bc_program_exprs_name);
		s = bc_vm_process(vm, vm->exprs.v, false);
		if (s) return s;
	}

	for (i = 0; !s && i < vm->files.len; ++i)
		s = bc_vm_file(vm, *((char**) bc_vec_item(&vm->files, i)));
	if (s && s != BC_STATUS_QUIT) return s;

	if ((BC_IS_BC || !vm->files.len) && !vm->exprs.len) s = bc_vm_stdin(vm);

	return s;
}

void bc_vm_shutdown() {
#if BC_ENABLE_HISTORY
	// This must always run to ensure that the terminal is back to normal.
	bc_history_free(&vm->history);
#endif // BC_ENABLE_HISTORY
#ifndef NDEBUG
	bc_vec_free(&vm->files);
	bc_vec_free(&vm->exprs);
	bc_program_free(&vm->prog);
	bc_parse_free(&vm->prs);
	free(vm->env_args);
	free(vm);
#endif // NDEBUG
}

BcStatus bc_vm_boot(int argc, char *argv[], const char *env_len) {

	BcStatus s;
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

	vm->line_len = bc_vm_envLen(env_len);

	bc_vec_init(&vm->files, sizeof(char*), NULL);
	bc_vec_init(&vm->exprs, sizeof(uchar), NULL);

	bc_program_init(&vm->prog);
	vm->parse_init(&vm->prs, &vm->prog, BC_PROG_MAIN);

#if BC_ENABLE_HISTORY
	bc_history_init(&vm->history);
#endif // BC_ENABLE_HISTORY

#if BC_ENABLED
	if (BC_IS_BC) {
		vm->flags |= BC_FLAG_S * (getenv("POSIXLY_CORRECT") != NULL);
		s = bc_vm_envArgs(vm);
		if (s) goto exit;
	}
#endif // BC_ENABLED

	s = bc_args(argc, argv);
	if (s) goto exit;

	vm->ttyin = isatty(STDIN_FILENO);
	vm->flags |= vm->ttyin && isatty(STDOUT_FILENO) ? BC_FLAG_I : 0;

	if (BC_I && !(vm->flags & BC_FLAG_Q)) bc_vm_info(NULL);

	s = bc_vm_exec();

	bc_vm_shutdown();

exit:
	return s != BC_STATUS_ERROR ? BC_STATUS_SUCCESS : s;
}
