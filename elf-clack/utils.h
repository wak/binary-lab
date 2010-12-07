#ifndef UTILS_H
#define UTILS_H

#include <elf.h>
#include <stdlib.h>

#include "analyze.h"

#define PRINT_FULL(v, e)				\
	printf("    %-8s = %#lx\n", #e, v->e);
#define PRINT_HALF(v, e)				\
	printf("    %-8s = %#x\n", #e, v->e);

const char *segname(Elf64_Phdr *phdr);
void *emalloc(size_t size);

void startup(int argc, char **argv);
void endup(void);
void print_segment(Segment *segment);
void print_section(Section *section);
void print_ehdr(Elf64_Ehdr *ehdr);
void print_phdr(Elf64_Phdr *phdr);

#endif /* UTILS_H */
