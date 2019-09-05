ACLOCAL_AMFLAGS = -I m4

lib_LTLIBRARIES = libnm_wrapper.la
noinst_HEADERS = libnm_wrapper_internal.h
libnm_wrapper_la_CFLAGS = $(GLIB_CFLAGS) $(NM_CFLAGS) -I../include/
libnm_wrapper_la_LDFLAGS = -version-info 0:0:0
libnm_wrapper_la_SOURCES = libnm_wrapper.c libnm_wrapper_device.c

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libnm_wrapper.pc
