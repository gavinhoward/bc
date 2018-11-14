/* bc.c - An implementation of POSIX bc.
 *
 * Copyright 2018 Gavin D. Howard <yzena.tech@gmail.com>
 *
 * See http://pubs.opengroup.org/onlinepubs/9699919799/utilities/bc.html

USE_BC(NEWTOY(bc, "i(interactive)l(mathlib)q(quiet)s(standard)w(warn)", TOYFLAG_USR|TOYFLAG_BIN|TOYFLAG_LOCALE))

config BC
  bool "bc"
  default n
  help
    usage: bc [-ilqsw] [file ...]

    bc is a command-line calculator with a Turing-complete language.

    options:

      -i  --interactive  force interactive mode
      -l  --mathlib      use predefined math routines:

                         s(expr)  =  sine of expr in radians
                         c(expr)  =  cosine of expr in radians
                         a(expr)  =  arctangent of expr, returning radians
                         l(expr)  =  natural log of expr
                         e(expr)  =  raises e to the power of expr
                         j(n, x)  =  Bessel function of integer order n of x

      -q  --quiet        don't print version and copyright
      -s  --standard     error if any non-POSIX extensions are used
      -w  --warn         warn if any non-POSIX extensions are used

*/

#define FOR_bc
#include "toys.h"

GLOBALS(
  long i;

  long ttyin;
  unsigned long sig;
  unsigned long sigc;
  unsigned long signe;
  long sig_other;
)

