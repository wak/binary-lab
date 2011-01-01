#include <elf.h>
#include <lib.h>
#include <link.h>
#include <ldsodefs.h>

extern void _start (void) rtld_local;

struct program_info {
	int argc;
	char **argv;
	char **envp;
	void *entry;
	ElfW(Ehdr) *ehdr;
	ElfW(Phdr) *phdr;
	ElfW(Half) phnum;
};

static struct program_info *program_info;

DEFINE_GLO_VAR(struct rtld_global_ro, _rtld_global_ro) = {
};
DEFINE_GLO_VAR(struct rtld_global, _rtld_global) = {
	._dl_stack_flags = 0,
	._namespace = NULL,
	._rpath = { NULL },
};
DEFINE_GLO_VAR(int, errno) = 0;

static void print_maps(void)
{
#if DEBUG_PRINT_MAPS
	int fd;
	mprint_start("PRINT MAPS");
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
	mprint_end();
#endif
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

#define AT_PRINT(v)						\
	MPRINTF(BOOTPARAMS, "  auxv[%2d] %12s = %#lx\n",	\
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
			dprintf("unknown auxv %2d: %#lx\n",
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

	MPRINT_START(BOOTPARAMS, "BOOT PARAMETERS");
	MPRINTF(BOOTPARAMS, "stack: %p\n", params);
	argc = *(int *) params;
	pargv = params + 1;
	penvp = pargv + argc + 1;
	pauxv = penvp;
	for (envc = 0; *pauxv++ != 0; envc++)
		;

	argv = (char **) pargv;
	envp = (char **) penvp;
	MPRINTF(BOOTPARAMS, "argc: %p (-> %d)\n", params, argc);
	MPRINTF(BOOTPARAMS, "argv: %p\n", pargv);
	for (i = 0; i < argc; i++)
		MPRINTF(BOOTPARAMS, "  argv[%d]: %s\n", i, argv[i]);
	MPRINTF(BOOTPARAMS, "envp: %p (nr: %d)\n", penvp, envc);
/*
	for (i = 0; envp[i] != NULL; i++)
		MPRINTF(BOOTPARAMS, "  envp[%d]: %s\n", i, envp[i]);
*/
	MPRINTF(BOOTPARAMS, "auxv: %p\n", pauxv);
	parse_auxv((ElfW(auxv_t *)) pauxv);

	program_info->argc = argc;
	program_info->argv = argv;
	program_info->envp = envp;
	MPRINT_END(BOOTPARAMS);
}

static void print_program_info(void)
{
	MPRINT_START(PROGINFO, "PROGRAM INFO");
	MPRINTF(PROGINFO, "argc: %d\n", program_info->argc);
	MPRINTF(PROGINFO, "argv: %p\n", program_info->argv);
	MPRINTF(PROGINFO, "envp: %p\n", program_info->envp);
	MPRINTF(PROGINFO, "ehdr: %p\n", program_info->ehdr);
	MPRINTF(PROGINFO, "phdr: %p\n", program_info->phdr);
	MPRINTF(PROGINFO, "phnum: %d\n", program_info->phnum);
	MPRINTF(PROGINFO, "entry: %p\n", program_info->entry);
	MPRINT_END(PROGINFO);
}

static ElfW(Addr) loader_main(struct program_info *pi)
{
	ElfW(Phdr) *ph;
	struct link_map *rtld_map, *main_map;

	main_map = NULL;

	rtld_map = create_link_map();
	if (program_info->entry == _start) {
		rtld_map->l_addr = (ElfW(Addr)) _begin;
		//dprintf("set l_addr to %lx\n", rtld_map->l_addr);
	}
	rtld_map->l_phdr = pi->phdr;
	rtld_map->l_phnum = pi->phnum;
	rtld_map->l_entry = (ElfW(Addr)) pi->entry;
	for (ph = pi->phdr; ph < &pi->phdr[pi->phnum]; ph++) {
		switch (ph->p_type) {
		case PT_PHDR:
			rtld_map->l_addr = (ElfW(Addr)) ph - ph->p_vaddr;
			break;
		case PT_DYNAMIC:
			rtld_map->l_ld = (void *) rtld_map->l_addr + ph->p_vaddr;
			break;
		case PT_LOAD: {
			ElfW(Addr) mapstart;
			ElfW(Addr) allocend;

			/* Remember where the main program starts in memory.  */
			mapstart = (rtld_map->l_addr
				    + (ph->p_vaddr & ~(GLRO(dl_pagesize) - 1)));
			if (rtld_map->l_map_start > mapstart)
				rtld_map->l_map_start = mapstart;

			/* Also where it ends.  */
			allocend = rtld_map->l_addr + ph->p_vaddr + ph->p_memsz;
			if (rtld_map->l_map_end < allocend)
				rtld_map->l_map_end = allocend;
			if ((ph->p_flags & PF_X) && allocend > rtld_map->l_text_end)
				rtld_map->l_text_end = allocend;
		}
			break;
		case PT_TLS:
			dputs_die("PT_TLS (Thread Local Storage) not implemented\n");
			break;
		case PT_GNU_STACK:
			/* スタックの権限 */
			GL(dl_stack_flags) = ph->p_flags;
			break;
		case PT_GNU_RELRO:
			rtld_map->l_relro_addr = ph->p_vaddr;
			rtld_map->l_relro_size = ph->p_memsz;
			break;
		case PT_GNU_EH_FRAME:
		case PT_INTERP:
			/* glibcだといろいろしている様子 */
			break;
		default:
			dprintf("unknown segment type (%x)\n", ph->p_type);
		}
	}
	GL(namespace) = rtld_map;
	if (program_info->entry == _start) {
		rtld_map->l_name = __strdup("RTLD");
		main_map = map_object(rtld_map, program_info->argv[1]);
		rtld_map->l_next = main_map;
		main_map->l_prev = rtld_map;
		parse_dynamic(main_map);
	} else {
		rtld_map->l_name = __strdup("main-program");
	}
	parse_dynamic(rtld_map);
	map_object_deps(main_map == NULL ? rtld_map : main_map);
	print_maps();
	reloc_all();

	return (pi->entry == _start) ? 
		main_map->l_addr + main_map->l_entry :
		rtld_map->l_addr + rtld_map->l_entry;
}

ElfW(Addr) __attribute__((regparm(3))) loader_start(void *params)
{
	ElfW(Addr) entry;
	char **rpath;
	struct program_info pi = {
		.argc = -1,
		.argv = NULL,
		.envp = NULL,
		.entry = _start,
		.phdr = NULL,
		.ehdr = NULL,
		.phnum = -1,
	};
	rpath = GL(rpath);
	*rpath++ = "/lib";
	*rpath++ = "/usr/lib";
	*rpath++ = ".";
	*rpath++ = "./sample";
	*rpath++ = NULL;

	program_info = &pi;
	dputs("Hello, Dynamic Linker and Loader!\n\n");

	extern void malloc_init(void);
	malloc_init();

	parse_params(params);
	//syscall(SYS_exit, 0);
	//print_maps();
/*
	if (program_info->entry == _start) {
		dputs("I'm not Program Interpreter.\nSee you!\n");
		syscall(SYS_exit, 0);
	}
*/
	//reloc_self();
	print_program_info();
	assert(program_info->phdr != NULL);
	assert(program_info->phnum != -1);
	assert(program_info->entry != 0);
	print_maps();
	entry = loader_main(program_info);

	dprintf("\n\n<----------- ENTRY POINT ----------->\n\n");
//	((void (*)(void)) )();

//	print_maps();

	return entry;
}
