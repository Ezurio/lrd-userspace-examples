ACLOCAL_AMFLAGS = -I m4

if BUILD_NM_EXAMPLES
bin_PROGRAMS = nm_device_status nm_device_status_monitor
lib_LTLIBRARIES = libnm_wrapper.la
pkgconfig_DATA = libnm_wrapper.pc
endif

noinst_HEADERS = libnm_wrapper_internal.h
libnm_wrapper_la_CFLAGS = $(GLIB_CFLAGS) $(NM_CFLAGS) -I../include/
libnm_wrapper_la_LDFLAGS = -version-info 0:0:0
libnm_wrapper_la_SOURCES = libnm_wrapper.c libnm_wrapper_device.c

pkgconfigdir = $(libdir)/pkgconfig

nm_device_status_SOURCES = nm_device_status.c
nm_device_status_CFLAGS = \
	-I../include/ \
	$(GLIB_CFLAGS) \
	$(GIO_CFLAGS) \
	$(NM_CFLAGS)

nm_device_status_LDADD = \
	libnm_wrapper.la \
	$(GLIB_LIBS) \
	$(GIO_LIBS) \
	$(NM_LIBS)

nm_device_status_monitor_SOURCES = nm_device_status_monitor.c
nm_device_status_monitor_CFLAGS = \
	-I../include/ \
	$(GLIB_CFLAGS) \
	$(GIO_CFLAGS) \
	$(NM_CFLAGS)

nm_device_status_monitor_LDADD = \
	libnm_wrapper.la \
	$(GLIB_LIBS) \
	$(GIO_LIBS) \
	$(NM_LIBS)
