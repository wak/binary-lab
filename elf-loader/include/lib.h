#ifndef LIB_H
#define LIB_H

#include <loader.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <link.h>

#define HIDDEN(symbol) asm(".hidden " #symbol "\n\r");
#define __set_errno(err) (errno = err)

DECLARE_GLO_VAR(int, errno);
DECLARE_GLO_VAR(size_t, __pagesize);

// /usr/include/asm/unistd_64.h
// /usr/include/bits/syscall.h

/* system calls */
extern long int syscall(long int __sysno, ...) rtld_local;
extern void _exit(int status) __attribute__ ((noreturn));


/* string.h */
extern size_t strlen(const char *) rtld_local;

/* stdlib.h */
void *memcpy(void *s1, const void *s2, size_t n);
void *memset(void *s, int c, size_t n);
void *malloc(size_t size);
void free(void *ptr);
void *realloc(void *ptr, size_t size);
#undef alloca
#define alloca(size) __builtin_alloca(size)

#define emalloc(size)					\
	_emalloc(size, __LINE__, __FILE__, __func__)
extern void *_emalloc(size_t size, int line,
		      const char *file, const char *func) rtld_local;

/* ctype.h 
 * REF: uClibc-0.9.31/libc/sysdeps/linux/common/bits/uClibc_ctype.h
 */
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

/* for Debug */
void print_mark(const char *str);
void print_mark_end(void);

extern int dputs(const char *) rtld_local;
static inline void dputs_die(const char *m) { dputs(m); _exit(1); }

void dprintf_die(const char *format, ...)
	__attribute__ ((format (printf, 1, 2)));
int dprintf(const char *format, ...)
	__attribute__ ((format (printf, 1, 2)));
int dsprintf(char *buf, size_t size, const char *format, ...)
	__attribute__ ((format (printf, 3, 4)));

#undef assert
#define assert(cond)				\
	if (!(cond)) {				\
		dprintf("Assert error: L.%d [%s] %s: %s\n",		\
			__LINE__, __FILE__, __func__, #cond);		\
		syscall(SYS_exit, 1);					\
	}


/* Others */
void malloc_init(void);

#endif
