linker := ../rtld.so
CFLAGS := -Wall -Wl,--dynamic-linker,`realpath $(linker)`

hello:
	$(call reset-linker)
	gcc $(CFLAGS) -o hello hello.c

clean:
	$(Q)rm -f hello
