/*
 * Mach Operating System
 * Copyright (c) 1991,1990,1989,1988 Carnegie Mellon University
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
 * Hardware trap/fault handler.
 */

#include <sys/types.h>
#include <string.h>

#include <riscv64/trap.h>
#include <riscv64/locore.h>
#include <riscv64/model_dep.h>
#include <machine/spl.h>	/* for spl_t */
#include <machine/db_interface.h>

#include <mach/exception.h>
#include <mach/kern_return.h>
#include "vm_param.h"
#include <mach/machine/thread_status.h>

#include <vm/vm_fault.h>
#include <vm/vm_kern.h>
#include <vm/vm_map.h>

#include <kern/ast.h>
#include <kern/debug.h>
#include <kern/printf.h>
#include <kern/thread.h>
#include <kern/task.h>
#include <kern/sched.h>
#include <kern/sched_prim.h>
#include <kern/exception.h>

#if MACH_KDB
#include <ddb/db_break.h>
#include <ddb/db_run.h>
#include <ddb/db_watch.h>
#endif

#if	MACH_KDB
boolean_t	debug_all_traps_with_kdb = FALSE;
extern struct db_watchpoint *db_watchpoint_list;
extern boolean_t db_watchpoints_inserted;

void
thread_kdb_return(void)
{
	thread_t thread = current_thread();
	struct riscv64_saved_state *regs = USER_REGS(thread);

	panic("TODO: not implemented");
}
#endif	/* MACH_KDB */

#if	MACH_TTD
extern boolean_t kttd_enabled;
boolean_t debug_all_traps_with_kttd = TRUE;
#endif	/* MACH_TTD */

static void
user_page_fault_continue(kern_return_t kr)
{
	thread_t thread = current_thread();
	struct riscv64_saved_state *regs = USER_REGS(thread);

	panic("TODO: not implemented");
}


static char *trap_type[] = {
	"Divide error",
	"Debug trap",
	"NMI",
	"Breakpoint",
	"Overflow",
	"Bounds check",
	"Invalid opcode",
	"No coprocessor",
	"Double fault",
	"Coprocessor overrun",
	"Invalid TSS",
	"Segment not present",
	"Stack bounds",
	"General protection",
	"Page fault",
	"(reserved)",
	"Coprocessor error"
};
#define TRAP_TYPES (sizeof(trap_type)/sizeof(trap_type[0]))

char *trap_name(unsigned int trapnum)
{
	return trapnum < TRAP_TYPES ? trap_type[trapnum] : "(unknown)";
}

/*
 * Trap from kernel mode.  Only page-fault errors are recoverable,
 * and then only in special circumstances.  All other errors are
 * fatal.
 */
void kernel_trap(struct riscv64_saved_state *regs)
{
	unsigned long	code;
	unsigned long	subcode;
	unsigned long	type;
	vm_map_t	map;
	kern_return_t	result;
	thread_t	thread;
	extern char _start[], etext[];


	panic("TODO: not implemented");
}


/*
 *	Trap from user mode.
 *	Return TRUE if from emulated system call.
 */
int user_trap(struct riscv64_saved_state *regs)
{
	int	exc = 0;	/* Suppress gcc warning */
	unsigned long	code;
	unsigned long	subcode;
	unsigned long	type;
	thread_t thread = current_thread();

	panic("TODO: not implemented");
}

#define	V86_IRET_PENDING 0x4000

/*
 * Handle exceptions for i386.
 *
 * If we are an AT bus machine, we must turn off the AST for a
 * delayed floating-point exception.
 *
 * If we are providing floating-point emulation, we may have
 * to retrieve the real register values from the floating point
 * emulator.
 */
void
riscv64_exception(
	int	exc,
	int	code,
	long	subcode)
{
	spl_t	s;

	/*
	 * Turn off delayed FPU error handling.
	 */
	panic("TODO: not implemented");

	exception(exc, code, subcode);
	/*NOTREACHED*/
}

#if	MACH_PCSAMPLE > 0
/*
 * return saved state for interrupted user thread
 */
unsigned
interrupted_pc(const thread_t t)
{
	panic("TODO: not implemented");
	return 0;
}
#endif	/* MACH_PCSAMPLE > 0 */

#if	MACH_KDB

void
db_debug_all_traps (boolean_t enable)
{
	debug_all_traps_with_kdb = enable;
}

#endif	/* MACH_KDB */

void handle_double_fault(struct riscv64_saved_state *regs)
{
	/* dump_ss(regs); */ /* TODO: implement */
	panic("DOUBLE FAULT! This is critical\n");
}
