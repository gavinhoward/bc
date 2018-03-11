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
 * Definitions for bc's virtual machine.
 *
 */

#ifndef BC_VM_H
#define BC_VM_H

#include <bc.h>
#include <program.h>
#include <parse.h>

#define BC_VM_BUF_SIZE (1024)

typedef struct BcVm {

  BcProgram program;
  BcParse parse;

  int filec;
  const char** filev;

} BcVm;

// ** Exclude start. **
BcStatus bc_vm_init(BcVm *vm, int filec, const char *filev[]);

void bc_vm_free(BcVm *vm);

BcStatus bc_vm_exec(BcVm *vm);
// ** Exclude end. **

#endif // BC_VM_H
