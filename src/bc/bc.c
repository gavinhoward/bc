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

#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <limits.h>

#include <status.h>
#include <io.h>
#include <bc.h>

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

static void bc_sigint(int sig) {

  struct sigaction act;
  ssize_t err;

  sigemptyset(&act.sa_mask);
  act.sa_handler = bc_sigint;

  sigaction(SIGINT, &act, NULL);

  if (sig == SIGINT) {
    err = write(STDERR_FILENO, bc_program_sigint_msg,
                strlen(bc_program_sigint_msg));
    if (err >= 0) bcg.bc_sig = 1;
  }
}

static BcStatus bc_signal(Bc *bc) {

  BcStatus status;
  BcFunc *func;
  BcInstPtr *ip;

  bcg.bc_sig = 0;

  status = bc_vec_npop(&bc->prog.stack, bc->prog.stack.len - 1);

  if (status) return status;

  func = bc_vec_item(&bc->prog.funcs, 0);

  if (!func) return BC_STATUS_EXEC_UNDEFINED_FUNC;

  ip = bc_vec_top(&bc->prog.stack);

  if (!ip) return BC_STATUS_EXEC_BAD_STMT;

  ip->idx = func->code.len;
  bc->parse.lex.idx = bc->parse.lex.len;

  return BC_STATUS_SUCCESS;
}

static BcStatus bc_process(Bc *bc, const char *text)
{
  BcStatus status = bc_parse_text(&bc->parse, text);

  if (status && status != BC_STATUS_LEX_EOF &&
      (status = bc_error_file(status, bc->parse.lex.file, bc->parse.lex.line)))
  {
    return status;
  }

  do {

    status = bc_parse_parse(&bc->parse);

    if (status && status != BC_STATUS_LEX_EOF) {
      status = bc_error_file(status, bc->parse.lex.file, bc->parse.lex.line);
      if (status) return status;
    }

    if (bcg.bc_sig && (!bcg.bc_interactive || (status = bc_signal(bc)))) {
      if ((status = bc_error(status))) return status;
    }

    if (status) {

      if (status != BC_STATUS_LEX_EOF && status != BC_STATUS_QUIT &&
          status != BC_STATUS_LIMITS)
      {
        status = bc_error_file(status, bc->prog.file, bc->parse.lex.line);
        if (status) return status;
      }
      else if (status == BC_STATUS_QUIT) {
        break;
      }
      else if (status == BC_STATUS_LIMITS) {
        bc_program_limits(&bc->prog);
        status = BC_STATUS_SUCCESS;
        continue;
      }
      else status = BC_STATUS_SUCCESS;

      while (!status && bc->parse.token.type != BC_LEX_NEWLINE &&
             bc->parse.token.type != BC_LEX_SEMICOLON)
      {
        status = bc_lex_next(&bc->parse.lex, &bc->parse.token);
      }
    }

  } while (!status);

  if (status != BC_STATUS_LEX_EOF && status != BC_STATUS_QUIT &&
      (status = bc_error(status)))
  {
    return status;
  }

  if (BC_PARSE_CAN_EXEC(&bc->parse)) {

    status = bc->exec(&bc->prog);

    if (bcg.bc_interactive) {

      fflush(stdout);

      if (status && (status = bc_error(status))) return status;

      if (bcg.bc_sig) {

        status = bc_signal(bc);

        fprintf(stderr, "%s", bc_program_ready_prompt);
        fflush(stderr);
      }
    }
    else {

      if (status && (status = bc_error(status))) return status;

      if (bcg.bc_sig && (status = bc_signal(bc)))
        status = bc_error(status);
    }
  }

  return status;
}

static BcStatus bc_file(Bc *bc, const char *file) {

  BcStatus status;
  char *data;
  BcFunc *main_func;
  BcInstPtr *ip;

  bc->prog.file = file;

  status = bc_io_fread(file, &data);

  if (status) return status;

  status = bc_parse_file(&bc->parse, file);

  if (status) goto err;

  status = bc_process(bc, data);

  if (status) goto err;

  main_func = bc_vec_item(&bc->prog.funcs, BC_PROGRAM_MAIN);
  ip = bc_vec_item(&bc->prog.stack, 0);

  if (!main_func) status = BC_STATUS_EXEC_UNDEFINED_FUNC;
  else if (!ip) status = BC_STATUS_EXEC_BAD_STACK;
  else if (main_func->code.len > ip->idx)
    status = BC_STATUS_EXEC_FILE_NOT_EXECUTABLE;

err:

  free(data);

  return status;
}

static BcStatus bc_stdin(Bc *bc) {

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

  bc->prog.file = bc_program_stdin_name;

  status = bc_parse_file(&bc->parse, bc_program_stdin_name);

  if (status) return status;

  n = BC_BUF_SIZE;
  bufn = BC_BUF_SIZE;
  buffer = malloc(BC_BUF_SIZE + 1);

  if (!buffer) return BC_STATUS_MALLOC_FAIL;

  buffer[0] = '\0';

  buf = malloc(BC_BUF_SIZE + 1);

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

    status = bc_process(bc, buffer);

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

BcStatus bc_main(unsigned int flags, unsigned int filec, char *filev[]) {

  BcStatus status;
  Bc bc;
  struct sigaction act;
  size_t i;

  sigemptyset(&act.sa_mask);
  act.sa_handler = bc_sigint;

  if (sigaction(SIGINT, &act, NULL) < 0) return BC_STATUS_EXEC_SIGACTION_FAIL;

  bcg.bc_interactive = (flags & BC_FLAG_INTERACTIVE) || (isatty(0) && isatty(1));

  bcg.bc_std = flags & BC_FLAG_STANDARD;
  bcg.bc_warn = flags & BC_FLAG_WARN;

  if (!(flags & BC_FLAG_QUIET) && (printf("%s", bc_header) < 0)) return BC_STATUS_IO_ERR;

  bc.exec = (flags & BC_FLAG_CODE) ? bc_program_print : bc_program_exec;

  status = bc_program_init(&bc.prog);

  if (status) return status;

  status = bc_parse_init(&bc.parse, &bc.prog);

  if (status) goto parse_err;

  if (flags & BC_FLAG_MATHLIB) {

    if ((status = bc_parse_file(&bc.parse, bc_lib_name))) goto err;
    if ((status = bc_parse_text(&bc.parse, bc_lib))) goto err;

    while (!status) status = bc_parse_parse(&bc.parse);
    if (status != BC_STATUS_LEX_EOF) goto err;

    // Make sure to execute the math library.
    if ((status = bc.exec(&bc.prog))) goto err;
  }

  for (i = 0; !status && i < filec; ++i) status = bc_file(&bc, filev[i]);

  if (status) return status == BC_STATUS_QUIT ? BC_STATUS_SUCCESS : status;

  status = bc_stdin(&bc);

  status = status == BC_STATUS_QUIT ? BC_STATUS_SUCCESS : status;

  return status;

err:

  bc_parse_free(&bc.parse);

parse_err:

  bc_program_free(&bc.prog);

  return status;
}
