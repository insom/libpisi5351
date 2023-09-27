.PHONY: all fmt
all: fmt ft8 wspr

fmt: *.c *.h
	clang-format -i *.c *.h

ft8_lib/libft8.a:
	(cd ft8_lib && make lib)

common.o: common.c
	gcc -c common.c

libpigpio.a:
	(cd pigpio && make)
	ar rc libpigpio.a pigpio/*.o

ft8: ft8.c ft8_lib/libft8.a common.o libpigpio.a
	gcc -c ft8.c
	gcc -o ft8 common.o ft8.o ft8_lib/libft8.a libpigpio.a

wspr: wspr.c common.o libpigpio.a
	gcc -c wspr.c
	gcc -o wspr common.o wspr.o libpigpio.a

.PHONY: clean
clean:
	(cd ft8_lib && make clean)
	rm -f test t w wspr ft8 *.o
