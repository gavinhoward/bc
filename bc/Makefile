CFLAGS += -Wall -Wextra -I./include/ -I../arbprec/include -std=c99 -fsanitize=address -fsanitize=undefined -g -O0

BC_OBJ = $(shell for i in *.c ; do printf "%s\n" $${i%.c}.o ; done )

BC_EXEC = bc

all:

	$(MAKE) create_bc

create_bc: $(BC_OBJ)

	$(CC) $(CFLAGS) -o $(BC_EXEC) *.o ../arbprec/libarbprec.a

clean:

	$(RM) $(BC_OBJ)
	$(RM) $(BC_EXEC)
