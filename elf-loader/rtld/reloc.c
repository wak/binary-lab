#include <elf.h>
#include <loader.h>
#include <lib.h>

static void reloc_rela(ElfW(Ehdr) *ehdr, ElfW(Rela) *rela,
		       size_t part_size, unsigned long count)
{
	void *begin = ehdr;
	//unsigned long add = (unsigned long) (begin - 0);

	assert(part_size == sizeof(*rela));
	for (; count-- > 0; rela++) {
		unsigned long *reloc = begin + rela->r_offset;
		switch (ELF64_R_TYPE(rela->r_info)) {
		case R_X86_64_RELATIVE:
			*reloc = rela->r_addend + (unsigned long) begin;
			dprintf("  reloc RELATIVE 0x%p <- + %lx\n",
				reloc, rela->r_addend);
			break;
		default:
			dprintf("  unknown reloc type (%lx)\n",
				ELF64_R_TYPE(rela->r_info));
		}
	}
}

void reloc_elf(ElfW(Ehdr) *ehdr)
{
	int i, j, rela_count;
	void *begin = ehdr;

	ElfW(Phdr) *phdr = begin + ehdr->e_phoff;
	ElfW(Dyn) *dyn_rela, *dyn_relasz, *dyn_relaent;

	print_mark("RELOCAION");
	rela_count = 0;
	dyn_rela = dyn_relasz = dyn_relaent = NULL;
	for (i = 0; i < ehdr->e_phnum; i++, phdr++) {
		ElfW(Dyn) *dyn;
		if (phdr->p_type != PT_DYNAMIC)
			continue;
		dputs("  Dynamic segument found\n");
		dyn = begin + phdr->p_offset;
		for (j = 0; j < phdr->p_memsz / sizeof(ElfW(Dyn)); j++, dyn++) {
			switch (dyn->d_tag) {
			case DT_NULL:
				goto endof_dt;
			case DT_RELA:
				if (rela_count++ > 1)
					dputs_die("too many DT_RELA (not supported)\n");
				dyn_rela = dyn;
				break;
			case DT_RELASZ:
				dyn_relasz = dyn; /* Total size of Rela relocs */
				break;
			case DT_RELAENT:
				dyn_relaent = dyn; /* Size of one Rela reloc */
				break;
			case DT_RELACOUNT:
				break;
			case DT_REL:
				dputs_die("DT_REL not support\n");
				continue;
			default:
				continue;
			}
		}
	endof_dt:
		;
	}
	if (rela_count) {
		if (dyn_relasz == NULL || dyn_relaent == NULL)
			dputs_die("DT_RELASZ not found");
		/*
		reloc_rela(ehdr,
			   begin + dyn_rela->d_un.d_ptr,
			   dyn_relaent->d_un.d_val,
			   dyn_relasz->d_un.d_val / dyn_relaent->d_un.d_val);
		*/
	}
	print_mark_end();
}
HIDDEN(reloc_elf);

/* REF: _dl_relocate_object [glibc/elf/dl-reloc.c] */
int relocate_object(struct link_map *l)
{
	int i, j, rela_count;

	const ElfW(Phdr) *phdr = l->l_phdr;
	const ElfW(Dyn) *dyn_rela, *dyn_relasz, *dyn_relaent;

	print_mark("RELOCAION");
	rela_count = 0;
	dyn_rela = dyn_relasz = dyn_relaent = NULL;
	for (i = 0; i < l->l_phnum; i++, phdr++) {
		const ElfW(Dyn) *dyn;
		if (phdr->p_type != PT_DYNAMIC)
			continue;
		dputs("  Dynamic segument found\n");
		dyn = (ElfW(Dyn) *) (l->l_addr + phdr->p_offset);
		for (j = 0; j < phdr->p_memsz / sizeof(ElfW(Dyn)); j++, dyn++) {
			//DT_ADDRRNGHI & DT_ADDRRNGLO
			switch (dyn->d_tag) {
			case DT_NULL:
				goto endof_dt;
			case DT_RELA:
				if (rela_count++ > 1)
					dputs_die("too many DT_RELA (not supported)\n");
				dyn_rela = dyn;
				break;
			case DT_RELASZ:
				dyn_relasz = dyn; /* Total size of Rela relocs */
				break;
			case DT_RELAENT:
				dyn_relaent = dyn; /* Size of one Rela reloc */
				break;
			case DT_RELACOUNT:
				break;
			case DT_REL:
				dputs_die("DT_REL not support\n");
				continue;
			default:
				continue;
			}
		}
	endof_dt:
		;
	}
	if (rela_count) {
		if (dyn_relasz == NULL || dyn_relaent == NULL)
			dputs_die("DT_RELASZ not found");
		/*
		reloc_rela(l->l_addr + dyn_rela->d_un.d_ptr,
			   dyn_relaent->d_un.d_val,
			   dyn_relasz->d_un.d_val / dyn_relaent->d_un.d_val);
		*/
	}
	print_mark_end();

	return 0;
}
HIDDEN(relocate_object);
