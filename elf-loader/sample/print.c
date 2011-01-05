#include <unistd.h>
#include <sys/syscall.h>

extern unsigned long g_count;

void print(const char *line)
{
	int i;

	for (i = 0; line[i]; i++)
		;
	syscall(SYS_write, 1, line, i);
}

void print_ulong(unsigned long n)
{
	int i, j, dig, len;
	char buffer[100]="current: ";
	char *s = buffer + 9;

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

void do_count(void)
{
	print_ulong(g_count++);
}
