CC = clang
AR = ar
CFLAGS= -Wall -pedantic -Wshadow -Wpointer-arith -Wcast-align -static\
	-Wwrite-strings -Wmissing-prototypes -Wmissing-declarations\
	-Wnested-externs -Winline -Wno-long-long  -Wunused-variable\
	-Wstrict-prototypes -Werror -ansi -D_XOPEN_SOURCE=600\
	-I. -fPIC -g -std=c99 -Qunused-arguments -D_BSD_SOURCE
OFILES = smq.o
LIB = libsmq
TESTBINS = smq_test

all:V: $LIB.a man

$LIB.a:Q: $OFILES
	echo "AR $target"
	$AR -crs $target $prereq

clean:V:
	rm -f $TESTBINS *.out $LIB.a *.o

nuke:V:clean
	rm -f $OFILES *.out *.out *.plist

test:V: $TESTBINS

smq_test:Q:$LIB.a smq_test.o
	echo "LD $target"
	$CC $CFLAGS -o $target $target.o -lpthread -L. -lsmq

audit: 
	rats -i -w3 *.[ch]
	flawfinder *.[ch]
	make $OFILES CC="clang" CFLAGS="$CFLAGS --analyze"

man:V:
	mandoc -Tlint $LIB.3

%.o:Q:%.c
	echo "CC $stem.c"
	$CC $CFLAGS -c -static $stem.c

