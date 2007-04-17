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

CFLAGS+=-g -I.
LDFLAGS+=-L. -g

RANLIB=ranlib

TARGETS=libmtemplate.a mtc

LIBMTEMPLATE_OBJS=strstcpy.o mobject.o mnamespace.o helpers.o mtemplate.o 
COMPAT_OBJS=vis.o strlcpy.o strlcat.o

all: $(TARGETS)

libmtemplate.a: $(LIBMTEMPLATE_OBJS) $(COMPAT_OBJS)
	$(AR) rv $@ $(LIBMTEMPLATE_OBJS) $(COMPAT_OBJS)
	$(RANLIB) $@

mtc: mtc.o libmtemplate.a
	$(CC) -o $@ mtc.o $(LDFLAGS) -lmtemplate

clean:
	rm -f *.o $(TARGETS) core *.core
	cd regress && make clean

test: all
	cd regress && make
