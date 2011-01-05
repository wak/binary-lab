#include <elf.h>
#include <lib.h>
#include <ldsodefs.h>

/* sysdeps/generic/ldsodefs.h */
/* REF: _dl_map_object [glibc/elf/dl-load.c] */
/* REF: _dl_map_object_deps */
/* REF: _dl_open_worker [dl-open.c] */

void print_rpath(void)
{
	int i;

	for (i = 0; GL(rpath)[i] != NULL; i++)
		dprintf("RPATH[%d] = %s\n", i, GL(rpath)[i]);
	dprintf("RPATH[%d] = NULL\n", i);
}
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
		if (fd >= 0) {
			//MPRINTF(LOAD, "Library found %s => %s\n", soname, namebuf);
			*realname = __strdup(namebuf);
			return fd;
		}
	}
	return -1;
}

static void print_namespace(void)
{
	int i, nr_objects;
	struct link_map *l, *last;

	MPRINT_START(LOAD, "NAMESPACE INFORMATION");
	i = nr_objects = 0;
	last = NULL;
	for (l = GL(namespace); l != NULL; l = l->l_next) {
		int search;

		MPRINTF(LOAD, "[%2d]: %s\n", i, l->l_name);
		MPRINTF(LOAD, "  load address: 0x%lx\n", l->l_addr);
		MPRINTF(LOAD, "  search list(#%d): ", l->l_searchlist.r_nlist);
		for (search = 0; search < l->l_searchlist.r_nlist; search++)
			MPRINTF(LOAD, "\"%s\" => ", l->l_searchlist.r_list[search]->l_name);
		MPRINTF(LOAD, "END\n");
		i++;
		nr_objects++;
		last = l;
	}
	/* 戻るのもつながっているかテスト */
	for (; last != NULL; last = last->l_prev)
		nr_objects--;
	assert(nr_objects == 0);
	MPRINT_END(LOAD);
}

/*
 * 実装上の都合など，いろいろとチェック
 */
static void check_dynamic_assert(const struct link_map *l)
{
	/* ライブラリ名の取得にはシンボルテーブルが必要 */
	if (D_VALID(l, DT_NEEDED)) {
		assert(l->l_info[DT_SYMTAB] != NULL);
	}
	/* 再配置情報をチェック */
	if (D_VALID(l, DT_RELA)) {
		/* 実装上，構造体のサイズと一致していなければならない */
		if (D_PTR(l, DT_RELAENT))
			assert(D_VAL(l, DT_RELAENT) == sizeof(ElfW(Rela)));
		assert(D_VALID(l, DT_RELASZ)); /* Relaの要素数はDT_RELASZから取得 */
		assert(D_VALID(l, DT_SYMTAB)); /* シンボル情報はシンボルテーブルから取得 */
		assert(D_VALID(l, DT_STRTAB)); /* その名前は文字列テーブルから取得 */
	}
	/* GOTまわりの再配置には，再配置テーブルと要素数が必要 */
	if (D_VALID(l, DT_PLTGOT)) {
		assert(D_VALID(l, DT_PLTREL));		/* 再配置テーブル */
		assert(D_VALID(l, DT_PLTRELSZ));	/* テーブルサイズ */
		assert(D_VALID(l, DT_PLTREL));		/* 再配置種別は， */
		assert(D_VAL(l, DT_PLTREL) == DT_RELA); /* RELA形式のみ実装 */
	}
	/* 実装上，シンボルのエントリサイズは，構造体のサイズと同じでなければならない */
	if (D_VALID(l, DT_SYMENT)) {
		assert(D_VAL(l, DT_SYMENT) == sizeof(ElfW(Sym)));
	}
	if (D_VALID(l, DT_HASH)) {
		assert(l->l_buckets != NULL);
		assert(l->l_chain != NULL);
	}
}

