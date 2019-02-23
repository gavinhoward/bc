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
#include <bc.h>

#if BC_ENABLE_SIGNALS
#ifndef _WIN32
static void bc_vm_sig(int sig) {
	int err = errno;
	if (sig == SIGINT) {
		size_t n = vm->sig_len;
		if (BC_ERR(write(STDERR_FILENO, vm->sig_msg, n) != (ssize_t) n))
			sig = 0;
	}
	vm->sig = (uchar) sig;
	errno = err;
}
#else // _WIN32
static BOOL WINAPI bc_vm_sig(DWORD sig) {
	if (sig == CTRL_C_EVENT) bc_vm_puts(vm->sig_msg, stderr);
	vm->sig = (uchar) sig;
	return TRUE;
}
#endif // _WIN32
#endif // BC_ENABLE_SIGNALS

void bc_vm_info(const char* const help) {
	bc_vm_printf("%s %s\n", vm->name, BC_VERSION);
	bc_vm_puts(bc_copyright, stdout);
	if (help) bc_vm_printf(help, vm->name);
}

static BcStatus bc_vm_printError(BcError e, const char* const fmt,
                                 size_t line, va_list args)
{
	size_t id = (size_t) bc_err_ids[e];

	// Make sure all of stdout is written first.
	fflush(stdout);

	fprintf(stderr, fmt, vm->err_ids[id]);
	vfprintf(stderr, vm->err_msgs[e], args);

	assert(vm->file);

	// This is the condition for parsing vs runtime.
	// If line is not 0, it is parsing.
	if (line) {
		fprintf(stderr, "\n    %s", vm->file);
		fprintf(stderr, bc_err_line, line);
	}
	else {
		BcInstPtr *ip = bc_vec_item_rev(&vm->prog.stack, 0);
		BcFunc *f = bc_vec_item(&vm->prog.fns, ip->func);
		fprintf(stderr, "\n    Function: %s", f->name);
		if (ip->func != BC_PROG_MAIN && ip->func != BC_PROG_READ)
			fprintf(stderr, "()");
	}

	fputs("\n\n", stderr);
	fflush(stderr);

	return (BcStatus) (id + 1);
}

BcStatus bc_vm_error(BcError e, size_t line, ...) {

	BcStatus s;
	va_list args;

#if BC_ENABLED
	assert(e < BC_ERROR_POSIX_START || e > BC_ERROR_POSIX_END);
#endif // BC_ENABLED

	va_start(args, line);
	s = bc_vm_printError(e, vm->error_header, line, args);
	va_end(args);

	return s;
}

#if BC_ENABLED
BcStatus bc_vm_posixError(BcError e, size_t line, ...) {

	BcStatus s;
	va_list args;
	int p = (int) BC_S, w = (int) BC_W;

	assert(e >= BC_ERROR_POSIX_START && e <= BC_ERROR_POSIX_END);

	if (BC_UNLIKELY(!(p || w))) return BC_STATUS_SUCCESS;

	va_start(args, line);
	s = bc_vm_printError(e, p ? vm->error_header : vm->warn_header, line, args);
	va_end(args);

	return p ? s : BC_STATUS_SUCCESS;
}

static BcStatus bc_vm_envArgs(void) {

	BcStatus s;
	BcVec v;
	char *env_args = getenv(bc_args_env_name), *buf;

	if (!env_args) return BC_STATUS_SUCCESS;

	vm->env_args = bc_vm_strdup(env_args);
	buf = vm->env_args;

	bc_vec_init(&v, sizeof(char*), NULL);
	bc_vec_push(&v, &bc_args_env_name);

	while (*buf) {

		if (!isspace(*buf)) {

			bc_vec_push(&v, &buf);

			while (*buf && !isspace(*buf)) buf += 1;

			if (*buf) {
				*buf = '\0';
				buf += 1;
			}
		}
		else buf += 1;
	}

	// Make sure to push a NULL pointer at the end.
	buf = NULL;
	bc_vec_push(&v, &buf);

	s = bc_args((int) v.len - 1, (char**) v.v);

	bc_vec_free(&v);

	return s;
}
#endif // BC_ENABLED

