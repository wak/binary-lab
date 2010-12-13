
CFLAGS := -Wall -g -O0 -std=gnu99 -fPIC -nostdlib -nostartfiles 
CFLAGS += -I/home/wak/src/uClibc-0.9.31/include

loader.src := loader.c loader.h base.S hidden.c
loader.obj := loader.o base.o hidden.o
obj += ~/src/uClibc-0.9.31/lib/libc.a

RTLD := ld-wak.so

make: $(RTLD)
	@ruby -e 'puts "","=" * 70'
	readelf -r $(RTLD)

rebuild: clean make

$(RTLD): $(loader.obj) ld.lds
	gcc -nostdlib -nostartfiles -shared -T ld.lds \
		-Wl,-z,combreloc -Wl,-z,relro -Wl,--hash-style=both \
		-Wl,-z,defs -Wl,-soname=$(RTLD) \
		-o $(RTLD) $(loader.obj) $(obj)

base.o: base.S
	gcc $(CFLAGS) -c -o base.o base.S

loader.o: loader.c loader.h
	@cpp $(CFLAGS) loader.c > loader.cpp
	@gcc $(CFLAGS) -S loader.c
	gcc $(CFLAGS) -c -o loader.o loader.c

clean:
	rm -f $(RTLD) loader.s loader.cpp \
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
