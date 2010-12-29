#include <sys/syscall.h>

#define MESSAGE "hello, world!\n"

extern long int syscall(long int sysno, ...);
extern void print(const char *);
extern void print_ulong(unsigned long n);
extern void do_count(void);
extern unsigned long g_count;

void _start(void)
{
	syscall(SYS_write, 1, MESSAGE, sizeof(MESSAGE)-1);
	print("hello, world by print()\n");
	do_count();
	g_count = 0;
	do_count();
	print_ulong(g_count);
	syscall(SYS_exit, 0);
}
