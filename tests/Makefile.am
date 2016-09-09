# -*- mode: makefile -*-

EXTRA_DIST = 

include $(top_srcdir)/Makefile.common

LIBS += $(BOOST_UNIT_TEST_FRAMEWORK_LIBS)

if DARWIN
  # Because stupid
  LIBS += -lc++abi
endif

TESTS = unique_resource
check_PROGRAMS = unique_resource

unique_resource_SOURCES = unique_resource.cpp
unique_resource_LDADD = $(LIBS)
