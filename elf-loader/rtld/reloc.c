#include <elf.h>
#include <lib.h>
#include <ldsodefs.h>
#include <lookup.h>

extern void runtime_resolve(ElfW(Word)) rtld_local;

void *got_fixup(struct link_map *l, ElfW(Word) reloc_offset)
{
	Elf64_Addr *got;
	ElfW(Rela) *jmprel, *rela;
	ElfW(Xword) pltrelsz;
	ElfW(Sym) *symtab, *sym;
	ElfW(Word) symidx;
	const char *strtab, *name;
	struct sym_val symval;
	void *jmp;

	mprint_start_fmt("GOT Trampline (%s, off:%u)",
			 l->l_name, reloc_offset);
	got      = (Elf64_Addr *) D_PTR(l, l_info[DT_PLTGOT]);
	jmprel   = (void *) D_PTR(l, l_info[DT_JMPREL]);
	pltrelsz = D_VAL(l, l_info[DT_PLTRELSZ]);
	rela = &jmprel[reloc_offset];

//	dprintf("%d %d\n", pltrelsz, sizeof(ElfW(Rela)));
	assert(pltrelsz/sizeof(ElfW(Rela)) > reloc_offset);

	symidx = ELF64_R_SYM(rela->r_info);
	symtab = (void *) D_PTR(l, l_info[DT_SYMTAB]);
	strtab = (void *) D_PTR(l, l_info[DT_STRTAB]);

	sym = &symtab[symidx];
	name = &strtab[sym->st_name];

	mprintf("name: %s\n", name);
	if (lookup_symbol(name, &symval) != 0)
		dprintf_die("symbol %s not found\n", name);
	jmp = (void *) (symval.m->l_addr + symval.s->st_value);
	mprintf("symbol found: in %s, val:%#x => %p\n",
		symval.m->l_name, symval.s->st_value, jmp);
//	dprintf_die("got_fixup %s, %d\n", l->l_name, reloc_offset);

	got[3 + reloc_offset] = (Elf64_Addr) jmp;
	mprint_end();

	return jmp;
}
HIDDEN(got_fixup);

static void reloc_rela(struct link_map *l, ElfW(Rela) *rela, unsigned long count)
{
	for (; count-- > 0; rela++) {
		unsigned long *reloc = (void *) (l->l_addr + rela->r_offset);
		switch (ELF64_R_TYPE(rela->r_info)) {
		case R_X86_64_RELATIVE:
			dprintf("  reloc RELATIVE *%p(%lx) += %lx + %lx\n",
				reloc, *reloc, l->l_addr, rela->r_addend);
			*reloc += l->l_addr + rela->r_addend;
			break;
		case R_X86_64_JUMP_SLOT:
			dprintf("  reloc JUMP_SLOT *%p(%lx) += %lx + %lx\n",
				reloc, *reloc, l->l_addr, rela->r_addend);
			*reloc += l->l_addr + rela->r_addend;
			break;
		default:
			dprintf_die("  unknown reloc type (%lx)\n",
				    ELF64_R_TYPE(rela->r_info));
			break;
		}
	}
}

/* REF: _dl_relocate_object [glibc/elf/dl-reloc.c] */
static int relocate_object(struct link_map *l)
{

#define D(name) (l->l_info[DT_##name])
#define D_INFO_VAL(name) (l->l_info[DT_##name]->d_un.d_val)
#define D_INFO_PTR(name) (l->l_info[DT_##name]->d_un.d_ptr + l->l_addr)
//	const char *strtab = (const void *) D_INFO_PTR(STRTAB);

	mprint_start_fmt("RELOCAION (%s)", l->l_name);
	assert("strtab");

	if (D(RELA) != NULL) {
		if (D(RELAENT))
			assert(D_INFO_VAL(RELAENT) == sizeof(ElfW(Rela)));
		assert(D(RELASZ) != NULL);
		//dprintf("base:   %p, ptr: %lx\n", l->l_addr, D(RELA)->d_un.d_ptr);
		reloc_rela(l,
			   (ElfW(Rela)*) D_INFO_PTR(RELA),
			   D_INFO_VAL(RELASZ) / sizeof(ElfW(Rela)));
	}

	if (D(PLTGOT) != NULL) {
		Elf64_Addr *got;
		ElfW(Addr) jmprel;
		ElfW(Xword) pltrel, pltrelsz;

		assert(D(PLTREL) != NULL);
		assert(D(PLTRELSZ) != NULL);
		got      = (Elf64_Addr *) D_INFO_PTR(PLTGOT);
		jmprel   = D_INFO_PTR(JMPREL);
		pltrel   = D_INFO_VAL(PLTREL);
		pltrelsz = D_INFO_VAL(PLTRELSZ);
		dprintf("  GOT address: %p\n", (void *) got);
		dprintf("       JMPREL: %p (Address of PLT relocs. [.rela.plt])\n",
			(void *) jmprel);
		dprintf("       PLTREL: %#lx (Type of reloc in PLT)\n", pltrel);
		dprintf("     PLTRELSZ: %#lx bytes (size in bytes of PLT relocs)\n",
			pltrelsz);
		if (pltrel != DT_RELA)
			dprintf_die("PLTREL type is not DT_RELA. not supported\n");
		reloc_rela(l, (void *) jmprel, pltrelsz / sizeof(ElfW(Rela)));

		got[1] = (Elf64_Addr) l;  /* Identify this shared object.  */
		got[2] = (Elf64_Addr) &runtime_resolve;
	}
	mprint_end();
	return 0;
#undef D_INFO_PTR
#undef D_INFO_VAL
}
HIDDEN(relocate_object);

void reloc_all(void)
{
	struct link_map *l;

	l = GL(namespace);
	assert(l != NULL);
	while (l->l_next)
		l = l->l_next;
	while (l) {
		relocate_object(l);
		l = l->l_prev;
	}
}
HIDDEN(reloc_all);
