#ifndef LINK_H
#define LINK_H

#include <elf.h>
#include <list.h>

#include <defs.h>

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

	//struct libname_list *l_libname; //soname list

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


	unsigned int l_relocated:1;	/* Nonzero if object's relocations done.  */
	unsigned int l_init_called:1;	/* Nonzero if DT_INIT function called.  */
	unsigned int l_global:1;	/* Nonzero if object in _dl_global_scope.  */
	unsigned int l_reserved:2;	/* Reserved for internal use.  */
	unsigned int l_phdr_allocated:1; /* Nonzero if the data structure pointed
					    to by `l_phdr' is allocated.  */
	unsigned int l_soname_added:1;	/* Nonzero if the SONAME is for sure in
					    the l_libname list.  */
	unsigned int l_faked:1;		/* Nonzero if this is a faked descriptor
					   without associated file.  */
	unsigned int l_need_tls_init:1; /* Nonzero if GL(dl_init_static_tls)
					   should be called on this link map
					   when relocation finishes.  */
	unsigned int l_used:1;		/* Nonzero if the DSO is used.  */
	unsigned int l_auditing:1;	/* Nonzero if the DSO is used in auditing.  */
	unsigned int l_audit_any_plt:1; /* Nonzero if at least one audit module
					   is interested in the PLT interception.*/
	unsigned int l_removed:1;	/* Nozero if the object cannot be used anymore
					   since it is removed.  */
	unsigned int l_contiguous:1;	/* Nonzero if inter-segment holes are
					   mprotected or if no holes are present at
					   all.  */

	/* Information used to change permission after the relocations are
	   done.  */
	ElfW(Addr) l_relro_addr;
	size_t l_relro_size;

	/* Linked at GL(namespace) */
//	struct list_head list;
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

/* REF: _dl_new_object [dl-object.c]  */
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
	l->l_next = l->l_prev = NULL;
//	INIT_LIST_HEAD(&l->list);
	l->l_searchlist.r_list = NULL;
	l->l_searchlist.r_nlist = 0;
}

#endif
