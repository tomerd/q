AC_PREREQ([2.60])

AC_INIT([Q], [1.0], [tomer.doron@gmail.com], [Q], [https://github.com/tomerd/q])

AC_USE_SYSTEM_EXTENSIONS

AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.10 -Wall no-define])

AC_CONFIG_HEADERS([config.h])

AM_PROG_AR

AC_LANG([C++])
AM_PROG_CC_C_O
AC_PROG_CXX([clang++ g++])

dnl check for C++11 support
AX_CXX_COMPILE_STDCXX_11([noext])
AS_IF([test "x$HAVE_CXX11" != "x1"],
      [AC_MSG_ERROR([** A compiler with C++11 language features is required.])])
CXXFLAGS="$CXXFLAGS -stdlib=libc++"

LT_INIT

AC_SUBST([Q_API_VERSION], [1.0])
AC_SUBST([Q_SO_VERSION], [1:0:0])

AC_CONFIG_FILES([Makefile
				 src/Makefile
                 q-${Q_API_VERSION}.pc:q.pc.in])
AC_OUTPUT
