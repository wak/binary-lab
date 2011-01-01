#include <stdarg.h>
#include <lib.h>


//int errno = 0;

static int debug_indent = 0;

size_t __strlen(const char *s)
{
	register const char *p;

	for (p=s ; *p ; p++);
	return p - s;
}
HIDDEN(__strlen);

/* Copy SRC to DEST.  */
char *__strcpy(char *dest, const char *src)
{
	char *dst = dest;

	while ((*dst = *src) != '\0') {
		src++;
		dst++;
	}

	return dest;
}
HIDDEN(__strcpy);

int __strcmp(register const char *s1, register const char *s2)
{
	int r;

	while (((r = ((int)(*((unsigned char *)s1))) - *((unsigned char *)s2++))
		== 0) && *s1++);

	return r;
}
HIDDEN(__strcmp);

char *__strdup(const char *s)
{
	char *newp = emalloc(__strlen(s));
	__strcpy(newp, s);
	return newp;
}
HIDDEN(__strdup);

// not print newline
int dputs(const char *s)
{
	if (s == NULL)
		s = "(NULL)\n";
	return _syscall(SYS_write, 2, s, __strlen(s));
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
HIDDEN(memset);

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
HIDDEN(memcpy);

void *mmap(void *start, size_t length, int prot, int flags,
	   int fd, off_t offset)
{
	return (void *)
		_syscall(SYS_mmap,
			 start, length, prot, flags, fd, offset);
}
HIDDEN(mmap);

int munmap(void *start, size_t length)
{
	return _syscall(SYS_munmap, start, length);
}
HIDDEN(munmap);

int __mprotect(const void *addr, size_t len, int prot)
{
	return _syscall(SYS_mprotect, addr, len, prot);
}
HIDDEN(__mprotect);

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
static void _print_mark(const char *str)
{
	char mark[MARK_SIZE+2];
	int len, i, indent;

	indent = debug_indent * DEBUG_INDENT;
	assert(indent < sizeof(mark)-3);
	memset(mark, '=', sizeof(mark));
	memset(mark, ' ', indent);
	len = __strlen(str);
	for (i = 0; i < len; i++)
		mark[indent+i+2] = str[i];
	mark[indent + 1] = mark[indent+i+2] = ' ';
	mark[sizeof(mark)-2] = '\n';
	mark[sizeof(mark)-1] = '\0';
	dputs(mark);
	debug_indent++;
}

void mprint_start_fmt(const char *format, ...)
{
	va_list arg;
	int rv;
	char buffer[MARK_SIZE];

	va_start(arg, format);
	rv = dvsprintf(buffer, sizeof(buffer), format, arg);
	va_end(arg);
	_print_mark(buffer);
}
HIDDEN(mprint_start_fmt);

void mprintf(const char *format, ...)
{
	va_list arg;
	int rv, indent;
	char buffer[1024];

	indent = debug_indent * DEBUG_INDENT;
	memset(buffer, ' ', indent);
	va_start(arg, format);
	rv = dvsprintf(buffer+indent, sizeof(buffer)-indent, format, arg);
	va_end(arg);
	dputs(buffer);
}
HIDDEN(mprintf);

void mprint_end(void)
{
	int indent;
	char mark[MARK_SIZE+2];
	const char *arrow = "--\n";

	debug_indent--;
	if (debug_indent < 0)
		debug_indent = 0;
//	dputs("\n");
//	return;
	indent = debug_indent * DEBUG_INDENT;
	memset(mark, ' ', sizeof(mark));
	__strcpy(mark+indent, arrow);
//	memset(mark+indent, '-', sizeof(mark)-indent);
//	mark[sizeof(mark)-3] = '\n';
	mark[sizeof(mark)-2] = '\n';
	mark[sizeof(mark)-1] = '\0';
	dputs(mark);
}
HIDDEN(mprint_end);

int dvsprintf(char *buffer, size_t buffer_size,
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
		if (*fmt != '%') {
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
			fsharp = 1;
			goto print_x;
		case 'P':
			number = va_arg(arg, long);
			size = sizeof(unsigned long);
			padding = sizeof(unsigned long) * 2;
			fsharp = 1;
			fzero = 1;
			goto print_x;
		case 'x':
		case 'd':
		case 'u':
			if (fll)
				number = va_arg(arg, long long);
			else if (fl)
				number = va_arg(arg, long);
			else
				number = va_arg(arg, int);
			if (*fmt == 'd')
				goto print_d;
			else if (*fmt == 'x')
				goto print_x;
			else
				goto print_u;
		print_u: {
			char *rem = current_pos;
			do {
				PUT('0' + (number % 10));
				number /= 10;
				padding--;
			} while(number != 0);
			while (padding-- > 0)
				PUT(' ');
			reverse(rem, current_pos-1);
			break;
		}
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
				PUT('-');
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
			if (str == NULL)
				str = "(NULL)";
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
HIDDEN(dvsprintf);

int dsnprintf(char *buf, size_t size, const char *format, ...)
{
	va_list arg;
	int rv;

	va_start(arg, format);
	rv = dvsprintf(buf, size, format, arg);
	va_end(arg);

	return rv;
}
HIDDEN(dsnprintf);

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
HIDDEN(dprintf);

void _dprintf_die(unsigned int line, const char *file,
		  const char *func, const char *format, ...)
{
	va_list arg;
	int rv;
	char buffer[1024];

	va_start(arg, format);
	rv = dvsprintf(buffer, sizeof(buffer), format, arg);
	va_end(arg);
	dputs(buffer);

	dprintf(" in %s [%s:%d]\n", func, file, line);

	_exit(1);
}
HIDDEN(_dprintf_die);

void *_emalloc(size_t size,
	       int line, const char *file, const char *func)
{
	void *newp;

	newp = malloc(size);
	if (newp == NULL) {
		dprintf("Malloc failed in %s [%s:%d] (0x%lx byte)\n",
			func, file, line, size);
		_syscall(SYS_exit, 1);
	}
	return newp;
}
HIDDEN(_emalloc);

void __attribute__ ((noreturn,noinline)) _exit(int status)
{
	while (1)
		_syscall(SYS_exit, 1);
}
HIDDEN(_exit);

int __open(const char *file, int oflag, ...)
{
	mode_t mode = 0;

	if (oflag & O_CREAT) {
		va_list arg;
		va_start(arg, oflag);
		mode = va_arg(arg, mode_t);
		va_end(arg);
	}

	return _syscall(SYS_open, file, oflag, mode);
}
HIDDEN(__open);

//#include <syscalls-common.h>
//_syscall1(int, close, int, fd)
int __close(int fd)
{
	return _syscall(SYS_close, fd);
}
HIDDEN(__close);

int __fstat(int fd, struct stat *buf)
{
	return _syscall(SYS_fstat, fd, buf);
}
HIDDEN(__fstat);

int __read(int fd, char *buf, size_t count)
{
	return _syscall(SYS_read, fd, buf, count);
}
HIDDEN(__read);

off_t __lseek(int fd, off_t offset, int whence)
{
	return _syscall(SYS_lseek, fd, offset, whence);
}
HIDDEN(__lseek);
