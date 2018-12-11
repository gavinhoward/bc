/* vi: set sw=4 ts=4: */
/*
 * Licensed under GPLv2 or later, see file LICENSE in this source tree.
 * Copyright (c) 2018 Gavin D. Howard and contributors.
 *
 * ** Automatically generated from https://github.com/gavinhoward/bc **
 * **        Do not edit unless you know what you are doing.         **
 */
//config:config BC
//config:	bool "bc (45 kb; 49 kb when combined with dc)"
//config:	default y
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
//config:	bool "dc (38 kb; 49 kb when combined with bc)"
//config:	default y
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
//config:config FEATURE_DC_SMALL
//config:	bool "Minimal dc implementation (4.2 kb), not using bc code base"
//config:	depends on DC && !BC
//config:	default y
//config:
//config:config FEATURE_DC_LIBM
//config:	bool "Enable power and exp functions (requires libm)"
//config:	default y
//config:	depends on FEATURE_DC_SMALL
//config:	help
//config:	Enable power and exp functions.
//config:	NOTE: This will require libm to be present for linking.
//config:
//config:config FEATURE_BC_SIGNALS
//config:	bool "Enable bc/dc signal handling"
//config:	default y
//config:	depends on (BC || DC) && !FEATURE_DC_SMALL
//config:	help
//config:	Enable signal handling for bc and dc.
//config:
//config:config FEATURE_BC_LONG_OPTIONS
//config:	bool "Enable bc/dc long options"
//config:	default y
//config:	depends on (BC || DC) && !FEATURE_DC_SMALL
//config:	help
//config:	Enable long options for bc and dc.

//applet:IF_BC(APPLET(bc, BB_DIR_USR_BIN, BB_SUID_DROP))
//applet:IF_DC(APPLET(dc, BB_DIR_USR_BIN, BB_SUID_DROP))

//kbuild:lib-$(CONFIG_BC) += bc.o
//kbuild:lib-$(CONFIG_DC) += bc.o

//See www.gnu.org/software/bc/manual/bc.html
//usage:#define bc_trivial_usage
//usage:       "[-sqliw] FILE..."
//usage:
//usage:#define bc_full_usage "\n"
//usage:     "\nArbitrary precision calculator"
//usage:     "\n"
//usage:     "\n	-i	Interactive"
//usage:     "\n	-q	Quiet"
//usage:     "\n	-l	Load standard math library"
//usage:     "\n	-s	Be POSIX compatible"
//usage:     "\n	-w	Warn if extensions are used"
///////:     "\n	-v	Version"
//usage:     "\n"
//usage:     "\n$BC_LINE_LENGTH changes output width"
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
//usage:       "[-eSCRIPT]... [-fFILE]... [FILE]..."
//usage:
//usage:#define dc_full_usage "\n"
//usage:     "\nTiny RPN calculator. Operations:"
//usage:     "\n+, -, *, /, %, ^, exp, ~, divmod, |, "
//usage:       "modular exponentiation,"
//usage:     "\np - print top of the stack (without popping)"
//usage:     "\nf - print entire stack"
//usage:     "\nk - pop the value and set the precision"
//usage:     "\ni - pop the value and set input radix"
//usage:     "\no - pop the value and set output radix"
//usage:     "\nExamples: dc -e'2 2 + p' -> 4, dc -e'8 8 * 2 2 + / p' -> 16"
//usage:
//usage:#define dc_example_usage
//usage:       "$ dc -e'2 2 + p'\n"
//usage:       "4\n"
//usage:       "$ dc -e'8 8 \\* 2 2 + / p'\n"
//usage:       "16\n"
//usage:       "$ dc -e'0 1 & p'\n"
//usage:       "0\n"
//usage:       "$ dc -e'0 1 | p'\n"
//usage:       "1\n"
//usage:       "$ echo '72 9 / 8 * p' | dc\n"
//usage:       "64\n"

#include "libbb.h"
#include "common_bufsiz.h"

#if ENABLE_FEATURE_DC_SMALL
# include "dc.c"
#else
