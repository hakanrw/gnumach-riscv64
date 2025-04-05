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

#include <stddef.h>
#include <string.h>

#include <mach/std_types.h>
#include <mach/kern_return.h>
#include <mach/thread_status.h>
#include <mach/exec/exec.h>
#include <mach/xen.h>

#include "vm_param.h"
#include <kern/counters.h>
#include <kern/debug.h>
#include <kern/thread.h>
#include <kern/sched_prim.h>
#include <kern/slab.h>
#include <vm/vm_kern.h>
#include <vm/pmap.h>

#include <riscv64/thread.h>
#include <riscv64/proc_reg.h>
#include <riscv64/db_interface.h>
#include "pcb.h"


#if	NCPUS > 1
#include <riscv64/mp_desc.h>
#endif

struct kmem_cache	pcb_cache;

vm_offset_t	kernel_stack[NCPUS];	/* top of active_stack */

/*
 *	stack_attach:
 *
 *	Attach a kernel stack to a thread.
 */

void stack_attach(
	thread_t 	thread,
	vm_offset_t 	stack,
	void 		(*continuation)(thread_t))
{
	counter(if (++c_stacks_current > c_stacks_max)
			c_stacks_max = c_stacks_current);

	thread->kernel_stack = stack;

	panic("TODO: not implemented");
}

/*
 *	stack_detach:
 *
 *	Detaches a kernel stack from a thread, returning the old stack.
 */

vm_offset_t stack_detach(thread_t thread)
{
	vm_offset_t	stack;

	counter(if (--c_stacks_current < c_stacks_min)
			c_stacks_min = c_stacks_current);

	stack = thread->kernel_stack;
	thread->kernel_stack = 0;

	return stack;
}

#if	NCPUS > 1
#define	curr_gdt(mycpu)		(mp_gdt[mycpu])
#define	curr_ktss(mycpu)	(mp_ktss[mycpu])
#else
#define	curr_gdt(mycpu)		((void)(mycpu), gdt)
#define	curr_ktss(mycpu)	((void)(mycpu), (struct task_tss *)&ktss)
#endif

#define	gdt_desc_p(mycpu,sel) \
	((struct real_descriptor *)&curr_gdt(mycpu)[sel_idx(sel)])

void switch_ktss(pcb_t pcb)
{
	panic("TODO: not implemented");
}

/* If NEW_IOPB is not null, the SIZE denotes the number of bytes in
   the new bitmap.  Expects iopb_lock to be held.  */
void
update_ktss_iopb (unsigned char *new_iopb, io_port_t size)
{
	panic("TODO: not implemented");
}

/*
 *	stack_handoff:
 *
 *	Move the current thread's kernel stack to the new thread.
 */

void stack_handoff(
	thread_t	old,
	thread_t	new)
{
	int		mycpu = cpu_number();
	vm_offset_t	stack;

	panic("TODO: not implemented");
}

/*
 * Switch to the first thread on a CPU.
 */
void load_context(thread_t new)
{
	panic("TODO: not implemented");
}

/*
 * Switch to a new thread.
 * Save the old thread`s kernel state or continuation,
 * and return it.
 */
thread_t switch_context(
	thread_t	old,
	continuation_t	continuation,
	thread_t	new)
{

	panic("TODO: not implemented");
}

void pcb_module_init(void)
{
	kmem_cache_init(&pcb_cache, "pcb", sizeof(struct pcb),
			KERNEL_STACK_ALIGN, NULL, 0);

	panic("TODO: not implemented");
}

void pcb_init(task_t parent_task, thread_t thread)
{
	pcb_t		pcb;

	pcb = (pcb_t) kmem_cache_alloc(&pcb_cache);
	if (pcb == 0)
		panic("pcb_init");

	counter(if (++c_threads_current > c_threads_max)
			c_threads_max = c_threads_current);

	/*
	 *	We can't let random values leak out to the user.
	 */
	memset(pcb, 0, sizeof *pcb);
	simple_lock_init(&pcb->lock);


	panic("TODO: not implemented");
}

void pcb_terminate(thread_t thread)
{
	pcb_t		pcb = thread->pcb;

	counter(if (--c_threads_current < c_threads_min)
			c_threads_min = c_threads_current);

	panic("TODO: not implemented");
}

/*
 *	pcb_collect:
 *
 *	Attempt to free excess pcb memory.
 */

void pcb_collect(__attribute__((unused)) const thread_t thread)
{
}


/*
 *	thread_setstatus:
 *
 *	Set the status of the specified thread.
 */

kern_return_t thread_setstatus(
	thread_t		thread,
	int			flavor,
	thread_state_t		tstate,
	unsigned int		count)
{
	panic("TODO: not implemented");

	return(KERN_SUCCESS);
}

/*
 *	thread_getstatus:
 *
 *	Get the status of the specified thread.
 */

kern_return_t thread_getstatus(
	thread_t		thread,
	int			flavor,
	thread_state_t		tstate,	/* pointer to OUT array */
	unsigned int		*count)		/* IN/OUT */
{
	panic("TODO: Not implemented");

	return(KERN_SUCCESS);
}

/*
 * Alter the thread`s state so that a following thread_exception_return
 * will make the thread return 'retval' from a syscall.
 */
void
thread_set_syscall_return(
	thread_t	thread,
	kern_return_t	retval)
{
	panic("TODO: Not implemented");
}

/*
 * Return preferred address of user stack.
 * Always returns low address.  If stack grows up,
 * the stack grows away from this address;
 * if stack grows down, the stack grows towards this
 * address.
 */
vm_offset_t
user_stack_low(vm_size_t stack_size)
{
	return (VM_MAX_USER_ADDRESS - stack_size);
}

/*
 * Allocate argument area and set registers for first user thread.
 */
vm_offset_t
set_user_regs(vm_offset_t stack_base, /* low address */
	      vm_offset_t stack_size,
	      const struct exec_info *exec_info,
	      vm_size_t arg_size)
{
	vm_offset_t	arg_addr;
	struct riscv64_saved_state *saved_state;

	panic("TODO: not implemented");

	return (arg_addr);
}
