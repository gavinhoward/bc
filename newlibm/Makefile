CFLAGS += -Wall -Wextra -I./include/

LDFLAGS += "-Wl,-rpath=$(PWD)"

OBJ = $(shell for i in src/*.c ; do printf "%s\n" $${i%.c}.o ; done )

LIBNAME = m

STATLIB = lib$(LIBNAME).a

DYNLIB = lib$(LIBNAME).so

DYNCC = CC="$(CC) -fpic"

DESTDIR ?= ../

.PHONY: uninstall

PREFIX ?= /src/

INCLUDES = $(shell for i in include/$(PREFIX)/*.h ; do printf "%s\n" $$i ; done )

INCLUDES_TARG = $(shell for i in include/$(PREFIX)/*.h ; do printf "%s\n" $(DESTDIR)/$$i ; done )

TEST_OBJ = $(shell for i in tests/*.c ; do printf "%s\n" $${i%.c} ; done )

LDLIBS += -L. -l$(LIBNAME)

all:

	$(MAKE) static
	#$(MAKE) cleanobj
	#$(MAKE) dynamic
	$(MAKE) create_test

static: $(OBJ)

	$(MAKE) static_link

dynwrap: $(OBJ)

dynamic: 

	$(MAKE) $(DYNCC) dynwrap
	$(MAKE) dynamic_link

static_link:

	$(AR) -cvq $(STATLIB) $(OBJ)

dynamic_link:

	$(CC) -shared -Wl,-soname,$(DYNLIB) -o $(DYNLIB) $(OBJ)

create_test: $(TEST_OBJ)

cleanobj:

	$(RM) $(OBJ) $(OBJ) 

clean:
	$(MAKE) cleanobj
	$(RM) $(DYNLIB) 
	$(RM) $(STATLIB) 
	$(RM) $(TEST_OBJ)
	$(RM) uninstall

install:

	mkdir -p $(DESTDIR)/lib/
	mkdir -p $(DESTDIR)/include/$(PREFIX)
	cp $(STATLIB) $(DESTDIR)/lib/
	cp $(DYNLIB) $(DESTDIR)/lib/
	cp $(INCLUDES) $(DESTDIR)/include/$(PREFIX)
	$(MAKE) create_uninstall

create_uninstall:

	printf "%s\n" "#!/bin/sh" > uninstall
	printf "%s\n" "set -x" >> uninstall
	printf "%s\n" "rm $(INCLUDES_TARG)" >> uninstall
	printf "%s\n" "rm $(DESTDIR)/lib/$(STATLIB)" >> uninstall
	printf "%s\n" "rm $(DESTDIR)/lib/$(DYNLIB)" >> uninstall
	
uninstall:

	chmod +x uninstall
	./uninstall
	rm uninstall
