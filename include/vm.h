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

#if !defined(BC_ENABLED) && !defined(DC_ENABLED)
#error Must define BC_ENABLED, DC_ENABLED, or both
#endif

// ** Exclude start. **
#define VERSION_STR(V) #V
#define VERSION_STR2(V) VERSION_STR(V)
#define BC_VERSION VERSION_STR2(VERSION)
// ** Exclude end. **

#define BC_FLAG_X (1<<0)
#define BC_FLAG_W (1<<1)
#define BC_FLAG_S (1<<2)
#define BC_FLAG_Q (1<<3)
#define BC_FLAG_L (1<<4)
#define BC_FLAG_I (1<<5)

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

typedef struct BcVm {

	BcParse parse;
	BcProgram prog;

} BcVm;

// ** Exclude start. **
typedef struct BcGlobals {

	unsigned long sig;
	unsigned long sigc;
	unsigned long signe;
	long sig_other;

	long tty;
	long ttyin;
	long posix;
	long warn;

	const char *name;
	const char *sig_msg;
	bool bc;

} BcGlobals;
// ** Exclude end. **

BcStatus bc_vm_header(const char* const help);

BcStatus bc_vm_error(BcStatus s, const char *file, size_t line);
BcStatus bc_vm_posix_error(BcStatus s, const char *file,
                           size_t line, const char *msg);

// ** Exclude start. **
BcStatus bc_vm_file(BcVm *vm, const char *file);
BcStatus bc_vm_stdin(BcVm *vm);

BcStatus bc_vm_exec(unsigned int flags, BcVec *exprs, BcVec *files,
                    BcParseInit parse_init, BcParseExpr parse_expr);
// ** Exclude end. **

#ifdef BC_ENABLED
// ** Exclude start. **
extern const char bc_name[];
// ** Exclude end. **
extern const char bc_sig_msg[];
#endif // BC_ENABLED

// ** Exclude start. **
#ifdef DC_ENABLED
extern const char dc_name[];
extern const char dc_sig_msg[];
#endif // DC_ENABLED

extern const char bc_copyright[];

extern const char bc_lib[];
extern const char *bc_lib_name;

extern const char bc_err_fmt[];
extern const char bc_err_line[];
extern const char *bc_errs[];
extern const uint8_t bc_err_indices[];
extern const char *bc_err_descs[];
// ** Exclude end. **

extern BcGlobals bcg;

#endif // BC_VM_H
