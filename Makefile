CFLAGS=     -Wall
CFLAGS+=    -Werror
CFLAGS+=    -Wpointer-arith
CFLAGS+=    -Wno-uninitialized
CFLAGS+=    -Wstrict-prototypes
CFLAGS+=    -Wmissing-prototypes
CFLAGS+=    -Wunused
CFLAGS+=    -Wsign-compare
CFLAGS+=    -Wbounded
CFLAGS+=    -Wshadow

CFLAGS+=    -g
CFLAGS+=    -I.

RANLIB=ranlib

all: libxobject.a

libxobject.a: xobject.o vis.o strlcpy.o
	$(AR) rv $@ xobject.o vis.o strlcpy.o
	$(RANLIB) $@

clean:
	rm -f *.o libxobject.a core *.core
	cd regress && make clean

test: all
	cd regress && make
