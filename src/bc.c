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
#include <ctype.h>
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

void bc_sig(int sig) {
  if (sig == SIGINT) {
    if (write(2, bc_sig_msg, sizeof(bc_sig_msg) - 1) >= 0)
      bcg.sig += (bcg.signe = bcg.sig == bcg.sigc);
  }
  else bcg.sig_other = 1;
}

BcStatus bc_error(BcStatus s) {
  if (!s || s >= BC_STATUS_POSIX_NAME_LEN) return s;
  fprintf(stderr, bc_err_fmt, bc_errs[bc_err_indices[s]], bc_err_descs[s]);
  return s * !bcg.tty;
}

BcStatus bc_error_file(BcStatus s, const char *file, size_t line) {
  if (!s || !file || s >= BC_STATUS_POSIX_NAME_LEN) return s;
  fprintf(stderr, bc_err_fmt, bc_errs[bc_err_indices[s]], bc_err_descs[s]);
  fprintf(stderr, "    %s", file);
  fprintf(stderr, bc_err_line + 3 * !line, line);
  return s * !bcg.tty;
}

BcStatus bc_posix_error(BcStatus s, const char *file,
                        size_t line, const char *msg)
{
  int p = (int) bcg.posix, w = (int) bcg.warn;

  if (!(p || w) || s < BC_STATUS_POSIX_NAME_LEN || !file)
    return BC_STATUS_SUCCESS;

  fprintf(stderr, "\n%s %s: %s\n", bc_errs[bc_err_indices[s]],
          p ? "error" : "warning", bc_err_descs[s]);

  if (msg) fprintf(stderr, "    %s\n", msg);
  fprintf(stderr, "    %s", file);
  fprintf(stderr, bc_err_line + 3 * !line, line);

  return s * !!p;
}

BcStatus bc_process(Bc *bc, const char *text) {

  BcStatus s = bc_lex_text(&bc->parse.lex, text);

  if (s) return bc_error_file(s, bc->parse.lex.file, bc->parse.lex.line);

  while (bc->parse.lex.token.type != BC_LEX_EOF) {

    if ((s = bc_parse_parse(&bc->parse)) == BC_STATUS_LIMITS) {

      s = BC_STATUS_IO_ERR;

      if (putchar('\n') == EOF) return s;

      if (printf("BC_BASE_MAX     = %zu\n", (size_t) BC_MAX_BASE) < 0 ||
          printf("BC_DIM_MAX      = %zu\n", (size_t) BC_MAX_DIM) < 0 ||
          printf("BC_SCALE_MAX    = %zu\n", (size_t) BC_MAX_SCALE) < 0 ||
          printf("BC_STRING_MAX   = %zu\n", (size_t) BC_MAX_STRING) < 0 ||
          printf("Max Exponent    = %ld\n", (long) LONG_MAX) < 0 ||
          printf("Number of Vars  = %zu\n", (size_t) SIZE_MAX) < 0)
      {
        return s;
      }

      if (putchar('\n') == EOF) return s;
    }
    else if (s == BC_STATUS_QUIT || bcg.sig_other ||
        (s && (s = bc_error_file(s, bc->parse.lex.file, bc->parse.lex.line))))
    {
      return s;
    }
  }

  if (BC_PARSE_CAN_EXEC(&bc->parse)) {
    s = bc_program_exec(&bc->prog);
    if (bcg.tty) fflush(stdout);
    if (s && s != BC_STATUS_QUIT) s = bc_error(s);
  }

  return s;
}

BcStatus bc_file(Bc *bc, const char *file) {

  BcStatus s;
  char *data;
  BcFunc *main_func;
  BcInstPtr *ip;

  bc->prog.file = file;
  if ((s = bc_io_fread(file, &data))) return s;

  bc_lex_init(&bc->parse.lex, file);
  if ((s = bc_process(bc, data))) goto err;

  main_func = bc_vec_item(&bc->prog.funcs, BC_PROGRAM_MAIN);
  ip = bc_vec_item(&bc->prog.stack, 0);

  if (main_func->code.len < ip->idx) s = BC_STATUS_EXEC_FILE_NOT_EXECUTABLE;

err:
  free(data);
  return s;
}

BcStatus bc_concat(char **buffer, size_t *n, char *buf, size_t total_len) {

  if (total_len > *n) {

    char *temp = realloc(*buffer, total_len + 1);
    if (!temp) return BC_STATUS_MALLOC_FAIL;

    *buffer = temp;
    *n = total_len;
  }

  strcat(*buffer, buf);

  return BC_STATUS_SUCCESS;
}

