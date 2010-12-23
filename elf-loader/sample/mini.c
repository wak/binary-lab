#include <sys/syscall.h>

#define MESSAGE "hello, world!\n"

extern long int syscall(long int sysno, ...);

void _start(void)
{
	syscall(SYS_write, 1, MESSAGE, sizeof(MESSAGE)-1);
	syscall(SYS_exit, 0);
}
