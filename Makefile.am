
ACLOCAL_AMFLAGS = -I m4 --install

lib_LIBRARIES = libvinetic.a
libvinetic_a_SOURCES = libvinetic.c
libvinetic_a_CFLAGS = $(AM_CFLAGS) -Wall -fPIC -DPIC

nobase_include_HEADERS = \
	libvinetic/libvinetic.h
nobase_nodist_include_HEADERS = \
	libvinetic/version.h

CLEANFILES = *~
