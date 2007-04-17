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
CFLAGS+=    -I..

LDFLAGS=-g 
LIBS=../libmobject.a

TARGETS=mobject_t0 mobject_t1 mobject_t2 t_strstcpy
EXEC_TARGETS= t_strstcpy_exec mobject_t0_exec mobject_t1_exec mobject_t2_exec

all: ../libmobject.a $(TARGETS) t_start $(EXEC_TARGETS)

t_start:
	@echo Running tests

mobject_t0_exec: mobject_t0
	@./mobject_t0

mobject_t0: mobject_t0.o ../libmobject.a 
	$(CC) -o $@ mobject_t0.o $(LDFLAGS) $(LIBS)

mobject_t1_exec: mobject_t1
	@./mobject_t1

mobject_t1: mobject_t1.o ../libmobject.a 
	$(CC) -o $@ mobject_t1.o $(LDFLAGS) $(LIBS)

mobject_t2_exec: mobject_t2
	@./mobject_t2

mobject_t2: mobject_t2.o ../libmobject.a 
	$(CC) -o $@ mobject_t2.o $(LDFLAGS) $(LIBS)

t_strstcpy_exec: t_strstcpy
	@./t_strstcpy

t_strstcpy: t_strstcpy.o ../libmobject.a 
	$(CC) -o $@ t_strstcpy.o $(LDFLAGS) $(LIBS)

clean:
	rm -f *.o $(TARGETS) core *.core
