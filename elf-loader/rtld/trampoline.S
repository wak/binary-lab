/* REF: sysdeps/x86_64/dl-trampoline.S */

.text
.global runtime_resolve
	.type runtime_resolve, @function
	.hidden runtime_resolve
.align 16
//	.cfi_startproc
runtime_resolve:
	subq $56,%rsp
//	.cfi_adjust_cfa_offset 72 # Incorporate PLT
	movq %rax,(%rsp)	# Preserve registers otherwise clobbered.
	movq %rcx, 8(%rsp)
	movq %rdx, 16(%rsp)
	movq %rsi, 24(%rsp)
	movq %rdi, 32(%rsp)
	movq %r8, 40(%rsp)
	movq %r9, 48(%rsp)
	movq 64(%rsp), %rsi	# Copy args pushed by PLT in register.
	movq %rsi, %r11		# Multiply by 24
	addq %r11, %rsi
	addq %r11, %rsi
	shlq $3, %rsi
	movq 56(%rsp), %rdi	# %rdi: link_map, %rsi: reloc_offset
	call got_fixup		# Call resolver.
	movq %rax, %r11		# Save return value
	movq 48(%rsp), %r9	# Get register content back.
	movq 40(%rsp), %r8
	movq 32(%rsp), %rdi
	movq 24(%rsp), %rsi
	movq 16(%rsp), %rdx
	movq 8(%rsp), %rcx
	movq (%rsp), %rax
	addq $72, %rsp		# Adjust stack(PLT did 2 pushes)
//	.cfi_adjust_cfa_offset -72
	jmp *%r11		# Jump to function address.
//	.cfi_endproc
	.size runtime_resolve, .-runtime_resolve
