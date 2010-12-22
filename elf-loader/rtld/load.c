#include <elf.h>
#include <loader.h>
#include <lib.h>
#include <ldsodefs.h>

/* sysdeps/generic/ldsodefs.h */
/* REF: _dl_map_object [glibc/elf/dl-load.c] */
/* REF: _dl_map_object_deps */
/* REF: _dl_open_worker [dl-open.c] */


static int open_path(const char *soname, char **realname)
{
	int i, fd;
	char namebuf[1024];
	const char *rpath;

	*realname = NULL;
	for (i = 0; GL(rpath)[i] != NULL; i++) {
		rpath = GL(rpath)[i];
		dsnprintf(namebuf, sizeof(namebuf), "%s/%s", rpath, soname);
		fd = __open(namebuf, O_RDONLY);
		if (fd > 0) {
			dprintf("  Library found %s => %s\n", soname, namebuf);
			*realname = __strdup(namebuf);
			break;
		}
	}
	return fd;
}

static void print_namespace(void)
{
	int i;
	struct link_map *l;

	print_mark("NAMESPACE INFORMATION");
	i = 0;
	for (l = GL(namespace); l != NULL; l = l->l_next) {
		int search;

		dprintf("  [%2d]: %s\n", i, l->l_name);
		dprintf("    load address: 0x%lx\n", l->l_addr);
		dprintf("    search list(#%d): ", l->l_searchlist.r_nlist);
		for (search = 0; search < l->l_searchlist.r_nlist; search++)
			dprintf("\"%s\" => ", l->l_searchlist.r_list[search]->l_name);
		dprintf("END\n");
		i++;
	}
	print_mark_end();
}

static void parse_dynamic(link_map *map)
{
	ElfW(Dyn) *dyn;

	assert(map->l_ld != NULL);
	dprintf("                %p\n", map->l_ld);
	dputs("========== HELLO =============\n");
	for (dyn = map->l_ld; dyn->d_tag != DT_NULL; dyn++) {
	dputs("========== PIYO =============\n");
		if (dyn->d_tag < DT_NUM)
			map->l_info[dyn->d_tag] = dyn;
		switch (dyn->d_tag) {
		case DT_STRTAB:		     /* .dynstr */
			break;
		case DT_NEEDED:
			break;
		case DT_SONAME:
			dprintf("DT_SONAME %lx\n", dyn->d_un.d_val);
			break;
		default:
			break;
		}
	}
	if (map->l_info[DT_NEEDED])
		assert(map->l_info[DT_SYMTAB] != NULL);

	if (map->l_info[DT_NEEDED]) {
		const char *strtab = (const void *) D_PTR (map, l_info[DT_STRTAB]);
		const char *soname;

		print_mark("DT_NEEDED");
		for (dyn = map->l_ld; dyn->d_tag != DT_NULL; dyn++) {
			if (dyn->d_tag != DT_NEEDED)
				continue;
			soname = &strtab[dyn->d_un.d_val];
			dprintf("  NEEDED: %s\n", soname);
		}
		print_mark_end();
	}
}

struct filebuf
{
	ssize_t len;
# define FILEBUF_SIZE 832
	char buf[FILEBUF_SIZE] __attribute__ ((aligned (__alignof (ElfW(Ehdr)))));
};