static size_t bc_vm_envLen(const char *var) {

	char *lenv = getenv(var);
	size_t i, len = BC_NUM_PRINT_WIDTH;
	int num;

	if (!lenv) return len;

	len = strlen(lenv);

	for (num = 1, i = 0; num && i < len; ++i) num = isdigit(lenv[i]);
	if (num) {
		len = (size_t) atoi(lenv) - 1;
		if (len < 2 || len >= UINT16_MAX) len = BC_NUM_PRINT_WIDTH;
	}
	else len = BC_NUM_PRINT_WIDTH;

	return len;
}

void bc_vm_shutdown(void) {
#if BC_ENABLE_NLS
	catclose(vm->catalog);
#endif // BC_ENABLE_NLS
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

static void bc_vm_exit(BcError e) {
	BcStatus s = bc_vm_err(e);
	bc_vm_shutdown();
	exit((int) s);
}

size_t bc_vm_arraySize(size_t n, size_t size) {
	size_t result = n * size;
	if (BC_ERR(n != 0 && result / n != size))
		bc_vm_exit(BC_ERROR_FATAL_ALLOC_ERR);
	return result;
}

size_t bc_vm_growSize(size_t a, size_t b) {
	size_t result = a + b;
	if (BC_ERR(result < a || result < b)) bc_vm_exit(BC_ERROR_FATAL_ALLOC_ERR);
	return result;
}

void* bc_vm_malloc(size_t n) {
	void* ptr = malloc(n);
	if (BC_ERR(!ptr)) bc_vm_exit(BC_ERROR_FATAL_ALLOC_ERR);
	return ptr;
}

void* bc_vm_realloc(void *ptr, size_t n) {
	void* temp = realloc(ptr, n);
	if (BC_ERR(!temp)) bc_vm_exit(BC_ERROR_FATAL_ALLOC_ERR);
	return temp;
}

char* bc_vm_strdup(const char *str) {
	char *s = strdup(str);
	if (BC_ERR(!s)) bc_vm_exit(BC_ERROR_FATAL_ALLOC_ERR);
	return s;
}

size_t bc_vm_printf(const char *fmt, ...) {

	va_list args;
	int ret;

	va_start(args, fmt);
	ret = vprintf(fmt, args);
	va_end(args);

	if (BC_ERR(ret < 0 || ferror(stdout))) bc_vm_exit(BC_ERROR_FATAL_IO_ERR);

	return (size_t) ret;
}

void bc_vm_puts(const char *str, FILE *restrict f) {
	if (BC_ERR(fputs(str, f) == EOF || ferror(f)))
		bc_vm_exit(BC_ERROR_FATAL_IO_ERR);
}

void bc_vm_putchar(int c) {
	if (BC_ERR(putchar(c) == EOF || ferror(stdout)))
		bc_vm_exit(BC_ERROR_FATAL_IO_ERR);
}

void bc_vm_fflush(FILE *restrict f) {
	if (BC_ERR(fflush(f) == EOF || ferror(f)))
		bc_vm_exit(BC_ERROR_FATAL_IO_ERR);
}

static void bc_vm_clean() {

	BcProgram *prog = &vm->prog;
	BcVec *fns = &prog->fns;
	BcFunc *f = bc_vec_item(fns, BC_PROG_MAIN);
	BcInstPtr *ip = bc_vec_item(&prog->stack, 0);
	bool good = false;

#if BC_ENABLED
	if (BC_IS_BC) good = !BC_PARSE_NO_EXEC(&vm->prs);
#endif // BC_ENABLED

#if DC_ENABLED
	if (!BC_IS_BC) {

		size_t i;

		for (i = 0; i < vm->prog.vars.len; ++i) {
			BcVec *arr = bc_vec_item(&vm->prog.vars, i);
			BcNum *n = bc_vec_top(arr);
			if (arr->len != 1 || BC_PROG_STR(n)) break;
		}

		if (i == vm->prog.vars.len) {

			for (i = 0; i < vm->prog.arrs.len; ++i) {

				BcVec *arr = bc_vec_item(&vm->prog.arrs, i);
				size_t j;

				assert(arr->len == 1);

				arr = bc_vec_top(arr);

				for (j = 0; j < arr->len; ++j) {
					BcNum *n = bc_vec_item(arr, j);
					if (BC_PROG_STR(n)) break;
				}

				if (j != arr->len) break;
			}

			good = (i == vm->prog.arrs.len);
		}
	}
#endif // DC_ENABLED

	// If this condition is true, we can get rid of strings,
	// constants, and code. This is an idea from busybox.
	if (good && prog->stack.len == 1 && !prog->results.len &&
	    ip->idx == f->code.len)
	{
#if BC_ENABLED
		bc_vec_npop(&f->labels, f->labels.len);
#endif // BC_ENABLED
		bc_vec_npop(&f->strs, f->strs.len);
		bc_vec_npop(&f->consts, f->consts.len);
		bc_vec_npop(&f->code, f->code.len);
		ip->idx = 0;
#if DC_ENABLED
		if (!BC_IS_BC) bc_vec_npop(fns, fns->len - BC_PROG_REQ_FUNCS);
#endif // DC_ENABLED
	}
}

static BcStatus bc_vm_process(const char *text, bool is_stdin) {

	BcStatus s;

	s = bc_parse_text(&vm->prs, text);
	if (BC_ERR(s)) goto err;

	while (vm->prs.l.t != BC_LEX_EOF) {
		s = vm->parse(&vm->prs);
		if (BC_ERR(s)) goto err;
	}

#if BC_ENABLED
	if (BC_PARSE_NO_EXEC(&vm->prs)) goto err;
#endif // BC_ENABLED

	s = bc_program_exec(&vm->prog);
	if (BC_I) bc_vm_fflush(stdout);

err:
	bc_vm_clean();
	return s == BC_STATUS_QUIT || !BC_I || !is_stdin ? s : BC_STATUS_SUCCESS;
}

static BcStatus bc_vm_file(const char *file) {

	BcStatus s;
	char *data;

	bc_lex_file(&vm->prs.l, file);
	s = bc_read_file(file, &data);
	if (BC_ERR(s)) return s;

	s = bc_vm_process(data, false);
	if (BC_ERR(s)) goto err;

#if BC_ENABLED
	if (BC_ERR(BC_PARSE_NO_EXEC(&vm->prs)))
		s = bc_parse_err(&vm->prs, BC_ERROR_PARSE_BLOCK);
#endif // BC_ENABLED

err:
	free(data);
	return s;
}

static BcStatus bc_vm_stdin(void) {

	BcStatus s = BC_STATUS_SUCCESS;
	BcVec buf, buffer;
	size_t string = 0;
	bool comment = false, done = false;

	bc_lex_file(&vm->prs.l, bc_program_stdin_name);

	bc_vec_init(&buffer, sizeof(uchar), NULL);
	bc_vec_init(&buf, sizeof(uchar), NULL);
	bc_vec_pushByte(&buffer, '\0');
	s = bc_read_line(&buf, ">>> ");

	// This loop is complex because the vm tries not to send any lines that end
	// with a backslash to the parser. The reason for that is because the parser
	// treats a backslash+newline combo as whitespace, per the bc spec. In that
	// case, and for strings and comments, the parser will expect more stuff.
	for (; !done && !BC_STATUS_IS_ERROR(s) && buf.len > 1 && BC_NO_SIGNAL &&
	       s != BC_STATUS_SIGNAL; s = bc_read_line(&buf, ">>> "))
	{
		char c2, *str = buf.v;
		size_t i, len = buf.len - 1;

		done = (s == BC_STATUS_EOF);

		if (len >= 2 && str[len - 1] == '\n' && str[len - 2] == '\\') {
			bc_vec_concat(&buffer, buf.v);
			continue;
		}

		for (i = 0; i < len; ++i) {

			bool notend = len > i + 1;
			uchar c = (uchar) str[i];

			if (!comment && (i - 1 > len || str[i - 1] != '\\')) {
				if (BC_IS_BC) string ^= c == '"';
				else if (c == ']') string -= 1;
				else if (c == '[') string += 1;
			}

			if (BC_IS_BC && !string && notend) {

				c2 = str[i + 1];

				if (c == '/' && !comment && c2 == '*') {
					comment = true;
					i += 1;
				}
				else if (c == '*' && comment && c2 == '/') {
					comment = false;
					i += 1;
				}
			}
		}

		bc_vec_concat(&buffer, buf.v);

		if (string || comment) continue;
		if (len >= 2 && str[len - 2] == '\\' && str[len - 1] == '\n') continue;

		s = bc_vm_process(buffer.v, true);
		if (BC_ERR(s)) goto err;

		bc_vec_empty(&buffer);
	}

	if (BC_ERR(s && s != BC_STATUS_EOF)) goto err;
	else if (BC_NO_ERR(!s) && BC_SIGNAL) s = BC_STATUS_SIGNAL;
	else if (!BC_STATUS_IS_ERROR(s)) {
		if (BC_ERR(comment))
			s = bc_parse_err(&vm->prs, BC_ERROR_PARSE_COMMENT);
		else if (BC_ERR(string))
			s = bc_parse_err(&vm->prs, BC_ERROR_PARSE_STRING);
#if BC_ENABLED
		else if (BC_ERR(BC_PARSE_NO_EXEC(&vm->prs)))
			s = bc_parse_err(&vm->prs, BC_ERROR_PARSE_BLOCK);
#endif // BC_ENABLED
	}

err:
	bc_vec_free(&buf);
	bc_vec_free(&buffer);
	return s;
}

#if BC_ENABLED
static BcStatus bc_vm_load(const char *name, const char *text) {

	BcStatus s;

	bc_lex_file(&vm->prs.l, name);
	s = bc_parse_text(&vm->prs, text);

	while (BC_NO_ERR(!s) && vm->prs.l.t != BC_LEX_EOF) s = vm->parse(&vm->prs);

	return s;
}
#endif // BC_ENABLED

static void bc_vm_gettext() {

	size_t i;
#if BC_ENABLE_NLS
	BcVec dir;
	char *dot, id = 0;
	int set, msg;

	bc_vec_init(&dir, sizeof(char), NULL);

	bc_vec_string(&dir, strlen(BC_LOCALEDIR), BC_LOCALEDIR);
	bc_vec_concat(&dir, "/");
	bc_vec_concat(&dir, vm->locale);

	dot = strrchr(dir.v, '.');

	if (dot) {
		uintptr_t idx = ((uintptr_t) dot) - ((uintptr_t) dir.v);
		bc_vec_npop(&dir, dir.len - idx);
		bc_vec_pushByte(&dir, '\0');
	}

	bc_vec_concat(&dir, ".cat");
	vm->catalog = catopen(dir.v, NL_CAT_LOCALE);

	set = msg = 1;

	vm->error_header = catgets(vm->catalog, set, msg, bc_err_fmt);
#if BC_ENABLED
	vm->warn_header = catgets(vm->catalog, set, msg + 1, bc_warn_fmt);
#endif // BC_ENABLED

	for (set += 1, msg = 1; msg < BC_ERR_IDX_NELEMS + 1; ++msg)
		vm->err_ids[msg - 1] = catgets(vm->catalog, set, msg, bc_errs[msg - 1]);

	i = 0;
	id = bc_err_ids[i];

	for (set = id + 3, msg = 1; i < BC_ERROR_NELEMS; ++i, ++msg) {

		if (id != bc_err_ids[i]) {
			msg = 1;
			id = bc_err_ids[i];
			set = id + 3;
		}

		vm->err_msgs[i] = catgets(vm->catalog, set, msg, bc_err_msgs[i]);
	}

	bc_vec_free(&dir);

#else // BC_ENABLE_NLS

	vm->error_header = bc_err_fmt;
#if BC_ENABLED
	vm->warn_header = bc_warn_fmt;
#endif // BC_ENABLED

	for (i = 0; i < BC_ERR_IDX_NELEMS; ++i) vm->err_ids[i] = bc_errs[i];
	for (i = 0; i < BC_ERROR_NELEMS; ++i) vm->err_msgs[i] = bc_err_msgs[i];
#endif // BC_ENABLE_NLS
}

static BcStatus bc_vm_exec(void) {

	BcStatus s = BC_STATUS_SUCCESS;
	size_t i;

#if BC_ENABLED
	if (BC_IS_BC && vm->flags & BC_FLAG_L) {
		s = bc_vm_load(bc_lib_name, bc_lib);
		if (BC_ERR(s)) return s;
#if BC_ENABLE_EXTRA_MATH
		if (!BC_IS_POSIX) {
			s = bc_vm_load(bc_lib2_name, bc_lib2);
			if (BC_ERR(s)) return s;
		}
#endif // BC_ENABLE_EXTRA_MATH
	}
#endif // BC_ENABLED

	if (vm->exprs.len) {
		bc_lex_file(&vm->prs.l, bc_program_exprs_name);
		s = bc_vm_process(vm->exprs.v, false);
		if (BC_ERR(s)) return s;
	}

	for (i = 0; BC_NO_ERR(!s) && i < vm->files.len; ++i)
		s = bc_vm_file(*((char**) bc_vec_item(&vm->files, i)));
	if (BC_ERR(s && s != BC_STATUS_QUIT)) return s;

	if ((BC_IS_BC || !vm->files.len) && !vm->exprs.len) s = bc_vm_stdin();

	return s;
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
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGQUIT, &sa, NULL);
#else // _WIN32
	SetConsoleCtrlHandler(bc_vm_sig, TRUE);
#endif // _WIN32
#endif // BC_ENABLE_SIGNALS

	bc_vm_gettext();

	vm->line_len = (uint16_t) bc_vm_envLen(env_len);

	bc_vec_init(&vm->files, sizeof(char*), NULL);
	bc_vec_init(&vm->exprs, sizeof(uchar), NULL);

	bc_program_init(&vm->prog);
	bc_parse_init(&vm->prs, &vm->prog, BC_PROG_MAIN);

#if BC_ENABLE_HISTORY
	bc_history_init(&vm->history);
#endif // BC_ENABLE_HISTORY

#if BC_ENABLED
	if (BC_IS_BC) {
		vm->flags |= BC_FLAG_S * (getenv("POSIXLY_CORRECT") != NULL);
		s = bc_vm_envArgs();
		if (BC_ERR(s)) goto exit;
	}
#endif // BC_ENABLED

	s = bc_args(argc, argv);
	if (BC_ERR(s)) goto exit;

	vm->flags |= isatty(STDIN_FILENO) ? BC_FLAG_TTYIN : 0;
	vm->flags |= BC_TTYIN && isatty(STDOUT_FILENO) ? BC_FLAG_I : 0;

	vm->max_ibase = BC_IS_BC && !BC_IS_POSIX ? BC_NUM_MAX_IBASE :
	                                           BC_NUM_MAX_POSIX_IBASE;

	if (BC_I && !(vm->flags & BC_FLAG_Q)) bc_vm_info(NULL);

	s = bc_vm_exec();

exit:
	bc_vm_shutdown();
	return !BC_STATUS_IS_ERROR(s) ? BC_STATUS_SUCCESS : s;
}
