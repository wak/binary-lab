
.text
	.align 16
.globl _start
		.hidden _start
.globl _dl_start_user
_start:
		jmp _start
	movq %rsp, %rdi
	call loader_start
	//call _dl_start
/*
_dl_start_user:
	# Save the user entry point address in %r12.
	movq %rax, %r12
	# See if we were run as a command with the executable file
	# name as an extra leading argument.
	movl _dl_skip_args(%rip), %eax
	# Pop the original argument count.
	popq %rdx
	# Adjust the stack pointer to skip _dl_skip_args words.
	leaq (%rsp,%rax,8), %rsp
	# Subtract _dl_skip_args from argc.
	subl %eax, %edx
	# Push argc back on the stack.
	pushq %rdx
	# Call _dl_init (struct link_map *main_map, int argc, char **argv, char **env)
	# argc -> rsi
	movq %rdx, %rsi
	# Save %rsp value in %r13.
	movq %rsp, %r13
	# And align stack for the _dl_init_internal call. 
	andq $-16, %rsp
	# _dl_loaded -> rdi
	movq _rtld_local(%rip), %rdi
	# env -> rcx
	leaq 16(%r13,%rdx,8), %rcx
	# argv -> rdx
	leaq 8(%r13), %rdx
	# Clear %rbp to mark outermost frame obviously even for constructors.
	xorl %ebp, %ebp
	# Call the function to run the initializers.
	call _dl_init_internal@PLT
	# Pass our finalizer function to the user in %rdx, as per ELF ABI.
	leaq _dl_fini(%rip), %rdx
	# And make sure %rsp points to argc stored on the stack.
	movq %r13, %rsp
	# Jump to the user's entry point.
	jmp *%r12
.previous
*/


.global syscall
syscall:
	movq %rdi, %rax		/* Syscall number -> rax.  */
	movq %rsi, %rdi		/* shift arg1 - arg5.  */
	movq %rdx, %rsi
	movq %rcx, %rdx
	movq %r8, %r10
	movq %r9, %r8
	movq 8(%rsp),%r9	/* arg6 is on the stack.  */
	syscall			/* Do the system call.  */
	cmpq $-4095, %rax	/* Check %rax for error.  */
	ret			/* Return to caller.  */
