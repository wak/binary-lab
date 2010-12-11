#include <elf.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>

#define ElfW(type) struct Elf64_##type

void loader_start(void)
{
//	char buf[199];

//	ElfW(auxv_t) *auxv;

	syscall(SYS_write, 1, "hello, world\n", 13);
}
