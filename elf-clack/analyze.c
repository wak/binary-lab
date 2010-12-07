#include <stdio.h>

#include "analyze.h"
#include "utils.h"

LIST_HEAD(sections);
LIST_HEAD(segments);

void *elf_start = NULL;
void *elf_end = NULL;

Segment *find_segment_by_type(Elf64_Word p_type)
{
	Segment *segment;

	list_for_each_entry(segment, &segments, list) {
		if (segment->phdr->p_type == p_type)
			return segment;
	}
	return NULL;
}

Segment *find_segment_by_offset(Elf64_Off offset)
{
	Segment *segment;

	list_for_each_entry(segment, &segments, list) {
		Elf64_Phdr *phdr = segment->phdr;
		if (phdr->p_offset <= offset && offset < phdr->p_offset+phdr->p_filesz)
			return segment;
	}
	return NULL;
}

Segment *find_segment_by_vaddr(Elf64_Addr vaddr)
{
	Segment *segment;

	list_for_each_entry(segment, &segments, list) {
		Elf64_Phdr *phdr = segment->phdr;
		if (phdr->p_vaddr <= vaddr && vaddr < phdr->p_vaddr + phdr->p_memsz)
			return segment;
	}
	return NULL;
}

Section *find_segsection_by_vaddr(Segment *seg, Elf64_Addr vaddr)
{
	Section *section;

	list_for_each_entry(section, &seg->segment_sections, segment_sections) {
		Elf64_Shdr *shdr = section->shdr;
		if (shdr->sh_addr <= vaddr && vaddr < shdr->sh_addr + shdr->sh_size)
			return section;
	}
	return NULL;
}

static void init_sections(void)
{
	unsigned long i;
	char *p = NULL;

	if (EHDR->e_shstrndx != SHN_UNDEF)
		p = elf_start + SHDR(EHDR->e_shstrndx)->sh_offset;
	for (i = 0; i < EHDR->e_shnum; i++) {
		Section *newp = emalloc(sizeof(Section));
		newp->shdr = SHDR(i);
		newp->index = i;
		INIT_LIST_HEAD(&newp->segment_sections);
		if (newp->shdr->sh_offset == 0) {
			newp->segment = NULL;
			newp->section = NULL;
		} else {
			newp->section = elf_start + newp->shdr->sh_offset;
			// this not good. because section in many segments.
			newp->segment = find_segment_by_offset(newp->shdr->sh_offset);
		}
		newp->name = NULL;
		if (p)
			newp->name = p + newp->shdr->sh_name;
		list_add_tail(&newp->list, &sections);
	}	
}

static void init_segments_early(void)
{
	unsigned long i;

	for (i = 0; i < EHDR->e_phnum; i++) {
		Segment *newp = emalloc(sizeof(Segment));
		newp->phdr = PHDR(i);
		newp->index = i;
		newp->segment = elf_start + newp->phdr->p_offset;
		INIT_LIST_HEAD(&newp->segment_sections);
		list_add_tail(&newp->list, &segments);
	}	
}

static void init_segments(void)
{
	Section *section;

	list_for_each_entry(section, &sections, list) {
		if (!section->segment)
			continue;
		list_add_tail(&section->segment_sections, &section->segment->segment_sections);
	}
}

void analyze(void)
{
	init_segments_early();
	init_sections();
	init_segments();
}
