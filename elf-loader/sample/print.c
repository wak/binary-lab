#include <unistd.h>
#include <sys/syscall.h>

static int reloc = 0;
static int *relocp = &reloc;

//extern unsigned long so_global;

void print(const char *line)
{
	int i;

//	so_global = 1;
	for (i = 0; line[i]; i++)
		;
	syscall(SYS_write, 1, line, i);
}
