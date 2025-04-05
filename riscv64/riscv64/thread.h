/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 *	File:	machine/thread.h
 *
 *	This file contains the structure definitions for the thread
 *	state as applied to RISC-V processors.
 */

#ifndef	_RISCV64_THREAD_H_
#define _RISCV64_THREAD_H_

#include <mach/boolean.h>
#include <mach/machine/vm_types.h>
#include <mach/machine/fp_reg.h>
#include <mach/machine/thread_status.h>

#include <kern/lock.h>


/*
 *	riscv64_saved_state:
 *
 *	This structure corresponds to the state of user registers
 *	as saved upon kernel entry.  It lives in the pcb.
 *	It is also pushed onto the stack for exceptions in the kernel.
 */

struct riscv64_saved_state {
	unsigned long	t6;
	unsigned long	t5;
	unsigned long	t4;
	unsigned long	t3;
	unsigned long	s11;
	unsigned long	s10;
	unsigned long	s9;
	unsigned long	s8;
	unsigned long	s7;
	unsigned long	s6;
	unsigned long	s5;
	unsigned long	s4;
	unsigned long	s3;
	unsigned long	s2;
	unsigned long	a7;
	unsigned long	a6;
	unsigned long	a5;
	unsigned long	a4;
	unsigned long	a3;
	unsigned long	a2;
	unsigned long	a1;
	unsigned long	a0;
	unsigned long	s1;
	unsigned long	fp;
	unsigned long	t2;
	unsigned long	t1;
	unsigned long	t0;
	unsigned long	tp;
	unsigned long	gp;
	unsigned long	sp;
	unsigned long	ra;

	unsigned long	scause;
	unsigned long	stval;
	unsigned long	sepc;
	unsigned long	sstatus;
};

/*
 *	riscv64_exception_link:
 *
 *	This structure lives at the high end of the kernel stack.
 *	It points to the current thread`s user registers.
 */
struct riscv64_exception_link {
	struct riscv64_saved_state *saved_state;
};

/*
 *	riscv64_kernel_state:
 *
 *	This structure corresponds to the state of kernel registers
 *	as saved in a context-switch.  It lives at the base of the stack.
 */

struct riscv64_kernel_state {
	unsigned long	s11;
	unsigned long	s10;
	unsigned long	s9;
	unsigned long	s8;
	unsigned long	s7;
	unsigned long	s6;
	unsigned long	s5;
	unsigned long	s4;
	unsigned long	s3;
	unsigned long	s2;
	unsigned long	s1;
	unsigned long	fp;
};

/*
 *	Save area for user floating-point state.
 *	Allocated only when necessary.
 */

struct riscv64_fpsave_state {
	boolean_t		fp_valid;

	struct {
		struct riscv64_fp_save	fp_save_state;
		struct riscv64_fp_regs	fp_regs;
	};
};

/*
 *	riscv64_interrupt_state:
 *
 *	This structure describes the set of registers that must
 *	be pushed on the current ring-0 stack by an interrupt before
 *	we can switch to the interrupt stack.
 */

struct riscv64_interrupt_state {
	/* User registers saved during interrupt/exception */
	unsigned long	s5;	/* Saved register (x21) */
	unsigned long	s4;	/* Saved register (x20) */
	unsigned long	s3;	/* Saved register (x19) */
	unsigned long	s2;	/* Saved register (x18) */
	unsigned long	a7;	/* Function argument (x17) */
	unsigned long	a6;	/* Function argument (x16) */
	unsigned long	a5;	/* Function argument (x15) */
	unsigned long	a4;	/* Function argument (x14) */
	unsigned long	a3;	/* Function argument (x13) */
	unsigned long	a2;	/* Function argument (x12) */
	unsigned long	a1;	/* Function argument / return value (x11) */
	unsigned long	a0;	/* Function argument / return value (x10) */
	unsigned long	s1;	/* Saved register (x9) */
	unsigned long	fp;	/* Saved register / frame pointer (x8) */
	unsigned long	t2;	/* Temporary register (x7) */
	unsigned long	t1;	/* Temporary register (x6) */
	unsigned long	t0;	/* Temporary register (x5) */
	unsigned long	tp;	/* Thread pointer (x4) */
	unsigned long	gp;	/* Global pointer (x3) */
	unsigned long	sp;	/* Stack pointer (x2) */
	unsigned long	ra;	/* Return address (x1) */

	/* Control/status registers */
	unsigned long	sepc;	/* Supervisor Exception Program Counter */
	unsigned long	sstatus;/* Supervisor Status Register */
	unsigned long	scause;	/* Supervisor Cause Register */
	unsigned long	stval;	/* Supervisor Trap Value */
};

/*
 *	riscv64_machine_state:
 *
 *	This structure corresponds to special machine state.
 *	It lives in the pcb.  It is not saved by default.
 */

struct riscv64_machine_state {
	struct user_ldt	*	ldt;
	struct riscv64_fpsave_state *ifps;
	struct riscv64_debug_state ids;
};

typedef struct pcb {
	/* START of the exception stack.
	 * NOTE: this area is used as exception stack when switching
	 * CPL, and it MUST be big enough to save the thread state and
	 * switch to a proper stack area, even considering recursive
	 * exceptions, otherwise it could corrupt nearby memory */
	struct riscv64_interrupt_state iis[2];	/* interrupt and NMI */
	unsigned long pad;	   /* ensure exception stack is aligned to 16 */

	struct riscv64_saved_state iss;

	/* END of exception stack*/
	struct riscv64_machine_state ims;
	decl_simple_lock_data(, lock)
	unsigned short init_control;		/* Initial FPU control to set */
#ifdef LINUX_DEV
	void *data;
#endif /* LINUX_DEV */
} *pcb_t;

/*
 *	On the kernel stack is:
 *	stack:	...
 *		struct riscv64_exception_link
 *		struct riscv64_kernel_state
 *	stack+KERNEL_STACK_SIZE
 */

#define STACK_IKS(stack)	\
	((struct riscv64_kernel_state *)((stack) + KERNEL_STACK_SIZE) - 1)
#define STACK_IEL(stack)	\
	((struct riscv64_exception_link *)STACK_IKS(stack) - 1)

#define KERNEL_STACK_ALIGN 16

#define USER_STACK_ALIGN 16

#define USER_REGS(thread)	(&(thread)->pcb->iss)


#define syscall_emulation_sync(task)	/* do nothing */


/* #include_next "thread.h" */

#endif	/* _RISCV64_THREAD_H_ */
