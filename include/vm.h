/*
 * *****************************************************************************
 *
 * Copyright (c) 2018-2019 Gavin D. Howard and contributors.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * *****************************************************************************
 *
 * Definitions for bc's VM.
 *
 */

#ifndef BC_VM_H
#define BC_VM_H

#include <stddef.h>
#include <limits.h>

#include <signal.h>

#if BC_ENABLE_NLS

#	ifdef _WIN32
#	error NLS is not supported on Windows.
#	endif // _WIN32

#include <nl_types.h>

#endif // BC_ENABLE_NLS

#include <status.h>
#include <parse.h>
#include <program.h>
#include <history.h>

#if !BC_ENABLED && !DC_ENABLED
#error Must define BC_ENABLED, DC_ENABLED, or both
#endif

// CHAR_BIT must be at least 6.
#if CHAR_BIT < 6
#error CHAR_BIT must be at least 6.
#endif

#ifndef BC_ENABLE_NLS
#define BC_ENABLE_NLS (0)
#endif // BC_ENABLE_NLS

#ifndef BC_ENABLE_SIGNALS
#define BC_ENABLE_SIGNALS (1)
#endif // BC_ENABLE_SIGNALS

#ifndef MAINEXEC
#define MAINEXEC bc
#endif

#ifndef EXECPREFIX
#define EXECPREFIX
#endif

#define GEN_STR(V) #V
#define GEN_STR2(V) GEN_STR(V)

#define BC_VERSION GEN_STR2(VERSION)
#define BC_EXECPREFIX GEN_STR2(EXECPREFIX)
#define BC_MAINEXEC GEN_STR2(MAINEXEC)

// Windows has deprecated isatty().
#ifdef _WIN32
#define isatty _isatty
#endif // _WIN32

#define DC_FLAG_X (1<<0)
#define BC_FLAG_W (1<<1)
#define BC_FLAG_S (1<<2)
#define BC_FLAG_Q (1<<3)
#define BC_FLAG_L (1<<4)
#define BC_FLAG_I (1<<5)
#define BC_FLAG_G (1<<6)
#define BC_FLAG_TTYIN (1<<7)
#define BC_TTYIN (vm->flags & BC_FLAG_TTYIN)

#define BC_S (BC_ENABLED && (vm->flags & BC_FLAG_S))
#define BC_W (BC_ENABLED && (vm->flags & BC_FLAG_W))
#define BC_L (BC_ENABLED && (vm->flags & BC_FLAG_L))
#define BC_I (BC_ENABLED && (vm->flags & BC_FLAG_I))
#define BC_G (BC_ENABLED && (vm->flags & BC_FLAG_G))
#define DC_X (DC_ENABLED && (vm->flags & DC_FLAG_X))

#define BC_MAX(a, b) ((a) > (b) ? (a) : (b))
#define BC_MIN(a, b) ((a) < (b) ? (a) : (b))

#define BC_MAX_OBASE ((unsigned long) ULONG_MAX)
#define BC_MAX_DIM ((unsigned long) (SIZE_MAX - 1))
#define BC_MAX_SCALE ((unsigned long) (SIZE_MAX / BC_BASE - 1))
#define BC_MAX_STRING ((unsigned long) (SIZE_MAX - 1))
#define BC_MAX_NAME BC_MAX_STRING
#define BC_MAX_NUM BC_MAX_SCALE
#define BC_MAX_EXP ((unsigned long) ULONG_MAX)
#define BC_MAX_VARS ((unsigned long) (SIZE_MAX - 1))

#define BC_IS_BC (BC_ENABLED && (!DC_ENABLED || vm->name[0] != 'd'))
#define BC_IS_POSIX (BC_S || BC_W)

#if BC_ENABLE_SIGNALS

#define BC_SIG BC_UNLIKELY(vm->sig)
#define BC_NO_SIG BC_LIKELY(!vm->sig)
#define BC_SIGINT (vm->sig == SIGINT)

#ifdef SIGQUIT
#define BC_SIGTERM (vm->sig == SIGTERM || vm->sig == SIGQUIT)
#else // SIGQUIT
#define BC_SIGTERM (vm->sig == SIGTERM)
#endif // SIGQUIT

#else // BC_ENABLE_SIGNALS
#define BC_SIG (0)
#define BC_NO_SIG (1)
#endif // BC_ENABLE_SIGNALS

#define bc_vm_err(e) (bc_vm_error((e), 0))
#define bc_vm_verr(e, ...) (bc_vm_error((e), 0, __VA_ARGS__))

#define BC_IO_ERR(e, f) (BC_ERR((e) == EOF || ferror(f)))
#define BC_STATUS_IS_ERROR(s) \
	((s) >= BC_STATUS_ERROR_MATH && (s) <= BC_STATUS_ERROR_PARSE)
#define BC_ERROR_SIGNAL_ONLY(s) (BC_ENABLE_SIGNALS && BC_ERR(s))

typedef struct BcVm {

	BcParse prs;
	BcProgram prog;

	size_t nchars;

	const char* file;

#if BC_ENABLE_SIGNALS
	const char *sigmsg;
	uchar siglen;
	uchar sig;
#endif // BC_ENABLE_SIGNALS

	uint16_t line_len;
	uchar max_ibase;

	uint8_t flags;
	uchar read_ret;

	BcVec files;
	BcVec exprs;

	const char *name;
	const char *help;

	char *env_args;

#if BC_ENABLE_HISTORY
	BcHistory history;
#endif // BC_ENABLE_HISTORY

	BcLexNext next;
	BcParseParse parse;
	BcParseExpr expr;

	const char *func_header;

	const char *err_ids[BC_ERR_IDX_NELEMS + BC_ENABLED];
	const char *err_msgs[BC_ERROR_NELEMS];

	const char *locale;

#if BC_ENABLE_NLS
	nl_catd catalog;
#endif // BC_ENABLE_NLS

} BcVm;

#if BC_ENABLED
BcStatus bc_vm_posixError(BcError e, size_t line, ...);
#endif // BC_ENABLED

void bc_vm_info(const char* const help);
BcStatus bc_vm_boot(int argc, char *argv[], const char *env_len);
void bc_vm_shutdown(void);

size_t bc_vm_printf(const char *fmt, ...);
void bc_vm_puts(const char *str, FILE *restrict f);
void bc_vm_putchar(int c);
void bc_vm_fflush(FILE *restrict f);

size_t bc_vm_arraySize(size_t n, size_t size);
size_t bc_vm_growSize(size_t a, size_t b);
void* bc_vm_malloc(size_t n);
void* bc_vm_realloc(void *ptr, size_t n);
char* bc_vm_strdup(const char *str);

BcStatus bc_vm_error(BcError e, size_t line, ...);

#if BC_ENABLED
extern const char bc_lib[];
extern const char *bc_lib_name;
#if BC_ENABLE_EXTRA_MATH
extern const char bc_lib2[];
extern const char *bc_lib2_name;
#endif // BC_ENABLE_EXTRA_MATH
#endif // BC_ENABLED

extern const char bc_copyright[];
extern const char* const bc_err_line;
extern const char* const bc_err_func_header;
extern const char *bc_errs[];
extern const uchar bc_err_ids[];
extern const char* const bc_err_msgs[];

extern BcVm *vm;

#endif // BC_VM_H
