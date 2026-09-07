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
/*
 * Processor registers for RISC-V.
 */
#ifndef	_RISCV64_PROC_REG_H_
#define	_RISCV64_PROC_REG_H_

#define csr_read(csr)						\
({								\
	unsigned long __val;					\
	asm volatile ("csrr %0, " #csr : "=r" (__val));		\
	__val;							\
})

#define csr_write(csr, val)					\
({								\
	unsigned long __val = (unsigned long)(val);		\
	asm volatile ("csrw " #csr ", %0" :: "r" (__val));	\
})

#define SATP_MODE_BARE	0UL
#define SATP_MODE_SV39	8UL
#define SATP_MODE_SV48	9UL

#define SATP_MODE_SHIFT		60
#define SATP_ASID_SHIFT		44
#define SATP_PPN_SHIFT		0
#define SATP_PPN_MASK		0xFFFFFFFFFFFUL

#define SATP_MODE(mode)		((unsigned long)(mode) << SATP_MODE_SHIFT)

#define satp_read()		csr_read(satp)
#define satp_write(val)		csr_write(satp, val)

#define satp_sv39(pa)		(SATP_MODE(SATP_MODE_SV39) | \
				 (((unsigned long)(pa)) >> 12))

#define sfence_vma()						\
({								\
	asm volatile ("sfence.vma" ::: "memory");		\
})

#define sfence_vma_addr(va)					\
({								\
	asm volatile ("sfence.vma %0" :: "r" (va) : "memory");	\
})

#define SSTATUS_SPP	(1UL << 8)
#define SSTATUS_SPIE	(1UL << 5)
#define SSTATUS_SIE	(1UL << 1)
#define SSTATUS_SUM	(1UL << 18)

#define sstatus_read()		csr_read(sstatus)
#define sstatus_write(val)	csr_write(sstatus, val)

#define stvec_read()		csr_read(stvec)
#define stvec_write(val)	csr_write(stvec, val)

#define sscratch_read()		csr_read(sscratch)
#define sscratch_write(val)	csr_write(sscratch, val)

#define sepc_read()		csr_read(sepc)
#define sepc_write(val)		csr_write(sepc, val)

#define scause_read()		csr_read(scause)
#define stval_read()		csr_read(stval)

#endif	/* _RISCV64_PROC_REG_H_ */
