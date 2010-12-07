CFLAGS := -g -Wall -Xlinker --dynamic-linker -Xlinker /usr/lib64/debug/ld-2.7.so
MYLIB := utils.o analyze.o
HEADER := utils.h analyze.h
BIN := show crack runner hello hello-static mini

all: $(BIN)
show: show.o $(MYLIB)
crack: crack.o crack_code.o $(MYLIB)
hello-static: hello.c
	gcc -o hello-static $(CFLAGS) --static hello.c

mini: mini.o
	ld -o mini mini.o

utils.o: utils.c $(HEADER)
analyze.o: analyze.c $(HEADER)

runmini: crack mini
	./crack mini && echo '---------' && ./mini.crack

runhello: crack hello
	./crack hello && echo '---------' &&  ./hello.crack

runhello-static: crack hello-static
	./crack hello-static && echo '---------' &&  ./hello-static.crack

clean:
	rm -f *.o
	rm -f *.crack
	rm -f $(BIN)
	rm -f elf_crack.tar.gz elf_crack.zip

archive:
	git archive --prefix='elf_crack/' --format=tar master | gzip > elf_crack.tar.gz

zip:
	git archive --prefix='elf_crack/' --format=zip master > elf_crack.zip
