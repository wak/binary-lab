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
 
	/* CLK_TCK as reported by the kernel.  */
	int _dl_clktck;

	/* If nonzero print warnings messages.  */
	int _dl_verbose;

	/* Do we do lazy relocations?  */
	int _dl_lazy;

	/* Syscall handling improvements.  This is very specific to x86.  */
	//EXTERN uintptr_t _dl_sysinfo;
};
#define DL_NNS 16
#define MAX_PATH 5
struct rtld_global
{
	ElfW(Word) _dl_stack_flags;
	
	struct link_namespaces
	{
		/* A pointer to the map for the main map.  */
		struct link_map *_ns_loaded;
		/* Number of object in the _dl_loaded list.  */
		unsigned int _ns_nloaded;
		/* Direct pointer to the searchlist of the main object.  */
		struct r_scope_elem *_ns_main_searchlist;
		/* This is zero at program start to signal that the global scope map is
		   allocated by rtld.  Later it keeps the size of the map.  It might be
		   reset if in _dl_close if the last global object is removed.  */
		size_t _ns_global_scope_alloc;
		/* Keep track of changes to each namespace' list.  */
		//struct r_debug _ns_debug;
	} _dl_ns[DL_NNS];

	struct link_map *_namespace;
//	struct list_head _namespace;
	/* Library search pathes */
	char *_rpath[MAX_PATH+1];
};

DECLARE_GLO_VAR(struct rtld_global_ro, _rtld_global_ro);
DECLARE_GLO_VAR(struct rtld_global, _rtld_global);
#define GLRO(name) _rtld_global_ro._##name
#define GL(name) _rtld_global._##name

#endif
