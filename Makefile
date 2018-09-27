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

SRC = $(sort $(wildcard src/*.c))
OBJ = $(SRC:.c=.o)

BC_SRC = $(sort $(wildcard src/bc/*.c))
BC_OBJ = $(BC_SRC:.c=.o)

DC_SRC = $(sort $(wildcard src/dc/*.c))
DC_OBJ = $(DC_SRC:.c=.o)

GEN_DIR = gen
GEN_EXEC = strgen

BC_LIB = $(GEN_DIR)/lib.bc
BC_LIB_C = $(GEN_DIR)/lib.c
BC_LIB_O = $(GEN_DIR)/lib.o

BC_HELP = $(GEN_DIR)/bc_help.txt
BC_HELP_C = $(GEN_DIR)/bc_help.c
BC_HELP_O = $(GEN_DIR)/bc_help.o

DC_HELP = $(GEN_DIR)/dc_help.txt
DC_HELP_C = $(GEN_DIR)/dc_help.c
DC_HELP_O = $(GEN_DIR)/dc_help.o

BC_EXEC = bc
DC_EXEC = dc

PREFIX ?= /usr/local

INSTALL = ./install.sh

-include config.mak

CFLAGS += -Wall -Wextra -pedantic -std=c99 -funsigned-char
CPPFLAGS += -I./include/ -D_POSIX_C_SOURCE=200809L

LDLIBS += -lm

HOSTCC ?= $(CC)

all: CPPFLAGS += -DDC_CONFIG -DBC_CONFIG
all: clean $(DC_HELP_O) $(BC_HELP_O) $(BC_LIB_O) $(BC_OBJ) $(DC_OBJ) $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) $(DC_OBJ) $(BC_OBJ) $(BC_LIB_O) $(BC_HELP_O) $(DC_HELP_O) \
		$(LDLIBS) $(LDFLAGS) -o $(BC_EXEC)
	ln -s ./$(BC_EXEC) $(DC_EXEC)

$(GEN_EXEC):
	$(HOSTCC) -o $(GEN_EXEC) $(GEN_DIR)/$(GEN_EXEC).c

$(BC_LIB_C): $(GEN_EXEC)
	$(GEN_EMU) ./$(GEN_EXEC) $(BC_LIB) $(BC_LIB_C) bc_lib bc_lib_name

$(BC_HELP_C): $(GEN_EXEC)
	$(GEN_EMU) ./$(GEN_EXEC) $(BC_HELP) $(BC_HELP_C) bc_help

$(DC_HELP_C): $(GEN_EXEC)
	$(GEN_EMU) ./$(GEN_EXEC) $(DC_HELP) $(DC_HELP_C) dc_help

$(DC_EXEC): CPPFLAGS += -DDC_CONFIG
$(DC_EXEC): clean $(DC_OBJ) $(DC_HELP_O) $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) $(DC_OBJ) $(DC_HELP_O) $(LDLIBS) $(LDFLAGS) -o $(DC_EXEC)

$(BC_EXEC): CPPFLAGS += -DBC_CONFIG
$(BC_EXEC): clean $(BC_OBJ) $(BC_LIB_O) $(BC_HELP_O) $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) $(BC_OBJ) $(BC_LIB_O) $(BC_HELP_O) $(LDLIBS) $(LDFLAGS) -o $(BC_EXEC)

main: clean $(BC_OBJ) $(BC_LIB_O) $(BC_HELP_O)
	$(CC) $(CFLAGS) $(BC_OBJ) $(BC_LIB_O) $(BC_HELP_O) $(LDLIBS) $(LDFLAGS)

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
	@echo "    GEN_EMU   Emulator to run $(GEN_EXEC) under (leave empty if not necessary)"

test:
	tests/all.sh

valgrind:
	tests/all.sh valgrind --leak-check=full --show-leak-kinds=all ./bc

timeconst:
	tests/timeconst.sh

clean:
	$(RM) -f $(OBJ)
	$(RM) -f $(BC_OBJ)
	$(RM) -f $(DC_OBJ)
	$(RM) -f $(BC_EXEC)
	$(RM) -f $(DC_EXEC)
	$(RM) -f $(GEN_EXEC)
	$(RM) -f $(BC_LIB_C)
	$(RM) -f $(BC_LIB_O)
	$(RM) -f $(BC_HELP_C)
	$(RM) -f $(BC_HELP_O)

clean_tests: clean
	$(RM) -f tests/parse.txt tests/parse_results.txt
	$(RM) -f tests/print.txt tests/print_results.txt
	$(RM) -f tests/bessel.txt tests/bessel_results.txt
	$(RM) -f .log_test.txt .log_bc.txt
	$(RM) -f .math.txt .results.txt .ops.txt
	$(RM) -f .test.txt

install:
	$(INSTALL) $(DESTDIR)$(PREFIX)/bin $(BC_EXEC) $(DC_EXEC)

uninstall:
	$(RM) -f $(DESTDIR)$(PREFIX)/bin/$(BC_EXEC)
	$(RM) -f $(DESTDIR)$(PREFIX)/bin/$(DC_EXEC)

.PHONY: all help clean clean_tests install uninstall test
