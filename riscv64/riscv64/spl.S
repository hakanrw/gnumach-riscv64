/* 
 * Copyright (c) 1995 Shantanu Goel
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * THE AUTHOR ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  THE AUTHOR DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 */

/*
 * spl routines for the riscv64.
 */

#include <mach/machine/asm.h>
#include <riscv64/ipl.h>
#include <riscv64/riscv64asm.h>
#include <riscv64/cpu_number.h>

#if NCPUS > 1
#define mb lock; addl $0,(%esp)
#else
#define mb
#endif

/*
 * Program XEN evt masks from %eax.
 */
#define XEN_SETMASK()				\
	/* TODO: IMPLEMENT */
ENTRY(spl0)
	/* TODO: implement */

/*
 * Historically, SETIPL(level) was called
 * for spl levels 1-6, now we have combined
 * all the intermediate levels into the highest level
 * such that interrupts are either on or off,
 * since modern hardware can handle it.
 * This simplifies the interrupt handling
 * especially for the linux drivers.
 */
Entry(splsoftclock)
ENTRY(spl1)
ENTRY(spl2)
ENTRY(spl3)
Entry(splnet)
Entry(splhdw)
ENTRY(spl4)
Entry(splbio)
Entry(spldcm)
ENTRY(spl5)
Entry(spltty)
Entry(splimp)
Entry(splvm)
ENTRY(spl6)
Entry(splclock)
Entry(splsched)
Entry(splhigh)
Entry(splhi)
ENTRY(spl7)
	/* TODO: implement */
ENTRY(splx)
	/* TODO: implement */
/*
 * Like splx() but returns with interrupts disabled and does
 * not return the previous ipl.  This should only be called
 * when returning from an interrupt.
 */
	.align	TEXT_ALIGN
	.globl	splx_cli
splx_cli:
	/* TODO: implement */
/*
 * NOTE: This routine must *not* use %ecx, otherwise
 * the interrupt code will break.
 */
	.align	TEXT_ALIGN
	.globl	spl
spl:
	/* TODO: implement */
ENTRY(sploff)
	/* TODO: implement */
ENTRY(splon)
	/* TODO: implement */
	.data
	.align	DATA_ALIGN
softclkpending:
	.long	0
	.text

ENTRY(setsoftclock)
	/* TODO: implement */