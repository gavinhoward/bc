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
 * Definitions for all of bc.
 *
 */

#ifndef BC_H
#define BC_H

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
#define BC_FLAG_C (1<<5)

#define BC_MAX(a, b) ((a) > (b) ? (a) : (b))

#define BC_MIN(a, b) ((a) < (b) ? (a) : (b))

#define BC_INVALID_IDX ((size_t) -1)

#define BC_BASE_MAX_DEF (999)
#define BC_DIM_MAX_DEF (INT_MAX)
#define BC_SCALE_MAX_DEF (LONG_MAX)
#define BC_STRING_MAX_DEF (INT_MAX)

#define BC_BUF_SIZE (1024)

typedef struct Bc {

  BcParse parse;
  BcProgram prog;
  BcProgramExecFunc exec;

} Bc;

// ** Exclude start. **
typedef struct BcGlobals {

  long interactive;
  long std;
  long warn;

  unsigned long sig_int;
  unsigned long sig_int_catches;
  long sig_other;

} BcGlobals;

BcStatus bc_main(unsigned int flags, unsigned int filec, char *filev[]);
// ** Exclude end. **

BcStatus bc_error(BcStatus st);
BcStatus bc_error_file(BcStatus st, const char *file, size_t line);

BcStatus bc_posix_error(BcStatus st, const char *file,
                        size_t line, const char *msg);

BcStatus bc_fread(const char *path, char** buf);

extern BcGlobals bcg;

extern const char bc_lib[];
extern const char *bc_lib_name;

extern const char *bc_header;
extern const char *bc_err_types[];
extern const uint8_t bc_err_type_indices[];
extern const char *bc_err_descs[];

#endif // BC_H
