# -*- mode: autoconf -*-

AC_INIT([unique_resource],[0.0.1],)

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([m4])

AM_INIT_AUTOMAKE([-Wno-portability subdir-objects dist-bzip2 nostdinc foreign])
AC_CONFIG_HEADERS([include/_config.hpp:include/config.ac])

AC_PROG_CXX
AC_CONFIG_CXX_WARNINGS

dnl AC_ENABLE_CXX_DIALECT([c++11])
AC_ENABLE_CXX_DIALECT([c++1z])

AC_PROG_LIBTOOL
AC_PROG_MAKE_SET(gmake)
AM_PROG_AS

BOOST_REQUIRE([1.61.0])

BOOST_SYSTEM([])
BOOST_THREADS([])
BOOST_TIMER([])
BOOST_TEST([])

AM_CONDITIONAL([DARWIN],[test `uname` == Darwin])
AM_CONDITIONAL([LINUX], [test `uname` == Linux])

AC_CONFIG_FILES(Makefile)
AC_CONFIG_FILES(include/Makefile)
AC_CONFIG_FILES(examples/Makefile)
AC_CONFIG_FILES(tests/Makefile)

AC_OUTPUT()

