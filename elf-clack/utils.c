#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <elf.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <linux/unistd.h>

#include "utils.h"
#include "analyze.h"

void *emalloc(size_t size)
{
	void *p = malloc(size);

	if (!p) {
		fprintf(stderr, "malloc failed\n");
		exit(EXIT_FAILURE);
	}
	return p;
}

const char *segname(Elf64_Phdr *phdr)
{
	Elf64_Word type = phdr->p_type;

	switch (type) {
#define CASE(n) case PT_##n: return #n
	CASE(NULL);
	CASE(LOAD);
	CASE(DYNAMIC);
	CASE(INTERP);
	CASE(NOTE);
	CASE(SHLIB);
	CASE(PHDR);
	CASE(TLS);
	CASE(NUM);
	CASE(LOOS);
	CASE(GNU_EH_FRAME);
	CASE(GNU_STACK);
	CASE(GNU_RELRO);
	CASE(LOSUNW);
	//CASE(SUNWBSS);
	CASE(SUNWSTACK);
	CASE(HISUNW);
	//CASE(HIOS);
	CASE(LOPROC);
	CASE(HIPROC);
#undef CASE
	}
	return "UNKNOWN";
}

void startup(int argc, char **argv)
{
	int fd;
	size_t length;
	void *addr;
	struct stat st;

	if (argc < 2) {
		fprintf(stderr, "too few argments.\n");
		exit(EXIT_FAILURE);
	}
	fd = open(argv[1], O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "open failed: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	if (fstat(fd, &st) < 0) {
		fprintf(stderr, "stat failed: %s\n", strerror(errno));
		goto err_close;
	}
	addr = mmap(NULL, st.st_size, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
	if (addr == MAP_FAILED) {
		fprintf(stderr, "mmap failed: %s, length=%lu\n",
			strerror(errno), length);
		goto err_close;
	}
	close(fd);
	elf_start = addr;
	elf_end = addr + st.st_size;
	return;

err_close:
	close(fd);
	exit(EXIT_FAILURE);
}

void endup(void)
{
	if (munmap(elf_start, elf_end-elf_start) < 0) {
		fprintf(stderr, "munmap failed: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	elf_start = elf_end = NULL;
}

void print_segment(Segment *segment)
{
	Section *section;
	Elf64_Phdr *phdr;

	phdr = segment->phdr;
	printf(" [%lu] %s (%#lx)\n",
	       segment->index, segname(phdr), (void *) phdr - elf_start);
	printf("    Sections: ");
	list_for_each_entry(section, &segment->segment_sections, segment_sections) {
		printf("%s ", section->name ? : "no_name");
	}
	puts("");
	print_phdr(phdr);
}

void print_phdr(Elf64_Phdr *phdr)
{
	PRINT_FULL(phdr, p_offset);
	PRINT_FULL(phdr, p_vaddr);
	PRINT_FULL(phdr, p_paddr);
	PRINT_FULL(phdr, p_filesz);
	PRINT_FULL(phdr, p_memsz);
	PRINT_FULL(phdr, p_align);
}

void print_section(Section *section)
{
	Elf64_Shdr *shdr;

	shdr = section->shdr;
	printf(" [%lu] %s (%#lx)\n",
	       section->index, section->name, (void *) shdr - elf_start);
	if (section->segment)
		printf("    Segment: %s\n", segname(section->segment->phdr));
	PRINT_HALF(shdr, sh_name);
	PRINT_FULL(shdr, sh_addr);
	PRINT_FULL(shdr, sh_offset);
	PRINT_FULL(shdr, sh_size);
	PRINT_FULL(shdr, sh_entsize);
	PRINT_FULL(shdr, sh_addralign);
}

void print_ehdr(Elf64_Ehdr *ehdr)
{
	printf("ELF Header:\n");
	PRINT_HALF(ehdr, e_ehsize);
	PRINT_FULL(ehdr, e_entry);
	puts("");
	PRINT_FULL(ehdr, e_phoff);
	PRINT_HALF(ehdr, e_phentsize);
	PRINT_HALF(ehdr, e_phnum);
	puts("");
	PRINT_FULL(ehdr, e_shoff);
	PRINT_HALF(ehdr, e_shentsize);
	PRINT_HALF(ehdr, e_shnum);
	puts("");
	PRINT_HALF(ehdr, e_shstrndx);
}
