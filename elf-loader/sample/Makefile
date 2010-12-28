linker := ../rtld.so
CFLAGS := -Wall
CFLAGS-RTLD := -Wl,--dynamic-linker,`realpath $(linker)`
CFLAGS-NO   := -nostdlib -nostartfiles

LIBGCC-DIR := $(shell dirname $(shell gcc -print-libgcc-file-name))

BIN := hello mini mini-ld-linux
CLEANBIN := $(BIN) hello-gdb hello-myglibc hello-uclibc hello-noglibc

all: $(BIN)

remake: clean all

hello:
	$(call reset-linker)
	gcc $(CFLAGS) -o hello hello.c


#
# DEPENDENCIES
#   mini -+- print.so
#         |    +- global.so (yet)
#         |    `- syscall.so
#         |
#         +- global.so (yet)
#         `- syscall.so
#
SOFLAGS := -Wall -nostdlib -nostartfiles -fPIC -shared
%.so: %.c
	gcc $(SOFLAGS) -o $@ $<
syscall.so: syscall.S
	gcc $(SOFLAGS) -o syscall.so syscall.S
print.so: print.c syscall.so global.so
	gcc $(SOFLAGS) -o print.so print.c syscall.so global.so
mini: mini.c syscall.so global.so print.so
	gcc $(CFLAGS) -c -o mini.o mini.c
	gcc $(CFLAGS) $(CFLAGS-NO) $(CFLAGS-RTLD) -o mini \
		 mini.o syscall.so global.so print.so
mini-ld-linux: mini.c syscall.so global.so print.so
	gcc $(CFLAGS) -g $(CFLAGS-NO) -o mini-ld-linux \
		-Wl,--dynamic-linker,/usr/lib64/debug/ld-2.7.so \
		 mini.c syscall.so global.so print.so

run-mini-ld-linux: mini-ld-linux
	LD_LIBRARY_PATH=. ./mini-ld-linux


UCLIBC := /home/wak/src/uClibc-0.9.31
hello-uclibc:
	gcc -Wall \
		-nostdlib -nostartfiles -I$(UCLIBC)/include \
		-include $(UCLIBC)/include/libc-symbols.h \
		-Wl,--dynamic-linker,$(UCLIBC)/lib/ld64-uClibc-0.9.31.so \
		-Wl,-rpath,$(UCLIBC)/lib \
		-o hello-uclibc hello.c \
		$(UCLIBC)/lib/libc.so.0 $(UCLIBC)/lib/crt1.o

## GDB settings
# define ldso
#   cd ~/src/libc6-dbg/glibc-2.7/glibc-2.7
#   directory ~/src/libc6-dbg/glibc-2.7/glibc-2.7
# end
# ldso
hello-gdb:
	gcc -g -Wall \
		-Wl,--dynamic-linker,/usr/lib64/debug/ld-2.7.so \
		-o hello-gdb hello.c

hello-myglibc:
	gcc -g -Wall \
		-Wl,--dynamic-linker,/tmp/glibc-obj/elf/ld.so \
		-o hello-myglibc hello.c

clean:
	$(Q)rm -f $(CLEANBIN) *.o *.s *.so
