CC=gcc
CFLAGS=-c -g

TARGET=main wc map

all: $(TARGET) clean

$(TARGET): $(TARGET).c $(TARGET).o 
	$(CC) $(CFLAGS) $@.c
	$(CC) $@.o -o $@
	
clean:
	rm *.o

cleanexe:
	rm main map wc

