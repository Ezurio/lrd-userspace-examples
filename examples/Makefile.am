ACLOCAL_AMFLAGS = -I m4

if BUILD_EXAMPLES
bin_PROGRAMS = nm_device_status nm_device_status_monitor
endif

nm_device_status_SOURCES = nm_device_status.c
nm_device_status_CPPFLAGS = \
	-I../include/ \
	$(GLIB_CFLAGS) \
	$(GIO_CFLAGS) \
	$(NM_CFLAGS)
nm_device_status_LDADD = \
	$(top_builddir)/lib/libnm_sdk.la \
	$(GLIB_LIBS) \
	$(GIO_LIBS) \
	$(NM_LIBS)

nm_device_status_monitor_SOURCES = nm_device_status_monitor.c
nm_device_status_monitor_CPPFLAGS = \
	-I../include/ \
	$(GLIB_CFLAGS) \
	$(GIO_CFLAGS) \
	$(NM_CFLAGS)
nm_device_status_monitor_LDADD = \
	$(top_builddir)/lib/libnm_sdk.la \
	$(GLIB_LIBS) \
	$(GIO_LIBS) \
	$(NM_LIBS)