static struct link_map *
map_object_fd(struct link_map *loader, int fd,
	      const char *soname, char *realname)
{
	struct link_map *l;
	struct stat st;
	struct filebuf fb;
	const ElfW(Ehdr) *header;
	const ElfW(Phdr) *phdr;
	const ElfW(Phdr) *ph;
	size_t maplength;
	ElfW(Addr) mapstart, allocend;

	l = emalloc(sizeof(struct link_map));
	init_link_map(l);

	fb.len = __read(fd, fb.buf, sizeof(fb.buf));
	if (fb.len < 0)
		dprintf_die("read error");

	header = (void *) fb.buf;
	if (header->e_type != ET_DYN)
		dprintf("not supported (!ET_DYN)");

	l->l_entry = header->e_entry;
	l->l_phnum = header->e_phnum;

	maplength = header->e_phnum * sizeof (ElfW(Phdr));
	if (header->e_phoff + maplength <= (size_t) fb.len)
		phdr = (void *) (fb.buf + header->e_phoff);
	else {
		phdr = alloca(maplength);
		__lseek (fd, header->e_phoff, SEEK_SET);
		if ((size_t) __read(fd, (void *) phdr, maplength) != maplength)
			dprintf_die("read failed");
	}

	/* PT_LOADのv_addrはファイルのオフセットと一致していない場合がある。その場合に備える必要あり。 */
	mapstart = ~0;
	allocend = 0;
	for (ph = phdr; ph < &phdr[l->l_phnum]; ++ph) {
		switch (ph->p_type) {
		case PT_LOAD: {
			ElfW(Addr) t;

			t = ph->p_vaddr & ~(GLRO(dl_pagesize) - 1);
			if (t < mapstart)
				mapstart = t;
			t = ph->p_vaddr + ph->p_memsz;
			if (t > allocend)
				allocend = ph->p_vaddr + ph->p_memsz;
			break;
		}
		case PT_DYNAMIC:
			l->l_ld = (void *) ph->p_vaddr;
			l->l_ldnum = ph->p_memsz / sizeof (ElfW(Dyn));
			break;
		case PT_PHDR:
			l->l_phdr = (void *) ph->p_vaddr;
			break;
		case PT_TLS:
			/* write here! */
			dputs("PT_TLS not supported\n");
		}
	}

	print_mark_fmt("MAPPING INFO (%s)", soname);
/* 	if (__fstat(fd, &st) < 0) */
/* 		dprintf_die("fstat failed"); */
	maplength = allocend - mapstart;
	dprintf("  size: 0x%lx\n", maplength);
	l->l_map_start = (ElfW(Addr))
		mmap(NULL, maplength,
		     PROT_WRITE|PROT_READ|PROT_EXEC,
		     MAP_PRIVATE, fd, 0);
	l->l_name = realname;
	l->l_addr = l->l_map_start - mapstart;
	dprintf("  map address: 0x%lx\n", l->l_map_start);
	dprintf("  base address: 0x%lx\n", l->l_addr);
	dprintf("  DYNAMIC     : 0x%lx\n", (unsigned long) l->l_ld);
	if (l->l_ld != NULL)
		l->l_ld = (void *) ((ElfW(Addr))l->l_ld + l->l_addr);
	dprintf("  l_ld        : 0x%lx\n", l->l_ld);
	//new->l_libname = soname;
	print_mark_end();

	parse_dynamic(l);

	return l;
}

struct link_map *map_object(struct link_map *loader, const char *soname)
{
	int fd;
	char *realname;
	struct link_map *new, *l;

	/* search loaded library */
	for (l = GL(namespace); l != NULL; l = l->l_next) {
		//dputs(l->l_name);
		/* write here */
	}
	fd = open_path(soname, &realname);
	if (fd < 0)
		dprintf_die(" Library not found. (%s)\n", soname);
	new = map_object_fd(loader, fd, soname, realname);
	__close(fd);

	return new;
}
HIDDEN(map_object);

void map_object_deps(link_map *map)
{
	int i, nlist;
	ElfW(Dyn) *dyn;
	struct link_map *link_end, *l;

	parse_dynamic(map);
	print_namespace();
	nlist = 0;
	link_end = map;
	for (dyn = map->l_ld; dyn->d_tag != DT_NULL; dyn++) {
		struct link_map *new;
		const char *soname;
		const char *strtab =
			(const void *) D_PTR (map, l_info[DT_STRTAB]);

		if (dyn->d_tag != DT_NEEDED)
			continue;
		soname = &strtab[dyn->d_un.d_val];
		dprintf("  NEEDED: %s\n", soname);
		new = map_object(map, soname);

		/* fixme l_prev */
		link_end->l_next = new;
		link_end = link_end->l_next;
		nlist++;

		/* write here */
	}
	map->l_searchlist.r_list =
		emalloc(sizeof(struct link_map *) * nlist);
	map->l_searchlist.r_nlist = nlist;
	for (i = 0, l = map->l_next; i < nlist; i++, l = l->l_next) {
		assert(l != NULL);
		map->l_searchlist.r_list[i] = l;
	}

	print_namespace();
}
HIDDEN(map_object_deps);
