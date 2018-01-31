CC=gcc
CFLAGS=-c -g

TARGET=main wc map

all: $(TARGET)

#see https://www.gnu.org/software/make/manual/make.html#Static-Pattern
$(TARGET): %:%.c
	$(CC) $(CFLAGS) $@.c
	$(CC) $@.o -o $@

cleanall: clean cleanexe

clean:
	-rm *.o

cleanexe:
	-rm main map wc

