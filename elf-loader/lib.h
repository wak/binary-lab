#ifndef LIB_H
#define LIB_H

#include "loader.h"
#include <sys/syscall.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

// /usr/include/asm/unistd_64.h
// /usr/include/bits/syscall.h

extern int errno;

#define HIDDEN(symbol) asm(".hidden " #symbol "\n\r");

#define __set_errno(err) (errno = err)
#undef alloca
#define alloca(size) __builtin_alloca(size)
extern long int syscall(long int __sysno, ...) rtld_local;
extern size_t strlen(const char *) rtld_local;
void *memcpy(void *s1, const void *s2, size_t n);
void *memset(void *s, int c, size_t n);
void *malloc(size_t size);
void free(void *ptr);
void *realloc(void *ptr, size_t size);

// for debug
extern int dputs(const char *) rtld_local;


#endif
