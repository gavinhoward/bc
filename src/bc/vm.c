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
 * The virtual machine.
 *
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

#include <bc.h>
#include <io.h>
#include <vm.h>

static void bc_vm_sigint(int sig) {

  struct sigaction act;
  ssize_t err;

  sigemptyset(&act.sa_mask);
  act.sa_handler = bc_vm_sigint;

  sigaction(SIGINT, &act, NULL);

  if (sig == SIGINT) {
    err = write(STDERR_FILENO, bc_program_sigint_msg,
                strlen(bc_program_sigint_msg));
    if (err >= 0) bcg.bc_signal = 1;
  }
}

static BcStatus bc_vm_signal(BcVm *vm) {

  BcStatus status;
  BcFunc *func;
  BcInstPtr *ip;

  bcg.bc_signal = 0;

  status = bc_vec_npop(&vm->program.stack, vm->program.stack.len - 1);

  if (status) return status;

  func = bc_vec_item(&vm->program.funcs, 0);

  if (!func) return BC_STATUS_EXEC_UNDEFINED_FUNC;

  ip = bc_vec_top(&vm->program.stack);

  if (!ip) return BC_STATUS_EXEC_BAD_STMT;

  ip->idx = func->code.len;

  return BC_STATUS_SUCCESS;
}

static BcStatus bc_vm_process(BcVm *vm, const char *text) {

  BcStatus status;

  status = bc_parse_text(&vm->parse, text);

  if (status && status != BC_STATUS_LEX_EOF &&
      (status = bc_error_file(status, vm->parse.lex.file, vm->parse.lex.line)))
  {
    return status;
  }

  do {

    status = bc_parse_parse(&vm->parse);

    if (status && status != BC_STATUS_LEX_EOF) {
      status = bc_error_file(status, vm->parse.lex.file, vm->parse.lex.line);
      if (status) return status;
    }

    if (bcg.bc_signal && (!bcg.bc_interactive || (status = bc_vm_signal(vm)))) {
      if ((status = bc_error(status))) return status;
    }

    if (status) {

      if (status != BC_STATUS_LEX_EOF && status != BC_STATUS_QUIT &&
          status != BC_STATUS_LIMITS)
      {
        status = bc_error_file(status, vm->program.file, vm->parse.lex.line);
        if (status) return status;
      }
      else if (status == BC_STATUS_QUIT) {
        break;
      }
      else if (status == BC_STATUS_LIMITS) {
        bc_program_limits(&vm->program);
        status = BC_STATUS_SUCCESS;
        continue;
      }
      else status = BC_STATUS_SUCCESS;

      while (!status && vm->parse.token.type != BC_LEX_NEWLINE &&
             vm->parse.token.type != BC_LEX_SEMICOLON)
      {
        status = bc_lex_next(&vm->parse.lex, &vm->parse.token);
      }
    }

  } while (!status);

  if (status != BC_STATUS_LEX_EOF && status != BC_STATUS_QUIT &&
      (status = bc_error(status)))
  {
    return status;
  }

  if (BC_PARSE_CAN_EXEC(&vm->parse)) {

    status = vm->exec(&vm->program);

    if (bcg.bc_interactive) {

      fflush(stdout);

      if (status && (status = bc_error(status))) return status;

      if (bcg.bc_signal) {

        status = bc_vm_signal(vm);

        fprintf(stderr, "%s", bc_program_ready_prompt);
        fflush(stderr);
      }
    }
    else {

      if (status && (status = bc_error(status))) return status;

      if (bcg.bc_signal && (status = bc_vm_signal(vm)))
        status = bc_error(status);
    }
  }

  return status;
}

static BcStatus bc_vm_execFile(BcVm *vm, int idx) {

  BcStatus status;
  const char *file;
  char *data;
  BcFunc *main_func;
  BcInstPtr *ip;

  file = vm->filev[idx];
  vm->program.file = file;

  status = bc_io_fread(file, &data);

  if (status) return status;

  status = bc_parse_file(&vm->parse, file);

  if (status) goto err;

  status = bc_vm_process(vm, data);

  if (status) goto err;

  main_func = bc_vec_item(&vm->program.funcs, BC_PROGRAM_MAIN);
  ip = bc_vec_item(&vm->program.stack, 0);

  if (!main_func) status = BC_STATUS_EXEC_UNDEFINED_FUNC;
  else if (!ip) status = BC_STATUS_EXEC_BAD_STACK;
  else if (main_func->code.len > ip->idx)
    status = BC_STATUS_EXEC_FILE_NOT_EXECUTABLE;

err:

  free(data);

  status = bc_error(status);

  return status;
}

static BcStatus bc_vm_execStdin(BcVm *vm) {

  BcStatus status;
  char *buf;
  char *buffer;
  char *temp;
  size_t n;
  size_t bufn;
  size_t slen;
  size_t total_len;
  bool string;
  bool comment;

  vm->program.file = bc_program_stdin_name;

  status = bc_parse_file(&vm->parse, bc_program_stdin_name);

  if (status) return status;

  n = BC_VM_BUF_SIZE;
  bufn = BC_VM_BUF_SIZE;
  buffer = malloc(BC_VM_BUF_SIZE + 1);

  if (!buffer) return BC_STATUS_MALLOC_FAIL;

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
  while ((!status || status != BC_STATUS_QUIT) &&
         !(status = bc_io_getline(&buf, &bufn)))
  {
    size_t len, i;

    len = strlen(buf);
    slen = strlen(buffer);
    total_len = slen + len;

    if (len == 1 && buf[0] == '"') string = !string;
    else if (len > 1 || comment) {

      for (i = 0; i < len; ++i) {

        char c;
        bool notend;

        notend = len > i + 1;

        c = buf[i];

        if (c == '"') string = !string;
        else if (c == '/' && notend && !comment && buf[i + 1] == '*') {
          comment = true;
          break;
        }
        else if (c == '*' && notend && comment && buf[i + 1] == '/') {
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

    status = bc_vm_process(vm, buffer);

    buffer[0] = '\0';
  }

  status = !status || status == BC_STATUS_QUIT || status == BC_STATUS_LEX_EOF ?
             BC_STATUS_SUCCESS : status;

exit_err:

  free(buf);

buf_err:

  free(buffer);

  return status;
}

BcStatus bc_vm_init(BcVm *vm, BcProgramExecFunc exec, int filec, char *filev[])
{
  BcStatus status;
  struct sigaction act;

  sigemptyset(&act.sa_mask);
  act.sa_handler = bc_vm_sigint;

  if (sigaction(SIGINT, &act, NULL) < 0) return BC_STATUS_EXEC_SIGACTION_FAIL;

  status = bc_program_init(&vm->program);

  if (status) return status;

  status = bc_parse_init(&vm->parse, &vm->program);

  if (status) {
    bc_program_free(&vm->program);
    return status;
  }

  vm->exec = exec;
  vm->filec = filec;
  vm->filev = filev;

  return BC_STATUS_SUCCESS;
}

void bc_vm_free(BcVm *vm) {
  bc_parse_free(&vm->parse);
  bc_program_free(&vm->program);
}

BcStatus bc_vm_exec(BcVm *vm) {

  BcStatus status;
  int num_files, i;

  status = BC_STATUS_SUCCESS;

  num_files = vm->filec;

  for (i = 0; !status && i < num_files; ++i) status = bc_vm_execFile(vm, i);

  if (status) return status == BC_STATUS_QUIT ? BC_STATUS_SUCCESS : status;

  status = bc_vm_execStdin(vm);

  status = status == BC_STATUS_QUIT ? BC_STATUS_SUCCESS : status;

  return status;
}
