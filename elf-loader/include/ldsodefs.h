#ifndef LDSODEFS_H
#define LDSODEFS_H

#include <scope.h>

extern void map_object_deps(link_map *map);
extern void reloc_all(void);
typedef int bool;
#define true 1
#define false 1

struct rtld_global_ro
{
	/* Cached value of `getpagesize ()'.  */
	size_t _dl_pagesize;
};

#define MAX_PATH 5
struct rtld_global
{
	ElfW(Word) _dl_stack_flags;
	struct link_map *_namespace;
	char *_rpath[MAX_PATH+1];	     /* Library search pathes */
};

DECLARE_GLO_VAR(struct rtld_global_ro, _rtld_global_ro);
DECLARE_GLO_VAR(struct rtld_global, _rtld_global);
#define GLRO(name) _rtld_global_ro._##name
#define GL(name) _rtld_global._##name

#endif
