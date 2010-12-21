#ifndef LOADER_H
#define LOADER_H

#include <link.h>

#define rtld_local __attribute__ ((visibility("hidden")))
#define _rtld_local_ro __attribute__ ((visibility("hidden")))
#define attribute_relro __attribute__ ((section (".data.rel.ro")))

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

/* USAGE:
 *   DEFINE_GLO_VAR(int, global) = 0;
 */

struct program_info {
	int argc;
	char **argv;
	char **envp;
	void * entry;
	ElfW(Ehdr) *ehdr;
	ElfW(Phdr) *phdr;
	ElfW(Half) phnum;
};

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

#define GLRO(name) _rtld_global_ro._##name

#endif