void parse_dynamic(struct link_map *map)
{
	ElfW(Dyn) *dyn;

	MPRINT_START_FMT(LOAD, "DYNAMIC INFO (%s)", map->l_name);
	assert(map->l_ld != NULL);
	for (dyn = map->l_ld; dyn->d_tag != DT_NULL; dyn++) {
		if (dyn->d_tag < DT_NUM)
			map->l_info[dyn->d_tag] = dyn;
		switch (dyn->d_tag) {
		case DT_AUXILIARY:
			dputs_die("DT_AUXILIARY not supported");
			break;
		case DT_FILTER:
			dputs_die("DT_FILTER not supported");
			break;
		default:
			break;
		}
	}

	const char *strtab = (const void *) D_PTR(map, DT_STRTAB);

	/* パス指定があれば追加する．本当は，そのライブラリのNEEDEDを探すときのみ
	 * 有効のにすべき． */
	for (dyn = map->l_ld; dyn->d_tag != DT_NULL; dyn++) {
		char **rpath, *newpath;
		if (dyn->d_tag != DT_RPATH)
			continue;
		assert(strtab != NULL);
		newpath = (void *)(strtab + dyn->d_un.d_ptr);
		for (rpath = GL(rpath); *rpath != NULL; rpath++)
			if (__strcmp(*rpath, newpath) == 0)
				break;
		if (*rpath == NULL) {
			*rpath++ = __strdup(newpath);
			*rpath = NULL;
			MPRINTF(LOAD, "RPATH: %s\n", newpath);
		}
	}
	/* この共有ライブラリのSONAMEを表示する */
	if (map->l_info[DT_SONAME]) {
		MPRINTF(LOAD, "SONAME: %s\n",
			&strtab[map->l_info[DT_SONAME]->d_un.d_val]);
	}
	/* 依存している共有ライブラリを表示する */
	if (map->l_info[DT_NEEDED]) {
		for (dyn = map->l_ld; dyn->d_tag != DT_NULL; dyn++) {
			if (dyn->d_tag != DT_NEEDED)
				continue;
			MPRINTF(LOAD, "NEEDED: %s\n", &strtab[dyn->d_un.d_val]);
		}
	}
	/* REF: _dl_setup_hash [dl-lookup.c] */
	/* シンボルハッシュテーブルを使用する準備をする */
	if (map->l_info[DT_HASH]) {
		Elf_Symndx *hash;
		Elf_Symndx nchain;

		hash = (void *) D_PTR(map, DT_HASH);
		map->l_nbuckets = *hash++;
		nchain = *hash++;
		map->l_buckets = hash;
		hash += map->l_nbuckets;
		map->l_chain = hash;
	}
	check_dynamic_assert(map);	     /* いろいろチェック */
	MPRINT_END(LOAD);
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
parse_phdr(struct link_map *l, struct loadcmd *loadcmds,
	   const ElfW(Phdr) *phdr, bool *has_holes)
{
	int nloadcmds;
	struct loadcmd *c;
	unsigned int stack_flags = PF_R|PF_W|PF_X;
	const ElfW(Phdr) *ph;

	*has_holes = false;

	nloadcmds = 0;
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
			/* Thread Local Storage 未実装． */
			dputs("PT_TLS not supported\n");
			break;
		}
	}
	return nloadcmds;
}

