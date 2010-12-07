/* From GLIBC source code */
/*
 * #define SETUP_PIC_REG(reg)                                        \
 * 	.ifndef __i686.get_pc_thunk.reg                                 ;\
 * 	.section .gnu.linkonce.t.__i686.get_pc_thunk.reg,"ax",@progbits ;\
 * 	.globl __i686.get_pc_thunk.reg                                  ;\
 * 	.hidden __i686.get_pc_thunk.reg                                 ;\
 * 	.type __i686.get_pc_thunk.reg,@function                         ;\
 * 	__i686.get_pc_thunk.reg:                                         \
 * 	movq (%rsp), %r##reg                                            ;\
 * 	ret                                                             ;\
 * 	.size __i686.get_pc_thunk.reg, . - __i686.get_pc_thunk.reg      ;\
 * 	.previous                                                       ;\
 * 	.endif                                                          ;\
 * 	call __i686.get_pc_thunk.reg
 * 
 * #define LOAD_PIC_REG(reg) \
 * 	SETUP_PIC_REG(reg) ;\
 * 	addq $_GLOBAL_OFFSET_TABLE_, %r##reg
 *
 */

/* USAGE */
/*
 * .global get_got
 * 	.type	get_got, @function
 * get_got:		
 * 	pushq	%rbp
 * 	movq	%rsp, %rbp
 * 	LOAD_PIC_REG(ax)
 * 	leave
 * 	ret
 */ 

/* .gnu.linkonce.t.* are special sections for linker */
.ifndef get_next_rip
	.section .gnu.linkonce.t.get_next_rip,"ax",@progbits
		.globl get_next_rip
		.hidden get_next_rip
		.type get_next_rip,@function
		get_next_rip:
			movq (%rsp), %rax
			ret
		.size get_next_rip, . - get_next_rip
	.previous
.endif

.global get_got
	.type get_got, @function
get_got:
	pushq %rbp
	movq %rsp, %rbp
	call get_next_rip
	addq $_GLOBAL_OFFSET_TABLE_, %rax
	leave
	ret
