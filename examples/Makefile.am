# -*- mode: makefile -*-

EXTRA_DIST = 

include $(top_srcdir)/Makefile.common

if DARWIN
  # Because stupid
  LIBS += -lc++abi
endif

bin_PROGRAMS = wrapper

wrapper_SOURCES = wrapper.cpp
wrapper_LDADD = $(LIBS)
