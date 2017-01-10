TARGET=life_seq life_pthread life_omp life_mpi
CFLAGS=-std=gnu99 -fopenmp -g -Wall -Wextra -fdiagnostics-color=auto # $(shell pkg-config --cflags glib-2.0)
LDFLAGS=-fopenmp
LIBS= -lm # $(shell pkg-config --libs glib-2.0)
GENGETOPT=gengetopt
CC=gcc

ifdef DEBUG
CFLAGS+=-ggdb -O0 -DDEBUG=1 -DCGL_DEBUG=1
else
CFLAGS+=-O3
endif

SRC_seq= life_seq.c
SRC_pthread= life_pthread.c
SRC_omp= life_omp.c
SRC_mpi= life_mpi.c

OBJ_seq=$(SRC_seq:.c=.o)
OBJ_pthread=$(SRC_pthread:.c=.o)
OBJ_omp=$(SRC_omp:.c=.o)
OBJ_mpi=$(SRC_mpi:.c=.o)

DEP=$(SRC:.c=.d)

all: $(TARGET)

-include $(DEP)


life_seq: $(OBJ_seq)
	$(CC) $^ -o $@ $(LDFLAGS) $(LIBS)

life_pthread: $(OBJ_pthread)
	$(CC) $^ -o $@ $(LDFLAGS) $(LIBS)

life_omp: $(OBJ_omp)
	$(CC) $^ -o $@ $(LDFLAGS) $(LIBS)

life_mpi: CC=mpicc
life_mpi:

life_mpi: $(OBJ_mpi)
	$(CC) $^ -o $@ $(LDFLAGS) $(LIBS)

%.o: %.c
	@$(CC) -MM $(CFLAGS) $*.c > $*.d
	$(CC) -c $(CFLAGS) $*.c -o $*.o

clean:
	$(RM) $(TARGET) $(OBJ) $(DEP) *.d *.o

mrproper: clean
	$(RM) $(TARGET)

genopt: matprod.ggo
	$(GENGETOPT) -u"INPUT FILES" < $^

