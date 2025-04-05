/*
 * Mach Operating System
 * Copyright (c) 1993,1992,1991,1990 Carnegie Mellon University
 * Copyright (c) 1991 IBM Corporation
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation,
 * and that the nema IBM not be used in advertising or publicity
 * pertaining to distribution of the software without specific, written
 * prior permission.
 *
 * CARNEGIE MELLON AND IBM ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON AND IBM DISCLAIM ANY LIABILITY OF ANY KIND FOR
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

/* TODO: implement */
/*
 * Allocate recovery and retry tables.
 */
	/* TODO: implement */
/*
 * Timing routines.
 */
#if	STAT_TIME

#define	TIME_TRAP_UENTRY
#define	TIME_TRAP_SENTRY
#define	TIME_TRAP_UEXIT
#define	TIME_INT_ENTRY
#define	TIME_INT_EXIT

#else	/* microsecond timing */

/*
 * Microsecond timing.
 * Assumes a free-running microsecond counter.
 * no TIMER_MAX check needed.
 */

timer_normalize:

/*
 * Switch to a new timer.
 */
ENTRY(timer_switch)

/*
 * Initialize the first timer for a CPU.
 */
ENTRY(start_timer)

#endif	/* accurate timing */

/**/

/*
 * Trap/interrupt entry points.
 *
 * All traps must create the following save area on the kernel stack:
 *
 *	gs
 *	fs
 *	es
 *	ds
 *	edi
 *	esi
 *	ebp
 *	cr2 if page fault - otherwise unused
 *	ebx
 *	edx
 *	ecx
 *	eax
 *	trap number
 *	error code
 *	eip
 *	cs
 *	eflags
 *	user esp - if from user
 *	user ss  - if from user
 *	es       - if from V86 thread
 *	ds       - if from V86 thread
 *	fs       - if from V86 thread
 *	gs       - if from V86 thread
 *
 */

/*
 * General protection or segment-not-present fault.
 * Check for a GP/NP fault in the kernel_return
 * sequence; if there, report it as a GP/NP fault on the user's instruction.
 *
 * esp->     0:	trap code (NP or GP)
 *	     4:	segment number in error
 *	     8	eip
 *	    12	cs
 *	    16	eflags
 *	    20	old registers (trap is from kernel)
 */
ENTRY(t_gen_prot)

ENTRY(t_segnp)
					/* indicate fault type */

trap_check_kernel_exit:
take_fault:				/* if none of the above: */

/*
 * GP/NP fault on IRET: CS or SS is in error.
 * All registers contain the user's values.
 *
 * on SP is
 *  0	trap number
 *  4	errcode
 *  8	eip
 * 12	cs		--> trapno
 * 16	efl		--> errcode
 * 20	user eip
 * 24	user cs
 * 28	user eflags
 * 32	user esp
 * 36	user ss
 */
fault_iret:

/*
 * Fault restoring a segment register.  The user's registers are still
 * saved on the stack.  The offending segment register has not been
 * popped.
 */
fault_popl_ds:
fault_popl_es:
fault_popl_fs:
fault_popl_gs:

push_es:
push_fs:
push_gs:
push_segregs:

/*
 * Debug trap.  Check for single-stepping across system call into
 * kernel.  If this is the case, taking the debug trap has turned
 * off single-stepping - save the flags register with the trace
 * bit set.
 */
ENTRY(t_debug)

/*
 * Page fault traps save cr2.
 */
ENTRY(t_page_fault)

/*
 * All 'exceptions' enter here with:
 *	esp->   trap number
 *		error code
 *		old eip
 *		old cs
 *		old eflags
 *		old esp		if trapped from user
 *		old ss		if trapped from user
 */
ENTRY(alltraps)
trap_push_segs:
trap_set_segs:
trap_from_user:
_take_trap:

/*
 * Return from trap or system call, checking for ASTs.
 * On PCB stack.
 */

_return_from_trap:

_return_to_user:

/*
 * Return from kernel mode to interrupted thread.
 */

_return_from_kernel:
_kret_popl_gs:
_kret_popl_fs:
_kret_popl_es:
_kret_popl_ds:
_kret_iret:


/*
 * Trap from kernel mode.  No need to switch stacks.
 */
trap_from_kernel:
#if	MACH_KDB || MACH_TTD
#else	/* MACH_KDB || MACH_TTD */
#endif	/* MACH_KDB || MACH_TTD */


/*
 *	Called as a function, makes the current thread
 *	return from the kernel as if from an exception.
 */

ENTRY(thread_exception_return)
ENTRY(thread_bootstrap_return)

/*
 *	Called as a function, makes the current thread
 *	return from the kernel as if from a syscall.
 *	Takes the syscall's return code as an argument.
 */

ENTRY(thread_syscall_return)

ENTRY(call_continuation)


/* XXX handle NMI - at least print a warning like Linux does.  */

/*
 * All interrupts enter here.
 * old %eax on stack; interrupt number in %eax.
 */
ENTRY(all_intrs)

int_from_intstack:

_return_to_iret_i:			/* ( label for kdb_kintr) */
					/* must have been on kernel segs */

stack_overflowed:

