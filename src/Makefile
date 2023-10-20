CC=clang
CFLAGS=-Wall
CLIBS=-lncurses

all: build run clean

build:
	$(CC) $(CFLAGS) $(CLIBS) main.c -o debug.o

run:
	./debug.o

clean:
	rm -f *.o

release:
	$(CC) -Ofast $(CLIBS) main.c -o todolist.o
