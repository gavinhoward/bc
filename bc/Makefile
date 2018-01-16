GIT_VERSION := $(shell git describe --abbrev=10 --dirty --always --tags)

CFLAGS += -Wall -Wextra -I./include/ -I../arbprec/include -std=c99 -fsanitize=address -fsanitize=undefined -g -O0 -DGIT_VERSION=\"$(GIT_VERSION)\"

BC_OBJ = $(shell for i in src/*.c ; do printf "%s\n" $${i%.c}.o ; done )

BC_EXEC = bc

all:
	$(MAKE) $(BC_EXEC)

$(BC_EXEC): $(BC_OBJ) ../arbprec/libarbprec.a

clean:

	$(RM) $(BC_OBJ)
	$(RM) $(BC_EXEC)

