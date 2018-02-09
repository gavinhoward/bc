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

static const char* const bc_stdin_filename = "<stdin>";

static const char* const bc_ready_prompt = "ready for more input\n\n";

static const char* const bc_sigint_msg =
  "\n\nSIGINT detected (type \"quit\" to exit)\n\n";

static void bc_vm_sigint(int sig) {

  struct sigaction act;

  sigemptyset(&act.sa_mask);
  act.sa_handler = bc_vm_sigint;

  sigaction(SIGINT, &act, NULL);

  if (sig == SIGINT) {
    write(STDERR_FILENO, bc_sigint_msg, strlen(bc_sigint_msg));
    bc_had_sigint = 1;
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
    return BC_STATUS_VM_SIGACTION_FAIL;
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
             status == BC_STATUS_VM_HALT ?
                 BC_STATUS_SUCCESS : status;
    goto exec_err;
  }

  status = bc_vm_execStdin(vm);

  status = status == BC_STATUS_PARSE_QUIT ||
         status == BC_STATUS_VM_HALT ?
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
    return BC_STATUS_VM_FILE_ERR;
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
    status = BC_STATUS_VM_FILE_READ_ERR;
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

    if (bc_had_sigint && !bc_interactive) {
      goto read_err;
    }
    else {
      bc_had_sigint = 0;
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

          if (bc_had_sigint) {
            fprintf(stderr, bc_ready_prompt);
            fflush(stderr);
            bc_had_sigint = 0;
          }
        }
        else if (bc_had_sigint) {
          bc_had_sigint = 0;
          goto read_err;
        }
      }
      else {
        status = BC_STATUS_VM_FILE_NOT_EXECUTABLE;
      }

    } while (!status);
  }
  else {

    do {

      if (BC_PARSE_CAN_EXEC(&vm->parse)) {

        bc_program_printCode(&vm->program);

        if (bc_interactive) {

          fflush(stdout);

          if (bc_had_sigint) {
            fprintf(stderr, bc_ready_prompt);
            fflush(stderr);
            bc_had_sigint = 0;
          }
        }
        else if (bc_had_sigint) {
          bc_had_sigint = 0;
          goto read_err;
        }
      }
      else {
        status = BC_STATUS_VM_FILE_NOT_EXECUTABLE;
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

    if (!bc_had_sigint) {

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

          if (bc_had_sigint) {
            fprintf(stderr, bc_ready_prompt);
            bc_had_sigint = 0;
          }
        }
        else if (bc_had_sigint) {
          bc_had_sigint = 0;
          goto exit_err;
        }
      }
      else {

        bc_program_printCode(&vm->program);

        if (bc_interactive) {

          fflush(stdout);

          if (bc_had_sigint) {
            fprintf(stderr, bc_ready_prompt);
            bc_had_sigint = 0;
          }
        }
        else if (bc_had_sigint) {
          bc_had_sigint = 0;
          goto exit_err;
        }
      }
    }

    buffer[0] = '\0';
  }

  status = !status || status == BC_STATUS_PARSE_QUIT ||
           status == BC_STATUS_VM_HALT ||
           status == BC_STATUS_LEX_EOF ||
           status == BC_STATUS_PARSE_EOF ?
               BC_STATUS_SUCCESS : status;

exit_err:

  free(buf);

buf_err:

  free(buffer);

  return status;
}
