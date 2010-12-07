#include <linux/unistd.h>

	.text
.globl crack_code
	.type	crack, @function
/*
 * STACK:
 *   [kernel pushed]
 *   original rax
 *   original rdx
 *   original rdi
 *   original length_of_string
 *
 * %rdi: pointer to string
 * %rsi: pointer to _start
 */
crack_code:
  /*
    Already original registers saved:
      pushq %rax
      pushq %rdx
      pushq %rdi

    GLIBC Use (I know):
      rax, rdx, rdi
    Can destroy registers (_start setted):
      rsi, r8, rcx
  */
  
  // DESTROY: rax, rsi, rdi, rdx
  movq  $__NR_write, %rax
  popq  %rdx       // length
  pushq %rsi       // save _start
  movq  %rdi, %rsi // pointer to string
  movq  $1,   %rdi // file discriptor
  syscall

  popq %rsi  // restore _start
  popq %rdi
  popq %rdx
  popq %rax

  jmp *%rsi

  movl $__NR_exit, %eax
  movq $0, %rdi
  syscall
