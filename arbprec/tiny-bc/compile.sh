#! /bin/sh

rm bc

clang -Wall -Wextra -g -O0 -o bc bc.c vm.c parse.c segarray.c program.c stack.c lex.c
