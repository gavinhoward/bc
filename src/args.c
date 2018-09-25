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
 * Code for processing command-line arguments.
 *
 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <getopt.h>

#include <status.h>
#include <vector.h>
#include <vm.h>
#include <bc.h>
#include <args.h>

const struct option bc_args_opts[] = {

  { "help", no_argument, NULL, 'h' },
  { "interactive", no_argument, NULL, 'i' },
  { "mathlib", no_argument, NULL, 'l' },
  { "quiet", no_argument, NULL, 'q' },
  { "standard", no_argument, NULL, 's' },
  { "version", no_argument, NULL, 'v' },
  { "warn", no_argument, NULL, 'w' },
  { 0, 0, 0, 0 },

};

const char* const bc_args_short_opts = "hilqsvw";
const char* const bc_args_env_name = "BC_ENV_ARGS";

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
        if (printf(bc_help, argv[0]) < 0) return BC_STATUS_IO_ERR;
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
        if (printf("%s", bc_vm_header) < 0) return BC_STATUS_IO_ERR;
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

    c = getopt_long(argc, argv, bc_args_short_opts, bc_args_opts, &opt_idx);

  } while (c != -1);

  if (do_exit) exit((int) status);

  if (argv[optind] && strcmp(argv[optind], "--") == 0) ++optind;

  for (i = optind; !status && i < argc; ++i)
    status = bc_vec_push(files, 1, argv + i);

  return status;
}
