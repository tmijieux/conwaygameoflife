TARGET=life_omp_db life_pthread_db
CFLAGS=-std=gnu99 -fopenmp -g -Wall -Wextra -fdiagnostics-color=auto # $(shell pkg-config --cflags glib-2.0)
LDFLAGS=-fopenmp
LIBS= #-lm $(shell pkg-config --libs glib-2.0)
GENGETOPT=gengetopt
CC=gcc

ifdef DEBUG
CFLAGS+=-ggdb -O0 -DDEBUG=1 -DCGL_DEBUG=1
else
CFLAGS+=-O3
endif

SRC_omp_db= life_omp_db.c
SRC_pthread_db= life_pthread_db.c

OBJ_omp_db=$(SRC_omp_db:.c=.o)
OBJ_pthread_db=$(SRC_pthread_db:.c=.o)

DEP=$(SRC:.c=.d)

all: $(TARGET)

-include $(DEP)

life_omp_db: $(OBJ_omp_db)
	$(CC) $^ -o $@ $(LDFLAGS) $(LIBS)

life_pthread_db: $(OBJ_pthread_db)
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

