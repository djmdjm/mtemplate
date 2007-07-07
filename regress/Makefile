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

CFLAGS+=    -g
CFLAGS+=    -I..

LDFLAGS=-g 
LIBS=../libmtemplate.a

BIN_TARGETS=	t_strstcpy
BIN_TARGETS+=	mobject_t0 mobject_t1 mobject_t2 mobject_t3
BIN_TARGETS+=	mtemplate_t0
EXEC_TARGETS=	t_strstcpy_exec
EXEC_TARGETS+=	mobject_t0_exec mobject_t1_exec mobject_t2_exec mobject_t3_exec
EXEC_TARGETS+=	mtemplate_t0_exec

all: $(LIBS) $(BIN_TARGETS) t_start $(EXEC_TARGETS)

t_start:
	@echo Running tests

mobject_t0_exec: mobject_t0
	@./mobject_t0

mobject_t0: mobject_t0.o $(LIBS) 
	$(CC) -o $@ mobject_t0.o $(LDFLAGS) $(LIBS)

mobject_t1_exec: mobject_t1
	@./mobject_t1

mobject_t1: mobject_t1.o $(LIBS) 
	$(CC) -o $@ mobject_t1.o $(LDFLAGS) $(LIBS)

mobject_t2_exec: mobject_t2
	@./mobject_t2

mobject_t2: mobject_t2.o $(LIBS) 
	$(CC) -o $@ mobject_t2.o $(LDFLAGS) $(LIBS)

mobject_t3_exec: mobject_t3
	@./mobject_t3

mobject_t3: mobject_t3.o $(LIBS) 
	$(CC) -o $@ mobject_t3.o $(LDFLAGS) $(LIBS)

t_strstcpy_exec: t_strstcpy
	@./t_strstcpy

t_strstcpy: t_strstcpy.o $(LIBS) 
	$(CC) -o $@ t_strstcpy.o $(LDFLAGS) $(LIBS)

mtemplate_t0_exec: mtemplate_t0
	@./mtemplate_t0

mtemplate_t0: mtemplate_t0.o ../libmtemplate.a 
	$(CC) -o $@ mtemplate_t0.o $(LDFLAGS) $(LIBS)

clean:
	rm -f *.o $(BIN_TARGETS) core *.core *~
