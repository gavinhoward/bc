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

#include <stdint.h>
#include <stdlib.h>
#include <limits.h>

#include <status.h>
#include <parse.h>
#include <program.h>

#define BC_FLAG_W (1<<0)
#define BC_FLAG_S (1<<1)
#define BC_FLAG_Q (1<<2)
#define BC_FLAG_L (1<<3)
#define BC_FLAG_I (1<<4)

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

#define BC_BUF_SIZE (1024)
#define BC_MAX_LINE (1<<20)

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
	long posix;
	long warn;

} BcGlobals;
// ** Exclude end. **

BcStatus bc_vm_set_sig();

BcStatus bc_vm_error(BcStatus s, const char *file, size_t line);
BcStatus bc_vm_posix_error(BcStatus s, const char *file,
                           size_t line, const char *msg);

BcStatus bc_vm_file(BcVm *bc, const char *file);
BcStatus bc_vm_stdin(BcVm *bc);

BcStatus bc_vm_exec(unsigned int flags, BcVec *files);

extern BcGlobals bcg;

extern const char bc_header[];

extern const char bc_lib[];
extern const char *bc_lib_name;

extern const char bc_err_fmt[];
extern const char bc_err_line[];
extern const char *bc_errs[];
extern const uint8_t bc_err_indices[];
extern const char *bc_err_descs[];

extern const char bc_sig_msg[34];
extern const ssize_t bc_sig_msg_len;

#endif // BC_VM_H
