AC_PREREQ([2.60])

AC_INIT([Q], [1.0], [tomer.doron@gmail.com], [Q], [https://github.com/tomerd/q])

AC_USE_SYSTEM_EXTENSIONS

AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.10 -Wall no-define])

AC_CONFIG_HEADERS([config.h])

#AM_PROG_AR
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

AC_LANG([C++])
AM_PROG_CC_C_O
AC_PROG_CXX([clang++ g++])

AC_CHECK_PROG([USING_CLANG], [clang++], [yes], [no])

dnl check for C++11 support
AX_CXX_COMPILE_STDCXX_11([noext])
AS_IF([test "x$HAVE_CXX11" != "x1"],
      [AC_MSG_ERROR([** A compiler with C++11 language features is required.])])

if test "$USING_CLANG" = "yes"; then
	CXXFLAGS="$CXXFLAGS -stdlib=libc++"
fi

dnl If --with-java, make with java bindings
AC_ARG_WITH(java,
    [ --with-java 	  - make with java bindings (default=no) ],
    [with_java=true])

dnl check for Java support
if test "$with_java" = "true"; then
	if test "$JAVAC" = ""; then
		AC_PROG_JAVAC
	fi
fi

dnl check for JNI support
if test "$JAVAC" = "javac"
then
	AC_JNI_INCLUDE_DIR
	
	#for JNI_INCLUDE_DIR in $JNI_INCLUDE_DIRS
	#do
	#	CXXFLAGS="$CXXFLAGS -I$JNI_INCLUDE_DIR"
	#done
	
	AM_CONDITIONAL(Q_JNI, true)
	
	dnl AC_PROG_JAVAH
	dnl AC_PATH_PROG(JAVAH,javah)
else
	AM_CONDITIONAL(Q_JNI, false)
fi

dnl If --with-berkeley, make with berkeley db support
AC_ARG_WITH(berkeley,
    [ --with-berkeley 	  - make with berkeley db support (default=no) ],
    [with_berkeley=true])

dnl check for berkeley support
if test "$with_berkeley" = "true"; then
	AX_BERKELEY_DB_CXX
fi	
if test "$DB_CXX_HEADER" = ""; then
	AM_CONDITIONAL(Q_BERKELEY, false)
else
	AM_CONDITIONAL(Q_BERKELEY, true)
fi

LT_INIT

AC_SUBST([Q_API_VERSION], [1.0])
AC_SUBST([Q_SO_VERSION], [1:0:0])

AC_CONFIG_FILES([Makefile
				 src/Makefile
                 q-${Q_API_VERSION}.pc:q.pc.in])
AC_OUTPUT
