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

	print_mark_fmt("DYNAMIC INFO (%s)", map->l_name);
	assert(map->l_ld != NULL);
	for (dyn = map->l_ld; dyn->d_tag != DT_NULL; dyn++) {
		if (dyn->d_tag < DT_NUM)
			map->l_info[dyn->d_tag] = dyn;
		/** MEMO
		 * DT_STRGAB: .dynstr
		 */
		switch (dyn->d_tag) {
		default:
			break;
		}
	}
	if (map->l_info[DT_NEEDED])
		assert(map->l_info[DT_SYMTAB] != NULL);

	const char *strtab = (const void *) D_PTR(map, l_info[DT_STRTAB]);
	if (map->l_info[DT_SONAME]) {
		dprintf("  SONAME: %s\n",
			&strtab[map->l_info[DT_SONAME]->d_un.d_val]);
	}
	if (map->l_info[DT_NEEDED]) {
		const char *soname;

		for (dyn = map->l_ld; dyn->d_tag != DT_NULL; dyn++) {
			if (dyn->d_tag != DT_NEEDED)
				continue;
			soname = &strtab[dyn->d_un.d_val];
			dprintf("  NEEDED: %s\n", soname);
		}
	}
	print_mark_end();
}

struct filebuf
{
	ssize_t len;
# define FILEBUF_SIZE 832
	char buf[FILEBUF_SIZE] __attribute__ ((aligned (__alignof (ElfW(Ehdr)))));
};

struct loadcmd
{
	ElfW(Addr) mapstart, mapend, dataend, allocend;
	off_t mapoff;
	int prot;
};
static size_t
_parse_phdr(struct link_map *l, struct loadcmd *loadcmds,
	    const ElfW(Phdr) *phdr, bool *has_holes)
{
	int nloadcmds;
	struct loadcmd *c;
	unsigned int stack_flags = PF_R|PF_W|PF_X;
	const ElfW(Phdr) *ph;

	*has_holes = false;
	ElfW(Addr) mapstart, allocend;

	nloadcmds = 0;
	mapstart = ~0;
	allocend = 0;
	for (ph = phdr; ph < &phdr[l->l_phnum]; ++ph) {
		switch (ph->p_type) {
		case PT_LOAD: {
			assert((ph->p_align & (GLRO(dl_pagesize) - 1)) == 0);
			assert(((ph->p_vaddr - ph->p_offset) & (ph->p_align - 1)) == 0);

			c = &loadcmds[nloadcmds++];
			c->mapstart = ph->p_vaddr & ~(GLRO(dl_pagesize) - 1);
			c->mapend   = ((ph->p_vaddr + ph->p_filesz + GLRO(dl_pagesize) - 1)
				       & ~(GLRO(dl_pagesize) - 1));
			c->dataend  = ph->p_vaddr + ph->p_filesz;
			c->allocend = ph->p_vaddr + ph->p_memsz;
			c->mapoff   = ph->p_offset & ~(GLRO(dl_pagesize) - 1);

			/* Determine whether there is a gap between the last segment
			   and this one.  */
			if (nloadcmds > 1 && c[-1].mapend != c->mapstart)
				*has_holes = true;

			c->prot = 0;
			if (ph->p_flags & PF_R)
				c->prot |= PROT_READ;
			if (ph->p_flags & PF_W)
				c->prot |= PROT_WRITE;
			if (ph->p_flags & PF_X)
				c->prot |= PROT_EXEC;
			break;
		}
		case PT_DYNAMIC:
			l->l_ld = (void *) ph->p_vaddr;
			l->l_ldnum = ph->p_memsz / sizeof (ElfW(Dyn));
			break;
		case PT_PHDR:
			l->l_phdr = (void *) ph->p_vaddr;
			break;
		case PT_GNU_STACK:
			stack_flags = ph->p_flags;
			break;
		case PT_TLS:
			/* write here! */
			dputs("PT_TLS not supported\n");
			break;
		}
	}
	return nloadcmds;
}

