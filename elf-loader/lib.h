#ifndef LIB_H
#define LIB_H

#include "loader.h"
#include <sys/syscall.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

// /usr/include/asm/unistd_64.h
// /usr/include/bits/syscall.h

//extern int errno;
DECLARE_GLO_VAR(int, errno);

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
int dprintf(const char *format, ...)
	__attribute__ ((format (printf, 1, 2)));

#define ERR_EXIT(err) {					\
	  syscall(SYS_write, 2, err, sizeof(err)-1);	\
	  syscall(SYS_exit, 1);				\
  }
#undef assert
#define assert(cond)				\
	if (!(cond)) {				\
		dprintf("Assert error: L.%d [%s] %s: %s\n",		\
			__LINE__, __FILE__, __func__, #cond);		\
		syscall(SYS_exit, 1);					\
	}


#endif