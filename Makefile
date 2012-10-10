CC = gcc
AR = ar
CFLAGS += -Wall -Wextra -pedantic -Wshadow -Wpointer-arith -Wcast-align
CFLAGS += -Wwrite-strings -Wmissing-prototypes -Wmissing-declarations
CFLAGS += -Wnested-externs -Winline -Wno-long-long -O2 -ansi
CFLAGS += -Wstrict-prototypes -Werror -I.
LDFLAGS += -L/usr/local/lib
LIB = libsmq
OBJS = smq.o
PREFIX = /usr/local
MANPREFIX = $(PREFIX)/man
TESTBINS = smq_test

all: $(LIB).a man

$(LIB).a: $(OBJS)
	@echo "AR $@"
	@$(AR) -crs $@ $?

clean: 
	rm -f *.core *.plist $(OBJS) $(TESTBINS) *.out $(LIB).a *.o

test: $(TESTBINS)

smq_test: $(OBJS) $@.o $(LIB).a
	@echo "LD $@"
	@$(CC) $(CFLAGS) -o $@ $@.o $(OBJS) -lpthread -L. -lsmq

install: $(LIB).a man
	install -d $(PREFIX)/lib
	install -d $(MANPREFIX)/man3
	install $(LIB).a $(PREFIX)/lib/$(LIB).a
	install $(LIB).3 $(MANPREFIX)/man3/$(LIB).3

uninstall:
	rm -f $(PREFIX)/lib/$(LIB).a
	rm -f $(MANPREFIX)/man3/$(LIB).3
	
audit: 
	rats -i -w3 *.[ch]
	flawfinder *.[ch]
	make $(OBJS) CC="clang" CFLAGS="$(CFLAGS) --analyze"

man:
	mandoc -Tlint $(LIB).3

.c.o:
	@echo "CC $@"
	@$(CC) $(CFLAGS) -c -static $<

.PHONY: all audit clean install man test uninstall
