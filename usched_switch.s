// usched_switch.s

	.code32

	.section	.text

	.globl		usched_switch
	.type		usched_switch,	@function
usched_switch:
	// Push the current frame pointer onto the stack, we don't really need this
	// because pusha saves these registers for us, but it's convinient for
	// debugging as it's required for proper tack unwinding.
	push		%ebp
	mov		%esp,		%ebp

	// Push all the current registers onto the stack.
	pusha
	pushf

	// The first argument: void **cur_stack
	mov		0x08(%ebp),	%eax
	// The second argument: void *next_stack
	mov		0x0c(%ebp),	%ebx

	// Save the current stack pointer to cur_stack.
	movl		%esp,		0x00(%eax)

	// Restore the next stack pointer from next_stack.  The base pointer will be
	// incorrect until the popa instruction, but we aren't going to use it until
	// after it's restored.
	movl		%ebx,		%esp

	// Pop all the next registers from the stack.
	popf
	popa

	// Pop the next frame pointer from the stack, remember %ebp will be restored
	// from the popa instruction, so the stack pointer will still be correct.
	mov		%ebp,		%esp
	pop		%ebp

	// Here we return back to the next task, where this function was called from.
	ret
	.size		usched_switch,	.-usched_switch

	.globl		usched_switch_stack_init
	.type		usched_switch_stack_init, @function
usched_switch_stack_init:
	push		%ebp
	mov		%esp,		%ebp

	// The first argument: void *stack
	mov		0x08(%ebp),	%eax

	// Emulate the actions of saving the stack during a task switch.

	//push		%ebp
	// Our base pointer should start out zero, because thre is no caller for this
	// stack we are constructing, and doing so will result in a zero base pointer
	// being crated by the bootstrap function signifying the end of the stack.
	subl		$0x04,		%eax
	movl		$0x00,		0x00(%eax)

	//pusha
	// The pusha instruction pushes the %eax, %ebx, %ecx, %edx, %esp, %ebp, %esi,
	// and %edi registers onto the stack, in that order.
	// We don't really care what the contents of any of them except %esp and %ebp.
	// The value stored for %esp is the contents before pushing any registers, so
	// we must save taht now.
	mov		%eax,		%ecx
	// Here are the %eax, %ebx, %ecx, and %edx registers, we'll let them take on
	// whatever so happens to be in that memory region.
	subl		$0x10,		%eax
	// Here is the %esp register, which reflects the contents that %esp would have
	// before pushing %eax through %edx onto the stack.
	subl		$0x04,		%eax
	movl		%ecx,		0x00(%eax)
	// Here is the %ebp register, which should match the stack pointer, as it would
	// have been moved to the base pointer immediately before using pusha.
	subl		$0x04,		%eax
	movl		%ecx,		0x00(%eax)
	// Here are the %esi and %edi registers, we'll also let them take on whatever
	// so happens to be in that memory region.
	subl		$0x08,		%eax

	//pushf
	// We'll clear out our un-priviledged flags bits.
	subl		$0x04,		%eax
	movl		$0x00,		0x00(%eax)

	// Now our stack is ready to be passed to usched_switch as next_stack.

	mov		%ebp,		%esp
	pop		%ebp
	ret
	.size		usched_switch_stack_init, .-usched_switch_stack_init

// vim: set ts=8 sw=8 noet syn=asm:
