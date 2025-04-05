/* 
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University
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

#include <mach/machine/asm.h>

#include <riscv64/proc_reg.h>
#include <riscv64/riscv64asm.h>
#include <riscv64/cpu_number.h>

/*
 * Context switch routines for i386.
 */

ENTRY(Load_context)
	/* TODO: implement */
/*
 *	This really only has to save registers
 *	when there is no explicit continuation.
 */

ENTRY(Switch_context)
	/* TODO: implement */
ENTRY(Thread_continue)
	/* TODO: implement */
#if	NCPUS > 1
/*
 * void switch_to_shutdown_context(thread_t thread,
 *				   void (*routine)(processor_t),
 *				   processor_t processor)
 *
 * saves the kernel context of the thread,
 * switches to the interrupt stack,
 * continues the thread (with thread_continue),
 * then runs routine on the interrupt stack.
 *
 * Assumes that the thread is a kernel thread (thus
 * has no FPU state)
 */
ENTRY(switch_to_shutdown_context)
	/* TODO: implement */
#endif	/* NCPUS > 1 */
