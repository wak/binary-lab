#ifndef ANALYZE_H
#define ANALYZE_H

#include <stdlib.h>
#include <elf.h>
#include "list.h"

extern void *elf_start;
extern void *elf_end;

extern struct list_head segments;
extern struct list_head sections;

typedef struct {
	unsigned long index;
	Elf64_Phdr *phdr;
	void *segment;
	struct list_head segment_sections;
	struct list_head list;
} Segment;

typedef struct {
	unsigned long index;
	Elf64_Shdr *shdr;
	void *section;
	Segment *segment;
	char *name;
	struct list_head segment_sections;
	struct list_head list;
} Section;

#define EHDR ((Elf64_Ehdr *) elf_start)

static inline Elf64_Shdr *SHDR(unsigned long n)
{
	if (EHDR->e_shoff == 0)
		return NULL;
	return (elf_start + EHDR->e_shoff + EHDR->e_shentsize * n);
}
static inline Elf64_Phdr *PHDR(unsigned long n)
{
	if (EHDR->e_phoff == 0)
		return NULL;
	return (elf_start + EHDR->e_phoff + EHDR->e_phentsize * n);
}

void analyze(void);

Segment *find_segment_by_type(Elf64_Word p_type);
Segment *find_segment_by_offset(Elf64_Off offset);
Segment *find_segment_by_vaddr(Elf64_Addr vaddr);
Section *find_segsection_by_vaddr(Segment *seg, Elf64_Addr vaddr);

#endif /* ANALYZE_H */
