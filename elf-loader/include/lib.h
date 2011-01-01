#ifndef LIB_H
#define LIB_H

#include <stdarg.h>

#include <sys/syscall.h>
#include <linux/errno.h>
#include <scope.h>

#include <defs.h>

/* デバッグ表示用 */
#define DEBUG_INDENT 2
#define DEBUG_PRINT_BOOTPARAMS 0
#define DEBUG_PRINT_PROGINFO   0
#define DEBUG_PRINT_MAPS       0
#define DEBUG_PRINT_LOAD       0
#define DEBUG_PRINT_RELOC      0
#define DEBUG_PRINT_LAZYRELOC  0

#define __set_errno(err) (errno = err)

DECLARE_GLO_VAR(int, errno);
DECLARE_GLO_VAR(size_t, __pagesize);

// /usr/include/asm/unistd_64.h
// /usr/include/bits/syscall.h

/* unistd */
extern int __open(const char *file, int oflag, ...);
extern int __close(int fd);
extern int __read(int fd, char *buf, size_t count);
extern int __fstat(int fd, struct stat *buf);
extern off_t __lseek(int fd, off_t offset, int whence);
extern void _exit(int status) __attribute__ ((noreturn));

extern void *mmap(void *, size_t, int, int, int, off_t);
extern int munmap(void *start, size_t length);
extern int __mprotect(const void *addr, size_t len, int prot);

/* system calls */
extern long int syscall(long int sysno, ...);
#define _syscall(sysno, args...)			\
	({						\
		long int ret = syscall(sysno, ##args);	\
		ret;					\
	})

/* string.h */
extern size_t __strlen(const char *);
extern char *__strcpy(char *dest, const char *src);
extern char *__strdup(const char *s);
extern int __strcmp(register const char *s1, register const char *s2);

/* stdlib.h */
extern void *memcpy(void *s1, const void *s2, size_t n);
extern void *memset(void *s, int c, size_t n);
extern void *malloc(size_t size);
extern void free(void *ptr);
extern void *realloc(void *ptr, size_t size);
#define PRINT_HERE()							\
	dprintf("func: %s [%s:%d]", __func__, __FILE__, __LINE__)
#undef alloca
#define alloca(size) __builtin_alloca(size)
#define ealloca(size)							\
	({								\
		void *p = alloca(size);					\
		if (p == NULL) { PRINT_HERE(); _exit(1); }		\
		p;							\
	})
#define emalloc(size) _emalloc(size, __LINE__, __FILE__, __func__)
extern void *_emalloc(size_t size, int line,
		      const char *file, const char *func);

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
extern int dputs(const char *);
static inline void dputs_die(const char *m) { dputs(m); _exit(1); }

#define dprintf_die(fmt, args...) \
	_dprintf_die(__LINE__, __FILE__, __func__, fmt, ##args)
extern void _dprintf_die(unsigned int line,
			 const char *file,
			 const char *func,
			 const char *format, ...)
	__attribute__ ((format (printf, 4, 5)));
extern int dprintf(const char *format, ...)
	__attribute__ ((format (printf, 1, 2)));
extern int dsnprintf(char *buf, size_t size, const char *format, ...)
	__attribute__ ((format (printf, 3, 4)));
extern void mprint_start_fmt(const char *format, ...)
	__attribute__ ((format (printf, 1, 2)));

extern void mprintf(const char *format, ...);
static inline void mprint_start(const char *str) {
	mprint_start_fmt("%s", str);
}
extern void mprint_end(void);

static inline void p(void *ptr) {
	dprintf("Poiter: %p\n", (ptr));
}

#undef assert
#define assert(cond)						\
	if (!(cond)) {						\
		dprintf("Assert error: L.%d [%s] %s: %s\n",	\
			__LINE__, __FILE__, __func__, #cond);	\
		syscall(SYS_exit, 1);				\
	}

extern int dvsprintf(char *buffer, size_t buffer_size,
		     const char *format, va_list arg);

#define MPRINTF(name, fmt, arg...)					\
	(DEBUG_PRINT_##name ? mprintf(fmt, ##arg ) : (void) 0)
#define MPRINT_START(name, mark)				\
	(DEBUG_PRINT_##name ? mprint_start(mark) : (void) 0)
#define MPRINT_START_FMT(name, fmt, arg...)				\
	(DEBUG_PRINT_##name ? mprint_start_fmt(fmt, ##arg ) : (void) 0)
#define MPRINT_END(name)					\
	(DEBUG_PRINT_##name ? mprint_end() : (void) 0)

/* Others */
extern void malloc_init(void);

#endif
