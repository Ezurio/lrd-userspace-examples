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

PKG_CHECK_MODULES([LIBNL_GENL], [libnl-genl-3.0], [have_libnl-genl=yes], [have_libnl-genl=no])
AM_CONDITIONAL([LIBNL_GENL],  [test "$have_libnl-genl" = "yes"])
AC_SUBST(LIBNL_GENL_CFLAGS)
AC_SUBST(LIBNL_GENL_LIBS)

# Checks for header files.
AC_CONFIG_HEADERS([config.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

AC_ARG_ENABLE(nm-examples,
              AS_HELP_STRING([--enable-nm-examples], [Build networkmanager examples]),
              [enable_nm_examples=${enableval}], [enable_nm_examples=no])
if (test "${enable_nm_examples}" = "yes"); then
	AC_DEFINE(BUILD_NM_EXAMPLES, 1, [Define if you want to build networkmanager examples])
else
	AC_DEFINE(BUILD_NM_EXAMPLES, 0, [Define if you want to build networkmanager examples])
fi
AM_CONDITIONAL(BUILD_NM_EXAMPLES, test "${enable_nm_examples}" = "yes")


AC_ARG_ENABLE(nl-examples,
              AS_HELP_STRING([--enable-nl-examples], [Build netlink examples]),
              [enable_nl_examples=${enableval}], [enable_nl_examples=no])
if (test "${enable_nl_examples}" = "yes"); then
	AC_DEFINE(BUILD_NL_EXAMPLES, 1, [Define if you want to build netlink examples])
else
	AC_DEFINE(BUILD_NL_EXAMPLES, 0, [Define if you want to build netlink examples])
fi
AM_CONDITIONAL(BUILD_NL_EXAMPLES, test "${enable_nl_examples}" = "yes")

AC_ARG_ENABLE(cmux,
              AS_HELP_STRING([--enable-cmux], [Build cmux]),
              [enable_cmux=${enableval}], [enable_cmux=no])
if (test "${enable_cmux}" = "yes"); then
	AC_DEFINE(BUILD_CMUX, 1, [Define if you want to build cmux])
else
	AC_DEFINE(BUILD_CMUX, 0, [Define if you want to build cmux])
fi
AM_CONDITIONAL(BUILD_CMUX, test "${enable_cmux}" = "yes")

LT_INIT

AC_CONFIG_FILES([
Makefile
nl-examples/Makefile
nm-examples/Makefile
nm-examples/libnm_wrapper.pc
cmux/Makefile
include/Makefile
])

AC_OUTPUT
