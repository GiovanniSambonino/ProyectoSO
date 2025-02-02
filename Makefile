GCC = gcc
CFLAGS = -Wall -Wshadow

all: ex5 realzador publicador combine

ex5: ex5.o bmp.o filter.o
	$(CC) $(CFLAGS) -o ex5 ex5.o bmp.o filter.o

realzador: realzador.o bmp.o
	$(CC) $(CFLAGS) -o realzador realzador.o bmp.o

publicador: publicador.o
	$(CC) $(CFLAGS) -lrt -o publicador publicador.o

combine: combine.o bmp.o
	$(CC) $(CFLAGS) -o combine combine.o bmp.o

ex5.o: ex5.c bmp.h filter.h
	$(CC) $(CFLAGS) -c ex5.c

realzador.o: realzador.c bmp.h
	$(CC) $(CFLAGS) -c realzador.c

bmp.o: bmp.c bmp.h
	$(CC) $(CFLAGS) -c bmp.c

filter.o: filter.c filter.h bmp.h
	$(CC) $(CFLAGS) -c filter.c

publicador.o: publicador.c
	$(CC) $(CFLAGS) -c publicador.c

combine.o: combine.c bmp.h
	$(CC) $(CFLAGS) -c combine.c

clean:
	rm -f ex5 realzador publicador combine *.o
