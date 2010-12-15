#include <stdarg.h>
#include "lib.h"

DEFINE_GLO_VAR(int, errno) = 0;
//int errno = 0;

size_t strlen(const char *s)
{
	register const char *p;

	for (p=s ; *p ; p++);
	return p - s;
}
HIDDEN(strlen);

/* Copy SRC to DEST.  */
char *strcpy(char *dest, const char *src)
{
	char *dst = dest;

	while ((*dst = *src) != '\0') {
		src++;
		dst++;
	}

	return dest;
}
HIDDEN(strcpy);

// not print newline
int dputs(const char *s)
{
	return syscall(SYS_write, 2, s, strlen(s));
}
HIDDEN(dputs);

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

inline static void reverse(char *s, char *t)
{
	char tmp;
	for (; s < t; s++, t--) {
		tmp = *t;
		*t = *s;
		*s = tmp;
	}
}

int dprintf(const char *format, ...)
{
	va_list arg;
	int rv;
	const char *fmt;
	char buffer[1024], *s;
	long long number;
	int fl, fll, size;

	va_start(arg, format);
	
	s = buffer;
	for(fmt = format; *fmt; ++fmt) {
		if( *fmt != '%' ) {
			*s++ = *fmt;
			continue;
		}
		++fmt;
		fl = fll = 0;
		size = sizeof(int);
	cont:
		switch (*fmt) {
		case 'l':
			fmt++;
			if (fl) {
				fll = 1;
				size = sizeof(long long);
			} else {
				fl = 1;
				size = sizeof(long);
			}
			goto cont;
		case 'x':
		case 'd':
			if (fll)
				number = va_arg(arg, long long);
			else if (fl)
				number = va_arg(arg, long);
			else
				number = va_arg(arg, int);
			if (*fmt == 'd')
				goto print_d;
			else
				goto print_x;
		print_x: {
			int i;
			int dig;
			char *rem = s;
			for (i = 0; i < size*2; i++, number >>= 4)
				*s++ = (dig=(number&0xf)) > 9?
					'a' + dig - 10 :
					'0' + dig;
			reverse(rem, s-1);
			break;
		}
		print_d: {
			char *rem = s;

			for (; number != 0; number /= 10)
				*s++ = '0' + (number % 10);
			reverse(rem, s-1);
			break;
		}
		case 'c':
			*s++ = va_arg(arg, int);
			break;
		case 's': {
			const char *str = va_arg(arg, char *);
			strcpy(s, str);
			s += strlen(str);
			break;
		}
		default:
			*s++ = *fmt;
			break;
		}
	}
	*s = '\0';
	va_end(arg);

	dputs(buffer);
	return rv;
}
HIDDEN(dprintf)
