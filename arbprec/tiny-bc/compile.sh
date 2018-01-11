#! /bin/sh

rm bc 2> /dev/null

clang -fsanitize=address -fsanitize=undefined -I../include -Wall -Wextra -g -O0 -o bc bc.c vm.c parse.c segarray.c program.c stack.c lex.c error.c ../libarbprec.a
