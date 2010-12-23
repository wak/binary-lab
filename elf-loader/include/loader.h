#ifndef LOADER_H
#define LOADER_H

#include <link.h>
#include <defs.h>

#define rtld_local __attribute__ ((visibility("hidden")))

#define EXPORT_VAR(name, export) \
	extern typeof(name) export \
	__attribute__ ((alias (#name), visibility ("hidden")))

#define DEFINE_GLO_VAR(type, name)   \
	extern typeof(type) _##name; \
	EXPORT_VAR(_##name, name);   \
	type _##name

#define DECLARE_GLO_VAR(type, name) \
	extern typeof(type) name    \
	__attribute__ ((visibility("hidden")))

struct program_info {
	int argc;
	char **argv;
	char **envp;
	void * entry;
	ElfW(Ehdr) *ehdr;
	ElfW(Phdr) *phdr;
	ElfW(Half) phnum;
};

#define D_PTR(map, i) ((map)->i->d_un.d_ptr + (map)->l_addr)

struct rtld_global_ro
{
	/* Cached value of `getpagesize ()'.  */
	size_t _dl_pagesize;

	/* If nonzero print warnings messages.  */
	int _dl_verbose;

	/* Do we do lazy relocations?  */
	int _dl_lazy;
};
#define DL_NNS 16
#define MAX_PATH 5
struct rtld_global
{
	ElfW(Word) _dl_stack_flags;
	struct link_map *_namespace;
	/* Library search pathes */
	char *_rpath[MAX_PATH+1];
};

DECLARE_GLO_VAR(struct rtld_global_ro, _rtld_global_ro);
DECLARE_GLO_VAR(struct rtld_global, _rtld_global);
#define GLRO(name) _rtld_global_ro._##name
#define GL(name) _rtld_global._##name

#endif
