#include <unistd.h>
#include <sys/syscall.h>

void print(const char *line)
{
	int i;

	for (i = 0; line[i]; i++)
		;
	syscall(SYS_write, 1, line, i);
}
