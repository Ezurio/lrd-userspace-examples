ACLOCAL_AMFLAGS = -I m4

lib_LTLIBRARIES = libnm_wrapper.la
bin_PROGRAMS = nm_device_status nm_device_status_monitor

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libnm_wrapper.pc

AM_CFLAGS = $(GLIB_CFLAGS) $(LIBNM_CFLAGS) -I../include/
LDADD = libnm_wrapper.la $(GLIB_LIBS) $(LIBNM_LIBS)

noinst_HEADERS = libnm_wrapper_internal.h
libnm_wrapper_la_LDFLAGS = -version-info 0:0:0
libnm_wrapper_la_SOURCES = libnm_wrapper.c libnm_wrapper_device.c

nm_device_status_SOURCES = nm_device_status.c

nm_device_status_monitor_SOURCES = nm_device_status_monitor.c
