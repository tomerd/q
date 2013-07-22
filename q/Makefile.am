MAINTAINERCLEANFILES = ar-lib INSTALL Makefile.in src/Makefile.in configure depcomp compile config.guess ltmain.sh config.sub missing install-sh aclocal.m4 config.h.in config.h.in~ py-compile m4/ltsugar.m4 m4/libtool.m4 m4/ltversion.m4 m4/lt~obsolete.m4 m4/ltoptions.m4 lib-$(Q_API_VERSION).la

AUTOMAKE_OPTIONS = subdir-objects

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = \
src/core \
src/backends

# ideally we would link hiredis, lmdb, json statically but not their makefiles are not osx compliant
noinst_LTLIBRARIES = libhiredis.la liblmdb.la libjson.la

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

libjson_la_SOURCES = \
src/3rd-party/json/autolink.h \
src/3rd-party/json/config.h \
src/3rd-party/json/features.h \
src/3rd-party/json/forwards.h \
src/3rd-party/json/json_batchallocator.h \
src/3rd-party/json/json_internalarray.inl \
src/3rd-party/json/json_internalmap.inl \
src/3rd-party/json/json_reader.cpp \
src/3rd-party/json/json_value.cpp \
src/3rd-party/json/json_valueiterator.inl \
src/3rd-party/json/json_writer.cpp \
src/3rd-party/json/json.h \
src/3rd-party/json/reader.h \
src/3rd-party/json/value.h \
src/3rd-party/json/writer.h

lib_LTLIBRARIES = libq-@Q_API_VERSION@.la
libq_@Q_API_VERSION@_la_SOURCES =

nodist_EXTRA_libq_@Q_API_VERSION@_la_SOURCES = dummy.cpp

libq_@Q_API_VERSION@_la_LIBADD = \
src/core/libcore.la \
src/backends/libbackends.la \
libhiredis.la \
liblmdb.la \
libjson.la

libq_@Q_API_VERSION@_la_LDFLAGS = -version-info $(Q_SO_VERSION)


#q_libincludedir = $(libdir)/q-$(Q_API_VERSION)/include
#nodist_q_libinclude_HEADERS = qconfig.h

#pkgconfigdir = $(libdir)/pkgconfig
#pkgconfig_DATA = lib-$(Q_API_VERSION).pc

#dist_noinst_SCRIPTS = autogen.sh

