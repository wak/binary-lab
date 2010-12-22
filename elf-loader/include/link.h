#ifndef LINK_H
#define LINK_H

#include <elf.h>
#include <list.h>
#include <stdio.h>

#include <include-last.h>

#define ElfW(type) Elf64_##type


/* Structure to describe a single list of scope elements.  The lookup
   functions get passed an array of pointers to such structures.  */
struct r_scope_elem
{
	/* Array of maps for the scope.  */
	struct link_map **r_list;
	/* Number of entries in the scope.  */
	unsigned int r_nlist;
};

#ifndef DT_THISPROCNUM
# define DT_THISPROCNUM 0
#endif

struct link_map {
	ElfW(Addr) l_addr;	 /* Base address shared object is loaded at.  */
	char *l_name;		 /* Absolute file name object was found in.  */
	ElfW(Dyn) *l_ld;	 /* Dynamic section of the shared object.  */
	struct link_map *l_next, *l_prev;    /* Chain of loaded objects.  */
	
	const ElfW(Phdr) *l_phdr; /* Pointer to program header table in core.  */
	ElfW(Addr) l_entry;	  /* Entry point location.  */
	ElfW(Half) l_phnum;	  /* Number of program header entries.  */
	ElfW(Half) l_ldnum;	  /* Number of dynamic segment entries.  */


	/* Start and finish of memory map for this object.
	 * l_map_start need not be the same as l_addr.  */
	ElfW(Addr) l_map_start, l_map_end;
	/* End of the executable part of the mapping.  */
	ElfW(Addr) l_text_end;

	/** Indexed pointers to dynamic section.
	 *
	 * 0..DT_NUM
	 *     indexed by the processor-independent tags
	 *  ..+DT_THISPROCNUM
	 *     indexed by the tag minus DT_LOPROC
	 *  ..+DT_VERSIONTAGNUM
	 *     DT_VERSIONTAGIDX(tagvalue)
	 *  ..+DT_EXTRANUM
	 *     DT_EXTRATAGIDX(tagvalue)
	 *  ..+DT_VALNUM
	 *     DT_VALTAGIDX(tagvalue)
	 *  ..+DT_ADDRNUM
	 *     DT_ADDRTAGIDX(tagvalue)
	 */
	ElfW(Dyn) *l_info[DT_NUM + DT_THISPROCNUM + DT_VERSIONTAGNUM
			  + DT_EXTRANUM + DT_VALNUM + DT_ADDRNUM];

	struct r_scope_elem l_searchlist;

	int l_relocated;

	/* Information used to change permission after the relocations are
	   done.  */
	ElfW(Addr) l_relro_addr;
	size_t l_relro_size;

	struct list_head list;
};
typedef struct link_map link_map;

/* for link_map.l_info */
#define L_VERSYMIDX(sym) (DT_NUM + DT_THISPROCNUM	\
			  + DT_VERSIONTAGIDX(sym))
#define L_EXTRAIDX(tag)  (DT_NUM + DT_THISPROCNUM + DT_VERSIONTAGNUM	\
			  + DT_EXTRATAGIDX(dyn->d_tag))
#define L_VALIDX(tag)    (DT_NUM + DT_THISPROCNUM + DT_VERSIONTAGNUM	\
			  + DT_EXTRANUM + DT_VALTAGIDX(tag))
#define L_ADDRIDX(tag)   (DT_NUM + DT_THISPROCNUM + DT_VERSIONTAGNUM	\
			  + DT_EXTRANUM + DT_VALNUM + DT_ADDRTAGIDX(tag))

static inline void init_link_map(struct link_map *l)
{
	int i;

	*l = (struct link_map) {
		.l_addr = 0,
		.l_name = NULL,
		.l_ld = NULL,
		.l_next = NULL,
		.l_prev = NULL,
		.l_phdr = NULL,
		.l_entry = 0,
		.l_phnum = 0,
		.l_ldnum = 0,
		.l_relocated = 0,
	};
	l->l_relro_addr = l->l_relro_size = 0;
	l->l_text_end = l->l_map_end = 0;
	l->l_map_start = ~0;
	for (i = 0; i < sizeof(l->l_info) / sizeof(*l->l_info); i++)
		l->l_info[i] = NULL;
	INIT_LIST_HEAD(&l->list);
	l->l_searchlist.r_list = NULL;
	l->l_searchlist.r_nlist = 0;
}

#endif
