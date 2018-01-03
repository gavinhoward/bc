.PHONY: uninstall

PREFIX ?= /bin/

DESTDIR ?= ../

CFLAGS += -Wall -Wextra -I../include

LDFLAGS += -L../lib 

LDLIBS += -lreadline

OBJ = $(shell for i in *.c; do printf "%s\n" $${i%%.c} ; done)

OBJ_TARG =  $(shell for i in *.c; do printf "%s\n" $(DESTDIR)/$(PREFIX)/$${i%%.c} ; done)

all: $(OBJ)

install:

	-@mkdir -p $(DESTDIR)/$(PREFIX)/
	-@cp -a $(OBJ) $(DESTDIR)/$(PREFIX)/
	-printf "%s\n" "rm $(OBJ_TARG)" > uninstall

uninstall:

	-chmod +x uninstall
	-./uninstall

clean:

	$(RM) $(OBJ)
	$(RM) uninstall

