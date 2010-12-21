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

#define MARK_SIZE 80
void print_mark(const char *str)
{
	char mark[MARK_SIZE+2];
	int len, i;

	memset(mark, '-', sizeof(mark));
	len = strlen(str);
	for (i = 0; i < len; i++)
		mark[i+2] = str[i];
	mark[1] = mark[i+2] = ' ';
	mark[sizeof(mark)-2] = '\n';
	mark[sizeof(mark)-1] = '\0';
	dputs(mark);
}
void print_mark_end(void)
{
	char mark[MARK_SIZE+2];

	memset(mark, '-', sizeof(mark));
	mark[sizeof(mark)-2] = '\n';
	mark[sizeof(mark)-1] = '\0';
	dputs(mark);
}

static int dvsprintf(char *buffer, size_t buffer_size,
		     const char *format, va_list arg)
{
	const char *fmt;
	long long number;
	int fl, fll, fsharp, fzero, size, padding;
	size_t current_size = 0;
	char *current_pos = buffer;
	//char tmp[32];

#define PUT(val) {					\
		char c = (val);				\
		if (current_size + 1 >= buffer_size) {	\
			/* nothing */			\
		} else {				\
			current_pos++;			\
			buffer[current_size++] = (c);	\
		}					\
	}

	for(fmt = format; *fmt; ++fmt) {
		if( *fmt != '%' ) {
			PUT(*fmt);
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
				PUT('0');
				PUT('x');
			}
			rem = current_pos;
			i = 0;
			do {
				PUT((dig = (n & 0xf)) > 9?
				    'a' + dig - 10 :
				    '0' + dig);
				n >>= 4;
				padding--;
				i ++;
			} while (i < size*2 && n);
			while (padding-- > 0)
				PUT(fzero ? '0' : ' ');
			reverse(rem, current_pos-1);
			break;
		}
		print_d: {
			char *rem = current_pos;
			int minus = 0;
			if (number < 0) {
				minus = 1;
				number *= -1;
			}
			do {
				PUT('0' + (number % 10));
				number /= 10;
				padding--;
			} while(number != 0);
			if (minus) {
				PUT(' ');
			}
			while (padding-- > 0)
				PUT(' ');
			reverse(rem, current_pos-1);
			break;
		}
		case 'c':
			PUT(va_arg(arg, int));
			break;
		case 's': {
			const char *str = va_arg(arg, char *);
			for (; *str != '\0'; str++, padding--)
				PUT(*str);
			while (padding-- > 0)
				PUT(' ');
			break;
		}
		default:
			PUT(*fmt);
			break;
		}
	}
	*current_pos = '\0';

	return current_size;
#undef PUT
}

int dsprintf(char *buf, size_t size, const char *format, ...)
{
	va_list arg;
	int rv;

	va_start(arg, format);
	rv = dvsprintf(buf, size, format, arg);
	va_end(arg);

	return rv;
}
HIDDEN(dsprintf)

int dprintf(const char *format, ...)
{
	va_list arg;
	int rv;
	char buffer[1024];

	va_start(arg, format);
	rv = dvsprintf(buffer, sizeof(buffer), format, arg);
	va_end(arg);
	dputs(buffer);

	return rv;
}
HIDDEN(dprintf)

void dprintf_die(const char *format, ...)
{
	va_list arg;
	int rv;
	char buffer[1024];

	va_start(arg, format);
	rv = dvsprintf(buffer, sizeof(buffer), format, arg);
	va_end(arg);
	dputs(buffer);

	_exit(1);
}
HIDDEN(dprintf_die)

void *_emalloc(size_t size,
	       int line, const char *file, const char *func)
{
	void *newp;

	newp = malloc(size);
	if (newp == NULL) {
		dprintf("Malloc failed at L.%d [%s] %s (%lu byte)\n",
			line, file, func, size);
		syscall(SYS_exit, 1);
	}
	return newp;
}
HIDDEN(_emalloc)

void __attribute__ ((noreturn,noinline)) _exit(int status)
{
	while (1)
		syscall(SYS_exit, 1);
}
HIDDEN(_exit);
