#! /bin/sh

rm bc 2> /dev/null

clang -Wall -Wextra -g -O0 -o bc bc.c vm.c parse.c segarray.c program.c stack.c lex.c ../libarbprec.a
