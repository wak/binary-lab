
CFLAGS := -Wall -g -std=gnu99 -fPIC -nostdlib -nostartfiles

loader.src := loader.c loader.h base.S
loader.obj := loader.o base.o

rebuild: clean loader.so
	readelf -r loader.so

loader.so: $(loader.obj) ld.lds
	gcc -nostdlib -nostartfiles -shared -T ld.lds \
		-Wl,-z,combreloc -Wl,-z,relro -Wl,--hash-style=both \
		-Wl,-z,defs -Wl,-soname=wak-ld-linux.so \
		-o loader.so $(loader.obj)

base.o: base.S
	gcc $(CFLAGS) -c -o base.o base.S

loader.o: loader.c loader.h
	cpp $(CFLAGS) loader.c > loader.cpp
	gcc $(CFLAGS) -S loader.c
	gcc $(CFLAGS) -c -o loader.o loader.c

clean:
	rm -f loader.so loader.s loader.cpp \
		ld.lds $(loader.obj) \
		test.o

ld.lds:
	@echo Generate ld.lds
	@gcc -nostdlib -nostartfiles -shared  \
		-Wl,-z,combreloc -Wl,-z,relro -Wl,--hash-style=both \
		-Wl,-z,defs -Wl,--verbose 2>&1 |  \
		LC_ALL=C \
		sed -e '/^=========/,/^=========/!d;/^=========/d'    \
			-e 's/\. = 0 + SIZEOF_HEADERS;/& _begin = . - SIZEOF_HEADERS;/' \
		> ld.lds

.PHONY: clean rebuild
