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
MANPATH = man
TESTBINS = smq_test

all: $(LIB).a

$(LIB).a: $(OBJS)
	$(AR) -crs $@ $?

clean: 
	rm -f *.core *.plist $(OBJS) $(TESTBINS) *.out $(LIB).a *.o

test: $(TESTBINS)

smq_test: $(OBJS) $@.o $(LIB).a
	$(CC) $(CFLAGS) -o $@ $@.o $(OBJS) -lpthread -L. -lsmq

audit: 
	rats -i -w3 *.[ch]
	flawfinder *.[ch]
	make $(OBJS) CC="clang" CFLAGS="$(CFLAGS) --analyze"

%.o: %.c
	@echo "CC $@"
	$(CC) $(CFLAGS) -c -static $<

