#ifndef DEFS_H
#define DEFS_H

#ifndef NULL
# define NULL ((void *) 0)
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#ifdef errno
# undef errno
#endif

#endif
