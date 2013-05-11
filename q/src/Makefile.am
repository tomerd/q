lib_LTLIBRARIES = libq-@Q_API_VERSION@.la

JSON_SRC = \
3rd-party/json/autolink.h \
3rd-party/json/config.h \
3rd-party/json/features.h \
3rd-party/json/forwards.h \
3rd-party/json/json_batchallocator.h \
3rd-party/json/json_internalarray.inl \
3rd-party/json/json_internalmap.inl \
3rd-party/json/json_reader.cpp \
3rd-party/json/json_value.cpp \
3rd-party/json/json_valueiterator.inl \
3rd-party/json/json_writer.cpp \
3rd-party/json/json.h \
3rd-party/json/reader.h \
3rd-party/json/value.h \
3rd-party/json/writer.h

HIREDIS_SRC = \
3rd-party/hiredis/async.h 3rd-party/hiredis/async.c \
3rd-party/hiredis/dict.h 3rd-party/hiredis/dict.c \
3rd-party/hiredis/fmacros.h \
3rd-party/hiredis/hiredis.h 3rd-party/hiredis/hiredis.c \
3rd-party/hiredis/net.h 3rd-party/hiredis/net.c \
3rd-party/hiredis/sds.h 3rd-party/hiredis/sds.c

if Q_JNI
JNI_SRC = core/QlibJava.h core/QlibJava.cpp
else
JNI_SRC = ""
endif

if Q_BERKELEY
BERKELEY_SRC = berkeley/BerkeleyQ.h berkeley/BerkeleyQ.cpp
else
BERKELEY_SRC = ""
endif

libq_@Q_API_VERSION@_la_SOURCES = \
${JSON_SRC} \
${HIREDIS_SRC} \
core/Job.h core/Job.cpp \
core/JobCodec.h core/JobCodec.cpp \
core/JobError.h core/JobError.cpp \
core/Logger.h core/Logger.cpp \
core/Q.h core/Q.cpp \
core/QFactory.h core/QFactory.cpp \
core/Qlib.h core/Qlib.cpp \
${JNI_SRC} \
transient/TransientQ.h transient/TransientQ.cpp \
redis/RedisQ.h redis/RedisQ.cpp \
${BERKELEY_SRC}

libq_@Q_API_VERSION@_la_LDFLAGS = -version-info $(Q_SO_VERSION)

