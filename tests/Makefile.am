# -*- mode: makefile -*-

EXTRA_DIST = 

include $(top_srcdir)/Makefile.common

LIBS += $(BOOST_UNIT_TEST_FRAMEWORK_LIBS)

TESTS = legacy
check_PROGRAMS = legacy

legacy_SOURCES = legacy.cc
legacy_LDADD = $(LIBS)
