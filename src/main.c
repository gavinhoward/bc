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
 * The entry point for bc.
 *
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <locale.h>

#include <getopt.h>

#include <vector.h>
#include <bc.h>

const struct option bc_opts[] = {

  { "help", no_argument, NULL, 'h' },
  { "interactive", no_argument, NULL, 'i' },
  { "mathlib", no_argument, NULL, 'l' },
  { "quiet", no_argument, NULL, 'q' },
  { "standard", no_argument, NULL, 's' },
  { "version", no_argument, NULL, 'v' },
  { "warn", no_argument, NULL, 'w' },
  { 0, 0, 0, 0 },

};

const char *bc_short_opts = "hilqsvw";

const char *bc_help =
  "usage: bc [-hilqsvw] [long-options] [file...]\n"
  "\n"
  "bc is a command-line calculator with a Turing-complete language.\n"
  "\n"
  "  -h  --help         print this usage message and exit\n"
  "  -i  --interactive  force interactive mode\n"
  "  -l  --mathlib      use predefined math routines:\n\n"
  "                       s(expr)  =  sine of expr in radians\n"
  "                       c(expr)  =  cosine of expr in radians\n"
  "                       a(expr)  =  arctangent of expr, returning radians\n"
  "                       l(expr)  =  natural log of expr\n"
  "                       e(expr)  =  raises e to the power of expr\n"
  "                       j(n, x)  =  Bessel function of integer order n of x\n"
  "\n"
  "  -q  --quiet        don't print version and copyright\n"
  "  -s  --standard     error if any non-POSIX extensions are used\n"
  "  -w  --warn         warn if any non-POSIX extensions are used\n"
  "  -v  --version      print version information and copyright and exit\n\n";

const char *bc_env_args_name = "BC_ENV_ARGS";

BcGlobals bcg;

BcStatus bc_args(int argc, char *argv[], unsigned int *flags, BcVec *files) {

  BcStatus status;
  int c, i, opt_idx;
  bool do_exit;

  do_exit = false;
  opt_idx = c = 0;
  status = BC_STATUS_SUCCESS;
  optind = 1;

  do {

    switch (c) {

      case 0:
      {
        // This is the case when a long option is
        // found, so we don't need to do anything.
        break;
      }

      case 'h':
      {
        if (printf("%s", bc_help) < 0) return BC_STATUS_IO_ERR;
        do_exit = true;
        break;
      }

      case 'i':
      {
        (*flags) |= BC_FLAG_I;
        break;
      }

      case 'l':
      {
        (*flags) |= BC_FLAG_L;
        break;
      }

      case 'q':
      {
        (*flags) |= BC_FLAG_Q;
        break;
      }

      case 's':
      {
        (*flags) |= BC_FLAG_S;
        break;
      }

      case 'v':
      {
        if (printf("%s", bc_header) < 0) return BC_STATUS_IO_ERR;
        do_exit = true;
        break;
      }

      case 'w':
      {
        (*flags) |= BC_FLAG_W;
        break;
      }

      // Getopt printed an error message, but we should exit.
      case '?':
      default:
      {
        return BC_STATUS_INVALID_OPTION;
      }
    }

    c = getopt_long(argc, argv, bc_short_opts, bc_opts, &opt_idx);

  } while (c != -1);

  if (do_exit) exit(status);

  if (argv[optind] && strcmp(argv[optind], "--") == 0) ++optind;

  for (i = optind; !status && i < argc; ++i)
    status = bc_vec_push(files, argv + i);

  return status;
}

int main(int argc, char *argv[]) {

  BcStatus status;
  BcVec files, args;
  unsigned int flags;
  char *env_args, *buffer, *buf;

  setlocale(LC_ALL, "");

  memset(&bcg, 0, sizeof(BcGlobals));

  flags = 0;

  if ((status = bc_vec_init(&files, sizeof(char*), NULL))) return status;

  if ((env_args = getenv(bc_env_args_name))) {

    if ((status = bc_vec_init(&args, sizeof(char*), NULL))) goto err;
    if ((status = bc_vec_push(&args, &bc_env_args_name))) goto args_err;

    if (!(buffer = strdup(env_args))) {
      status = BC_STATUS_MALLOC_FAIL;
      goto args_err;
    }

    buf = buffer;

    while (*buf) {

      if (!isspace(*buf)) {

        if ((status = bc_vec_push(&args, &buf))) goto buf_err;

        while (*buf && !isspace(*buf)) ++buf;

        if (*buf) (*(buf++)) = '\0';
      }
      else ++buf;
    }

    status = bc_args(args.len, (char**) args.array, &flags, &files);
    if(status) goto buf_err;
  }

  if((status = bc_args(argc, argv, &flags, &files))) goto buf_err;

  flags |= BC_FLAG_S * (getenv("POSIXLY_CORRECT") != NULL);

  status = bc_main(flags, &files);

buf_err:

  if (env_args) free(buffer);

args_err:

  if (env_args) bc_vec_free(&args);

err:

  bc_vec_free(&files);

  return status;
}
