/*
 * Copyright 2018 Contributors
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.

 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.

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
 *******************************************************************************
 *
 * A special license exemption is granted to the Toybox project and to the
 * Android Open Source Project to use this source under the following BSD
 * 0-Clause License:
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
 *******************************************************************************
 *
 * The virtual machine.
 *
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

#include <bc/bc.h>
#include <bc/io.h>
#include <bc/vm.h>

static BcStatus bc_vm_execFile(BcVm* vm, int idx);
static BcStatus bc_vm_execStdin(BcVm* vm);

static BcStatus bc_vm_handleSignal(BcVm* vm);

static const char* const bc_stdin_filename = "<stdin>";

static const char* const bc_ready_prompt = "ready for more input\n\n";

static const char* const bc_sigint_msg =
  "\n\ninterrupt (type \"quit\" to exit)\n\n";

static void bc_vm_sigint(int sig) {

  struct sigaction act;

  sigemptyset(&act.sa_mask);
  act.sa_handler = bc_vm_sigint;

  sigaction(SIGINT, &act, NULL);

  if (sig == SIGINT) {
    write(STDERR_FILENO, bc_sigint_msg, strlen(bc_sigint_msg));
    bc_signal = 1;
  }
}

BcStatus bc_vm_init(BcVm* vm, int filec, const char* filev[]) {

  vm->filec = filec;
  vm->filev = filev;

  return BC_STATUS_SUCCESS;
}

BcStatus bc_vm_exec(BcVm* vm) {

  BcStatus status;
  int num_files;
  struct sigaction act;

  sigemptyset(&act.sa_mask);
  act.sa_handler = bc_vm_sigint;

  if (sigaction(SIGINT, &act, NULL) < 0) {
    return BC_STATUS_EXEC_SIGACTION_FAIL;
  }

  status = BC_STATUS_SUCCESS;

  num_files = vm->filec;

  status = bc_program_init(&vm->program);

  if (status) {
    return status;
  }

  status = bc_parse_init(&vm->parse, &vm->program);

  if (status) {
    goto parse_err;
  }

  for (int i = 0; !status && i < num_files; ++i) {
    status = bc_vm_execFile(vm, i);
  }

  if (status) {
    status = status == BC_STATUS_PARSE_QUIT ||
             status == BC_STATUS_EXEC_HALT ?
                 BC_STATUS_SUCCESS : status;
    goto exec_err;
  }

  status = bc_vm_execStdin(vm);

  status = status == BC_STATUS_PARSE_QUIT ||
           status == BC_STATUS_EXEC_HALT ?
               BC_STATUS_SUCCESS : status;

exec_err:

  bc_parse_free(&vm->parse);

parse_err:

  bc_program_free(&vm->program);

  return status;
}

static BcStatus bc_vm_execFile(BcVm* vm, int idx) {

  BcStatus status;
  FILE* f;
  size_t size;
  size_t read_size;
  const char* file;

  file = vm->filev[idx];
  vm->program.file = file;

  f = fopen(file, "r");

  if (!f) {
    return BC_STATUS_EXEC_FILE_ERR;
  }

  fseek(f, 0, SEEK_END);
  size = ftell(f);

  fseek(f, 0, SEEK_SET);

  char* data = malloc(size + 1);

  if (!data) {
    status = BC_STATUS_MALLOC_FAIL;
    goto data_err;
  }

  read_size = fread(data, 1, size, f);

  if (read_size != size) {
    status = BC_STATUS_IO_ERR;
    goto read_err;
  }

  data[size] = '\0';

  fclose(f);
  f = NULL;

  status = bc_parse_file(&vm->parse, file);

  if (status) {
    goto read_err;
  }

  status = bc_parse_text(&vm->parse, data);

  if (status) {
    goto read_err;
  }

  do {

    status = bc_parse_parse(&vm->parse);

    if (bc_signal && !bc_interactive) {
      goto read_err;
    }
    else {

      status = bc_vm_handleSignal(vm);

      if (status) return status;
    }

    if (status) {

      if (status != BC_STATUS_LEX_EOF &&
          status != BC_STATUS_PARSE_EOF &&
          status != BC_STATUS_PARSE_QUIT &&
          status != BC_STATUS_PARSE_LIMITS)
      {
        bc_error_file(status, vm->program.file, vm->parse.lex.line);
      }
      else if (status == BC_STATUS_PARSE_QUIT) {
        break;
      }
      else if (status == BC_STATUS_PARSE_LIMITS) {
        bc_program_limits(&vm->program);
        status = BC_STATUS_SUCCESS;
        continue;
      }

      while (vm->parse.token.type != BC_LEX_NEWLINE &&
             vm->parse.token.type != BC_LEX_SEMICOLON)
      {
        status = bc_lex_next(&vm->parse.lex, &vm->parse.token);

        if (status) {
          break;
        }
      }
    }

  } while (!status);

  if (status != BC_STATUS_PARSE_EOF &&
      status != BC_STATUS_LEX_EOF &&
      status != BC_STATUS_PARSE_QUIT)
  {
    goto read_err;
  }

  if (!bc_code) {

    do {

      if (BC_PARSE_CAN_EXEC(&vm->parse)) {

        status = bc_program_exec(&vm->program);

        if (status) {
          goto read_err;
        }

        if (bc_interactive) {

          fflush(stdout);

          if (bc_signal) {

            status = bc_vm_handleSignal(vm);

            fprintf(stderr, bc_ready_prompt);
            fflush(stderr);
          }
        }
        else if (bc_signal) {
          status = bc_vm_handleSignal(vm);
          goto read_err;
        }
      }
      else {
        status = BC_STATUS_EXEC_FILE_NOT_EXECUTABLE;
      }

    } while (!status);
  }
  else {

    do {

      if (BC_PARSE_CAN_EXEC(&vm->parse)) {

        bc_program_printCode(&vm->program);

        if (bc_interactive) {

          fflush(stdout);

          if (bc_signal) {

            status = bc_vm_handleSignal(vm);

            fprintf(stderr, bc_ready_prompt);
            fflush(stderr);
          }
        }
        else if (bc_signal) {
          status = bc_vm_handleSignal(vm);
          goto read_err;
        }
      }
      else {
        status = BC_STATUS_EXEC_FILE_NOT_EXECUTABLE;
      }

    } while (!status);
  }

read_err:

  bc_error(status);

  free(data);

data_err:

  if (f) {
    fclose(f);
  }

  return status;
}

static BcStatus bc_vm_execStdin(BcVm* vm) {

  BcStatus status;
  char* buf;
  char* buffer;
  char* temp;
  size_t n;
  size_t bufn;
  size_t slen;
  size_t total_len;
  bool string;
  bool comment;

  vm->program.file = bc_stdin_filename;

  status = bc_parse_file(&vm->parse, bc_stdin_filename);

  if (status) {
    return status;
  }

  n = BC_VM_BUF_SIZE;
  bufn = BC_VM_BUF_SIZE;
  buffer = malloc(BC_VM_BUF_SIZE + 1);

  if (!buffer) {
    return BC_STATUS_MALLOC_FAIL;
  }

  buffer[0] = '\0';

  buf = malloc(BC_VM_BUF_SIZE + 1);

  if (!buf) {
    status = BC_STATUS_MALLOC_FAIL;
    goto buf_err;
  }

  string = false;
  comment = false;

  // The following loop is complicated because the vm tries
  // not to send any lines that end with a backslash to the
  // parser. The reason for that is because the parser treats
  // a backslash newline combo as whitespace, per the bc spec.
  // Thus, the parser will expect more stuff. That is also
  // the case with strings and comments.
  while ((!status || status != BC_STATUS_PARSE_QUIT) &&
         bc_io_getline(&buf, &bufn) != BC_INVALID_IDX)
  {
    size_t len;

    len = strlen(buf);
    slen = strlen(buffer);
    total_len = slen + len;

    if (len == 1 && buf[0] == '"') {
      string = !string;
    }
    else if (len > 1) {

      for (uint32_t i = 0; i < len; ++i) {

        char c;
        bool notend;

        notend = len > i + 1;

        c = buf[i];

        if (c == '"') {
          string = !string;
        }
        else if (c == '/' && notend && comment && buf[i + 1] == '*') {
          comment = true;
        }
        else if (c == '*' && notend && !comment && buf[i + 1] == '/') {
          comment = false;
        }
      }

      if (string || comment || buf[len - 2] == '\\') {

        if (total_len > n) {

          temp = realloc(buffer, total_len + 1);

          if (!temp) {
            status = BC_STATUS_MALLOC_FAIL;
            goto exit_err;
          }

          buffer = temp;
          n = slen + len;
        }

        strcat(buffer, buf);

        continue;
      }
    }

    if (total_len > n) {

      temp = realloc(buffer, total_len + 1);

      if (!temp) {
        status = BC_STATUS_MALLOC_FAIL;
        goto exit_err;
      }

      buffer = temp;
      n = slen + len;
    }

    strcat(buffer, buf);

    status = bc_parse_text(&vm->parse, buffer);

    if (!bc_signal) {

      if (status) {

        if (status == BC_STATUS_PARSE_QUIT ||
            status == BC_STATUS_LEX_EOF ||
            status == BC_STATUS_PARSE_EOF)
        {
          break;
        }
        else if (status == BC_STATUS_PARSE_LIMITS) {
          bc_program_limits(&vm->program);
          status = BC_STATUS_SUCCESS;
        }
        else {
          bc_error(status);
          goto exit_err;
        }
      }
    }
    else if (status == BC_STATUS_PARSE_QUIT) {
      break;
    }
    else if (status == BC_STATUS_PARSE_LIMITS) {
      bc_program_limits(&vm->program);
      status = BC_STATUS_SUCCESS;
    }

    while (!status) {
      status = bc_parse_parse(&vm->parse);
    }

    if (status == BC_STATUS_PARSE_QUIT) {
      break;
    }
    else if (status == BC_STATUS_PARSE_LIMITS) {
      bc_program_limits(&vm->program);
      status = BC_STATUS_SUCCESS;
    }
    else if (status != BC_STATUS_LEX_EOF && status != BC_STATUS_PARSE_EOF) {

      bc_error_file(status, vm->program.file, vm->parse.lex.line);

      while (vm->parse.token.type != BC_LEX_NEWLINE &&
             vm->parse.token.type != BC_LEX_SEMICOLON)
      {
        status = bc_lex_next(&vm->parse.lex, &vm->parse.token);

        if (status && status != BC_STATUS_LEX_EOF) {
          bc_error_file(status, vm->program.file, vm->parse.lex.line);
          break;
        }
        else if (status == BC_STATUS_LEX_EOF) {
          status = BC_STATUS_SUCCESS;
          break;
        }
      }
    }

    if (BC_PARSE_CAN_EXEC(&vm->parse)) {

      if (!bc_code) {

        status = bc_program_exec(&vm->program);

        if (status) {
          bc_error(status);
          goto exit_err;
        }

        if (bc_interactive) {

          fflush(stdout);

          if (bc_signal) {
            status = bc_vm_handleSignal(vm);
            fprintf(stderr, bc_ready_prompt);
          }
        }
        else if (bc_signal) {
          status = bc_vm_handleSignal(vm);
          goto exit_err;
        }
      }
      else {

        bc_program_printCode(&vm->program);

        if (bc_interactive) {

          fflush(stdout);

          if (bc_signal) {
            status = bc_vm_handleSignal(vm);
            fprintf(stderr, bc_ready_prompt);
          }
        }
        else if (bc_signal) {
          status = bc_vm_handleSignal(vm);
          goto exit_err;
        }
      }
    }

    buffer[0] = '\0';
  }

  status = !status || status == BC_STATUS_PARSE_QUIT ||
           status == BC_STATUS_EXEC_HALT ||
           status == BC_STATUS_LEX_EOF ||
           status == BC_STATUS_PARSE_EOF ?
               BC_STATUS_SUCCESS : status;

exit_err:

  free(buf);

buf_err:

  free(buffer);

  return status;
}

static BcStatus bc_vm_handleSignal(BcVm* vm) {

  BcStatus status;
  BcFunc* func;
  BcInstPtr* ip;

  bc_signal = 0;

  while (vm->program.stack.len > 1) {

    status = bc_vec_pop(&vm->program.stack);

    if (status) return status;
  }

  func = bc_vec_item(&vm->program.funcs, 0);

  if (!func) return BC_STATUS_EXEC_UNDEFINED_FUNC;

  ip = bc_vec_top(&vm->program.stack);

  if (!ip) return BC_STATUS_EXEC_INVALID_STMT;

  ip->idx = func->code.len;

  return BC_STATUS_SUCCESS;
}
