BIN = hello

all: $(BIN)
hello: hello.c
	gcc -Wall -O0 -o hello hello.c

test: test.c
	gcc -Wall -Xlinker --dynamic-linker -Xlinker /usr/lib64/debug/ld-2.7.so -o test test.c /usr/lib64/libc.so

clean:
	rm -f $(BIN)
