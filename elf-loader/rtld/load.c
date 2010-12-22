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
			dprintf("\"%s\" ", l->l_searchlist.r_list[search]->l_name);
		dprintf("\n");
		i++;
	}
	print_mark_end();
}

struct link_map *map_object(struct link_map *loader, const char *soname)
{
	int fd;
	char *realname;
	struct link_map *l;

	/* search loaded library */
	for (l = GL(namespace); l != NULL; l = l->l_next) {
		dputs(l->l_name);
	}
	fd = open_path(soname, &realname);
	if (fd < 0)
		dprintf_die(" Library not found. (%s)\n", soname);
	//new = map_object_fd(map, fd);
	print_namespace();
}
HIDDEN(map_object);

void map_object_deps(link_map *map)
{
	ElfW(Dyn) *dyn;
	struct link_map *new;
	const char *soname;
	const char *strtab = (const void *) D_PTR (map, l_info[DT_STRTAB]);


	for (dyn = map->l_ld; dyn->d_tag != DT_NULL; dyn++) {
		if (dyn->d_tag != DT_NEEDED)
			continue;
		soname = &strtab[dyn->d_un.d_val];
		dprintf("  NEEDED: %s\n", soname);
		new = map_object(map, soname);
		/* write here */
	}
}
HIDDEN(map_object_deps);
