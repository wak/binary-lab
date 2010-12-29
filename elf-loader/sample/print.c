#include <unistd.h>
#include <sys/syscall.h>

static int reloc = 0;
static int *relocp = &reloc;

//extern unsigned long so_global;
extern unsigned long g_count;

void print(const char *line)
{
	int i;

//	so_global = 1;
	for (i = 0; line[i]; i++)
		;
	syscall(SYS_write, 1, line, i);
}

void do_count(void)
{
	int i, j, dig, len;
	char buffer[100]="current: ";
	char *s = buffer + 9;
	unsigned long n;

	g_count++;
	n = g_count++;
//	n = 0xABCDEF;
	i = 0;
	do {
		s[i] = (dig = (n & 0xf)) > 9?
			'a' + dig - 10 :
			'0' + dig;
		n >>= 4;
		i ++;
	} while (i < sizeof(n)*2 && n);
	len = i;
	for (j = 0; j < --i; j++) {
		int t;
		t = s[i];
		s[i] = s[j];
		s[j] = t;
	}
	s[len] = '\n';
	syscall(SYS_write, 1, buffer, (&s[len]-buffer) + 1);
}
