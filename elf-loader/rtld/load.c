#include <elf.h>
#include <loader.h>
#include <lib.h>
#include <ldsodefs.h>

/* sysdeps/generic/ldsodefs.h */
/* REF: _dl_map_object [glibc/elf/dl-load.c] */
/* REF: _dl_map_object_deps */
/* REF: _dl_open_worker [dl-open.c] */

void map_object_deps(link_map *map)
{
	ElfW(Dyn) *dyn;
	struct link_map *new;

	new = emalloc(sizeof(struct link_map));
	return;
	init_link_map(new);

	if (map->l_ld == NULL)
		return;
	for (dyn = map->l_ld; dyn->d_tag != DT_NULL; dyn++) {
		if (dyn->d_tag < DT_NUM)
			new->l_info[dyn->d_tag] = dyn;
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
	if (new->l_info[DT_NEEDED])
		assert(new->l_info[DT_SYMTAB] != NULL);
}
HIDDEN(map_object_deps);
