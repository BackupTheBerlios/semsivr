#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.58)
AC_INIT(rtpproxy, 0.2, sobomax@portaone.com)
AM_INIT_AUTOMAKE(rtpproxy, 0.2)
AM_MAINTAINER_MODE
AC_CONFIG_SRCDIR(main.c)
AM_CONFIG_HEADER(config.h)
AC_ARG_ENABLE(transcoder, [  --enable-transcoder build with transcoder support], 
                build_transcoder=yes)

AM_CONDITIONAL([TRANSCODER], [test "$build_transcoder" = yes])

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_LN_S
AC_PROG_RANLIB

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h netdb.h netinet/in.h stdlib.h string.h strings.h sys/socket.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_HEADER_TIME

# Checks for library functions.
AC_FUNC_ALLOCA
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_TYPE_SIGNAL
AC_CHECK_FUNC(dlclose, , [ AC_CHECK_LIB(dl, dlopen) ])
AC_CHECK_FUNCS([atexit gettimeofday memset mkdir socket strchr strrchr strdup strerror strtol strtoul ])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
