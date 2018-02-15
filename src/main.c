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
 * A special license exemption is granted to the Toybox project to use this
 * source under the following BSD 0-Clause License:
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
 * The entry point for bc.
 *
 */

#include <stdio.h>
#include <string.h>

#include <locale.h>

#include <getopt.h>

#include <bc/bc.h>

static const struct option bc_opts[] = {

  { "code", no_argument, NULL, 'c' },
  { "help", no_argument, NULL, 'h' },
  { "interactive", no_argument, NULL, 'i' },
  { "mathlib", no_argument, NULL, 'l' },
  { "quiet", no_argument, NULL, 'q' },
  { "standard", no_argument, NULL, 's' },
  { "version", no_argument, NULL, 'v' },
  { "warn", no_argument, NULL, 'w' },
  { 0, 0, 0, 0 },

};

static const char* const bc_short_opts = "chilqsvw";

static const char* const bc_help =
  "usage: bc [-hilqsvw] [long-options] [ file ... ]\n"
  "\n"
  "bc is a command-line calculator with a Turing-complete language.\n"
  "\n"
  "  -c  --code         print bytecode instead of executing it\n"
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

long bc_code = 0;
long bc_interactive = 0;
long bc_std = 0;
long bc_warn = 0;

long bc_signal = 0;

const char* const bc_mathlib = BC_MATHLIB_PATH "mathlib.bc";

const char* const bc_version = VERSION;

int main(int argc, char* argv[]) {

  unsigned int flags;
  unsigned int filec;
  const char** filev;

  setlocale(LC_ALL, "");

  flags = 0;

  // Getopt needs this.
  int opt_idx = 0;

  int c = getopt_long(argc, argv, bc_short_opts, bc_opts, &opt_idx);

  while (c != -1) {

    switch (c) {

      case 0:
      {
        // This is the case when a long option is
        // found, so we don't need to do anything.
        break;
      }

      case 'c':
      {
        flags |= BC_FLAG_CODE;
        break;
      }

      case 'h':
      {
        flags |= BC_FLAG_HELP;
        break;
      }

      case 'i':
      {
        flags |= BC_FLAG_INTERACTIVE;
        break;
      }

      case 'l':
      {
        flags |= BC_FLAG_MATHLIB;
        break;
      }

      case 'q':
      {
        flags |= BC_FLAG_QUIET;
        break;
      }

      case 's':
      {
        flags |= BC_FLAG_STANDARD;
        break;
      }

      case 'v':
      {
        flags |= BC_FLAG_VERSION;
        break;
      }

      case 'w':
      {
        flags |= BC_FLAG_WARN;
        break;
      }

      // Getopt printed an error message, but we should exit.
      case '?':
      default:
      {
        return BC_STATUS_INVALID_OPTION;
        break;
      }
    }

    c = getopt_long(argc, argv, bc_short_opts, bc_opts, &opt_idx);
  }

  if (flags & BC_FLAG_HELP) printf(bc_help);

  if (argv[optind] && strcmp(argv[optind], "--") == 0) {
    ++optind;
  }

  filec = argc - optind;
  filev = (const char**) (argv + optind);

  return bc_exec(flags, filec, filev);
}
