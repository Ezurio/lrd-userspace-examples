ACLOCAL_AMFLAGS = -I m4

if BUILD_NL_EXAMPLES
bin_PROGRAMS = get_stations
endif

get_stations_SOURCES = get_stations.c genl.c
get_stations_CFLAGS = \
	-I../include/ \
	$(LIBNL_GENL_CFLAGS) \
	$(LIBNL_GENL_LIBS)
