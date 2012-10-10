CC = clang
CFLAGS= -Wall -pedantic -Wshadow -Wpointer-arith -Wcast-align -static\
        -Wwrite-strings -Wmissing-prototypes -Wmissing-declarations\
        -Wnested-externs -Winline -Wno-long-long  -Wunused-variable\
        -Wstrict-prototypes -Werror -ansi -D_XOPEN_SOURCE=600\
        -I. -fPIC -g -std=c99 -Qunused-arguments -D_BSD_SOURCE
OFILES = smq.o
LIB = libsmq
TESTBINS = smq_test

all: $LIB.a

$LIB.a: $OFILES
	$AR -crs $target $prereq

clean: 
	rm -f *.core *.plist $(OBJS) $(TESTBINS) *.out $(LIB).a *.o

nuke:

test: $TESTBINS

smq_test: $OFILES $@.o $LIB.a
	$CC $CFLAGS -o $@ $@.o $OFILES -lpthread -L. -lsmq

audit: 
	rats -i -w3 *.[ch]
	flawfinder *.[ch]
	make $OFILES CC="clang" CFLAGS="$CFLAGS --analyze"

%.o: %.c
	@echo "CC $@"
        $(CC) $(CFLAGS) -c -static $<

