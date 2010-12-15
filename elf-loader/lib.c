#include "lib.h"

int errno;
extern int errno;

size_t strlen(const char *s)
{
	register const char *p;

	for (p=s ; *p ; p++);
	return p - s;
}

// not print newline
int dputs(const char *s)
{
	return syscall(SYS_write, 2, s, strlen(s));
}

typedef unsigned char uchar;

void *memset(void *s, int c, size_t n)
{
	register uchar *p = (uchar *) s;

	while (n) {
		*p++ = (uchar) c;
		--n;
	}

	return s;
}
HIDDEN(memset)

void *memcpy(void * s1, const void * s2, size_t n)
{
	register char *r1 = s1;
	register const char *r2 = s2;

	while (n) {
		*r1++ = *r2++;
		--n;
	}

	return s1;
}
HIDDEN(memcpy)

void *mmap(void *start, size_t length, int prot, int flags,
	   int fd, off_t offset) {
	return (void *)
		syscall(SYS_mmap,
			start, length, prot, flags, fd, offset);
}
HIDDEN(mmap);

int munmap(void *start, size_t length) {
	return syscall(SYS_munmap, start, length);
}
HIDDEN(munmap);

/* void * __curbrk rtld_hidden = NULL; */
/* int brk(void *addr) */
/* { */
/* 	void *newbrk = __syscall_brk(addr); */

/* 	__curbrk = newbrk; */

/* 	if (newbrk < addr) { */
/* 		__set_errno (ENOMEM); */
/* 		return -1; */
/* 	} */

/* 	return 0; */
/* } */
/* void * sbrk(intptr_t increment) */
/* { */
/*     void *oldbrk; */

/*     if (__curbrk == NULL) */
/* 	if (brk (NULL) < 0)	/\* Initialize the break.  *\/ */
/* 	    return (void *) -1; */

/*     if (increment == 0) */
/* 	return __curbrk; */

/*     oldbrk = __curbrk; */
/*     if (brk (oldbrk + increment) < 0) */
/* 	return (void *) -1; */

/*     return oldbrk; */
/* } */
