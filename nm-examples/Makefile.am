ACLOCAL_AMFLAGS = -I m4

lib_LTLIBRARIES = libnm_wrapper.la
bin_PROGRAMS = nm_device_status nm_device_status_monitor

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libnm_wrapper.pc

AM_CFLAGS = $(GLIB_CFLAGS) $(LIBNM_CFLAGS) -I../include/
LDADD = libnm_wrapper.la $(GLIB_LIBS) $(LIBNM_LIBS)

libnm_wrapper_la_LDFLAGS = -version-info 0:0:0
libnm_wrapper_la_SOURCES = libnm_wrapper.c libnm_wrapper_device.c
libnm_wrapper_la_HEADERS = ../include/libnm_wrapper.h ../include/libnm_wrapper_type.h
libnm_wrapper_ladir = $(includedir)

nm_device_status_SOURCES = nm_device_status.c

nm_device_status_monitor_SOURCES = nm_device_status_monitor.c
