CFLAGS=     -Wall
CFLAGS+=    -Werror
CFLAGS+=    -Wpointer-arith
CFLAGS+=    -Wno-uninitialized
CFLAGS+=    -Wstrict-prototypes
CFLAGS+=    -Wmissing-prototypes
CFLAGS+=    -Wunused
CFLAGS+=    -Wsign-compare
#CFLAGS+=    -Wbounded
CFLAGS+=    -Wshadow
CFLAGS+=    -Wno-pointer-sign
CFLAGS+=    -Wno-attributes

CFLAGS+=    -g
CFLAGS+=    -I.

RANLIB=ranlib

LIBMOBJECT_OBJS=	mobject.o mnamespace.o helpers.o
COMPAT_OBJS=		vis.o strlcpy.o strlcat.o strstcpy.o

all: libmobject.a

libmobject.a: $(LIBMOBJECT_OBJS) $(COMPAT_OBJS)
	$(AR) rv $@ $(LIBMOBJECT_OBJS) $(COMPAT_OBJS)
	$(RANLIB) $@

clean:
	rm -f *.o libmobject.a core *.core
	cd regress && make clean

test: all
	cd regress && make
