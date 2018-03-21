/* bc.c - An implementation of POSIX bc.
 *
 * Copyright 2018 Gavin D. Howard <yzena.tech@gmail.com>
 *
 * See http://pubs.opengroup.org/onlinepubs/9699919799/utilities/bc.html

USE_BC(NEWTOY(bc, "c(code)i(interactive)l(mathlib)q(quiet)s(standard)w(warn)", TOYFLAG_USR|TOYFLAG_BIN|TOYFLAG_LOCALE))

config BC
  bool "bc"
  default n
  help
    usage: bc [-cilqsw] [file ...]

    bc is a command-line calculator with a Turing-complete language.

    options:

      -c  print generated code (for debugging)
      -i  force interactive mode
      -l  use predefined math routines:

          s(expr)  =  sine of expr in radians
          c(expr)  =  cosine of expr in radians
          a(expr)  =  arctangent of expr, returning radians
          l(expr)  =  natural log of expr
          e(expr)  =  raises e to the power of expr
          j(n, x)  =  Bessel function of integer order n of x

      -q  don't print version and copyright
      -s  error if any non-POSIX extensions are used
      -w  warn if any non-POSIX extensions are used

*/

#include <assert.h>
#include <stdbool.h>

#define FOR_bc
#include "toys.h"

GLOBALS(
  long bc_interactive;

  long bc_sig;
)

