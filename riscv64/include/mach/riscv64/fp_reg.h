/* 
 * Mach Operating System
 * Copyright (c) 1992-1989 Carnegie Mellon University
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

#ifndef	_MACH_RISCV64_FP_REG_H_
#define	_MACH_RISCV64_FP_REG_H_

/*
 *	Floating point registers and status, as saved
 *	and restored by FP save/restore instructions.
 */
struct riscv64_fp_save	{
	unsigned short	fp_fcsr;	/* control and status register */
};

struct riscv64_fp_regs {
	unsigned long	fp_reg_word[32];
					/* space for 32 64-bit FP registers */
};

/*
 * Control and status register
 */

/* Exception Flags (fflags) - bits 4:0 */
#define RISCV_FFLAGS_NX   0x01        /* Inexact */
#define RISCV_FFLAGS_UF   0x02        /* Underflow */
#define RISCV_FFLAGS_OF   0x04        /* Overflow */
#define RISCV_FFLAGS_DZ   0x08        /* Divide by Zero */
#define RISCV_FFLAGS_NV   0x10        /* Invalid Operation */
#define RISCV_FFLAGS_MASK 0x1F        /* All flags mask */

/* Rounding Mode (frm) - bits 7:5 */
#define RISCV_FRM_RNE     0x00        /* Round to Nearest, ties to Even */
#define RISCV_FRM_RTZ     0x01        /* Round towards Zero */
#define RISCV_FRM_RDN     0x02        /* Round Down (towards negative infinity) */
#define RISCV_FRM_RUP     0x03        /* Round Up (towards positive infinity) */
#define RISCV_FRM_RMM     0x04        /* Round to Nearest, ties to Max Magnitude */
#define RISCV_FRM_DYN     0x07        /* Dynamic rounding mode */
#define RISCV_FRM_MASK    0x07        /* Rounding mode mask */

/* Combined masks for different register parts */
#define RISCV_FFLAGS_SHIFT 0
#define RISCV_FRM_SHIFT    5
#define RISCV_FRM_MASK_SHIFTED (RISCV_FRM_MASK << RISCV_FRM_SHIFT)

/* Full fcsr masks */
#define RISCV_FCSR_FFLAGS_MASK  0x1F       /* Exception flags in fcsr */
#define RISCV_FCSR_FRM_MASK     (0x7 << 5) /* Rounding mode in fcsr */
#define RISCV_FCSR_MASK         0xFF       /* All used bits in fcsr */

/*
 * Kind of floating-point support provided by kernel.
 */
#define	FP_NO		0		/* no floating point */
#define	FP_SOFT		1		/* software FP emulator */
#define FP_HARD         2		/* RISC-V double-precision floating extension */

#endif	/* _MACH_RISCV64_FP_REG_H_ */
