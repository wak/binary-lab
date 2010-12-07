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

#include "analyze.h"
#include "utils.h"

void print_section_header(void)
{
	Section *section;

	printf("Section Header:\n");
	list_for_each_entry(section, &sections, list) {
		print_section(section);
	}
}

void print_program_header(void)
{
	Segment *segment;

	printf("Program Header:\n");
	list_for_each_entry(segment, &segments, list) {
		print_segment(segment);
	}
}

void show_info(void)
{
	printf("Size of:\n");
	printf("  Elf64_Ehdr = %#lx\n", sizeof(Elf64_Ehdr));
	printf("  Elf64_Phdr = %#lx\n", sizeof(Elf64_Phdr));
	printf("  Elf64_Shdr = %#lx\n\n", sizeof(Elf64_Shdr));
}

void show(void)
{
	analyze();
	
	show_info();
	print_ehdr(elf_start);
	print_section_header();
	print_program_header();
}

int main(int argc, char **argv)
{
	startup(argc, argv);
	show();
	endup();

	return 0;
}
