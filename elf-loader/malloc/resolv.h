#if defined __GNUC__ && defined __GNUC_MINOR__
# define __GNUC_PREREQ(maj, min) \
	((__GNUC__ << 16) + __GNUC_MINOR__ >= ((maj) << 16) + (min))
#else
# define __GNUC_PREREQ(maj, min) 0
#endif

#if __GNUC_PREREQ (2,8)
# define __attribute_aligned__(size) __attribute__ ((__aligned__ (size)))
#else
# define __attribute_aligned__(size) /* Ignore */
#endif

# define MAP_UNINITIALIZE 0x4000000     /* For anonymous mmap, memory could
					   be uninitialized. */

#define __set_errno(err) (errno = err)
