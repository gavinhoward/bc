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

BC_ENABLED = BC_ENABLED
DC_ENABLED = DC_ENABLED

VERSION = 1.0

GEN_DIR = gen
GEN = strgen
GEN_EXEC = $(GEN_DIR)/$(GEN)
GEN_C = $(GEN_DIR)/$(GEN).c

BC_LIB = $(GEN_DIR)/lib.bc
BC_LIB_C = $(GEN_DIR)/lib.c
BC_LIB_O = $(GEN_DIR)/lib.o

BC_HELP = $(GEN_DIR)/bc_help.txt
BC_HELP_C = $(GEN_DIR)/bc_help.c
BC_HELP_O = $(GEN_DIR)/bc_help.o

DC_HELP = $(GEN_DIR)/dc_help.txt
DC_HELP_C = $(GEN_DIR)/dc_help.c
DC_HELP_O = $(GEN_DIR)/dc_help.o

BIN = bin

BC = bc
DC = dc
BC_EXEC = $(BIN)/$(BC)
DC_EXEC = $(BIN)/$(DC)

PREFIX ?= /usr/local

INSTALL = ./install.sh
LINK = ./link.sh
KARATSUBA = ./karatsuba.py

VALGRIND_ARGS = --error-exitcode=1 --leak-check=full --show-leak-kinds=all

-include config.mak

BC_NUM_KARATSUBA_LEN ?= 32

CFLAGS += -Wall -Wextra -pedantic -std=c99 -funsigned-char
CPPFLAGS += -I./include/ -D_POSIX_C_SOURCE=200809L -DVERSION=$(VERSION)
CPPFLAGS += -DBC_NUM_KARATSUBA_LEN=$(BC_NUM_KARATSUBA_LEN)

HOSTCC ?= $(CC)

all: CPPFLAGS += -D$(DC_ENABLED) -D$(BC_ENABLED)
all: make_bin clean_exe $(DC_HELP_O) $(BC_HELP_O) $(BC_LIB_O) $(BC_OBJ) $(DC_OBJ) $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) $(DC_OBJ) $(BC_OBJ) $(BC_LIB_O) $(BC_HELP_O) $(DC_HELP_O) -o $(BC_EXEC)
	$(LINK) $(BIN) $(DC)

$(GEN_EXEC):
	$(HOSTCC) $(CFLAGS) -o $(GEN_EXEC) $(GEN_C)

$(BC_LIB_C): $(GEN_EXEC) $(BC_LIB)
	$(GEN_EMU) $(GEN_EXEC) $(BC_LIB) $(BC_LIB_C) bc_lib bc_lib_name $(BC_ENABLED)

$(BC_HELP_C): $(GEN_EXEC) $(BC_HELP)
	$(GEN_EMU) $(GEN_EXEC) $(BC_HELP) $(BC_HELP_C) bc_help "" $(BC_ENABLED)

$(DC_HELP_C): $(GEN_EXEC) $(DC_HELP)
	$(GEN_EMU) $(GEN_EXEC) $(DC_HELP) $(DC_HELP_C) dc_help "" $(DC_ENABLED)

$(DC): CPPFLAGS += -D$(DC_ENABLED)
$(DC): make_bin clean_exe $(DC_OBJ) $(DC_HELP_O) $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) $(DC_OBJ) $(DC_HELP_O) -o $(DC_EXEC)

$(BC): CPPFLAGS += -D$(BC_ENABLED)
$(BC): make_bin clean_exe $(BC_OBJ) $(BC_LIB_O) $(BC_HELP_O) $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) $(BC_OBJ) $(BC_LIB_O) $(BC_HELP_O) -o $(BC_EXEC)

make_bin:
	mkdir -p $(BIN)

