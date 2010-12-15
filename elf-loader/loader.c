//#include <libc-symbols.h>  // for uClibc-0.9.31 (bug?)

#include <elf.h>
#include "loader.h"
#include "lib.h"

#define ElfW(type) Elf64_##type

#define MESSAGE "Hello, Dynamic Linker and Loader!\n"
#define ERR_EXIT(err) {					\
	  syscall(SYS_write, 2, err, sizeof(err)-1);	\
	  syscall(SYS_exit, 1);				\
  }

static void print_maps(void)
{
	int fd;

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
}

extern char _begin[] rtld_local;
extern char _etext[] rtld_local;
extern char _end[] rtld_local;

#define ECHO(m)						\
	syscall(SYS_write, 2, m  "\n", sizeof(m))

static void reloc_rela(ElfW(Rela) *rela, unsigned long count)
{
	void *begin = &_begin;
	//unsigned long add = (unsigned long) (begin - 0);

	for (; count-- > 0; rela++) {
		unsigned long *reloc = begin + rela->r_offset;
		switch (ELF64_R_TYPE(rela->r_info)) {
		case R_X86_64_RELATIVE:
			*reloc = rela->r_addend + (unsigned long) begin;
			break;
		default:
			ECHO("unknown relocation type");
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

	rela_count = 0;
	dyn_rela = dyn_relasz = dyn_relaent = NULL;
	for (i = 0; i < ehdr->e_phnum; i++, phdr++) {
		ElfW(Dyn) *dyn;
		if (phdr->p_type != PT_DYNAMIC)
			continue;
		ECHO("Dynamic segument found");
		dyn = begin + phdr->p_offset;
		for (j = 0; j < phdr->p_memsz / sizeof(ElfW(Dyn)); j++, dyn++) {
			switch (dyn->d_tag) {
			case DT_RELA:
				if (rela_count++ > 1)
					ERR_EXIT("too many DT_RELA (not supported)\n");
				dyn_rela = dyn; break;
			case DT_RELASZ:
				dyn_relasz = dyn; break;
			case DT_RELAENT:
				dyn_relaent = dyn; break;
			case DT_REL:
				ERR_EXIT("DT_REL not support\n");
			default:
				continue;
			}
		}
	}
	if (rela_count) {
		if (dyn_relaent == NULL)
			ERR_EXIT("DT_RELAENT not found");
		reloc_rela(begin + dyn_rela->d_un.d_ptr, dyn_relaent->d_un.d_val);
	}
}

void loader_start(void)
{
	reloc_self();
	print_maps();

	//fflush(stderr);
	dputs(MESSAGE);
	malloc(100000);
	print_maps();
	syscall(SYS_exit, 0);
	for (;;) ;
}
