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

  return st * !bcg.interactive;
}

BcStatus bc_error_file(BcStatus st, const char *file, size_t line) {

  if (!st || !file || st >= BC_STATUS_POSIX_NAME_LEN)
    return st == BC_STATUS_QUIT;

  fprintf(stderr, "\n%s error: %s\n", bc_err_types[bc_err_type_indices[st]],
          bc_err_descs[st]);

  fprintf(stderr, "    %s", file);
  fprintf(stderr, &":%d\n\n"[3 * !line], line);

  return st * !bcg.interactive;
}

BcStatus bc_posix_error(BcStatus st, const char *file,
                        size_t line, const char *msg)
{
  int s = bcg.std, w = bcg.warn;

  if (!(s || w) || st < BC_STATUS_POSIX_NAME_LEN || !file)
    return BC_STATUS_SUCCESS;

  fprintf(stderr, "\n%s %s: %s\n", bc_err_types[bc_err_type_indices[st]],
          s ? "error" : "warning", bc_err_descs[st]);

  if (msg) fprintf(stderr, "    %s\n", msg);

  fprintf(stderr, "    %s", file);
  fprintf(stderr, &":%d\n\n"[3 * !line], line);

  return st * !!s;
}

void bc_sig(int sig) {

  if (sig == SIGINT) {
    if (write(2, bc_program_sig_msg, strlen(bc_program_sig_msg)) >= 0)
      bcg.sig_int = 1;
  }
  else bcg.sig_other = 1;
}

BcStatus bc_signal(Bc *bc) {

  BcStatus st;
  BcFunc *func;
  BcInstPtr *ip;

  bcg.sig_int = 0;

  if ((st = bc_vec_npop(&bc->prog.stack, bc->prog.stack.len - 1))) return st;

  func = bc_vec_item(&bc->prog.funcs, 0);
  assert(func);
  ip = bc_vec_top(&bc->prog.stack);
  assert(ip);

  ip->idx = func->code.len;
  bc->parse.lex.idx = bc->parse.lex.len;

  return BC_STATUS_SUCCESS;
}

BcStatus bc_fread(const char *path, char **buf) {

  BcStatus status;
  FILE *f;
  size_t size, read;

  assert(path && buf);

  f = fopen(path, "r");

  if (!f) return BC_STATUS_EXEC_FILE_ERR;

  fseek(f, 0, SEEK_END);
  size = ftell(f);

  fseek(f, 0, SEEK_SET);

  *buf = malloc(size + 1);

  if (!*buf) {
    status = BC_STATUS_MALLOC_FAIL;
    goto malloc_err;
  }

  read = fread(*buf, 1, size, f);

  if (read != size) {
    status = BC_STATUS_IO_ERR;
    goto read_err;
  }

  (*buf)[size] = '\0';

  fclose(f);

  return BC_STATUS_SUCCESS;

read_err:

  free(*buf);

malloc_err:

  fclose(f);

  return status;
}

BcStatus bc_process(Bc *bc, const char *text) {

  BcStatus st = bc_lex_text(&bc->parse.lex, text, &bc->parse.token);

  if (st && st != BC_STATUS_LEX_EOF &&
      (st = bc_error_file(st, bc->parse.lex.file, bc->parse.lex.line)))
  {
    return st;
  }

  do {

    st = bc_parse_parse(&bc->parse);

    if (st == BC_STATUS_QUIT) return st;

    if (st && st != BC_STATUS_LEX_EOF) {
      st = bc_error_file(st, bc->parse.lex.file, bc->parse.lex.line);
      if (st) return st;
    }

    if (bcg.sig_other) return st;

    if (bcg.sig_int && (!bcg.interactive || (st = bc_signal(bc)))) {
      if ((st = bc_error(st))) return st;
    }

    if (st) {

      if (st != BC_STATUS_LEX_EOF && st != BC_STATUS_QUIT &&
          st != BC_STATUS_LIMITS)
      {
        st = bc_error_file(st, bc->prog.file, bc->parse.lex.line);
        if (st) return st;
      }
      else if (st == BC_STATUS_QUIT) {
        break;
      }
      else if (st == BC_STATUS_LIMITS) {
        bc_program_limits(&bc->prog);
        st = BC_STATUS_SUCCESS;
        continue;
      }
      else st = BC_STATUS_SUCCESS;

      while (!st && bc->parse.token.type != BC_LEX_NEWLINE &&
             bc->parse.token.type != BC_LEX_SEMICOLON)
      {
        st = bc_lex_next(&bc->parse.lex, &bc->parse.token);
      }
    }

  } while (!bcg.sig_other && !st);

  if (bcg.sig_other ||
      (st != BC_STATUS_LEX_EOF && st != BC_STATUS_QUIT && (st = bc_error(st))))
  {
    return st;
  }

  if (BC_PARSE_CAN_EXEC(&bc->parse)) {

    st = bc->exec(&bc->prog);

    if (bcg.sig_other || st == BC_STATUS_QUIT) return st;

    if (bcg.interactive) {

      fflush(stdout);

      if (st && (st = bc_error(st))) return st;

      if (bcg.sig_int) {
        st = bc_signal(bc);
        fprintf(stderr, "%s", bc_program_ready_prompt);
        fflush(stderr);
      }
    }
    else {
      if (st && (st = bc_error(st))) return st;
      if (bcg.sig_int && (st = bc_signal(bc))) st = bc_error(st);
    }
  }

  return st;
}

