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
 * Code common to all of bc.
 *
 */

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <limits.h>

#include <status.h>
#include <bc.h>
#include <vm.h>

BcStatus bc_error(BcStatus st) {

  if (!st || st == BC_STATUS_QUIT || st >= BC_STATUS_POSIX_NAME_LEN)
    return st == BC_STATUS_QUIT;

  fprintf(stderr, "\n%s error: %s\n\n",
          bc_err_types[bc_err_type_indices[st]], bc_err_descs[st]);

  return st * !bcg.bc_interactive;
}

BcStatus bc_error_file(BcStatus st, const char *file, size_t line) {

  if (!st || st == BC_STATUS_QUIT || !file || st >= BC_STATUS_POSIX_NAME_LEN)
    return st == BC_STATUS_QUIT;

  fprintf(stderr, "\n%s error: %s\n", bc_err_types[bc_err_type_indices[st]],
          bc_err_descs[st]);

  fprintf(stderr, "    %s", file);
  fprintf(stderr, &":%d\n\n"[3 * !line], line);

  return st * !bcg.bc_interactive;
}

BcStatus bc_posix_error(BcStatus st, const char *file,
                        size_t line, const char *msg)
{
  int s = bcg.bc_std, w = bcg.bc_warn;

  if (!(s || w) || st < BC_STATUS_POSIX_NAME_LEN || !file)
    return BC_STATUS_SUCCESS;

  fprintf(stderr, "\n%s %s: %s\n", bc_err_types[bc_err_type_indices[st]],
          s ? "error" : "warning", bc_err_descs[st]);

  if (msg) fprintf(stderr, "    %s\n", msg);

  fprintf(stderr, "    %s", file);
  fprintf(stderr, &":%d\n\n"[3 * !line], line);

  return st * !!s;
}

BcStatus bc_main(unsigned int flags, unsigned int filec, char *filev[]) {

  BcStatus status;
  BcVm vm;
  BcProgramExecFunc exec;

  bcg.bc_interactive = (flags & BC_FLAG_INTERACTIVE) || (isatty(0) && isatty(1));

  bcg.bc_std = flags & BC_FLAG_STANDARD;
  bcg.bc_warn = flags & BC_FLAG_WARN;

  if (!(flags & BC_FLAG_QUIET) && (printf("%s", bc_header) < 0)) return BC_STATUS_IO_ERR;

  exec = (flags & BC_FLAG_CODE) ? bc_program_print : bc_program_exec;
  if ((status = bc_vm_init(&vm, exec, filec, filev))) return status;

  if (flags & BC_FLAG_MATHLIB) {

    if ((status = bc_parse_file(&vm.parse, bc_lib_name))) goto err;
    if ((status = bc_parse_text(&vm.parse, bc_lib))) goto err;

    while (!status) status = bc_parse_parse(&vm.parse);
    if (status != BC_STATUS_LEX_EOF) goto err;

    // Make sure to execute the math library.
    if ((status = vm.exec(&vm.program))) goto err;
  }

  status = bc_vm_exec(&vm);

err:

  bc_vm_free(&vm);

  return status;
}
