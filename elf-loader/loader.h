#ifndef LOADER_H
#define LOADER_H

#define RTLD_START asm ("\n\
.text\n\
	.align 16\n\
.globl _start\n\
_start:\n\
	movq %rsp, %rdi\n\
	call loader_start\n\
");

#endif
