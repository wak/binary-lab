#include <elf.h>
#include <loader.h>
#include <lib.h>

#define ElfW(type) Elf64_##type

#define MESSAGE "Hello, Dynamic Linker and Loader!\n"

static void print_maps(void)
{
	int fd;

	dputs("- PRINT MAPS-----------------------\n");
	fd = syscall(SYS_open, "/proc/self/maps", O_RDONLY);
	if (fd < 0)
		ERR_EXIT("open failed\n");
	for (;;) {
		char line[1024];
		int r = syscall(SYS_read, fd, line, sizeof(line));
		if (r == 0)
			break;
		if (r < 0)
			ERR_EXIT("read error\n");
		syscall(SYS_write, 1, line, r);
	}
	syscall(SYS_close, fd);
	dputs("-----------------------------------\n");
}

extern char _begin[] rtld_local;
extern char _etext[] rtld_local;
extern char _end[] rtld_local;

static void reloc_rela(ElfW(Rela) *rela, size_t part_size, unsigned long count)
{
	void *begin = &_begin;
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

static void reloc_self(void)
{
	int i, j, rela_count;
	void *begin = &_begin;
	ElfW(Ehdr) *ehdr = begin;
	ElfW(Phdr) *phdr = begin + ehdr->e_phoff;
	ElfW(Dyn) *dyn_rela, *dyn_relasz, *dyn_relaent;

	dputs("- RELOCAION -------\n");
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
					ERR_EXIT("too many DT_RELA (not supported)\n");
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
				ERR_EXIT("DT_REL not support\n");
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
			ERR_EXIT("DT_RELASZ not found");
		reloc_rela(begin + dyn_rela->d_un.d_ptr,
			   dyn_relaent->d_un.d_val,
			   dyn_relasz->d_un.d_val / dyn_relaent->d_un.d_val);
	}
	dputs("-------------------\n");
}

static void parse_auxv(ElfW(auxv_t) *auxv)
{
	dprintf("auxv address: %p\n", auxv);
	for (; auxv->a_type != AT_NULL; auxv++) {
		switch (auxv->a_type) {
		default:
			dprintf("  unknown auxv %lx\n", auxv->a_type);
		}
	}
}
static void parse_params(ElfW(Off) *params)
{
	int argc, envc, i;
	char **argv, **envp;
	ElfW(Off) *pargv, *penvp, *pauxv;

	dprintf("  stack: %p\n", params);
	argc = *(int *) params;
	pargv = params + 1;
	penvp = pargv + argc + 1;
	pauxv = penvp;
	for (envc = 0; *pauxv++ != 0; envc++)
		;

	argv = (char **) pargv;
	envp = (char **) penvp;
	dprintf("  argc: %p (-> %d)\n", params, argc);
	dprintf("  argv: %p\n", pargv);
	for (i = 0; i < argc; i++)
		dprintf("    argv[%d]: %s\n", i, (char *)argv[i]);
	dprintf("  envp: %p (nr: %d)\n", penvp, envc);
/*
	for (i = 0; envp[i] != NULL; i++) {
		int j;
		dprintf("    envp[%d]: ", i);
		for (j = 0; envp[i][j] != '\0'; j++)
			dprintf("%c", envp[i][j]);
		dprintf("\n");
	}
*/
	dprintf("  auxv: %p\n", pauxv);
	//parse_auxv(pauxv);
}

void __attribute__((regparm(3))) loader_start(void *params)
{
	dputs(MESSAGE);

	reloc_self();
	parse_params(params);
	//print_maps();

	syscall(SYS_exit, 0);
	for (;;) ;
}