BcStatus bc_stdin(Bc *bc) {

  BcStatus s;
  char *buf, *buffer, c;
  size_t n, bufn, slen, total_len, len, i;
  bool string, comment, notend;

  bc->prog.file = bc_program_stdin_name;
  bc_lex_init(&bc->parse.lex, bc_program_stdin_name);

  n = bufn = BC_BUF_SIZE;

  if (!(buffer = malloc(BC_BUF_SIZE + 1))) return BC_STATUS_MALLOC_FAIL;

  if (!(buf = malloc(BC_BUF_SIZE + 1))) {
    s = BC_STATUS_MALLOC_FAIL;
    goto buf_err;
  }

  buffer[0] = '\0';
  string = comment = false;
  s = BC_STATUS_SUCCESS;

  // The following loop is complex because the vm tries not to send any lines
  // that end with a backslash to the parser. The reason for that is because the
  // parser treats a backslash+newline combo as whitespace, per the bc spec. In
  // that case, and for strings and comments, the parser will expect more stuff.
  while (!s && !((s = bc_io_getline(&buf, &bufn)) && s != BC_STATUS_BIN_FILE))
  {
    if (s == BC_STATUS_BIN_FILE) {
      s = putchar('\a') == EOF ? BC_STATUS_IO_ERR : BC_STATUS_SUCCESS;
      continue;
    }

    len = strlen(buf);
    slen = strlen(buffer);
    total_len = slen + len;

    if (len == 1 && buf[0] == '"') string = !string;
    else if (len > 1 || comment) {

      for (i = 0; i < len; ++i) {

        notend = len > i + 1;

        if ((c = buf[i]) == '"') string = !string;
        else if (c == '/' && notend && !comment && buf[i + 1] == '*') {
          comment = true;
          break;
        }
        else if (c == '*' && notend && comment && buf[i + 1] == '/')
          comment = false;
      }

      if (string || comment || buf[len - 2] == '\\') {
        if ((s = bc_concat(&buffer, &n, buf, total_len))) goto exit_err;
        continue;
      }
    }

    if ((s = bc_concat(&buffer, &n, buf, total_len))) goto exit_err;

    s = bc_process(bc, buffer);
    buffer[0] = '\0';
  }

  // I/O error will always happen when stdin is
  // closed. It's not a problem in that case.
  s = s == BC_STATUS_IO_ERR ? BC_STATUS_SUCCESS : s;

  if (string) s = BC_STATUS_LEX_NO_STRING_END;
  else if (comment) s = BC_STATUS_LEX_NO_COMMENT_END;

exit_err:
  free(buf);
buf_err:
  free(buffer);
  return s;
}

BcStatus bc_main(unsigned int flags, BcVec *files) {

  BcStatus status;
  Bc bc;
  struct sigaction sa;
  size_t i, len;
  char *lenv;
  int num;

  bcg.tty = (flags & BC_FLAG_I) || (isatty(0) && isatty(1));
  bcg.posix = flags & BC_FLAG_S;
  bcg.warn = flags & BC_FLAG_W;

  if ((lenv = getenv("BC_LINE_LENGTH"))) {
    len = strlen(lenv);
    for (num = 1, i = 0; num && i < len; ++i) num = isdigit(lenv[i]);
    if (!num || ((len = (size_t) atoi(lenv) - 1) < 2 && len >= INT32_MAX))
      len = BC_NUM_PRINT_WIDTH;
  }
  else len = BC_NUM_PRINT_WIDTH;

  if ((status = bc_program_init(&bc.prog, len))) return status;
  if ((status = bc_parse_init(&bc.parse, &bc.prog))) goto parse_err;

  sigemptyset(&sa.sa_mask);
  sa.sa_handler = bc_sig;
  sa.sa_flags = 0;

  if (sigaction(SIGINT, &sa, NULL) < 0 || sigaction(SIGPIPE, &sa, NULL) < 0 ||
      sigaction(SIGHUP, &sa, NULL) < 0 || sigaction(SIGTERM, &sa, NULL) < 0)
  {
    status = BC_STATUS_EXEC_SIGACTION_FAIL;
    goto err;
  }

  if (bcg.tty && !(flags & BC_FLAG_Q) && printf("%s", bc_header) < 0) {
    status = BC_STATUS_IO_ERR;
    goto err;
  }

  if (flags & BC_FLAG_L) {

    bc_lex_init(&bc.parse.lex, bc_lib_name);
    if ((status = bc_lex_text(&bc.parse.lex, bc_lib))) goto err;

    while (!status && bc.parse.lex.token.type != BC_LEX_EOF)
      status = bc_parse_parse(&bc.parse);

    if (status || (status = bc_program_exec(&bc.prog))) goto err;
  }

  for (i = 0; !bcg.sig_other && !status && i < files->len; ++i)
    status = bc_file(&bc, *((char**) bc_vec_item(files, i)));
  if (status || bcg.sig_other) goto err;

  status = bc_stdin(&bc);

err:
  bc_parse_free(&bc.parse);
parse_err:
  bc_program_free(&bc.prog);
  return status == BC_STATUS_QUIT ? BC_STATUS_SUCCESS : status;
}
