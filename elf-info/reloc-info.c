#include <stdio.h>
#include <string.h>
#include <elf.h>
#include "util.h"

typedef Elf64_Ehdr Ehdr;
typedef Elf64_Phdr Phdr;
typedef Elf64_Shdr Shdr;
typedef Elf64_Rela Rela;
typedef Elf64_Sym Sym;
typedef Elf64_Lib Lib;
typedef Elf64_Dyn Dyn;

Elf64_Shdr *find_shdr_by_name(Elf64_Ehdr *ehdr, const char *secname)
{
	int i;
	Elf64_Shdr *shdr;
	const char *shstr;
	void *base = ehdr;

	shdr = base + ehdr->e_shoff;
	shstr = base + shdr[ehdr->e_shstrndx].sh_offset;

	for (i = 0; i < ehdr->e_shnum; i++) {
		if (strcmp(secname, shstr + shdr[i].sh_name) == 0)
			return &shdr[i];
	}
	return NULL;
}

void *find_sec_by_name(Elf64_Ehdr *ehdr, const char *secname)
{
	void *base = ehdr;
	Elf64_Shdr *shdr;

	shdr = find_shdr_by_name(ehdr, secname);
	if (shdr)
		return base + shdr->sh_offset;
	return NULL;
}

void print_strtab(Ehdr *ehdr, const char *secname)
{
	int i, print_index, index;
	const Shdr *shdr = find_shdr_by_name(ehdr, secname);
	const char *shstr;

	printf("String Table (%s):\n", secname);
	if (!shdr) {
		printf(" not found\n");
		return;
	}
	if (shdr->sh_type != SHT_STRTAB) {
		printf(" this section not SHT_STRTAB\n");
		return;
	}
	index = 0;
	print_index = 1;
	shstr = (void *)ehdr + shdr->sh_offset;
	for (i = 0; i < shdr->sh_size; i++) {
		if (print_index) {
			printf("  [%2d]: ", index);
			print_index = 0;
		}
		if (shstr[i])
			putchar(shstr[i]);
		else {
			printf("\n");
			index++;
			print_index = 1;
		}
	}
}
const char *rel_type_to_str(Elf64_Word type)
{
	switch (type) {
#define E(type) case R_X86_64_##type: return "R_X86_64_"#type; break
		E(NONE); E(64); E(PC32); E(GOT32); E(PLT32); E(COPY);
		E(GLOB_DAT); E(JUMP_SLOT); E(RELATIVE); E(GOTPCREL);
#undef E
	default:
		return "unknown";
	}

}

char *find_symstr(char *symstr, size_t index)
{
	int i, c;

	c = 0;
	for (i = 0; c < index; i++) {
		if (!symstr[i])
			c++;
	}
	return symstr + i;
}
void print_rela(Ehdr *ehdr, char *secname, char *s_symtab, char *s_symstr)
{
	Shdr *rela_shdr   = find_shdr_by_name(ehdr, secname);
	Shdr *symtab_shdr = find_shdr_by_name(ehdr, s_symtab);
	Shdr *symstr_shdr = find_shdr_by_name(ehdr, s_symstr);
	Sym *symtab = NULL;
	Rela *rela;
	char *symstr;
	int i;

	printf("Relocation Information (rela) (%s): \n", secname);
	if (!rela_shdr   || rela_shdr->sh_type   != SHT_RELA   ||
	    !symtab_shdr || symtab_shdr->sh_type != SHT_DYNSYM ||
	    !symstr_shdr || symstr_shdr->sh_type != SHT_STRTAB) {
		puts("  BAD ARGUMENT");
		return;
	}
	symtab = (void *)ehdr + symtab_shdr->sh_offset;
	symstr = (void *)ehdr + symstr_shdr->sh_offset;

	printf("  Virt Address: %lx\n", rela_shdr->sh_addr);
	rela = (void *)ehdr + rela_shdr->sh_offset;
	printf("  %12s  %12s %20s %12s %s %3s + %s\n",
	       "Offset", "Info", "Type", "Sym.Value", "NDX", "Sym.Name", "Addend");
	for (i = 0; i < rela_shdr->sh_size/sizeof(Rela); i++) {
		Sym *sym = NULL;

		sym = &symtab[ELF64_R_SYM(rela[i].r_info)];
		printf("  %012lx  %12lx %20s %012lx %3d %s + %lx\n",
		       rela[i].r_offset,
		       rela[i].r_info,
		       rel_type_to_str(ELF64_R_TYPE(rela[i].r_info)),
		       sym->st_value,
		       sym->st_shndx,
		       &symstr[sym->st_name],
		       rela[i].r_addend);
	}
}

