/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>



###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num)						\
	.data;	\
		.long name;	\
	.text;	\
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
#define TRAPHANDLER_NOEC(names, nums)					\
	.data;	\
		.long names;	\
	.text;	\
	.globl names;							\
	.type names, @function;						\
	.align 2;							\
	names:								\
	pushl $0;							\
	pushl $(nums);							\
	jmp _alltraps

//.text
.data
	.globl traps

traps:
/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */
TRAPHANDLER_NOEC(trap_0, T_DIVIDE)
TRAPHANDLER_NOEC(trap_1, T_DEBUG)
TRAPHANDLER_NOEC(trap_2, T_NMI)
TRAPHANDLER_NOEC(trap_3, T_BRKPT)
TRAPHANDLER_NOEC(trap_4, T_OFLOW)
TRAPHANDLER_NOEC(trap_5, T_BOUND)
TRAPHANDLER_NOEC(trap_6, T_ILLOP)
TRAPHANDLER_NOEC(trap_7, T_DEVICE)
TRAPHANDLER(trap_8, T_DBLFLT)
TRAPHANDLER(trap_9, 9)
TRAPHANDLER(trap_10, T_TSS)
TRAPHANDLER(trap_11, T_SEGNP)
TRAPHANDLER(trap_12, T_STACK)
TRAPHANDLER(trap_13, T_GPFLT)
TRAPHANDLER(trap_14, T_PGFLT)
TRAPHANDLER(trap_15, 15)
TRAPHANDLER_NOEC(trap_16, T_FPERR)
TRAPHANDLER(trap_17, T_ALIGN)
TRAPHANDLER(trap_18, T_MCHK)
TRAPHANDLER(trap_19, T_SIMDERR)
TRAPHANDLER(trap_20, 20)
TRAPHANDLER(trap_21, 21)
TRAPHANDLER(trap_22, 22)
TRAPHANDLER(trap_23, 23)
TRAPHANDLER(trap_24, 24)
TRAPHANDLER(trap_25, 25)
TRAPHANDLER(trap_26, 26)
TRAPHANDLER(trap_27, 27)
TRAPHANDLER(trap_28, 28)
TRAPHANDLER(trap_29, 29)
TRAPHANDLER(trap_30, 30)
TRAPHANDLER(trap_31, 31)
TRAPHANDLER(trap_32, 32)
TRAPHANDLER(trap_33, 33)
TRAPHANDLER(trap_34, 34)
TRAPHANDLER(trap_35, 35)
TRAPHANDLER(trap_36, 36)
TRAPHANDLER(trap_37, 37)
TRAPHANDLER(trap_38, 38)
TRAPHANDLER(trap_39, 39)
TRAPHANDLER(trap_40, 40)
TRAPHANDLER(trap_41, 41)
TRAPHANDLER(trap_42, 42)
TRAPHANDLER(trap_43, 43)
TRAPHANDLER(trap_44, 44)
TRAPHANDLER(trap_45, 45)
TRAPHANDLER(trap_46, 46)
TRAPHANDLER(trap_47, 47)
TRAPHANDLER_NOEC(trap_48, T_SYSCALL)
/*
 * Lab 3: Your code here for _alltraps
 */
_alltraps:
	# build trap frame
	pushw $0x0;
	pushw %ds;
	pushw $0x0;
	pushw %es;
	pushal;
	movw $GD_KD,%ax;
	movw %ax,%ds;
	movw %ax,%es;
	pushl %esp;
	call trap
