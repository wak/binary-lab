#include <linux/unistd.h>

#define MESSAGE "hello, world!\n"

void _start(void)
{
	//int ret;

	// print "hello, world"
	__asm__ volatile (
		"movq $1, %%rdi     \n\t"
		"movq %1, %%rsi     \n\t"
		"movq %2, %%rdx     \n\t"
		"syscall            \n\t"
		: //"=a" (ret)
		: "a" (__NR_write),
		  "i" (MESSAGE),
		  "i" (sizeof(MESSAGE)-1)
		: "%rdi", "%rsi", "%rdx" );

	// exit
	__asm__ volatile (
		"movq $0, %%rdi     \n\t"
		"syscall            \n\t"
		: //"=a" (ret)
		: "a" (__NR_exit), "i" (0)
		: "%rdi");
}
