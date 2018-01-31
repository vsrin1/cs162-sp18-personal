CC=gcc
CFLAGS=-c -g

all: main wc map

main: main.o
	$(CC) main.o -o main

main.o:
	$(CC) $(CFLAGS) main.c

wc: wc.o
	$(CC) wc.o -o wc

wc.o:
	$(CC) $(CFLAGS) wc.c

map: map.o
	$(CC) map.o -o map

map.o:
	$(CC) $(CFLAGS) map.c

clean:
	rm *.o main map wc

