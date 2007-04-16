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
#CFLAGS+=    -Wno-pointer-sign
#CFLAGS+=    -Wno-attributes

CFLAGS+=    -g
CFLAGS+=    -I.

RANLIB=ranlib

LIBXOBJECT_OBJS=	xobject.o xnamespace.o helpers.o
COMPAT_OBJS=		vis.o strlcpy.o strlcat.o strstcpy.o

all: libxobject.a

libxobject.a: $(LIBXOBJECT_OBJS) $(COMPAT_OBJS)
	$(AR) rv $@ $(LIBXOBJECT_OBJS) $(COMPAT_OBJS)
	$(RANLIB) $@

clean:
	rm -f *.o libxobject.a core *.core
	cd regress && make clean

test: all
	cd regress && make
