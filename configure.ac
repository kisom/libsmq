AC_PREREQ([2.59])
AC_INIT([libsmq],
        [2.3], 
        [coder@kyleisom.net],
        [libsmq],
        [https://bitbucket.org/kisom/libsmq])
AM_INIT_AUTOMAKE([1.11 foreign])
AC_CONFIG_SRCDIR([src/smq.h])
AC_CHECK_HEADERS
AC_CANONICAL_HOST
AC_CONFIG_FILES([Makefile src/Makefile doc/Makefile test/Makefile])

AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_RANLIB
AM_PROG_CC_C_O

DARWIN_UNSUPP="
    ==============================================
    Darwin is *not* supported as a target platform
    (due to its lack of support for POSIX unnamed
    semaphores.)
    ==============================================
"

AS_IF([test "$host_os%[0-9]*" = "darwin"],
      [AC_MSG_ERROR($DARWIN_UNSUPP)], [])

NO_CUNIT_MSG="
    ==============================================
    Warning: CUnit was not found; will not be able
    to run unit tests!
    ==============================================
"
AC_SEARCH_LIBS([CU_initialize_registry], [cunit],
               [], [AC_MSG_WARN($NO_CUNIT_MSG)])


AC_OUTPUT