inline char *find_shdr_name(Ehdr *ehdr, Shdr *shdr)
{
	void *base = ehdr;
	Shdr *shdr_top = base + ehdr->e_shoff;
	Shdr *strtab_shdr = &shdr_top[ehdr->e_shstrndx];
	char *strtab = base + strtab_shdr->sh_offset;
	return &strtab[shdr->sh_name];
}
inline void *find_shdr_data(Ehdr *ehdr, Shdr *shdr)
{
	void *base = ehdr;
	return base + shdr->sh_offset;
}
inline void *find_phdr_data(Ehdr *ehdr, Phdr *phdr)
{
	void *base = ehdr;
	return base + phdr->p_offset;
}

void print_dynamic_entry(Ehdr *ehdr, Shdr *dyn_shdr)
{
	int i;
	void *data = find_shdr_data(ehdr, dyn_shdr);
	Dyn *dyn;
	char *symstr;

	int skipping = 0;

	symstr = find_sec_by_name(ehdr, ".dynstr");
	printf("Dynamic section: %s\n", find_shdr_name(ehdr, dyn_shdr));
	for (i = 0; i < dyn_shdr->sh_size / dyn_shdr->sh_entsize; i++) {
		dyn = data + dyn_shdr->sh_entsize * i;
		if (dyn->d_tag == DT_NEEDED) {
			if (skipping)
				putchar('\n');
			skipping = 0;
			printf("  DT_NEEDED: %s\n",
			       symstr ? &symstr[dyn->d_un.d_val] : "(unknown)");
		} else {
			if (!skipping)
				printf("  skip: ");
			putchar('.');
			skipping = 1;
		}
	}
	if (skipping)
		putchar('\n');
}

void print_dynamic_phdr_sub(Ehdr *ehdr, Phdr *phdr)
{
	int i;
	Dyn *dyn = find_phdr_data(ehdr, phdr);

	printf("----\n");
	for (i = 0; i < phdr->p_filesz / sizeof(Dyn); i++) {
		printf("%10lx ", dyn[i].d_tag);
		if (dyn[i].d_tag != DT_RELA) {
			puts("skip");
			continue;
		}
		printf("DT_RELA %lx\n", dyn[i].d_un.d_ptr);
	}
/* 	rela = (void *)ehdr + rela_shdr->sh_offset; */
/* 	printf("  %12s  %12s %20s %12s %s + %s\n", */
/* 	       "Offset", "Info", "Type", "Sym.Value", "Sym.Name", "Addend"); */
/* 	for (i = 0; i < rela_shdr->sh_size/sizeof(Rela); i++) { */
/* 		Sym *sym = NULL; */

/* 		sym = &symtab[ELF64_R_SYM(rela[i].r_info)]; */
/* 		printf("  %012lx  %12lx %20s %012lx %s + %lx\n", */
/* 		       rela[i].r_offset, */
/* 		       rela[i].r_info, */
/* 		       rel_type_to_str(ELF64_R_TYPE(rela[i].r_info)), */
/* 		       sym->st_value, */
/* 		       &symstr[sym->st_name], */
/* 		       rela[i].r_addend); */
/* 	} */
}

void print_dynamic_phdr(Ehdr *ehdr)
{
	int i;
	Phdr *phdr = (void *) ehdr + ehdr->e_phoff;

	for (i = 0; i < ehdr->e_phnum; i++)
		if (phdr[i].p_type == PT_DYNAMIC)
			print_dynamic_phdr_sub(ehdr, &phdr[i]);
}

void print_dynamic(Ehdr *ehdr)
{
	int i;
	void *base = ehdr;
	Shdr *shdr;

	shdr = base + ehdr->e_shoff;
	for (i = 0; i < ehdr->e_shnum; i++, shdr++) {
		if (shdr->sh_type == SHT_DYNAMIC) {
			print_dynamic_entry(ehdr, shdr);
		}
	}
}

int main(int argc, char **argv)
{
	Elf64_Ehdr *ehdr;
	void *base;

	if (argc < 2)
		die("usage: reloc-info binfile");
	ehdr = base = map_file(argv[1]);

	Elf64_Shdr *shdr;
	shdr = base + ehdr->e_shoff;

	printf("shstrndx = %d\n", ehdr->e_shstrndx);
	/* .symtab, .dynsym: struct Elf_Sym */
	/* .strtab, .dynstr: string table */
	print_strtab(ehdr, ".shstrtab");     /* section name table */
	print_strtab(ehdr, ".strtab");	     /* string table */
	print_strtab(ehdr, ".dynstr");	     /* dynamic string table */

	if (0) {
		int i;
		for (i = 0; i < ehdr->e_shnum; i++) {
			printf("Section[%d]: %s\n",
			       i, find_shdr_name(ehdr, &shdr[i]));
		}
	}

	print_rela(ehdr, ".rela.dyn", ".dynsym", ".dynstr"); /* dynamic relocation */
	print_rela(ehdr, ".rela.plt", ".dynsym", ".dynstr"); /* for PLT relocation */
//	print_rela(ehdr, ".rela.text");

	print_dynamic(ehdr);		     /* .dynamic section */
	print_dynamic_phdr(ehdr);

	return 0;
}
