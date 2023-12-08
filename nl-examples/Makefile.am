ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = get_stations

AM_CFLAGS = -Wall -Werror $(LIBNL_GENL_CFLAGS) -I../include/
AM_LDFLAGS = $(LIBNL_GENL_LIBS)

get_stations_SOURCES = get_stations.c genl.c
