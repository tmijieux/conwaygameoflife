TARGET=life_mpi
CFLAGS=-std=gnu99 -fopenmp -g -Wall -Wextra -fdiagnostics-color=auto # $(shell pkg-config --cflags glib-2.0)
LDFLAGS=-fopenmp
LIBS= -lm # $(shell pkg-config --libs glib-2.0)
GENGETOPT=gengetopt
CC=mpiicc

ifdef DEBUG
CFLAGS+=-ggdb -O0 -DDEBUG=1 -DCGL_DEBUG=1
else
CFLAGS+=-O3
endif

OBJ= main.o board.o proc.o timer.o ../perf/perf.o
DEP=$(OBJ:.o=.d)

all: $(TARGET)

-include $(DEP)

life_mpi: $(OBJ)
	$(CC) $^ -o $@ $(LDFLAGS) $(LIBS)

%.o: %.c
	@$(CC) -MM $(CFLAGS) $*.c > $*.d
	$(CC) -c $(CFLAGS) $*.c -o $*.o

clean:
	$(RM) $(TARGET) $(OBJ) $(DEP) *.d *.o

mrproper: clean
	$(RM) $(TARGET)

