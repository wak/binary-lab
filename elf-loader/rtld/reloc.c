#include <elf.h>
#include <loader.h>
#include <lib.h>

static void runtime_resolve(void)
{
	dprintf_die("runtime_resolve CALLED !!!");
}

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
			dprintf("  unknown reloc type (%lx)\n",
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
