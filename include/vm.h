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
 * Definitions for bc's VM.
 *
 */

#ifndef BC_VM_H
#define BC_VM_H

#include <stddef.h>
#include <limits.h>

#include <status.h>
#include <parse.h>
#include <program.h>
#include <history.h>

#if !BC_ENABLED && !DC_ENABLED
#error Must define BC_ENABLED, DC_ENABLED, or both
#endif

// CHAR_BIT must be at least 4.
#if CHAR_BIT < 4
#error CHAR_BIT must be at least 4.
#endif

// ** Exclude start. **
#define VERSION_STR(V) #V
#define VERSION_STR2(V) VERSION_STR(V)
#define BC_VERSION VERSION_STR2(VERSION)
// ** Exclude end. **

// Windows has deprecated isatty().
#ifdef _WIN32
#define isatty _isatty
#endif // _WIN32

#define DC_FLAG_X (1<<0)
#define BC_FLAG_W (1<<1)
#define BC_FLAG_V (1<<2)
#define BC_FLAG_S (1<<3)
#define BC_FLAG_Q (1<<4)
#define BC_FLAG_L (1<<5)
#define BC_FLAG_I (1<<6)

#define BC_MAX(a, b) ((a) > (b) ? (a) : (b))
#define BC_MIN(a, b) ((a) < (b) ? (a) : (b))

#define BC_MAX_OBASE ((unsigned long) 999)
#define BC_MAX_DIM ((unsigned long) INT_MAX)
#define BC_MAX_SCALE ((unsigned long) UINT_MAX)
#define BC_MAX_STRING ((unsigned long) UINT_MAX - 1)
#define BC_MAX_NAME BC_MAX_STRING
#define BC_MAX_NUM BC_MAX_STRING
#define BC_MAX_EXP ((unsigned long) LONG_MAX)
#define BC_MAX_VARS ((unsigned long) SIZE_MAX - 1)

#define BC_IS_BC (BC_ENABLED && (!DC_ENABLED || vm->name[0] == 'b'))

#if BC_ENABLE_SIGNALS
#define BC_SIGINT (vm->sig)
#else // BC_ENABLE_SIGNALS
#define BC_SIGINT (0)
#endif // BC_ENABLE_SIGNALS

#define BC_S (BC_ENABLED && (vm->flags & BC_FLAG_S))
#define BC_W (BC_ENABLED && (vm->flags & BC_FLAG_W))
#define BC_L (BC_ENABLED && (vm->flags & BC_FLAG_L))
#define BC_I (BC_ENABLED && (vm->flags & BC_FLAG_I))
#define DC_X (DC_ENABLED && (vm->flags & DC_FLAG_X))

#define bc_vm_err(e) (bc_vm_error((e), 0))

typedef struct BcVm {

	BcParse prs;
	BcProgram prog;

	const char* file;

	uint16_t line_len;

	// ** Exclude start. **
	uint8_t flags;
	uint8_t ttyin;

	uchar read_ret;

#if BC_ENABLE_SIGNALS
	uint8_t sig;
	uint8_t sig_len;
	const char *sig_msg;
#endif // BC_ENABLE_SIGNALS

	BcVec files;

	// ** Busybox exclude start. **
	BcVec exprs;
	// ** Busybox exclude end. **

	char *env_args;

	// ** Busybox exclude start. **
	const char *name;
	const char *help;
	// ** Busybox exclude end. **

#if BC_ENABLE_HISTORY
	BcHistory history;
#endif // BC_ENABLE_HISTORY

	BcParseInit parse_init;
	BcParseExpr parse_expr;
	// ** Exclude end. **

} BcVm;

#if BC_ENABLED
BcStatus bc_vm_posixError(BcError e, size_t line, ...);
#endif // BC_ENABLED

// ** Exclude start. **
void bc_vm_info(const char* const help);
BcStatus bc_vm_boot(int argc, char *argv[], const char *env_len);
void bc_vm_shutdown();
// ** Exclude end. **

// ** Busybox exclude start. **

void bc_vm_exit(BcError e);
void bc_vm_printf(const char *fmt, ...);
void bc_vm_puts(const char *str, FILE *restrict f);
void bc_vm_putchar(int c);
void bc_vm_fflush(FILE *restrict f);

// ** Exclude start. **
void* bc_vm_malloc(size_t n);
void* bc_vm_realloc(void *ptr, size_t n);
char* bc_vm_strdup(const char *str);
void bc_vm_exit(BcError e);

BcStatus bc_vm_error(BcError e, size_t line, ...);
// ** Exclude end. **

#if BC_ENABLED
// ** Exclude start. **
extern const char bc_name[];
// ** Exclude end. **
extern const char bc_sig_msg[];
#endif // BC_ENABLED

// ** Exclude start. **
#if DC_ENABLED
extern const char dc_name[];
extern const char dc_sig_msg[];
#endif // DC_ENABLED

extern const char bc_copyright[];

extern const char bc_lib[];
extern const char *bc_lib_name;

extern const char* const bc_err_fmt;
extern const char* const bc_warn_fmt;
extern const char* const bc_err_line;
extern const char *bc_errs[];
extern const char bc_err_ids[];
extern const char *bc_err_msgs[];
// ** Exclude end. **

// ** Busybox exclude end. **

extern BcVm *vm;

#endif // BC_VM_H
