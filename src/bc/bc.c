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

#include <bc.h>
#include <vm.h>

void bc_error(BcStatus status) {

  if (!status || status == BC_STATUS_PARSE_QUIT ||
      status == BC_STATUS_EXEC_HALT ||
      status >= BC_STATUS_POSIX_NAME_LEN)
  {
    return;
  }

  fprintf(stderr, "\n%s error: %s\n\n",
          bc_err_types[status], bc_err_descs[status]);
}

void bc_error_file(BcStatus status, const char *file, uint32_t line) {

  if (!status || status == BC_STATUS_PARSE_QUIT ||
      !file || status >= BC_STATUS_POSIX_NAME_LEN)
  {
    return;
  }

  fprintf(stderr, "\n%s error: %s\n", bc_err_types[status],
          bc_err_descs[status]);

  fprintf(stderr, "    %s", file);
  fprintf(stderr, &":%d\n\n"[3 * !line], line);
}

BcStatus bc_posix_error(BcStatus status, const char *file,
                        uint32_t line, const char *msg)
{
  if (!(bcg.bc_std || bcg.bc_warn) ||
      status < BC_STATUS_POSIX_NAME_LEN ||
      !file)
  {
    return BC_STATUS_SUCCESS;
  }

  fprintf(stderr, "\n%s %s: %s\n", bc_err_types[status],
          bcg.bc_std ? "error" : "warning", bc_err_descs[status]);

  if (msg) fprintf(stderr, "    %s\n", msg);

  fprintf(stderr, "    %s", file);
  fprintf(stderr, &":%d\n\n"[3 * !line], line);

  return bcg.bc_std ? status : BC_STATUS_SUCCESS;
}

BcStatus bc_exec(unsigned long long flags, unsigned int filec, char *filev[]) {

  BcStatus status;
  BcVm vm;
  BcProgramExecFunc exec;

  if ((flags & BC_FLAG_INTERACTIVE) || (isatty(0) && isatty(1))) {
    bcg.bc_interactive = 1;
  } else bcg.bc_interactive = 0;

  bcg.bc_std = flags & BC_FLAG_STANDARD;
  bcg.bc_warn = flags & BC_FLAG_WARN;

  if (!(flags & BC_FLAG_QUIET) && (printf("%s", bc_header) < 0))
    return BC_STATUS_IO_ERR;

  exec = (flags & BC_FLAG_CODE) ? bc_program_print : bc_program_exec;
  status = bc_vm_init(&vm, exec, filec, filev);

  if (status) return status;

  if (flags & BC_FLAG_MATHLIB) {

    status = bc_parse_file(&vm.parse, bc_lib_name);

    if (status) goto err;

    status = bc_parse_text(&vm.parse, (const char*) bc_lib);

    if (status) goto err;

    while (!status) status = bc_parse_parse(&vm.parse);

    if (status != BC_STATUS_LEX_EOF) goto err;

    // Make sure to execute the math library.
    status = vm.exec(&vm.program);

    if (status) goto err;
  }

  status = bc_vm_exec(&vm);

err:

  bc_vm_free(&vm);

  return status;
}
