#ifndef LIB_H
#define LIB_H

#include <loader.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

// /usr/include/asm/unistd_64.h
// /usr/include/bits/syscall.h

#define ElfW(type) Elf64_##type

DECLARE_GLO_VAR(int, errno);
DECLARE_GLO_VAR(size_t, __pagesize);

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
void malloc_init(void);

void print_mark(const char *str);
void print_mark_end(void);

// ctype.h
// from uClibc-0.9.31/libc/sysdeps/linux/common/bits/uClibc_ctype.h
#undef isdigit
#define isdigit(a) ((unsigned)((a) - '0') <= 9)
#undef isalpha
#define isalpha(c) \
	((sizeof(c) == sizeof(char)) \
	 ? (((unsigned char)(((c) | 0x20) - 'a')) < 26) \
	 : (((unsigned int)(((c) | 0x20) - 'a')) < 26))
#undef isxdigit
#define isxdigit(c) \
	(isdigit(c) \
	 || ((sizeof(c) == sizeof(char)) \
		 ? (((unsigned char)((((c)) | 0x20) - 'a')) < 6) \
		 : (((unsigned int)((((c)) | 0x20) - 'a')) < 6)))
#undef isprint
#define isprint(c) \
	((sizeof(c) == sizeof(char)) \
	 ? (((unsigned char)((c) - 0x20)) <= (0x7e - 0x20)) \
	 : (((unsigned int)((c) - 0x20)) <= (0x7e - 0x20)))

// for debug
extern int dputs(const char *) rtld_local;
int dprintf(const char *format, ...)
	__attribute__ ((format (printf, 1, 2)));
int dsprintf(char *buf, size_t size, const char *format, ...)
	__attribute__ ((format (printf, 3, 4)));

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
