.PHONY: all fmt
all: fmt ft8 wspr

fmt: *.c *.h
	clang-format -i *.c *.h

ft8_lib/libft8.a:
	(cd ft8_lib && make lib)

ft8: ft8.c ft8_lib/libft8.a
	gcc -c ft8.c
	gcc -o ft8 ft8.o ft8_lib/libft8.a -lpigpio

wspr: wspr.c
	gcc -c wspr.c
	gcc -o wspr wspr.o -lpigpio

.PHONY: clean
clean:
	(cd ft8_lib && make clean)
	rm -f test t w wspr ft8
