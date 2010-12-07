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

#define MESSAGE  "hello, ELF!\n"

void crack_code(void *str, void (*f)(void));

void do_crack(Segment *seg)
{
	void *segp = seg->segment;

	((char *)segp)[0] = 0x50; // pushq %rax
	((char *)segp)[1] = 0x52; // pushq %rdx
	((char *)segp)[2] = 0x57; // pushq %rdi
	segp += 3;

	// pushq length
	((char *)segp)[0] = 0x6a;
	((char *)segp)[1] = (char)(sizeof(MESSAGE)-1);
	segp += 2;

	// mov "string", %rdi
	((char *)segp)[0] = 0x48;
	((char *)segp)[1] = 0xc7;
	((char *)segp)[2] = 0xc7;
	segp += 3;
	((uint32_t *)segp)[0] = (uint32_t)(seg->phdr->p_vaddr + seg->phdr->p_filesz - sizeof(MESSAGE));
	segp += sizeof(uint32_t);

	// mov _start, %rsi
	((char *)segp)[0] = 0x48;
	((char *)segp)[1] = 0xc7;
	((char *)segp)[2] = 0xc6;
	segp += 3;
	((uint32_t *)segp)[0] = (uint32_t)EHDR->e_entry;
	segp += sizeof(uint32_t);

	memcpy(segp, (void *)crack_code, 100);

	// Insert string to end of segment
	memcpy(seg->segment + seg->phdr->p_filesz - sizeof(MESSAGE),
	       MESSAGE, sizeof(MESSAGE));

	EHDR->e_entry = seg->phdr->p_vaddr;
}

void *target_start = NULL;
void *my_memcpy(const char *hint, void *dst, void *src, size_t size)
{
	printf("[%-8s] memcpy(%#6lx, %#6lx, %#lx)\n",
	       hint,
	       dst-target_start, src-elf_start, size);
	return memcpy(dst, src, size);
}

/*
 * New Layout
 *
 *   ELF Header
 *   ( Original PHDR Space)
 *   Original Segments & Sections
 *   New Segment
 *   SHDR
 *   New PHDR
 */
void dump_elf(void *start, Segment *newseg)
{
	Segment *seg;
	void *phdr_addr, *shdr_addr;

	
	/*
	  // I try to add SHDR for new segment,
	  //   because I want `objdump -D' output.
	  // But these codes does not enough.
	  // (need sh_name?)

	  Section sec;
	  Elf64_Shdr sec_shdr;
	  EHDR->e_shnum++;
	  sec_shdr.sh_name = 0;
	  sec_shdr.sh_type = SHT_PROGBITS;
	  sec_shdr.sh_flags = SHF_WRITE | SHF_EXECINSTR;
	  sec_shdr.sh_addr = newseg->phdr->p_vaddr;
	  newseg->phdr->p_offset +=  EHDR->e_shentsize;
	  sec_shdr.sh_offset = newseg->phdr->p_offset;
	  sec_shdr.sh_link = 0;
	  sec_shdr.sh_info = 0;
	  sec_shdr.sh_addralign = 4;
	  sec_shdr.sh_entsize = newseg->phdr->p_memsz;
	  sec.shdr = &sec_shdr;
	*/

	target_start = start;

	phdr_addr = PHDR(0);
	shdr_addr = SHDR(0);
	EHDR->e_shoff = newseg->phdr->p_offset + newseg->phdr->p_filesz;
	EHDR->e_phoff = EHDR->e_shoff + EHDR->e_shentsize * EHDR->e_shnum;

	seg = find_segment_by_type(PT_PHDR);
	if (seg) {
		Elf64_Phdr *phdr = seg->phdr;

		phdr->p_paddr = phdr->p_vaddr  =
			newseg->phdr->p_vaddr - newseg->phdr->p_offset
			+ (EHDR->e_phoff % newseg->phdr->p_align);
		phdr->p_filesz = phdr->p_memsz = EHDR->e_phentsize * EHDR->e_phnum;
		phdr->p_offset = EHDR->e_phoff;
		phdr->p_align  = 0x8;
	}

	// write segments (exclude newseg)
	my_memcpy("copy old", start, elf_start, elf_end-elf_start);
	
	// to map PHDR and SHDR
	newseg->phdr->p_filesz +=
		(EHDR->e_phentsize * EHDR->e_phnum)
		+ (EHDR->e_shentsize * EHDR->e_shnum);
	newseg->phdr->p_memsz = newseg->phdr->p_filesz;

	// write new segment
	my_memcpy("newseg",
		  start+newseg->phdr->p_offset,
		  newseg->segment,
		  newseg->phdr->p_filesz);
	
	// write new EHDR
	my_memcpy("new EHDR", start, EHDR, sizeof(Elf64_Ehdr));

	// write SHDR
	my_memcpy("move SHDR",
		  start + EHDR->e_shoff,
		  shdr_addr,
		  EHDR->e_shentsize * EHDR->e_shnum);
/*
	my_memcpy("SHDR for newseg",
		  start + EHDR->e_shoff + EHDR->e_shentsize * EHDR->e_shnum,
		  &sec_shdr,
		  EHDR->e_shentsize);
*/

	// write PHDR (exclude newseg PHDR)
	my_memcpy("move PHDR",
		  start + EHDR->e_phoff,
		  phdr_addr,
		  EHDR->e_phentsize * (EHDR->e_phnum-1));
	// write newseg PHDR
	my_memcpy("newseg's PHDR",
		  start + EHDR->e_phoff + EHDR->e_phentsize * (EHDR->e_phnum-1),
		  newseg->phdr,
		  EHDR->e_phentsize);

	print_ehdr(EHDR);
}

