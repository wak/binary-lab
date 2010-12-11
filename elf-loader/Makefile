
CFLAGS := -Wall -fPIC -nostdlib -nostartfiles
# -I/home/wak/src/newlib-1.18.0/newlib/libc/include
#-fno-exceptions

loader.src := loader.c base.S
loader.obj := loader.o base.o

LIBGCC-DIR := $(shell dirname $(shell gcc -print-libgcc-file-name))
GLIB := libgcc.a libgcc_eh.a libgcov.a libgfortran.a \
		libgfortranbegin.a libgomp.a libstdc++.a libsupc++.a
GLIB := $(foreach i,$(GLIB),$(LIBGCC-DIR)/$i)

loader.so: $(loader.obj)
	gcc -nostdlib -nostartfiles -fPIC -shared -o loader.so  $(loader.obj)  \
		-Wl,-z,combreloc -Wl,-z,relro -Wl,--hash-style=both -Wl,-z,defs    \
		-Wl,-soname=wak-ld-linux.so -T ld.lds

base.o: base.S
	gcc --verbose $(CFLAGS) -c -o base.o base.S

loader.o: loader.c
	gcc $(CFLAGS) -c -o loader.o loader.c

#/home/wak/src/newlib-1.18.0/newlib/libc.a
clean:
	rm -f loader $(loader.obj)

loader2: $(loader.obj)
	ld  -static  -o loader $(loader.obj) \
		-L$(LIBGCC-DIR) \
		-L/lib \
		-L/usr/lib \
		-lc -lgcc -lgcc_eh -lc



ld.lds:
	gcc -nostdlib -nostartfiles -shared       \
		-Wl,-z,combreloc -Wl,-z,relro -Wl,--hash-style=both -Wl,-z,defs -Wl,--verbose 2>&1 |  \
		LC_ALL=C \
		sed -e '/^=========/,/^=========/!d;/^=========/d'    \
			-e 's/\. = 0 + SIZEOF_HEADERS;/& _begin = . - SIZEOF_HEADERS;/' \
		> ld.lds
ld.so:
	gcc -B/tmp/obj/   -nostdlib -nostartfiles -shared -o /tmp/obj/elf/ld.so   \
		-Wl,-z,combreloc -Wl,-z,relro -Wl,--hash-style=both -Wl,-z,defs       \
		/tmp/obj/elf/librtld.os -Wl,--version-script=/tmp/obj/ld.map          \
		-Wl,-soname=ld-linux-x86-64.so.2 -T /tmp/obj/elf/ld.so.lds

# /usr/lib/gcc/x86_64-linux-gnu/4.3.2/../../../../lib/crt1.o
# /usr/lib/gcc/x86_64-linux-gnu/4.3.2/../../../../lib/crti.o
# /usr/lib/gcc/x86_64-linux-gnu/4.3.2/crtbeginT.o

# --start-group
# -lgcc
# -lgcc_eh
# -lc
# --end-group
# /usr/lib/gcc/x86_64-linux-gnu/4.3.2/crtend.o
# /usr/lib/gcc/x86_64-linux-gnu/4.3.2/../../../../lib/crtn.o
# jan%

