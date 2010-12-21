#include <elf.h>
#include <loader.h>
#include <lib.h>
#include <link.h>
#include <ldsodefs.h>

extern void _start (void) rtld_local;
static struct program_info *program_info;

DEFINE_GLO_VAR(struct rtld_global_ro, _rtld_global_ro) = {
	._dl_lazy = 1,
};
DEFINE_GLO_VAR(struct rtld_global, _rtld_global) = {
	._dl_stack_flags = 0,
};

static void print_maps(void)
{
	int fd;

	print_mark("PRINT MAPS");
	fd = syscall(SYS_open, "/proc/self/maps", O_RDONLY);
	if (fd < 0)
		dputs_die("open failed\n");
	for (;;) {
		char line[1024];
		int r = syscall(SYS_read, fd, line, sizeof(line));
		if (r == 0)
			break;
		if (r < 0)
			dputs_die("read error\n");
		syscall(SYS_write, 1, line, r);
	}
	syscall(SYS_close, fd);
	print_mark_end();
}

extern char _begin[] rtld_local;
extern char _etext[] rtld_local;
extern char _end[] rtld_local;

#include <linux/auxvec.h>
#ifndef AT_RANDOM
# define AT_RANDOM 25	/* address of 16 random bytes */
#endif
#ifndef AT_EXECFN
# define AT_EXECFN  31	/* filename of program */
#endif
static void parse_auxv(ElfW(auxv_t) *auxv)
{
	int i;

#define AT_PRINT(v)				\
	dprintf("    auxv[%2d] %12s = %#lx\n",	\
		i, #v, auxv->a_un.a_val)
#define AT(v)					\
	case AT_##v:				\
		AT_PRINT(v);			\
	break;

	for (i = 0; auxv->a_type != AT_NULL; i++, auxv++) {
		switch (auxv->a_type) {
			AT(IGNORE);
			AT(EXECFD);
			AT(PHENT);
			AT(BASE);
			AT(FLAGS);
			AT(NOTELF);
			AT(UID);
			AT(EUID);
			AT(GID);
			AT(EGID);
			AT(CLKTCK);
			AT(SECURE);
			AT(PLATFORM);
			AT(HWCAP);
			// linux/auxvec.h
			AT(RANDOM);
			AT(EXECFN);

		case AT_ENTRY:
			program_info->entry = (void *) auxv->a_un.a_val;
			AT_PRINT(ENTRY);
			break;
		case AT_SYSINFO_EHDR:
			program_info->ehdr = (ElfW(Ehdr) *) auxv->a_un.a_val;
			if (program_info->phdr == NULL)
				program_info->phdr =
					(ElfW(Phdr) *) (auxv->a_un.a_val + program_info->ehdr->e_phoff);
			AT_PRINT(SYSINFO_EHDR);
			break;
		case AT_PHDR:
			program_info->phdr = (ElfW(Phdr) *) auxv->a_un.a_val;
			AT_PRINT(PHDR);
			break;
		case AT_PHNUM:
			program_info->phnum = auxv->a_un.a_val;
			AT_PRINT(PHNUM);
			break;
		case AT_PAGESZ:
			GLRO(dl_pagesize) = __pagesize = auxv->a_un.a_val;
			AT_PRINT(PAGESZ);
			break;
		default:
			dprintf("  unknown auxv %2d: %#lx\n",
				(int)auxv->a_type, auxv->a_un.a_val);
		}
	}
#undef AT
#undef AT_PRINT
}
static void parse_params(ElfW(Off) *params)
{
	int argc, envc, i;
	char **argv, **envp;
	ElfW(Off) *pargv, *penvp, *pauxv;

	print_mark("BOOT PARAMETERS");
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
		dprintf("    argv[%d]: %s\n", i, argv[i]);
	dprintf("  envp: %p (nr: %d)\n", penvp, envc);
/*
	for (i = 0; envp[i] != NULL; i++)
		dprintf("    envp[%d]: %s\n", i, envp[i]);
*/
	dprintf("  auxv: %p\n", pauxv);
	parse_auxv((ElfW(auxv_t *)) pauxv);

	program_info->argc = argc;
	program_info->argv = argv;
	program_info->envp = envp;
	print_mark_end();
}

static void print_program_info(void)
{
	print_mark("PROGRAM INFO");
	dprintf("  argc: %d\n", program_info->argc);
	dprintf("  argv: %p\n", program_info->argv);
	dprintf("  envp: %p\n", program_info->envp);
	dprintf("  ehdr: %p\n", program_info->ehdr);
	dprintf("  phdr: %p\n", program_info->phdr);
	dprintf("  phnum: %d\n", program_info->phnum);
	dprintf("  entry: %p\n", program_info->entry);
	print_mark_end();
}

static void loader_main(struct program_info *pi)
{
	ElfW(Phdr) *ph;
	struct link_map *main_map;

	main_map = emalloc(sizeof(struct link_map));
	init_link_map(main_map);
	main_map->l_phdr = pi->phdr;
	main_map->l_phnum = pi->phnum;
	for (ph = pi->phdr; ph < &pi->phdr[pi->phnum]; ph++) {
		switch (ph->p_type) {
		case PT_PHDR:
			main_map->l_addr = (ElfW(Addr)) ph - ph->p_vaddr;
			break;
		case PT_DYNAMIC:
			main_map->l_ld = (void *) main_map->l_addr + ph->p_vaddr;
			break;
		case PT_LOAD: {
			ElfW(Addr) mapstart;
			ElfW(Addr) allocend;

			/* Remember where the main program starts in memory.  */
			mapstart = (main_map->l_addr
				    + (ph->p_vaddr & ~(GLRO(dl_pagesize) - 1)));
			if (main_map->l_map_start > mapstart)
				main_map->l_map_start = mapstart;

			/* Also where it ends.  */
			allocend = main_map->l_addr + ph->p_vaddr + ph->p_memsz;
			if (main_map->l_map_end < allocend)
				main_map->l_map_end = allocend;
			if ((ph->p_flags & PF_X) && allocend > main_map->l_text_end)
				main_map->l_text_end = allocend;
		}
			break;
		case PT_TLS:
			dputs_die("PT_TLS not implemented\n");
			break;
		case PT_GNU_STACK:
			GL(dl_stack_flags) = ph->p_flags;
			break;
		case PT_GNU_RELRO:
			main_map->l_relro_addr = ph->p_vaddr;
			main_map->l_relro_size = ph->p_memsz;
			break;
		case PT_GNU_EH_FRAME:
		case PT_INTERP:
			/* ignore */
			break;
		default:
			dprintf("unknown segment type (%x)\n", ph->p_type);
		}
	}
	map_object_deps(main_map);
}

void __attribute__((regparm(3))) loader_start(void *params)
{
	struct program_info pi = {
		.argc = -1,
		.argv = NULL,
		.envp = NULL,
		.entry = _start,
		.phdr = NULL,
		.ehdr = NULL,
		.phnum = -1,
	};

	program_info = &pi;
	dputs("Hello, Dynamic Linker and Loader!\n\n");

	malloc_init();

	parse_params(params);
	//syscall(SYS_exit, 0);
	print_maps();

	if (program_info->entry == _start) {
		dputs("I'm not Program Interpreter.\nSee you!\n");
		syscall(SYS_exit, 0);
	}
	//reloc_self();
	print_program_info();
	assert(program_info->phdr != NULL);
	assert(program_info->phnum != -1);
	assert(program_info->entry != 0);
	loader_main(program_info);

	syscall(SYS_exit, 0);
	for (;;) ;
}
