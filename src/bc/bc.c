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

#include <assert.h>
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

  if (!st || st >= BC_STATUS_POSIX_NAME_LEN) return st == BC_STATUS_QUIT;

  fprintf(stderr, "\n%s error: %s\n\n", bc_err_types[bc_err_type_indices[st]],
          bc_err_descs[st]);

  return st * !bcg.bc_interactive;
}

BcStatus bc_error_file(BcStatus st, const char *file, size_t line) {

  if (!st || !file || st >= BC_STATUS_POSIX_NAME_LEN)
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

void bc_free() {
  bc_parse_free(&bcg.bc.parse);
  bc_program_free(&bcg.bc.prog);
}

static void bc_sigint(int sig) {
  if (write(2, bc_program_sig_msg, strlen(bc_program_sig_msg)) >= 0)
    bcg.bc_sig = ~(sig);
}

static void bc_signal_handler(int sig) {

  struct sigaction act;

  bc_free();

  sigemptyset(&act.sa_mask);
  act.sa_handler = SIG_DFL;

  raise(sig);
}

static BcStatus bc_signal() {

  BcStatus st;
  BcFunc *func;
  BcInstPtr *ip;

  bcg.bc_sig = 0;

  if ((st = bc_vec_npop(&bcg.bc.prog.stack, bcg.bc.prog.stack.len - 1)))
    return st;

  func = bc_vec_item(&bcg.bc.prog.funcs, 0);
  assert(func);
  ip = bc_vec_top(&bcg.bc.prog.stack);
  assert(ip);

  ip->idx = func->code.len;
  bcg.bc.parse.lex.idx = bcg.bc.parse.lex.len;

  return BC_STATUS_SUCCESS;
}

static BcStatus bc_process(const char *text) {

  BcStatus st = bc_lex_text(&bcg.bc.parse.lex, text, &bcg.bc.parse.token);

  if (st && st != BC_STATUS_LEX_EOF &&
      (st = bc_error_file(st, bcg.bc.parse.lex.file, bcg.bc.parse.lex.line)))
  {
    return st;
  }

  do {

    st = bc_parse_parse(&bcg.bc.parse);

    if (st && st != BC_STATUS_LEX_EOF) {
      st = bc_error_file(st, bcg.bc.parse.lex.file, bcg.bc.parse.lex.line);
      if (st) return st;
    }

    if (bcg.bc_sig && (!bcg.bc_interactive || (st = bc_signal()))) {
      if ((st = bc_error(st))) return st;
    }

    if (st) {

      if (st != BC_STATUS_LEX_EOF && st != BC_STATUS_QUIT &&
          st != BC_STATUS_LIMITS)
      {
        st = bc_error_file(st, bcg.bc.prog.file, bcg.bc.parse.lex.line);
        if (st) return st;
      }
      else if (st == BC_STATUS_QUIT) {
        break;
      }
      else if (st == BC_STATUS_LIMITS) {
        bc_program_limits(&bcg.bc.prog);
        st = BC_STATUS_SUCCESS;
        continue;
      }
      else st = BC_STATUS_SUCCESS;

      while (!st && bcg.bc.parse.token.type != BC_LEX_NEWLINE &&
             bcg.bc.parse.token.type != BC_LEX_SEMICOLON)
      {
        st = bc_lex_next(&bcg.bc.parse.lex, &bcg.bc.parse.token);
      }
    }

  } while (!st);

  if (st != BC_STATUS_LEX_EOF && st != BC_STATUS_QUIT && (st = bc_error(st)))
    return st;

  if (BC_PARSE_CAN_EXEC(&bcg.bc.parse)) {

    st = bcg.bc.exec(&bcg.bc.prog);

    if (bcg.bc_interactive) {

      fflush(stdout);

      if (st && (st = bc_error(st))) return st;

      if (bcg.bc_sig) {
        st = bc_signal();
        fprintf(stderr, "%s", bc_program_ready_prompt);
        fflush(stderr);
      }
    }
    else {
      if (st && (st = bc_error(st))) return st;
      if (bcg.bc_sig && (st = bc_signal())) st = bc_error(st);
    }
  }

  return st;
}

static BcStatus bc_file(const char *file) {

  BcStatus st;
  char *data;
  BcFunc *main_func;
  BcInstPtr *ip;

  bcg.bc.prog.file = file;

  if ((st = bc_io_fread(file, &data))) return st;

  bc_lex_init(&bcg.bc.parse.lex, file);

  if ((st = bc_process(data))) goto err;

  main_func = bc_vec_item(&bcg.bc.prog.funcs, BC_PROGRAM_MAIN);
  ip = bc_vec_item(&bcg.bc.prog.stack, 0);

  assert(main_func && ip);

  if (main_func->code.len > ip->idx) st = BC_STATUS_EXEC_FILE_NOT_EXECUTABLE;

err:

  free(data);

  return st;
}

static BcStatus bc_stdin() {

  BcStatus st;
  char *buf;
  char *buffer;
  char *temp;
  size_t n, bufn, slen, total_len;
  bool string, comment;

  bcg.bc.prog.file = bc_program_stdin_name;
  bc_lex_init(&bcg.bc.parse.lex, bc_program_stdin_name);

  n = BC_BUF_SIZE;
  bufn = BC_BUF_SIZE;
  buffer = malloc(BC_BUF_SIZE + 1);

  if (!buffer) return BC_STATUS_MALLOC_FAIL;

  buffer[0] = '\0';

  buf = malloc(BC_BUF_SIZE + 1);

  if (!buf) {
    st = BC_STATUS_MALLOC_FAIL;
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
  while ((!st || st != BC_STATUS_QUIT) &&
         !(st = bc_io_fgetline(&buf, &bufn, stdin)))
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
            st = BC_STATUS_MALLOC_FAIL;
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
        st = BC_STATUS_MALLOC_FAIL;
        goto exit_err;
      }

      buffer = temp;
      n = slen + len;
    }

    strcat(buffer, buf);
    st = bc_process(buffer);
    buffer[0] = '\0';
  }

  st = !st || st == BC_STATUS_QUIT || st == BC_STATUS_LEX_EOF ?
         BC_STATUS_SUCCESS : st;

