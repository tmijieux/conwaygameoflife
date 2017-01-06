TARGET=life_seq life_pthread
CFLAGS=-std=gnu99 -fopenmp -g -Wall -Wextra #$(shell pkg-config --cflags glib-2.0)
LDFLAGS=-fopenmp
LIBS=-lm #$(shell pkg-config --libs glib-2.0) 
GENGETOPT=gengetopt
CC=gcc

ifdef DEBUG
CFLAGS+=-ggdb -O0 -DDEBUG=1
else
CFLAGS+=-O3
endif

SRC_seq= life_seq.c
SRC_pthread= life_pthread.c

OBJ_seq=$(SRC_seq:.c=.o)
OBJ_pthread=$(SRC_pthread:.c=.o)

DEP=$(SRC:.c=.d)

all: $(TARGET)

-include $(DEP)


life_seq: $(OBJ_seq)
	$(CC) $^ -o $@ $(LDFLAGS) $(LIBS)

life_pthread: $(OBJ_pthread)
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

