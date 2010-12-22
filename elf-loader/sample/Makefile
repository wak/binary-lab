linker := ../rtld.so
CFLAGS := -Wall -Wl,--dynamic-linker,`realpath $(linker)`

BIN := hello

all: $(BIN)

remake: clean all

hello:
	$(call reset-linker)
	gcc $(CFLAGS) -o hello hello.c


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
	$(Q)rm -f $(BIN)