exit_err:

  free(buf);

buf_err:

  free(buffer);

  return st;
}

BcStatus bc_main(unsigned int flags, unsigned int filec, char *filev[]) {

  BcStatus status;
  struct sigaction sa1, sa2;
  size_t i;

  bcg.bc_interactive = (flags & BC_FLAG_I) || (isatty(0) && isatty(1));

  bcg.bc_std = flags & BC_FLAG_S;
  bcg.bc_warn = flags & BC_FLAG_W;

  bcg.bc.exec = (flags & BC_FLAG_C) ? bc_program_print : bc_program_exec;

  if ((status = bc_program_init(&bcg.bc.prog))) return status;

  if ((status = bc_parse_init(&bcg.bc.parse, &bcg.bc.prog))) {
    bc_program_free(&bcg.bc.prog);
    return status;
  }

  sigemptyset(&sa1.sa_mask);
  sa1.sa_handler = bc_sigint;
  sigemptyset(&sa2.sa_mask);
  sa2.sa_handler = bc_signal_handler;

  if (sigaction(SIGINT, &sa1, NULL) < 0 || sigaction(SIGPIPE, &sa2, NULL) < 0 ||
      sigaction(SIGHUP, &sa2, NULL) < 0 || sigaction(SIGTERM, &sa2, NULL) < 0)
  {
    status = BC_STATUS_EXEC_SIGACTION_FAIL;
    goto err;
  }

  if (!(flags & BC_FLAG_Q) && (printf("%s", bc_header) < 0)) {
    status = BC_STATUS_IO_ERR;
    goto err;
  }

  if (flags & BC_FLAG_L) {

    bc_lex_init(&bcg.bc.parse.lex, bc_lib_name);
    if ((status = bc_lex_text(&bcg.bc.parse.lex, bc_lib, &bcg.bc.parse.token)))
      goto err;

    while (!status) status = bc_parse_parse(&bcg.bc.parse);
    if (status != BC_STATUS_LEX_EOF) goto err;

    // Make sure to execute the math library.
    if ((status = bcg.bc.exec(&bcg.bc.prog))) goto err;
  }

  for (i = 0; !status && i < filec; ++i) status = bc_file(filev[i]);
  if (status) goto err;

  status = bc_stdin();

err:

  status = status == BC_STATUS_QUIT ? BC_STATUS_SUCCESS : status;

  bc_free();

  return status;
}
