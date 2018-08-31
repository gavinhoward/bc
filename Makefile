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

BC_SRC = $(sort $(wildcard src/*.c))
BC_OBJ = $(BC_SRC:.c=.o)

GEN = gen

BC_LIB = lib/lib.bc

BC_LIB_C = lib/lib.c
BC_LIB_O = lib/lib.o

BC_EXEC = bc

PREFIX ?= /usr/local

INSTALL = ./install.sh

-include config.mak

CFLAGS += -Wall -Wextra -pedantic -std=c99 -funsigned-char
CPPFLAGS += -I./include/ -D_POSIX_C_SOURCE=200809L

LDLIBS += -lm

HOSTCC ?= $(CC)

all: $(BC_EXEC)

help:
	@echo "available targets:"
	@echo ""
	@echo "    all        build bc (uses config.mak if there is one)"
	@echo "    clean      remove all build files"
	@echo "    install    install to $(PREFIX)/bin"
	@echo "    uninstall  uninstall from $(PREFIX)/bin"
	@echo "    test       runs the test suite"
	@echo "    timeconst  runs the test on the Linux timeconst.bc script"
	@echo "               the timeconst.bc script must be in the parent directory"
	@echo ""
	@echo "useful environment variables:"
	@echo ""
	@echo "    CC        C compiler"
	@echo "    HOSTCC    Host C compiler"
	@echo "    CFLAGS    C compiler flags"
	@echo "    CPPFLAGS  C preprocessor flags"
	@echo "    LDLIBS    Libraries to link to"
	@echo "    PREFIX    the prefix to install to"
	@echo "              if PREFIX is \"/usr\", $(BC_EXEC) will be installed to \"/usr/bin\""
	@echo "    GEN_EMU   Emulator to run $(GEN) under (leave empty if not necessary)"

$(GEN):
	$(HOSTCC) -o $(GEN) lib/$(GEN).c

$(BC_LIB_C): $(GEN)
	$(GEN_EMU) ./$(GEN) $(BC_LIB) $(BC_LIB_C)

$(BC_EXEC): $(BC_OBJ) $(BC_LIB_O)
	$(CC) $(CFLAGS) -o $(BC_EXEC) $(BC_OBJ) $(BC_LIB_O) $(LDLIBS) $(LDFLAGS)

test:
	tests/all.sh

timeconst:
	tests/timeconst.sh

clean:
	$(RM) $(BC_OBJ)
	$(RM) $(BC_EXEC)
	$(RM) $(GEN)
	$(RM) $(BC_LIB_C)
	$(RM) $(BC_LIB_O)
	$(RM) tests/parse.txt tests/parse_results.txt
	$(RM) tests/print.txt tests/print_results.txt
	$(RM) log_test.txt log_bc.txt

install: $(BC_EXEC)
	$(INSTALL) -Dm 755 $(BC_EXEC) $(DESTDIR)$(PREFIX)/bin/$(BC_EXEC)

uninstall:
	rm -rf $(DESTDIR)$(PREFIX)/bin/$(BC_EXEC)

.PHONY: all help clean install uninstall test
