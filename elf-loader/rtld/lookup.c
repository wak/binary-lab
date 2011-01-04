#include <link.h>
#include <lookup.h>
#include <ldsodefs.h>
#include <lib.h>

/* REF: _dl_elf_hash [dl-hash.h] */
static unsigned int
elf_hash (const char *name_arg)
{
	const unsigned char *name = (const unsigned char *) name_arg;
	unsigned long int hash = 0;
	while (*name != '\0')
	{
		unsigned long int hi;
		hash = (hash << 4) + *name++;
		hi = hash & 0xf0000000;
	  
		/* The algorithm specified in the ELF ABI is as
		   follows:

		   if (hi != 0)
		   hash ^= hi >> 24;

		   hash &= ~hi;

		   But the following is equivalent and a lot
		   faster, especially on modern processors.  */

		hash ^= hi;
		hash ^= hi >> 24;
	}
	return hash;
}

/* REF: do_lookup_x [do-lookup.h] */
int lookup_symbol(const struct link_map *skip,
		  const char *name, struct sym_val *result)
{
	unsigned int hash;
	struct link_map *l;
	Elf_Symndx symidx;
	int type_class = 1;

	/* セクションヘッダテーブルを意識しない限り，DT_SYMTABのサイズが分からな
	 * い．そのため，HASHを利用して探す． */
	hash = elf_hash(name);
	for (l = GL(namespace); l; l = l->l_next) {
		const ElfW(Sym) *symtab = (const void *) D_PTR(l, DT_SYMTAB);
		const char *strtab = (const void *) D_PTR(l, DT_STRTAB);
		const ElfW(Sym) *sym;

		if (l == skip)
			continue;
		if (symtab == NULL || strtab == NULL)
			continue;

		const ElfW(Sym) * __attribute__ ((__noinline__))
			check_match(const ElfW(Sym) *sym)
		{
			if ((sym->st_value == 0 /* No value.  */
			     /*&& ELFW(ST_TYPE)(sym->st_info) != STT_TLS*/)
			    || (type_class & (sym->st_shndx == SHN_UNDEF)))
				return NULL;

			if (!(ELFW(ST_TYPE) (sym->st_info) == STT_NOTYPE ||
			      ELFW(ST_TYPE) (sym->st_info) == STT_OBJECT ||
			      ELFW(ST_TYPE) (sym->st_info) == STT_FUNC ||
			      ELFW(ST_TYPE) (sym->st_info) == STT_COMMON))
				/* Ignore all but STT_NOTYPE, STT_OBJECT, STT_FUNC, and STT_COMMON
				   entries (and STT_TLS if TLS is supported) since these
				   are no code/data definitions.  */
				return NULL;

			if (__strcmp(strtab + sym->st_name, name) != 0)
				return NULL;

			return sym;
		}
		for (symidx = l->l_buckets[hash % l->l_nbuckets];
		     symidx != STN_UNDEF;
		     symidx = l->l_chain[symidx])
		{
			sym = check_match(&symtab[symidx]);
			if (sym != NULL) {
				result->s = sym;
				result->m = l;
				return 0;
			}
		}
	}
	return 1;
}
