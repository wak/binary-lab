#include <unistd.h>
#include <sys/syscall.h>

static int reloc = 0;
static int *relocp = &reloc;

void print(const char *line)
{
	int i;

	for (i = 0; line[i]; i++)
		;
	syscall(SYS_write, 1, line, i);
}
