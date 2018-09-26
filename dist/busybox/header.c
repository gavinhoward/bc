/* vi: set sw=4 ts=4: */
/*
 * Licensed under GPLv2 or later, see file LICENSE in this source tree.
 * Copyright (c) 2018 Gavin D. Howard and contributors.
 * Automatically generated from https://github.com/gavinhoward/bc
 */
//config:config BC
//config:	bool "bc (4.2 kb)"
//config:	default n
//config:	help
//config:	bc is a command-line, arbitrary-precision calculator with a Turing-complete
//config:	language. See the GNU bc manual (https://www.gnu.org/software/bc/manual/bc.html)
//config:	and bc spec (http://pubs.opengroup.org/onlinepubs/9699919799/utilities/bc.html)
//config:	for details.
//config:
//config:	This bc has four differences to the GNU bc:
//config:
//config:	  1) The period (.) can also be used as a shortcut for "last", as in the BSD bc.
//config:	  2) Arrays are copied before being passed as arguments to functions. This
//config:	     behavior is required by the bc spec.
//config:	  3) Arrays can be passed to the builtin "length" function to get the number of
//config:	     elements currently in the array. The following example prints "1":
//config:
//config:	       a[0] = 0
//config:	       length(a[])
//config:
//config:	  4) The precedence of the boolean "not" operator (!) is equal to that of the
//config:	     unary minus (-), or negation, operator. This still allows POSIX-compliant
//config:	     scripts to work while somewhat preserving expected behavior (versus C) and
//config:	     making parsing easier.
//config:
//config:	Options:
//config:
//config:	  -h  --help         print this usage message and exit
//config:	  -i  --interactive  force interactive mode
//config:	  -l  --mathlib      use predefined math routines:
//config:
//config:	                       s(expr)  =  sine of expr in radians
//config:	                       c(expr)  =  cosine of expr in radians
//config:	                       a(expr)  =  arctangent of expr, returning radians
//config:	                       l(expr)  =  natural log of expr
//config:	                       e(expr)  =  raises e to the power of expr
//config:	                       j(n, x)  =  Bessel function of integer order n of x
//config:
//config:	  -q  --quiet        don't print version and copyright
//config:	  -s  --standard     error if any non-POSIX extensions are used
//config:	  -w  --warn         warn if any non-POSIX extensions are used
//config:	  -v  --version      print version information and copyright and exit
//config:
//config:config DC
//config:	bool "dc (4.2 kb)"
//config:	default y
//config:	help
//config:	dc is a reverse-polish desk calculator which supports unlimited
//config:	precision arithmetic.

//applet:IF_BC(APPLET(bc, BB_DIR_USR_BIN, BB_SUID_DROP))
//applet:IF_DC(APPLET(dc, BB_DIR_USR_BIN, BB_SUID_DROP))

//kbuild:lib-$(CONFIG_BC) += bc.o

//usage:#define dc_trivial_usage
//usage:       "EXPRESSION..."
//usage:
//usage:#define dc_full_usage "\n\n"
//usage:       "Tiny RPN calculator. Operations:\n"
//usage:       "+, add, -, sub, *, mul, /, div, %, mod, "IF_FEATURE_DC_LIBM("**, exp, ")"and, or, not, xor,\n"
//usage:       "p - print top of the stack (without popping),\n"
//usage:       "f - print entire stack,\n"
//usage:       "o - pop the value and set output radix (must be 10, 16, 8 or 2).\n"
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
