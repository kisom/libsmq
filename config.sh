#!/bin/sh

TARGET="$(cat Makefile.in | grep 'TARGET :=' | awk -F' ' '{ print $3; }')"
echo "configuring ${TARGET}"

which sed 2>/dev/null 1>/dev/null
if [ $? -ne 0 ]; then
    echo "cannot find sed!" 1>&2
fi

OPSYS=$(uname -s)

echo "Configuring for ${OPSYS}..."
if [ "x${OPSYS}" = "xLinux" ]; then
    OS_CFLAGS="-D_BSD_SOURCE -D_POSIX_SOURCE -D_XOPEN_SOURCE"
else
    OS_CFLAGS=""
fi

if [ -z "${OS_CFLAGS}" ]; then
    echo "${OPSYS} requires no extra build flags."
else
    echo "${OPSYS} requires build flags ${OS_CFLAGS}"
fi

if [ -z "${PREFIX}" ]; then
    PREFIX="/usr/local"
fi    

if [ "${PREFIX}" = "/usr" ]; then
    MANDIR="$(PREFIX)/share/man"
elif [ "${PREFIX}" = "/usr/local" ]; then
    if [ "${OPSYS}" = "Darwin" ]; then
        MANDIR="${PREFIX}/share/man"
    else
        MANDIR="${PREFIX}/man"
    fi
else
    MANDIR="${PREFIX}/man"
fi

echo "prefix: ${PREFIX}"
echo "mandir: ${MANDIR}"

echo "writing new Makefile"
cat Makefile.in | sed -e "s|OS_CFLAGS|${OS_CFLAGS}|" |          \
                  sed -e "s|\$PREFIX|${PREFIX}|" |              \
                  sed -e "s|\$MANDIR|${MANDIR}|"        > Makefile


echo "done."
