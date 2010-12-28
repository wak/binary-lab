#include <sys/syscall.h>

#define MESSAGE "hello, world!\n"

extern long int syscall(long int sysno, ...);
extern void print(const char *);

void _start(void)
{
	syscall(SYS_write, 1, MESSAGE, sizeof(MESSAGE)-1);
	print("hello, world by print()\n");
	syscall(SYS_exit, 0);
}
