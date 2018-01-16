CFLAGS += -Wall -Wextra -I./include/ -fPIC

OBJ = $(shell for i in src/*.c ; do printf "%s\n" $${i%.c}.o ; done )

LIBNAME = arbprec

STATLIB = lib$(LIBNAME).a

DESTDIR ?= ../

.PHONY: uninstall

PREFIX ?= /arbprec/

INCLUDES = $(shell for i in include/$(PREFIX)/*.h ; do printf "%s\n" $$i ; done )

INCLUDES_TARG = $(shell for i in include/$(PREFIX)/*.h ; do printf "%s\n" $(DESTDIR)/$$i ; done )

TEST_OBJ = $(shell for i in tests/*.c ; do printf "%s\n" $${i%.c} ; done )

BC_OBJ = $(shell for i in tiny-bc/*.c ; do printf "%s\n" $${i%.c}.o ; done )

BC_EXEC = tiny-bc/bc

LDLIBS += -L. -l$(LIBNAME) -lm

all:

	$(MAKE) static
	$(MAKE) create_test
	#$(MAKE) create_bc

static: $(OBJ)

	$(MAKE) static_link

static_link:

	$(AR) -cvq $(STATLIB) $(OBJ)


create_test: $(TEST_OBJ)

create_bc: $(BC_OBJ) static

	cd tiny-bc && sh ./compile.sh

cleanobj:

	$(RM) $(OBJ)

clean:
	$(MAKE) cleanobj
	$(RM) $(STATLIB)
	$(RM) $(TEST_OBJ)
	$(RM) $(BC_OBJ)
	$(RM) $(BC_EXEC)
	$(RM) uninstall
	$(RM) log log2 log1

install:

	mkdir -p $(DESTDIR)/lib/
	mkdir -p $(DESTDIR)/include/$(PREFIX)
	cp $(STATLIB) $(DESTDIR)/lib/
	cp $(INCLUDES) $(DESTDIR)/include/$(PREFIX)
	$(MAKE) create_uninstall

create_uninstall:

	printf "%s\n" "#!/bin/sh" > uninstall
	printf "%s\n" "set -x" >> uninstall
	printf "%s\n" "rm $(INCLUDES_TARG)" >> uninstall
	printf "%s\n" "rm $(DESTDIR)/lib/$(STATLIB)" >> uninstall
	chmod +x uninstall

uninstall:

	chmod +x uninstall
	./uninstall
	rm uninstall
