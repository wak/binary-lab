#include <stdarg.h>
#include <lib.h>

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
	int fl, fll, fsharp, fzero, size, padding;

	va_start(arg, format);
	
	s = buffer;
	for(fmt = format; *fmt; ++fmt) {
		if( *fmt != '%' ) {
			*s++ = *fmt;
			continue;
		}
		++fmt;
		fl = fll = fsharp = fzero = 0;
		size = sizeof(int);
		padding = 0;
	cont:
		if (*fmt != '0' && isdigit(*fmt))
			for (; isdigit(*fmt); fmt++)
				padding = padding * 10 + *fmt - '0';
		switch (*fmt) {
		case '*':
			fmt++;
			padding = va_arg(arg, int);
			goto cont;
		case '#':
			fmt++;
			fsharp = 1;
			goto cont;
		case '0':
			fmt++;
			fzero = 1;
			goto cont;
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
		case 'p':
			number = va_arg(arg, long);
			size = sizeof(unsigned long);
			goto print_x;
		case 'P':
			number = va_arg(arg, long);
			size = sizeof(unsigned long);
			padding = sizeof(unsigned long) * 2;
			fzero = 1;
			goto print_x;
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
			int i, dig;
			unsigned long long n = number;
			char *rem;
			if (fsharp) {
				*s++ = '0';
				*s++ = 'x';
			}
			rem = s;
			i = 0;
			do {
				*s++ = (dig = (n & 0xf)) > 9?
					'a' + dig - 10 :
					'0' + dig;
				n >>= 4;
				padding--;
				i ++;
			} while (i < size*2 && n);
			while (padding-- > 0)
				*s++ = (fzero ? '0' : ' ');
			reverse(rem, s-1);
			break;
		}
		print_d: {
			char *rem = s;
			int minus = 0;
			if (number < 0) {
				minus = 1;
				number *= -1;
			}
			do {
				*s++ = '0' + (number % 10);
				number /= 10;
				padding--;
			} while(number != 0);
			if (minus)
				*s++ = '-';
			while (padding-- > 0)
				*s++ = ' ';
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