static void
map_object_loadcmd(struct link_map *l,
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
		if (c->mapend > c->mapstart) {
			/* セグメントをマップする */
			void *r = mmap((void *) (l->l_addr + c->mapstart),
				       c->mapend - c->mapstart, c->prot,
				       MAP_FIXED|MAP_PRIVATE, fd, c->mapoff);
			MPRINTF(LOAD, "mmap %#lx - %#lx  offset=%#lx\n",
				(l->l_addr + c->mapstart),
				(l->l_addr + c->mapstart)
				+ (c->mapend - c->mapstart),
				c->mapoff);
			if (r == MAP_FAILED)
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

		if (c->allocend > c->dataend) {
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
			if (zeropage > zero) {
				/* Zero the final part of the last page of the segment.  */
				if ((c->prot & PROT_WRITE) == 0) {
					/* Dag nab it.  */
					if (__mprotect((caddr_t)
						       (zero & ~(GLRO(dl_pagesize) - 1)),
						       GLRO(dl_pagesize), c->prot|PROT_WRITE) < 0)
						dprintf_die("mprotect failed");
				}
				memset((void *) zero, '\0', zeropage - zero);
				if ((c->prot & PROT_WRITE) == 0) {
					if (__mprotect((caddr_t)
						       (zero & ~(GLRO(dl_pagesize) - 1)),
						       GLRO(dl_pagesize), c->prot) < 0)
						dprintf_die("mprotect failed");
				}
			}
			if (zeroend > zeropage) {
				/* Map the remaining zero pages in from the zero fill FD.  */
				caddr_t mapat;
				mapat = mmap((caddr_t) zeropage, zeroend - zeropage,
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

	l = create_link_map();
	fb.len = __read(fd, fb.buf, sizeof(fb.buf));
	if (fb.len < 0)
		dprintf_die("read error");

	header = (void *) fb.buf;
	if (header->e_type != ET_DYN && header->e_type != ET_EXEC)
		dprintf_die("Unsupported ELF type (%d)", header->e_type);

	l->l_entry = header->e_entry;
	l->l_phnum = header->e_phnum;

	maplength = header->e_phnum * sizeof (ElfW(Phdr));
	if (header->e_phoff + maplength <= (size_t) fb.len)
		phdr = (void *) (fb.buf + header->e_phoff);
	else {
		phdr = ealloca(maplength);
		__lseek (fd, header->e_phoff, SEEK_SET);
		if (__read(fd, (void *) phdr, maplength) < 0)
			dprintf_die("read failed");
	}

	{
		struct loadcmd loadcmds[l->l_phnum], *c;
		size_t nloadcmds;
		bool has_holes = false, postmap = false;

		nloadcmds = parse_phdr(l, loadcmds, phdr, &has_holes);
		c = loadcmds;

		maplength = loadcmds[nloadcmds - 1].allocend - c->mapstart;
		if (header->e_type == ET_DYN) {
			l->l_map_start = (ElfW(Addr))
				mmap(NULL, maplength, c->prot, MAP_PRIVATE, fd, 0);
			l->l_map_end = l->l_map_start + maplength;
			l->l_addr    = l->l_map_start - c->mapstart;
			if (has_holes) {
				__mprotect((caddr_t) (l->l_addr + c->mapend),
					   loadcmds[nloadcmds - 1].mapstart - c->mapend,
					   PROT_NONE);
			}
			l->l_contiguous = 1;
			postmap = true;
		}
		map_object_loadcmd(l, fd, header, c,
				   nloadcmds, maplength, has_holes, postmap);
	}

	l->l_name = realname;
	if (l->l_ld != NULL)
		l->l_ld = (void *) ((ElfW(Addr))l->l_ld + l->l_addr);
	if (l->l_phdr != NULL)
		l->l_phdr = (void *) ((ElfW(Addr))l->l_phdr + l->l_addr);
	parse_dynamic(l);

	MPRINT_START_FMT(LOAD, "MAPPING INFO (%s)", soname);
	MPRINTF(LOAD, "size: 0x%lx\n", maplength);
	MPRINTF(LOAD, "map address: 0x%lx\n", l->l_map_start);
	MPRINTF(LOAD, "base address: 0x%lx\n", l->l_addr);
	MPRINTF(LOAD, "DYNAMIC     : 0x%lx\n", (unsigned long) l->l_ld);
	MPRINT_END(LOAD);

	return l;
}

struct link_map *map_object(struct link_map *loader, const char *soname)
{
	int fd;
	char *realname;
	struct link_map *new, *l;

	fd = open_path(soname, &realname);
	if (fd < 0)
		dprintf_die(" Library not found. (%s)\n", soname);
	/* ロード済みのライブラリを検索 */
	for (l = GL(namespace); l != NULL; l = l->l_next) {
		if (__strcmp(l->l_name, realname) == 0) {
			__close(fd);
			return l;
		}
	}
	new = map_object_fd(loader, fd, soname, realname);
	__close(fd);

	return new;
}

static void append_to_namespace(struct link_map *l)
{
	struct link_map **next, *prev;

	prev = NULL;
	next = &GL(namespace);
	for (; *next != NULL; prev = *next, next = &((*next)->l_next))
		if (*next == l)
			return;
	l->l_prev = prev;
	l->l_next = NULL;
	*next = l;
}

void map_object_deps(struct link_map *root_map)
{
	int nlist;
	ElfW(Dyn) *dyn;
	LIST_HEAD(runlist);

	MPRINT_START(LOAD, "IN map_obj_deps");
	parse_dynamic(root_map);
	struct runlist {
		struct link_map *map;
		struct list_head list;
	};
	inline void chain_runlist(struct link_map *map, struct runlist *r) {
		r->map = map;
		list_add_tail(&r->list, &runlist);
	}
	chain_runlist(root_map, ealloca(sizeof(runlist)));
	nlist = 0;
	struct runlist *runp;
	list_for_each_entry(runp, &runlist, list) {
		struct link_map *l = runp->map;

		for (dyn = l->l_ld; dyn->d_tag != DT_NULL; dyn++) {
			struct link_map *new;
			const char *soname;
			const char *strtab = (const void *) D_PTR(l, DT_STRTAB);

			if (dyn->d_tag != DT_NEEDED)
				continue;
			soname = &strtab[dyn->d_un.d_val];
			new = map_object(l, soname);
			if (!new->l_reserved) {
				MPRINTF(LOAD, "LOADED %s\n", new->l_name);
				new->l_reserved = 1;
				nlist++;
				chain_runlist(new, ealloca(sizeof(runlist)));
			}
			append_to_namespace(new);
		}
	}
	if (nlist > 0) {
		int i = 0;
		root_map->l_searchlist.r_list =
			emalloc(sizeof(struct link_map *) * nlist);
		root_map->l_searchlist.r_nlist = nlist;
		list_for_each_entry(runp, &runlist, list) {
			struct link_map *l = runp->map;
			if (l == root_map)
				continue;
			root_map->l_searchlist.r_list[i++] = l;
		}
	}
	print_namespace();

	MPRINT_END(LOAD);
}
