/* vi: set sw=4 ts=4: */
/*
 * Licensed under GPLv2 or later, see file LICENSE in this source tree.
 * Copyright (c) 2018 Gavin D. Howard and contributors.
 *
 * ** Automatically generated from https://github.com/gavinhoward/bc **
 * **        Do not edit unless you know what you are doing.         **
 */
//config:config BC
//config:	bool "bc (45.71 kb; 49.82 kb when combined with dc)"
//config:	default n
//config:	help
//config:	bc is a command-line, arbitrary-precision calculator with a
//config:	Turing-complete language. See the GNU bc manual
//config:	(https://www.gnu.org/software/bc/manual/bc.html) and bc spec
//config:	(http://pubs.opengroup.org/onlinepubs/9699919799/utilities/bc.html)
//config:	for details.
//config:
//config:	This bc has four differences to the GNU bc:
//config:
//config:	  1) The period (.) can also be used as a shortcut for "last", as in
//config:	     the BSD bc.
//config:	  2) Arrays are copied before being passed as arguments to
//config:	     functions. This behavior is required by the bc spec.
//config:	  3) Arrays can be passed to the builtin "length" function to get
//config:	     the number of elements currently in the array. The following
//config:	     example prints "1":
//config:
//config:	       a[0] = 0
//config:	       length(a[])
//config:
//config:	  4) The precedence of the boolean "not" operator (!) is equal to
//config:	     that of the unary minus (-), or negation, operator. This still
//config:	     allows POSIX-compliant scripts to work while somewhat
//config:	     preserving expected behavior (versus C) and making parsing
//config:	     easier.
//config:
//config:	Options:
//config:
//config:	  -i  --interactive  force interactive mode
//config:	  -l  --mathlib      use predefined math routines:
//config:
//config:	                       s(expr)  =  sine of expr in radians
//config:	                       c(expr)  =  cosine of expr in radians
//config:	                       a(expr)  =  arctangent of expr, returning
//config:	                                   radians
//config:	                       l(expr)  =  natural log of expr
//config:	                       e(expr)  =  raises e to the power of expr
//config:	                       j(n, x)  =  Bessel function of integer order
//config:	                                   n of x
//config:
//config:	  -q  --quiet        don't print version and copyright.
//config:	  -s  --standard     error if any non-POSIX extensions are used.
//config:	  -w  --warn         warn if any non-POSIX extensions are used.
//config:	  -v  --version      print version and copyright and exit.
//config:
//config:	Long options are only available if FEATURE_BC_LONG_OPTIONS is
//config:	enabled.
//config:
//config:config DC
//config:	bool "dc (38.27 kb; 49.82 kb when combined with bc)"
//config:	default n
//config:	help
//config:	dc is a reverse-polish notation command-line calculator which
//config:	supports unlimited precision arithmetic. See the FreeBSD man page
//config:	(https://www.unix.com/man-page/FreeBSD/1/dc/) and GNU dc manual
//config:	(https://www.gnu.org/software/bc/manual/dc-1.05/html_mono/dc.html)
//config:	for details.
//config:
//config:	This dc has a few differences from the two above:
//config:
//config:	  1) When printing a byte stream (command "P"), this bc follows what
//config:	     the FreeBSD dc does.
//config:	  2) This dc implements the GNU extensions for divmod ("~") and
//config:	     modular exponentiation ("|").
//config:	  3) This dc implements all FreeBSD extensions, except for "J" and
//config:	     "M".
//config:	  4) Like the FreeBSD dc, this dc supports extended registers.
//config:	     However, they are implemented differently. When it encounters
//config:	     whitespace where a register should be, it skips the whitespace.
//config:	     If the character following is not a lowercase letter, an error
//config:	     is issued. Otherwise, the register name is parsed by the
//config:	     following regex:
//config:
//config:	       [a-z][a-z0-9_]*
//config:
//config:	     This generally means that register names will be surrounded by
//config:	     whitespace.
//config:
//config:	     Examples:
//config:
//config:	       l idx s temp L index S temp2 < do_thing
//config:
//config:	     Also note that, like the FreeBSD dc, extended registers are not
//config:	     allowed unless the "-x" option is given.
//config:
//config:	Options:
//config:
//config:	  -v  --version            print version and copyright and exit.
//config:	  -x  --extended-register  enable extended register mode.
//config:
//config:	Long options are only available if FEATURE_BC_LONG_OPTIONS is
//config:	enabled.
//config:
//config:config FEATURE_BC_SIGNALS
//config:	bool "Enable bc/dc signal handling"
//config:	default n
//config:	help
//config:	Enable signal handling for bc and dc.
//config:
//config:config FEATURE_BC_LONG_OPTIONS
//config:	bool "Enable bc/dc long options"
//config:	default n
//config:	help
//config:	Enable long options for bc and dc.

//applet:IF_BC(APPLET(bc, BB_DIR_USR_BIN, BB_SUID_DROP))
//applet:IF_DC(APPLET(dc, BB_DIR_USR_BIN, BB_SUID_DROP))

//kbuild:lib-$(CONFIG_BC) += bc.o
//kbuild:lib-$(CONFIG_DC) += bc.o

//usage:#define bc_trivial_usage
//usage:       "EXPRESSION...\n"
//usage:       "function_definition\n"
//usage:
//usage:#define bc_full_usage "\n\n"
//usage:       "See www.gnu.org/software/bc/manual/bc.html\n"
//usage:
//usage:#define bc_example_usage
//usage:       "3 + 4.129\n"
//usage:       "1903 - 2893\n"
//usage:       "-129 * 213.28935\n"
//usage:       "12 / -1932\n"
//usage:       "12 % 12\n"
//usage:       "34 ^ 189\n"
//usage:       "scale = 13\n"
//usage:       "ibase = 2\n"
//usage:       "obase = A\n"
//usage:
//usage:#define dc_trivial_usage
//usage:       "EXPRESSION..."
//usage:
//usage:#define dc_full_usage "\n\n"
//usage:       "Tiny RPN calculator. Operations:\n"
//usage:       "+, add, -, sub, *, mul, /, div, %, mod, ^, exp, ~, divmod, |, "
//usage:       "modular exponentiation,\n"
//usage:       "p - print top of the stack (without popping),\n"
//usage:       "f - print entire stack,\n"
//usage:       "k - pop the value and set the precision.\n"
//usage:       "i - pop the value and set input radix.\n"
//usage:       "o - pop the value and set output radix.\n"
//usage:       "Examples: 'dc 2 2 add p' -> 4, 'dc 8 8 mul 2 2 + / p' -> 16"
//usage:
//usage:#define dc_example_usage
//usage:       "$ dc 2 2 + p\n"
//usage:       "4\n"
//usage:       "$ dc 8 8 \\* 2 2 + / p\n"
//usage:       "16\n"
//usage:       "$ dc 0 1 and p\n"
//usage:       "0\n"
//usage:       "$ dc 0 1 or p\n"
//usage:       "1\n"
//usage:       "$ echo 72 9 div 8 mul p | dc\n"
//usage:       "64\n"

#include "libbb.h"
