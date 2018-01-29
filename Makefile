CFLAGS += -Wall -Wextra -I./include/ -I../arbprec/include -std=c99 -D_POSIX_C_SOURCE -g -O0

BC_VERSION = 0.1

BC_MATHLIB_PATH = ""

CFLAGS += -DVERSION='"$(BC_VERSION)"' -DBC_MATHLIB_PATH='"$(BC_MATHLIB_PATH)"'

ifeq "$(CC)" "clang"
	CFLAGS += -fsanitize=address -fsanitize=undefined
endif

BC_OBJ = $(shell for i in src/bc/*.c ; do printf "%s\n" $${i%.c}.o ; done )

BC_EXEC = bc

all:
	$(MAKE) $(BC_EXEC)

$(BC_EXEC): $(BC_OBJ)

	$(CC) $(CFLAGS) -o $(BC_EXEC) ./src/*.c $(BC_OBJ) -larbprec

clean:

	$(RM) $(BC_OBJ)
	$(RM) $(BC_EXEC)

