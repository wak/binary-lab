#ifndef LDSODEFS_H
#define LDSODEFS_H

#include <scope.h>
#include <link.h>
//struct link_map;

extern void parse_dynamic(struct link_map *map);
extern void map_object_deps(struct link_map *map);
extern struct link_map *map_object(struct link_map *loader, const char *soname);
extern void reloc_all(void);

#include <stdbool.h>

//typedef int bool;
//#define true 1
//#define false 0

struct rtld_global_ro
{
	/* Cached value of `getpagesize ()'.  */
	size_t _dl_pagesize;
};

#define MAX_PATH 10
struct rtld_global
{
	ElfW(Word) _dl_stack_flags;
	struct link_map *_namespace;
	char *_rpath[MAX_PATH+1];	     /* Library search pathes */
};

DECLARE_GLO_VAR(struct rtld_global_ro, _rtld_global_ro);
DECLARE_GLO_VAR(struct rtld_global, _rtld_global);

#define GLRO(name) (_rtld_global_ro._##name)
#define GL(name) (_rtld_global._##name)

#endif