BcStatus bc_file(Bc *bc, const char *file) {

  BcStatus st;
  char *data;
  BcFunc *main_func;
  BcInstPtr *ip;

  bc->prog.file = file;

  if ((st = bc_fread(file, &data))) return st;

  bc_lex_init(&bc->parse.lex, file);

  if ((st = bc_process(bc, data))) goto err;

  main_func = bc_vec_item(&bc->prog.funcs, BC_PROGRAM_MAIN);
  ip = bc_vec_item(&bc->prog.stack, 0);

  assert(main_func && ip);

  if (main_func->code.len > ip->idx) st = BC_STATUS_EXEC_FILE_NOT_EXECUTABLE;

err:

  free(data);

  return st;
}

BcStatus bc_stdin(Bc *bc) {

  BcStatus st;
  char *buf;
  char *buffer;
  char *temp;
  size_t n, bufn, slen, total_len;
  bool string, comment;

  bc->prog.file = bc_program_stdin_name;
  bc_lex_init(&bc->parse.lex, bc_program_stdin_name);

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
  st = BC_STATUS_SUCCESS;

  // The following loop is complicated because the vm tries
  // not to send any lines that end with a backslash to the
  // parser. The reason for that is because the parser treats
  // a backslash newline combo as whitespace, per the bc spec.
  // Thus, the parser will expect more stuff. That is also
  // the case with strings and comments.
  while (!bcg.sig_other && (!st || st != BC_STATUS_QUIT) &&
         getline(&buf, &bufn, stdin) >= 0)
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
        else if (c == '*' && notend && comment && buf[i + 1] == '/')
          comment = false;
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
    st = bc_process(bc, buffer);
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
  Bc bc;
  struct sigaction sa;
  size_t i;

  bcg.interactive = (flags & BC_FLAG_I) || (isatty(0) && isatty(1));

  bcg.std = flags & BC_FLAG_S;
  bcg.warn = flags & BC_FLAG_W;

  bc.exec = (flags & BC_FLAG_C) ? bc_program_print : bc_program_exec;

  if ((status = bc_program_init(&bc.prog))) return status;

  if ((status = bc_parse_init(&bc.parse, &bc.prog))) goto parse_err;

  sigemptyset(&sa.sa_mask);
  sa.sa_handler = bc_sig;

  if (sigaction(SIGINT, &sa, NULL) < 0 || sigaction(SIGPIPE, &sa, NULL) < 0 ||
      sigaction(SIGHUP, &sa, NULL) < 0 || sigaction(SIGTERM, &sa, NULL) < 0)
  {
    status = BC_STATUS_EXEC_SIGACTION_FAIL;
    goto err;
  }

  if (!(flags & BC_FLAG_Q) && (printf("%s", bc_header) < 0)) {
    status = BC_STATUS_IO_ERR;
    goto err;
  }

  if (flags & BC_FLAG_L) {

    bc_lex_init(&bc.parse.lex, bc_lib_name);
    if ((status = bc_lex_text(&bc.parse.lex, bc_lib, &bc.parse.token)))
      goto err;

    while (!status) status = bc_parse_parse(&bc.parse);
    if (status != BC_STATUS_LEX_EOF) goto err;

    // Make sure to execute the math library.
    if ((status = bc.exec(&bc.prog))) goto err;
  }

  for (i = 0; !bcg.sig_other && !status && i < filec; ++i)
    status = bc_file(&bc, filev[i]);
  if (status || bcg.sig_other) goto err;

  status = bc_stdin(&bc);

err:

  status = status == BC_STATUS_QUIT ? BC_STATUS_SUCCESS : status;

  bc_parse_free(&bc.parse);

parse_err:

  bc_program_free(&bc.prog);

  return status;
}
