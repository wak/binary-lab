#include <stdio.h>
#include <elf.h>

#define N 100

static Elf64_Addr before[N];
static Elf64_Addr after[N];

extern Elf64_Addr *get_got(void);

void print_addr(const char *prefix, Elf64_Addr *p, size_t size)
{
	int i;
	for (i = 0; i < size; i++)
		printf("%s[%i] = %lx\n", prefix, i, p[i]);
	puts("");
}
void print_got(int size)
{
	Elf64_Addr *GOT;
	int i;

	if (size > N)
		size = N;
	GOT = get_got();
	for (i = 0; i < size; i++)
		before[i] = GOT[i];
	printf("GOT address: %p\n\n", GOT);
	for (i = 0; i < size; i++)
		after[i] = GOT[i];
	print_addr("before", before, size);
	print_addr(" after", after, size);
}
