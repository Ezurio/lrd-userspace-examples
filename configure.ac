AC_PREREQ([2.69])
AC_INIT([lrd-examples], 1.0.0, [support@ezurio.com])
AM_INIT_AUTOMAKE
AC_CONFIG_MACRO_DIRS([m4])

AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S

# Checks for header files.
AC_CONFIG_HEADERS([config.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

PKG_CHECK_MODULES(LIBNM, [libnm], [have_libnm=yes], [have_libnm=no])
PKG_CHECK_MODULES(LIBNL_GENL, [libnl-genl-3.0], [have_libnl_genl=yes], [have_libnl_genl=no])

AC_ARG_ENABLE(nm-examples, AS_HELP_STRING([--enable-nm-examples], [Build networkmanager examples]),
    [enable_libnm=${enableval}], [enable_libnm=${have_libnm}])
AM_CONDITIONAL(BUILD_NM_EXAMPLES, test "${enable_nm_examples}" = "yes")

AC_ARG_ENABLE(nl-examples, AS_HELP_STRING([--enable-nl-examples], [Build netlink examples]),
    [enable_libnl_genl=${enableval}], [enable_libnl_genl=${have_libnl_genl}])
AM_CONDITIONAL(BUILD_NL_EXAMPLES, test "${enable_nl_examples}" = "yes")

# Checks for libraries.
PKG_PROG_PKG_CONFIG

if test "$enable_nm_examples" != 'no'; then
PKG_CHECK_MODULES(GLIB, [glib-2.0])
AC_CONFIG_FILES([nm-examples/Makefile nm-examples/libnm_wrapper.pc])
fi

if test "$enable_nl_examples" != 'no'; then
AC_CONFIG_FILES([nl-examples/Makefile])
fi

LT_INIT

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
