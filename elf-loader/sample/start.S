
/* This is the canonical entry point, usually the first thing in the text
   segment.  The SVR4/i386 ABI (pages 3-31, 3-32) says that when the entry
   point runs, most registers' values are unspecified, except for:

   %rdx		Contains a function pointer to be registered with `atexit'.
		This is how the dynamic linker arranges to have DT_FINI
		functions called for shared libraries that have been loaded
		before this code runs.

   %rsp		The stack contains the arguments and environment:
		0(%rsp)			argc
		8(%rsp)			argv[0]
		...
		(8*argc)(%rsp)		NULL
		(8*(argc+1))(%rsp)	envp[0]
		...
					NULL
*/

#define BP_SYM(name) _BP_SYM (name)
#if __BOUNDED_POINTERS__
# define _BP_SYM(name) __BP_##name
#else
# define _BP_SYM(name) name
#endif

	.text
	.globl _start
	.type _start,@function
_start:
	/* Clear the frame pointer.  The ABI suggests this be done, to mark
	   the outermost frame obviously.  */
	xorl %ebp, %ebp

	/* Extract the arguments as encoded on the stack and set up
	   the arguments for __libc_start_main (int (*main) (int, char **, char **),
		   int argc, char *argv,
		   void (*init) (void), void (*fini) (void),
		   void (*rtld_fini) (void), void *stack_end).
	   The arguments are passed via registers and on the stack:
	main:		%rdi
	argc:		%rsi
	argv:		%rdx
	init:		%rcx
	fini:		%r8
	rtld_fini:	%r9
	stack_end:	stack.	*/

	movq %rdx, %r9		/* Address of the shared library termination
				   function.  */
	popq %rsi		/* Pop the argument count.  */
	movq %rsp, %rdx		/* argv starts just at the current stack top.  */
	/* Align the stack to a 16 byte boundary to follow the ABI.  */
	andq  $~15, %rsp

	pushq %rax		/* Push garbage because we push 8 more bytes.  */

	/* Provide the highest stack address to the user code (for stacks
	   which grow downwards).  */
	pushq %rsp

	call $BP_SYM (main)
loop:	
		jmp loop

	hlt			/* Crash if somehow `exit' does return.	 */
