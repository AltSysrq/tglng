#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.6])
AC_INIT([TGLNG], [0.0.1], [jasonlingle@yahoo.com])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/tglng.cxx])
AC_CONFIG_HEADERS([config.h])
AM_SILENT_RULES(yes)
AM_MAINTAINER_MODE

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h unistd.h glob.h regex.h pcre.h])
AC_SEARCH_LIBS([pcre_compile], [pcre])
AC_CHECK_FUNCS([pcre16_compile], [
  AC_DEFINE([PCRE_SUPPORTS_16_BIT], [1],
            [Indicates that PCRE supports 16-bit strings])])

AC_ARG_WITH([regexen],
AS_HELP_STRING([--with-regexen], [Force regex engine to NONE, POSIX, PCRE8, or PCRE16]))
AS_IF([test "x$with_regexen" = "xno" || test "x$with_regexen" = "xNONE"], [
  AC_DEFINE([FORCE_REGEX_NONE], [1], [Force regex engine to NONE])], [
  AS_IF([test "x$with_regexen" = "xPOSIX"], [
    AC_DEFINE([FORCE_REGEX_POSIX], [1], [Force regex engine to POSIX])], [
    AS_IF([test "x$with_regexen" = "xPCRE8"], [
      AC_DEFINE([FORCE_REGEX_PCRE8], [1], [Force regex engine to PCRE8])], [
      AS_IF([test "x$with_regexen" = "xPCRE16"], [
        AC_DEFINE([FORCE_REGEX_PCRE16], [1], [Force regex engine to PCRE16])], [
        AS_IF([test "x$with_regexen" = "xyes" || test "x$with_regexen" = "x"], [], [
          AC_MSG_ERROR([Unknown argument to --with-regexen])])])])])])


# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_SIZE_T

# Checks for library functions.
# We never malloc() or realloc() with size zero, so we don't need these
# checks.
dnl AC_FUNC_MALLOC
dnl AC_FUNC_REALLOC
AC_FUNC_FORK
AC_TYPE_PID_T
AC_CHECK_FUNCS([dup2 memmove memset strerror mkstemp glob])

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
