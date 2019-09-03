AC_PREREQ([2.69])
AC_INIT([lrd-examples], 1.0.0, [examples@lairdconnect.com])
AM_INIT_AUTOMAKE
AC_CONFIG_MACRO_DIRS([m4])

PKG_PROG_PKG_CONFIG

AC_PROG_CC
AM_PROG_CC_C_O

AC_PROG_LN_S

# Checks for libraries.
PKG_CHECK_MODULES([GLIB], [glib-2.0], [have_libglib=yes], [have_libglib=no])
AM_CONDITIONAL([GLIB],  [test "$have_libglib" = "yes"])
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

PKG_CHECK_MODULES([GIO], [gio-unix-2.0], [have_libgio=yes], [have_libgio=no])
AM_CONDITIONAL([GIO],  [test "$have_libgio" = "yes"])
AC_SUBST(GIO_CFLAGS)
AC_SUBST(GIO_LIBS)

PKG_CHECK_MODULES([NM], [libnm], [have_libnm=yes], [have_libnm=no])
AM_CONDITIONAL([NM],  [test "$have_libnm" = "yes"])
AC_SUBST(NM_CFLAGS)
AC_SUBST(NM_LIBS)

# Checks for header files.
AC_CONFIG_HEADERS([config.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

AC_ARG_ENABLE(examples,
              AS_HELP_STRING([--enable-examples], [Build examples]),
              [enable_examples=${enableval}], [enable_examples=no])
if (test "${enable_examples}" = "yes"); then
	AC_DEFINE(BUILD_EXAMPLES, 1, [Define if you want to build examples])
else
	AC_DEFINE(BUILD_EXAMPLES, 0, [Define if you want to build examples])
fi
AM_CONDITIONAL(BUILD_EXAMPLES, test "${enable_examples}" = "yes")

LT_INIT

AC_CONFIG_FILES([
Makefile
lib/Makefile
lib/libnm_sdk.pc
examples/Makefile
include/Makefile
])

AC_OUTPUT
