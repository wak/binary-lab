#include <elf.h>
#include <loader.h>
#include <lib.h>

static void reloc_rela(struct link_map *l, ElfW(Rela) *rela, unsigned long count)
{
	p(rela);
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
#define D_INF_PTR(map, name) ((map)->l_info[DT_##name]->d_un.d_ptr + (map)->l_addr)
	const char *strtab = (const void *) D_INF_PTR(l, STRTAB);

	print_mark("RELOCAION");
	assert("strtab");

	if (D(RELA) != NULL) {
		if (D(RELAENT))
			assert(D(RELAENT)->d_un.d_val == sizeof(ElfW(Rela)));
		assert(D(RELASZ) != NULL);
		//dprintf("base:   %p, ptr: %lx\n", l->l_addr, D(RELA)->d_un.d_ptr);
		reloc_rela(l,
			   (ElfW(Rela)*) (l->l_addr + D(RELA)->d_un.d_ptr),
			   D(RELASZ)->d_un.d_val / sizeof(ElfW(Rela)));
	}

	if (D(PLTGOT) != NULL) {
		ElfW(Addr) pltgot, jmprel;
		ElfW(Xword) pltrel, pltrelsz;

		assert(D(PLTREL) != NULL);
		assert(D(PLTRELSZ) != NULL);
		pltgot   = l->l_addr + D(PLTGOT)->d_un.d_ptr;
		jmprel   = l->l_addr + D(JMPREL)->d_un.d_ptr;
		pltrel   = D(PLTREL)->d_un.d_val;
		pltrelsz = D(PLTRELSZ)->d_un.d_val;
		dprintf("  GOT address: %p\n", (void *) pltgot);
		dprintf("       JMPREL: %p (Address of PLT relocs. [.rela.plt])\n",
			(void *) jmprel);
		dprintf("       PLTREL: %#lx (Type of reloc in PLT)\n", pltrel);
		dprintf("     PLTRELSZ: %#lx bytes (size in bytes of PLT relocs)\n",
			pltrelsz);
		if (pltrel != DT_RELA)
			dprintf_die("PLTREL type is not DT_RELA. not supported\n");
		reloc_rela(l, (void *) jmprel, pltrelsz / sizeof(ElfW(Rela)));
	}
	print_mark_end();
	return 0;
#undef D_INF_PTR
#undef D
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
		dprintf("relocating %s\n", l->l_name);
		relocate_object(l);
		l = l->l_prev;
	}
}
HIDDEN(reloc_all);
