#
# Copyright 2018 Gavin D. Howard
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
# REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
# INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
# LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
# OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
# PERFORMANCE OF THIS SOFTWARE.
#

CFLAGS ?= -Wall -Wextra -pedantic -I./include/ -std=c99 -D_POSIX_C_SOURCE -g -O0

ifeq "$(CC)" "clang"
	CFLAGS += -fsanitize=address -fsanitize=undefined
endif

LDLIBS += -lm

BC_OBJ = $(shell for i in src/bc/*.c ; do printf "%s\n" $${i%.c}.o ; done)

BC_MAIN_OBJ = $(shell for i in src/*.c ; do printf "%s\n" $${i%.c}.o ; done)

GEN_LIB = gen

GEN = gen

BC_LIB = src/lib/lib.bc

BC_C_LIB = src/bc/lib.c
BC_LIB_O = src/bc/lib.o

BC_EXEC = bc

all:
	$(MAKE) $(BC_EXEC)

$(GEN):
	$(CC) $(CFLAGS) -o $(GEN_LIB) src/lib/$(GEN_LIB).c

gen_run: $(GEN)
	./$(GEN) $(BC_LIB) $(BC_C_LIB)
	$(CC) $(CFLAGS) -c $(BC_C_LIB) -o $(BC_LIB_O)

$(BC_OBJ): gen_run

$(BC_EXEC): $(BC_OBJ) $(BC_MAIN_OBJ)
	$(CC) $(CFLAGS) -o $(BC_EXEC) $(BC_MAIN_OBJ) $(BC_OBJ) $(LDLIBS)

clean:
	$(RM) $(BC_OBJ)
	$(RM) $(BC_EXEC)
	$(RM) $(GEN)
	$(RM) $(BC_C_LIB)

