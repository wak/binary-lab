#ifndef LDSODEFS_H
#define LDSODEFS_H

#include <link.h>

extern void map_object_deps(link_map *map);
extern void reloc_all(void);
typedef int bool;
#define true 1
#define false 1

#endif
