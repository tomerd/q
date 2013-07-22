MAINTAINERCLEANFILES = ar-lib INSTALL Makefile.in src/Makefile.in configure depcomp compile config.guess ltmain.sh config.sub missing install-sh aclocal.m4 config.h.in config.h.in~ py-compile m4/ltsugar.m4 m4/libtool.m4 m4/ltversion.m4 m4/lt~obsolete.m4 m4/ltoptions.m4 lib-$(Q_API_VERSION).la

AUTOMAKE_OPTIONS = subdir-objects

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = \
src/core \
src/backends \
src/3rd-party/json

# ideally we would link hiredis, lmdb statically but not their makefile is not osx compliant
noinst_LTLIBRARIES = libhiredis.la liblmdb.la

libhiredis_la_SOURCES = \
src/3rd-party/hiredis/fmacros.h \
src/3rd-party/hiredis/async.h src/3rd-party/hiredis/async.c \
src/3rd-party/hiredis/dict.h src/3rd-party/hiredis/dict.c \
src/3rd-party/hiredis/hiredis.h src/3rd-party/hiredis/hiredis.c \
src/3rd-party/hiredis/net.h src/3rd-party/hiredis/net.c \
src/3rd-party/hiredis/sds.h src/3rd-party/hiredis/sds.c

liblmdb_la_SOURCES = \
src/3rd-party/mdb/libraries/liblmdb/midl.h src/3rd-party/mdb/libraries/liblmdb/midl.c \
src/3rd-party/mdb/libraries/liblmdb/lmdb.h src/3rd-party/mdb/libraries/liblmdb/mdb.c

lib_LTLIBRARIES = libq-@Q_API_VERSION@.la
libq_@Q_API_VERSION@_la_SOURCES =

nodist_EXTRA_libq_@Q_API_VERSION@_la_SOURCES = dummy.cpp

libq_@Q_API_VERSION@_la_LIBADD = \
src/core/libcore.la \
src/backends/libbackends.la \
src/3rd-party/json/libjson.la \
libhiredis.la \
liblmdb.la

libq_@Q_API_VERSION@_la_LDFLAGS = -version-info $(Q_SO_VERSION)


#q_libincludedir = $(libdir)/q-$(Q_API_VERSION)/include
#nodist_q_libinclude_HEADERS = qconfig.h

#pkgconfigdir = $(libdir)/pkgconfig
#pkgconfig_DATA = lib-$(Q_API_VERSION).pc

#dist_noinst_SCRIPTS = autogen.sh