void create_exe(const char *filename,
		size_t orig_size, size_t add_size,
		Segment *newseg)
{
	int fd;
	char name[100], *p;
	void *addr;
	size_t length;

	length = newseg->phdr->p_offset + newseg->phdr->p_filesz
		+ EHDR->e_shentsize * EHDR->e_shnum
		+ EHDR->e_phentsize * EHDR->e_phnum
		+ EHDR->e_shentsize;
	printf("Allocate %#lx bytes\n", length);

	p = strrchr(filename, '/');
	if (p)
		filename = p+1;
	snprintf(name, sizeof(name), "%s.crack", filename);
	fd = open(name, O_CREAT|O_RDWR|O_TRUNC, S_IXUSR|S_IWUSR|S_IRUSR);
	if (fd < 0) {
		fprintf(stderr, "open failed (%s): %s\n", name, strerror(errno));
		return;
	}
	if (ftruncate(fd, length) < 0) {
		fprintf(stderr, "truncate failed: %s, (fd, %lx)\n",
			strerror(errno), length);
		goto close;
	}
	addr = mmap(NULL, length, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if (addr == MAP_FAILED) {
		fprintf(stderr, "mmap failed: %s, length=%lu\n",
			strerror(errno), length);
		goto close;
	}
	dump_elf(addr, newseg);
	if (munmap(addr, length) < 0) {
		fprintf(stderr, "munmap failed: %s\n", strerror(errno));
		goto close;
	}
	printf("create %s\n", name);
close:
	if (close(fd) < 0)
		fprintf(stderr, "close failed: %s\n", strerror(errno));
}

Segment *add_new_segment(size_t *segment_size)
{
	Segment *seg;
	Section *sec;
	Elf64_Phdr *phdr;
	Elf64_Addr new_addr = 0;
	Elf64_Addr new_offset = 0;
	size_t size = 0;

	list_for_each_entry(seg, &segments, list) {
		Elf64_Phdr *phdr = seg->phdr;
		if (new_addr < phdr->p_vaddr + phdr->p_memsz)
			new_addr = phdr->p_vaddr + phdr->p_memsz;
		if (new_offset < phdr->p_offset + phdr->p_filesz)
			new_offset = phdr->p_offset + phdr->p_filesz;
	}
	list_for_each_entry(sec, &sections, list) {
		Elf64_Shdr *shdr = sec->shdr;
		if (new_offset < shdr->sh_offset + shdr->sh_size)
			new_offset = shdr->sh_offset + shdr->sh_size;
	}
	new_addr += 0x200000 - (new_addr % 0x200000);
	size += 0x200 - (new_offset % 0x200);
	new_offset += 0x200 - (new_offset % 0x200);

	size += *segment_size;
	seg = emalloc(sizeof(Segment));
	seg->segment = emalloc(*segment_size);
	seg->index   = EHDR->e_phnum++;
	INIT_LIST_HEAD(&seg->segment_sections);
	INIT_LIST_HEAD(&seg->list);

	phdr = seg->phdr = emalloc(sizeof(Elf64_Phdr));
	phdr->p_vaddr  = phdr->p_paddr = new_addr + new_offset;
	phdr->p_filesz = *segment_size;
	phdr->p_memsz  = *segment_size;
	phdr->p_offset = new_offset;
	phdr->p_align  = 0x200000;
	phdr->p_type   = PT_LOAD;
	phdr->p_flags  = (PF_R | PF_X);

	list_add_tail(&seg->list, &segments);
	print_segment(seg);
	
	*segment_size = size;
	return seg;
}

int main(int argc, char **argv)
{
	size_t new_size;
	Segment *segment;
	
	new_size = 1024;
	startup(argc, argv);
	analyze();
	segment = add_new_segment(&new_size);
	do_crack(segment);
	create_exe(argv[1],
		   elf_end - elf_start,
		   new_size, segment);
	endup();
	return 0;
}
