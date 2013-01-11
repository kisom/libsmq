AC_PREREQ([2.59])
AC_INIT([libsmq],
        [2.1], 
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

NO_CUNIT_MSG="
    ==============================================
    Warning: CUnit was not found; will not be able
    to run unit tests!
    ==============================================
"
AC_SEARCH_LIBS([CU_initialize_registry], [cunit],
               [], [AC_MSG_WARN($NO_CUNIT_MSG)])


BAD_DOC_WARNING="
    ==============================================
    Warning: the documentation is currently very
    much out of date. An update is in progress,
    but DO NOT TRUST THE DOCS.
    ==============================================
"
AC_MSG_WARN($BAD_DOC_WARNING)

AC_OUTPUT
