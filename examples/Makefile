ERROR_FLAGS = -std=gnu99 -Wall -Werror
CFLAGS = $(ERROR_FLAGS) -D_REENTRANT -DLinux -D_GNU_SOURCE -O0 -g
HEADER_DIR=../include
LIB_DIR=../

all: mem-bdw-top example

mem-bdw-top: mem-bdw-top.c
	gcc $(CFLAGS) -c mem-bdw-top.c -I$(HEADER_DIR)
	gcc $(CFLAGS) -o mem-bdw-top mem-bdw-top.o -L$(LIB_DIR) -lnumap

example: example.c
	gcc $(CFLAGS) -c example.c -I$(HEADER_DIR)
	gcc $(CFLAGS) -o example example.o -L$(LIB_DIR) -lnumap -lpthread

clean:
	rm -f *.o *.a *.so* example mem-bdw-top
