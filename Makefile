LDFLAGS=-lpigpio
CFLAGS=-Wall
CC=gcc

.PHONY: all
all: t w

t: test.c
	gcc -c test.c
	gcc -o t test.o -lpigpio

w: wspr.c
	gcc -c wspr.c
	gcc -o w wspr.o -lpigpio

.PHONY: clean
clean:
	rm -f test t w