help:
	@echo "all targets use config.mak, if there is one"
	@echo ""
	@echo "available targets:"
	@echo ""
	@echo "    all (default)  build bc and dc"
	@echo "    bc             build bc only"
	@echo "    clean          remove all build files"
	@echo "    clean_tests    remove all build files as well as generated tests"
	@echo "    dc             build dc only"
	@echo "    install        install to $(PREFIX)/bin"
	@echo "    uninstall      uninstall from $(PREFIX)/bin"
	@echo "    test           runs the test suite"
	@echo "    test_all       runs the test suite as well as the Linux timeconst.bc test"
	@echo "    test_bc        runs the bc test suite"
	@echo "    test_dc        runs the dc test suite"
	@echo "    timeconst      runs the test on the Linux timeconst.bc script"
	@echo "    valgrind       runs the test suite through valgrind"
	@echo "    valgrind_all   runs the test suite, and the Linux timeconst.bc test,"
	@echo "                   through valgrind"
	@echo "    valgrind_bc    runs the bc test suite through valgrind"
	@echo "    valgrind_dc    runs the dc test suite through valgrind"
	@echo ""
	@echo "useful environment variables:"
	@echo ""
	@echo "    CC        C compiler"
	@echo "    HOSTCC    Host C compiler"
	@echo "    CFLAGS    C compiler flags"
	@echo "    CPPFLAGS  C preprocessor flags"
	@echo "    PREFIX    the prefix to install to"
	@echo "              if PREFIX is \"/usr\", $(BC_EXEC) will be installed to \"/usr/bin\""
	@echo "    DESTDIR   For package creation"
	@echo "    GEN_EMU   Emulator to run $(GEN_EXEC) under (leave empty if not necessary)"

test_all: test timeconst

test: test_bc test_dc

test_bc:
	tests/all.sh bc

test_bc_all: test_bc timeconst

test_dc:
	tests/all.sh dc

timeconst:
	tests/bc/timeconst.sh

valgrind_all: valgrind valgrind_timeconst

valgrind: valgrind_bc valgrind_dc

valgrind_bc:
	tests/all.sh bc valgrind $(VALGRIND_ARGS) $(BC_EXEC)

valgrind_bc_all: valgrind_bc valgrind_timeconst

valgrind_dc:
	tests/all.sh dc valgrind $(VALGRIND_ARGS) $(DC_EXEC)

valgrind_timeconst:
	tests/bc/timeconst.sh valgrind $(VALGRIND_ARGS) $(BC_EXEC)

karatsuba:
	$(KARATSUBA)

karatsuba_test:
	$(KARATSUBA) 100 $(BC_EXEC)

coverage: CC = gcc
coverage: CFLAGS = -fprofile-arcs -ftest-coverage -O0 -g
coverage: CPPFLAGS += -D$(DC_ENABLED) -D$(BC_ENABLED)
coverage: all test_all

version:
	@echo "$(VERSION)"

libcname:
	@echo "$(BC_LIB_C)"

clean_exe:
	$(RM) -f $(OBJ)
	$(RM) -f $(BC_OBJ)
	$(RM) -f $(DC_OBJ)
	$(RM) -f $(BC_EXEC)
	$(RM) -f $(DC_EXEC)

clean: clean_exe
	$(RM) -f $(GEN_EXEC)
	$(RM) -f $(BC_LIB_C)
	$(RM) -f $(BC_LIB_O)
	$(RM) -f $(BC_HELP_C)
	$(RM) -f $(BC_HELP_O)

clean_tests: clean
	$(RM) -f tests/bc/parse.txt tests/bc/parse_results.txt
	$(RM) -f tests/bc/print.txt tests/bc/print_results.txt
	$(RM) -f tests/bc/bessel.txt tests/bc/bessel_results.txt
	$(RM) -f .log_test.txt .log_bc.txt
	$(RM) -f .math.txt .results.txt .ops.txt
	$(RM) -f .test.txt

install:
	$(INSTALL) $(DESTDIR)$(PREFIX)/$(BIN) $(BIN)

uninstall:
	$(RM) -f $(DESTDIR)$(PREFIX)/$(BC_EXEC)
	$(RM) -f $(DESTDIR)$(PREFIX)/$(DC_EXEC)

.PHONY: help clean clean_tests install uninstall test
