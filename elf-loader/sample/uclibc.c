#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>

int main(int argc, char **argv, char **envp)
{
	char buf[100];
//	write(1, "hello uclibc\n", 13);
//	fprintf(stderr, "hello, world\n");
	sprintf(buf, "argc: %d\n", argc);
	syscall(SYS_write, 1, buf, strlen(buf));
	syscall(SYS_write, 1, "hello uclibc\n", 13);
	syscall(SYS_exit, 100);
}
