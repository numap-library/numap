ERROR_FLAGS = -std=gnu99 -Wall -Werror
CFLAGS = $(ERROR_FLAGS) -D_REENTRANT -DLinux -D_GNU_SOURCE -O0 -g
INSTALL_PATH?=/usr/local

all:
	$(MAKE) -C src
	$(MAKE) -C examples

install: 
	cp -f include/numap.h /usr/include/.
	cp -f numap.so /usr/lib/. 

clean:
	$(MAKE) -C src clean
	$(MAKE) -C examples clean