/*
 *	Take an AST from an interrupt.
 *	On PCB stack.
 * sp->	gs	-> edx
 *	fs	-> ecx
 *	es	-> eax
 *	ds	-> trapno
 *	edx	-> code
 *	ecx
 *	eax
 *	eip
 *	cs
 *	efl
 *	esp
 *	ss
 */
ast_from_interrupt:

#if	MACH_KDB
/*
 * kdb_kintr:	enter kdb from keyboard interrupt.
 * Chase down the stack frames until we find one whose return
 * address is the interrupt handler.   At that point, we have:
 *
 * frame->	saved %ebp
 *		return address in interrupt handler
 * #ifndef MACH_XEN
 *		1st parameter iunit
 *		2nd parameter saved SPL
 *		3th parameter return address
 *		4th parameter registers
 *		saved SPL
 *		saved IRQ
 * #endif
 *		return address == return_to_iret_i
 *		saved %edx
 *		saved %ecx
 *		saved %eax
 *		saved %eip
 *		saved %cs
 *		saved %efl
 *
 * OR:
 * frame->	saved %ebp
 *		return address in interrupt handler
 * #ifndef MACH_XEN
 *		iunit
 *		saved SPL
 *		irq
 * #endif
 *		return address == return_to_iret
 *		pointer to save area on old stack
 *	      [ saved %ebx, if accurate timing ]
 *
 * old stack:	saved %gs
 *		saved %fs
 *		saved %es
 *		saved %ds
 *		saved %edx
 *		saved %ecx
 *		saved %eax
 *		saved %eip
 *		saved %cs
 *		saved %efl
 *
 * Call kdb, passing it that register save area.
 */

#ifdef	MACH_XEN
#define	RET_OFFSET	8
#else	/* MACH_XEN */
#define	RET_OFFSET	32
#endif	/* MACH_XEN */

ENTRY(kdb_kintr)

/*
 * On return from keyboard interrupt, we will execute
 * kdb_from_iret_i
 *	if returning to an interrupt on the interrupt stack
 * kdb_from_iret
 *	if returning to an interrupt on the user or kernel stack
 */
kdb_from_iret:
kdb_from_iret_i:			/* on interrupt stack */

#endif	/* MACH_KDB */

#if	MACH_TTD
/*
 * Same code as that above for the keyboard entry into kdb.
 */
ENTRY(kttd_intr)

/*
 * On return from keyboard interrupt, we will execute
 * ttd_from_iret_i
 *	if returning to an interrupt on the interrupt stack
 * ttd_from_iret
 *	if returning to an interrupt on the user or kernel stack
 */
ttd_from_iret:					/* save regs in known locations */

ttd_from_iret_i:			/* on interrupt stack */

#endif	/* MACH_TTD */

/*
 * System call enters through a call gate.  Flags are not saved -
 * we must shuffle stack to look like trap save area.
 *
 * esp->	old eip
 *		old cs
 *		old esp
 *		old ss
 *
 * eax contains system call number.
 */
ENTRY(syscall)
syscall_entry:

syscall_entry_2:

/*
 * Check for MACH or emulated system call
 */
syscall_entry_3:

/*
 * Native system call.
 */
syscall_native:

mach_call_call:

/*
 * Address out of range.  Change to page fault.
 * %esi holds failing address.
 */
mach_call_addr_push:
mach_call_addr:

/*
 * System call out of range.  Treat as invalid-instruction trap.
 * (? general protection?)
 */
mach_call_range:
/*
 * User space emulation of system calls.
 * edx - user address to handle syscall
 *
 * User stack will become:
 * uesp->	eflags
 *		eip
 * eax still contains syscall number.
 */
syscall_emul:
/*
 * Address error - address is in %edi.
 */
syscall_addr:
	/* TODO: implement */
END(syscall)


/**/
/*
 * Utility routines.
 */

/*
 * Copy from user address space - generic version.
 * arg0:	user address
 * arg1:	kernel address
 * arg2:	byte count
 */
ENTRY(copyin)

copyin_ret:

copyin_fail:

/*
 * Copy from user address space - version for copying messages.
 * arg0:	user address
 * arg1:	kernel address
 * arg2:	byte count - must be a multiple of four
 * arg3:	kernel byte count
 */
ENTRY(copyinmsg)

copyinmsg_ret:

copyinmsg_fail:

/*
 * Copy to user address space - generic version.
 * arg0:	kernel address
 * arg1:	user address
 * arg2:	byte count
 */
ENTRY(copyout)

copyout_ret:

copyout_fail:

/*
 * Copy to user address space - version for copying messages.
 * arg0:	kernel address
 * arg1:	user address
 * arg2:	byte count - must be a multiple of four
 */
ENTRY(copyoutmsg)

copyoutmsg_ret:

copyoutmsg_fail:

/*
 * int inst_fetch(int eip, int cs);
 *
 * Fetch instruction byte.  Return -1 if invalid address.
 */
ENTRY(inst_fetch)

_inst_fetch_fault:



/*
 * cpu_shutdown()
 * Force reboot
 */
null_idt:
	.space	8 * 32

null_idtr:
        .word   8 * 32 - 1

/* TODO: implement */