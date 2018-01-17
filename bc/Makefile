CFLAGS += -Wall -Wextra -I./include/ -I../arbprec/include -std=c99 -g -O0

ifeq "$(CC)" "clang"
	CFLAGS += -fsanitize=address -fsanitize=undefined
endif

BC_OBJ = $(shell for i in src/*.c ; do printf "%s\n" $${i%.c}.o ; done )

BC_EXEC = bc

all:
	$(MAKE) $(BC_EXEC)

$(BC_EXEC): $(BC_OBJ) ../arbprec/libarbprec.a

	$(CC) $(CFLAGS) -o $(BC_EXEC) ./*.c src/*.c ../arbprec/libarbprec.a

clean:

	$(RM) $(BC_OBJ)
	$(RM) $(BC_EXEC)

