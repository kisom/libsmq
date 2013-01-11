AM_CFLAGS = -pthread -Wall -Werror -std=c99 -D_XOPEN_SOURCE=700 -D_BSD_SOURCE \
             -I../src -O0 -g
AM_LDFLAGS = -lpthread

if LINK_LIBRT
echo "[+] will link librt"
AM_LDFLAGS += -lrt
endif

check_PROGRAMS = smq_test
smq_test_SOURCES = smq_test.c ../src/smq.c
