#include <elf.h>
#include <loader.h>
#include <lib.h>

static struct program_info {
	int argc;
	char **argv;
	char **envp;
	void * entry;
	ElfW(Ehdr) *ehdr;
	ElfW(Phdr) *phdr;
	ElfW(Half) phnum;
} program_info = {
	.argc = -1,
	.argv = NULL,
	.envp = NULL,
	.entry = NULL,
	.phdr = NULL,
	.ehdr = NULL,
	.phnum = -1,
};

#define MESSAGE "Hello, Dynamic Linker and Loader!\n"

static void print_maps(void)
{
	int fd;

	print_mark("PRINT MAPS");
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
	print_mark("AUXV");
#define AT_PRINT(v)				\
	dprintf("  %15s: %#lx\n",		\
		#v, auxv->a_un.a_val)
#define AT(v)					\
	case AT_##v:				\
		AT_PRINT(v);			\
	break;

	for (; auxv->a_type != AT_NULL; auxv++) {
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
			program_info.entry = (void *) auxv->a_un.a_val;
			AT_PRINT(ENTRY);
			break;
		case AT_SYSINFO_EHDR:
			program_info.ehdr = (ElfW(Ehdr) *) auxv->a_un.a_val;
			if (program_info.phdr == NULL)
				program_info.phdr =
					(ElfW(Phdr) *) (auxv->a_un.a_val + program_info.ehdr->e_phoff);
			AT_PRINT(SYSINFO_EHDR);
			break;
		case AT_PHDR:
			program_info.phdr = (ElfW(Phdr) *) auxv->a_un.a_val;
			AT_PRINT(PHDR);
			break;
		case AT_PHNUM:
			program_info.phnum = auxv->a_un.a_val;
			AT_PRINT(PHNUM);
			break;
		case AT_PAGESZ:
			__pagesize = auxv->a_un.a_val;
			AT_PRINT(PAGESZ);
			break;
		default:
			dprintf("  unknown auxv %2d: %#lx\n",
				(int)auxv->a_type, auxv->a_un.a_val);
		}
	}
#undef AT
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
		dprintf("    argv[%d]: %s\n", i, argv[i]);
	dprintf("  envp: %p (nr: %d)\n", penvp, envc);
/*
	for (i = 0; envp[i] != NULL; i++)
		dprintf("    envp[%d]: %s\n", i, envp[i]);
*/
	dprintf("  auxv: %p\n", pauxv);
	parse_auxv((ElfW(auxv_t *)) pauxv);

	program_info.argc = argc;
	program_info.argv = argv;
	program_info.envp = envp;
}

static void print_program_info(void)
{
	print_mark("PROGRAM INFO");
	dprintf("  argc: %d\n", program_info.argc);
	dprintf("  argv: %p\n", program_info.argv);
	dprintf("  envp: %p\n", program_info.envp);
	dprintf("  ehdr: %p\n", program_info.ehdr);
	dprintf("  phdr: %p\n", program_info.phdr);
	dprintf("  phnum: %d\n", program_info.phnum);
	dprintf("  entry: %p\n", program_info.entry);
	print_mark_end();
}

void __attribute__((regparm(3))) loader_start(void *params)
{
	parse_params(params);
	malloc_init();
	//syscall(SYS_exit, 0);
	dputs(MESSAGE);
	print_maps();

	//reloc_self();
	print_program_info();

	syscall(SYS_exit, 0);
	for (;;) ;
}
