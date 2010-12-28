#ifndef LOOKUP_H
#define LOOKUP_H

struct sym_val
{
	const ElfW(Sym) *s;
	struct link_map *m;
};

extern int lookup_symbol(const char *name, struct sym_val *result);

#endif
