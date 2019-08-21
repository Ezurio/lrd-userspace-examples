# Allow CROSS_COMPILE to specify compiler base
CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld
NM := $(CROSS_COMPILE)nm
AR := $(CROSS_COMPILE)ar

PKG_CONFIG ?= pkg-config

CFLAGS += $(shell $(PKG_CONFIG) --cflags glib-2.0 libnm)
LIBS   += $(shell $(PKG_CONFIG) --libs   glib-2.0 libnm)

LIBS   += -L. -ldl -lssl

CFLAGS += -Wall -Werror -I./include -fPIC
COMPILEONLY = -c
ifdef DEBUG
	CFLAGS += -ggdb
	CFLAGS += -DENGINEERING_RELEASE
endif

LIB = libnm_sdk

LIBNM_SDK_OBJS += lib/libnm_wrapper.o

.PHONY: all clean
.DEFAULT: all

all: $(LIB)

%.o: %.c
	$(CC) $(CFLAGS) $(COMPILEONLY) $^ -o $@

$(LIB): $(LIBNM_SDK_OBJS)
	$(CC) -shared -Wl,-soname,$(LIB).so.1 \
	-o $(LIB).so.1.0 $(_OBJS) -lc $(LIBS)
	ln -fs $(LIB).so.1.0 $(LIB).so

$(LIB).a:$(LIBNM_SDK_OBJS)
	$(AR) rcs $(LIB).a $(_OBJS)

clean:
	rm -f lib/*.o
