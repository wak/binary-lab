#include <libc-symbols.h>  // for uClibc-0.9.31 (bug?)
#include <elf.h>
//#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include "loader.h"

#define ElfW(type) struct Elf64_##type

RTLD_START

static void loader_start(void)
{
//	char buf[199];

//	ElfW(auxv_t) *auxv;
//	puts("hello");
//	syscall(SYS_write, 1, "hello, world\n", 13);
	//extern int f(void);
	//f();
	syscall(SYS_exit, 0);
}