static void
_map_object_loadcmd(struct link_map *l,
		    int fd,
		    const ElfW(Ehdr) *header,
		    const struct loadcmd *loadcmds,
		    const size_t nloadcmds,
		    size_t maplength,
		    bool has_holes,
		    int postmap)
{
	const struct loadcmd *c = loadcmds;

	if (postmap)
		goto postmap;

	/* Remember which part of the address space this object uses.  */
	l->l_map_start  = c->mapstart + l->l_addr;
	l->l_map_end    = l->l_map_start + maplength;
	l->l_contiguous = !has_holes;

	while (c < &loadcmds[nloadcmds]) {
		if (c->mapend > c->mapstart
		    /* Map the segment contents from the file.  */
		    && (mmap((void *) (l->l_addr + c->mapstart),
			     c->mapend - c->mapstart, c->prot,
			     MAP_FIXED|MAP_PRIVATE, fd, c->mapoff)
			== MAP_FAILED)) {
			dprintf_die("mmap failed");
		}

	postmap:
		if (c->prot & PROT_EXEC)
			l->l_text_end = l->l_addr + c->mapend;

		if (l->l_phdr == 0
		    && (ElfW(Off)) c->mapoff <= header->e_phoff
		    && ((size_t) (c->mapend - c->mapstart + c->mapoff)
			>= header->e_phoff + header->e_phnum * sizeof (ElfW(Phdr))))
			/* Found the program header in this segment.  */
			l->l_phdr = (void *) (c->mapstart + header->e_phoff - c->mapoff);

		if (c->allocend > c->dataend)
		{
			/* Extra zero pages should appear at the end of this segment,
			   after the data mapped from the file.   */
			ElfW(Addr) zero, zeroend, zeropage;

			zero = l->l_addr + c->dataend;
			zeroend = l->l_addr + c->allocend;
			zeropage = ((zero + GLRO(dl_pagesize) - 1)
				    & ~(GLRO(dl_pagesize) - 1));

			if (zeroend < zeropage)
				/* All the extra data is in the last page of the segment.
				   We can just zero it.  */
				zeropage = zeroend;

			if (zeropage > zero)
			{
				/* Zero the final part of the last page of the segment.  */
				if ((c->prot & PROT_WRITE) == 0) {
					/* Dag nab it.  */
					if (__mprotect ((caddr_t)
							(zero & ~(GLRO(dl_pagesize) - 1)),
							GLRO(dl_pagesize), c->prot|PROT_WRITE) < 0)
						dprintf_die("mprotect failed");
				}
				memset((void *) zero, '\0', zeropage - zero);
				if ((c->prot & PROT_WRITE) == 0) {
					if (__mprotect((caddr_t) (zero & ~(GLRO(dl_pagesize) - 1)),
						       GLRO(dl_pagesize), c->prot) < 0)
						dprintf_die("mprotect failed");
				}
			}

			if (zeroend > zeropage)
			{
				/* Map the remaining zero pages in from the zero fill FD.  */
				caddr_t mapat;
				mapat = mmap ((caddr_t) zeropage, zeroend - zeropage,
					      c->prot, MAP_ANON|MAP_PRIVATE|MAP_FIXED,
					      -1, 0);
				if (mapat == MAP_FAILED)
					dprintf_die("mmap(map zero-fill pages) failed");
			}
		}
		++c;
	}
}

static struct link_map *
map_object_fd(struct link_map *loader, int fd,
	      const char *soname, char *realname)
{
	struct link_map *l;
	struct filebuf fb;
	const ElfW(Ehdr) *header;
	const ElfW(Phdr) *phdr;
	size_t maplength;

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

	{
		struct loadcmd loadcmds[l->l_phnum], *c;
		size_t nloadcmds;
		bool has_holes = false;

		nloadcmds = _parse_phdr(l, loadcmds, phdr, &has_holes);
		c = loadcmds;

		maplength = loadcmds[nloadcmds - 1].allocend - c->mapstart;
		if (header->e_type == ET_DYN) {
			l->l_map_start = (ElfW(Addr))
				mmap(NULL, maplength, c->prot, MAP_PRIVATE, fd, 0);
			l->l_map_end = l->l_map_start + maplength;
			l->l_addr    = l->l_map_start - c->mapstart;
			if (has_holes)
				__mprotect ((caddr_t) (l->l_addr + c->mapend),
					    loadcmds[nloadcmds - 1].mapstart - c->mapend,
					    PROT_NONE);
			l->l_contiguous = 1;
			_map_object_loadcmd(l, fd, header, c,
					    nloadcmds, maplength, has_holes, 1);
		}
	}

	print_mark_fmt("MAPPING INFO (%s)", soname);

	dprintf("  size: 0x%lx\n", maplength);
	l->l_name = realname;
//	l->l_addr = l->l_map_start - mapstart;

	dprintf("  map address: 0x%lx\n", l->l_map_start);
	dprintf("  base address: 0x%lx\n", l->l_addr);
	dprintf("  DYNAMIC     : 0x%lx\n", (unsigned long) l->l_ld);
	if (l->l_ld != NULL)
		l->l_ld = (void *) ((ElfW(Addr))l->l_ld + l->l_addr);
	dprintf("  l_ld        : 0x%p\n", l->l_ld);
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
