GIT_VERSION := $(shell git describe --abbrev=10 --dirty --always --tags)

CFLAGS += -fPIC -Wall -Wextra -I./include/ -I../arbprec/include -std=c99 -g -O0 -DGIT_VERSION=\"$(GIT_VERSION)\"

ifeq "$(CC)" "clang"
	CFLAGS += -fsanitize=address -fsanitize=undefined
endif

BC_OBJ = $(shell for i in ./*.c ; do printf "%s\n" $${i%.c}.o ; done )
BC_OBJ += $(shell for i in src/*.c ; do printf "%s\n" $${i%.c}.o ; done )

BC_EXEC = bc

all:

	$(MAKE) create_bc

create_bc: $(BC_OBJ)

	$(CC) $(CFLAGS) -o $(BC_EXEC) ./*.c src/*.c ../arbprec/libarbprec.a

clean:

	$(RM) $(BC_OBJ)
	$(RM) $(BC_EXEC)
