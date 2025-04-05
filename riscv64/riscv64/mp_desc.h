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

#ifndef	_RISCV64_MP_DESC_H_
#define	_RISCV64_MP_DESC_H_

#include <mach/kern_return.h>

#if MULTIPROCESSOR

/*
 * The descriptor tables are together in a structure
 * allocated one per processor (except for the boot processor).
 */
struct mp_desc_table {
	/* TODO: implement */
};

/*
 * They are pointed to by a per-processor array.
 */
extern struct mp_desc_table	*mp_desc_table[NCPUS];

extern uint8_t solid_intstack[];

/*
 * Each CPU calls this routine to set up its descriptor tables.
 */
extern int mp_desc_init(int);


extern void interrupt_processor(int cpu);


#endif /* MULTIPROCESSOR */

extern void start_other_cpus(void);

extern kern_return_t cpu_control(int cpu, const int *info, unsigned int count);

extern void interrupt_stack_alloc(void);

#endif	/* _RISCV64_MP_DESC_H_ */
