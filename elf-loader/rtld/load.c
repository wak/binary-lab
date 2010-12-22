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
	if (fd < 0)
		dprintf_die(" Library not found. (%s)\n", soname);
	return fd;
}

void map_object_deps(link_map *map)
{
	ElfW(Dyn) *dyn;
	struct link_map *new;
	const char *soname;
	const char *strtab = (const void *) D_PTR (map, l_info[DT_STRTAB]);

	new = emalloc(sizeof(struct link_map));
	init_link_map(new);

	for (dyn = map->l_ld; dyn->d_tag != DT_NULL; dyn++) {
		if (dyn->d_tag != DT_NEEDED)
			continue;
		soname = &strtab[dyn->d_un.d_val];
		dprintf("  NEEDED: %s\n", soname);
	}
}
HIDDEN(map_object_deps);
