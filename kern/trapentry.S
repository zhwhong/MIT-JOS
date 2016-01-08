/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>

#include <kern/picirq.h>


###################################################################
# exceptions/interrupts
###################################################################

/* The TRAPHANDLER macro defines a globally-visible function for handling
 * a trap.  It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 */ 
#define TRAPHANDLER(name, num)						\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps

.text

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */

 	TRAPHANDLER(double_fault, T_DBLFLT)
 	TRAPHANDLER(invalid_TSS, T_TSS)
 	TRAPHANDLER(segment_not_present, T_SEGNP)
 	TRAPHANDLER(stack_segment, T_STACK)
 	TRAPHANDLER(general_protection, T_GPFLT)
 	TRAPHANDLER(page_fault, T_PGFLT)
 	TRAPHANDLER(alignment_check, T_ALIGN)

 	TRAPHANDLER_NOEC(divide_error, T_DIVIDE)
 	TRAPHANDLER_NOEC(debug, T_DEBUG)
 	TRAPHANDLER_NOEC(nmi, T_NMI)
 	TRAPHANDLER_NOEC(break_point, T_BRKPT)
 	TRAPHANDLER_NOEC(overflow, T_OFLOW)
 	TRAPHANDLER_NOEC(bounds, T_BOUND)
 	TRAPHANDLER_NOEC(invalid_op, T_ILLOP)
 	TRAPHANDLER_NOEC(device_not_available, T_DEVICE)
 	TRAPHANDLER_NOEC(float_point_error, T_FPERR)
 	TRAPHANDLER_NOEC(system_call, T_SYSCALL)
 	TRAPHANDLER_NOEC(machine_check, T_MCHK)
 	TRAPHANDLER_NOEC(SIMD_float_point_error, T_SIMDERR)
 	TRAPHANDLER_NOEC(timer, IRQ_OFFSET + IRQ_TIMER)

/*
 * Lab 3: Your code here for _alltraps
 */
_alltraps:
	pushl %ds
	pushl %es
	pushal
	movl $GD_KD, %eax
	movw %ax, %ds
	movw %ax, %es
	pushl %esp
	call trap
	popl %esp
	popal
	popl %es
	popl %ds
	iret
