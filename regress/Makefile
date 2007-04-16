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
CFLAGS+=    -I..

LDFLAGS=-g 
LIBS=../libxobject.a

TARGETS=t0 t1 t2 t_strstcpy
EXEC_TARGETS= t_strstcpy_exec t0_exec t1_exec t2_exec

all: ../libxobject.a $(TARGETS) t_start $(EXEC_TARGETS)

t_start:
	@echo Running tests

t0_exec: t0
	@./t0

t0: t0.o ../libxobject.a 
	$(CC) -o $@ t0.o $(LDFLAGS) $(LIBS)

t1_exec: t1
	@./t1

t1: t1.o ../libxobject.a 
	$(CC) -o $@ t1.o $(LDFLAGS) $(LIBS)

t2_exec: t2
	@./t2

t2: t2.o ../libxobject.a 
	$(CC) -o $@ t2.o $(LDFLAGS) $(LIBS)

t_strstcpy_exec: t_strstcpy
	@./t_strstcpy

t_strstcpy: t_strstcpy.o ../libxobject.a 
	$(CC) -o $@ t_strstcpy.o $(LDFLAGS) $(LIBS)

clean:
	rm -f *.o $(TARGETS) core *.core
