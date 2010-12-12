//#include <libc-symbols.h>  // for uClibc-0.9.31 (bug?)
#include <elf.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include "loader.h"
#include <fcntl.h>

//#define ElfW(type) struct Elf64_##type

#define MESSAGE "Hello, Dynamic Linker and Loader!\n"
#define ERR_EXIT(err) {					\
	  syscall(SYS_write, 2, err, sizeof(err)-1);	\
	  syscall(SYS_exit, 1);				\
  }

static void print_maps(void)
{
	int fd;

	fd = syscall(SYS_open, "/proc/self/maps", O_RDONLY);
	if (fd < 0)
		ERR_EXIT("open failed\n");
	for (;;) {
		char line[1024];
		int r = syscall(SYS_read, fd, line, sizeof(line));
		if (r == 0)
			break;
		if (r < 0)
			ERR_EXIT("read error\n");
		syscall(SYS_write, 1, line, r);
	}
	syscall(SYS_close, fd);
}

void loader_start(void)
{
	print_maps();

	syscall(SYS_write, 1, MESSAGE, sizeof(MESSAGE)-1);
	syscall(SYS_exit, 0);
	for (;;) ;
}
