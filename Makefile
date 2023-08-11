LDFLAGS=-lpigpio
CFLAGS=-Wall
CC=gcc

.PHONY: all
all: t

t: test.c
	gcc -c test.c
	gcc -o t test.o -lpigpio

.PHONY: clean
clean:
	rm -f test t
