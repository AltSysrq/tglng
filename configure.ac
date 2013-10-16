#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.6])
AC_INIT([TGLNG], [1.0.3], [jason@lin.gl])
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
AC_CHECK_HEADERS([fcntl.h unistd.h glob.h regex.h pcre.h getopt.h])

# Handle regex engine stuff
AC_SEARCH_LIBS([pcre_compile], [pcre])
AC_SEARCH_LIBS([pcre16_compile], [pcre pcre16])
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
# Don't check for regcomp as suggested by autoscan; if regex.h exists, it is
# generally safe to say that the functions defined within exist as well.
AC_CHECK_FUNCS([dup2 memmove memset strerror mkstemp getopt setenv setlocale],
  [],
  AC_MSG_ERROR([A required function could not be found.]))

AC_CHECK_FUNCS([glob])

AC_CHECK_FUNCS([getopt_long],
  [AC_DEFINE([USE_GETOPT_LONG], [1], [Use getopt_long instead of getopt])],
  [])

# AC_CHECK_HEADER_STDBOOL isn't necessary since we're using C++ which already
# mandates bool

# AC_FUNC_ERROR_AT_LINE isn't necessary since we don't actually use THAT
# error() (autoscan gets confused by Interpreter::error()).

AC_CONFIG_FILES([Makefile src/Makefile doc/Makefile])
AC_OUTPUT
