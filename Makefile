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

CFLAGS += -Wall -Wextra -pedantic -I./include/ -std=c99 -funsigned-char -D_POSIX_C_SOURCE=200809L

LDLIBS += -lm

BC_OBJ = $(shell for i in src/*.c ; do printf "%s\n" $${i%.c}.o ; done)

GEN = gen

BC_LIB = lib/lib.bc

BC_C_LIB = src/lib.c
BC_LIB_O = src/lib.o

BC_EXEC = bc

PREFIX ?= /usr/local

all: CFLAGS += -g -O0
all: $(BC_EXEC)

help:
	@echo "available targets:"
	@echo ""
	@echo "    all         build a debug executable"
	@echo "    release     build a release executable"
	@echo "    minrelease  build a min size executable"
	@echo "    reldebug    build a release executable with debug info"
	@echo "    mathlib     create the library C source file"
	@echo "    clean       remove all build files"
	@echo "    install     install to $(PREFIX)/bin"
	@echo "    replace     move the currently installed $(BC_EXEC) and install to $(PREFIX)/bin"
	@echo "    uninstall   uninstall from $(PREFIX)/bin"
	@echo ""
	@echo "useful environment variables:"
	@echo ""
	@echo "CC      C compiler"
	@echo "CFLAGS  C compiler flags"
	@echo "PREFIX  the prefix to install to"
	@echo "        if PREFIX is \"/usr\", $(BC_EXEC) will be installed to \"/usr/bin\""

release: CFLAGS += -O3 -DNDEBUG
release: clean $(BC_EXEC)

minrelease: CFLAGS += -Os -DNDEBUG
minrelease: clean $(BC_EXEC)

reldebug: CLAGS += -O1 -g
reldebug: clean $(BC_EXEC)

$(GEN):
	$(CC) $(CFLAGS) -o $(GEN) lib/$(GEN).c

mathlib: $(GEN)
	./$(GEN) $(BC_LIB) $(BC_C_LIB)
	$(CC) $(CFLAGS) -c $(BC_C_LIB) -o $(BC_LIB_O)

$(BC_OBJ): mathlib

$(BC_EXEC): $(BC_OBJ)
	$(CC) $(CFLAGS) -o $(BC_EXEC) $(BC_OBJ) $(LDLIBS)

clean:
	$(RM) $(BC_OBJ)
	$(RM) $(BC_EXEC)
	$(RM) $(GEN)
	$(RM) $(BC_C_LIB)

install:
	cp $(BC_EXEC) $(PREFIX)/bin

replace:
	cp --backup=t $(BC_EXEC) $(PREFIX)/bin/$(BC_EXEC)

uninstall:
	rm -rf $(PREFIX)/bin/$(BC_EXEC)
