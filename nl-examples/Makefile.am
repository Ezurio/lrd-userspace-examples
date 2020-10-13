ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = get_stations

get_stations_SOURCES = get_stations.c genl.c
get_stations_CFLAGS = \
	-I../include/ \
	$(LIBNL_GENL_CFLAGS) \
	$(LIBNL_GENL_LIBS)
