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
CFLAGS+=    -I..

LDFLAGS=-g 
LIBS=../libxobject.a

TARGETS=t0
EXEC_TARGETS= t0_exec

all: ../libxobject.a $(TARGETS) $(EXEC_TARGETS)

t0_exec: t0
	./t0

t0: t0.o
	$(CC) -o $@ t0.o $(LDFLAGS) $(LIBS)

clean:
	rm -f *.o $(TARGETS) core *.core
