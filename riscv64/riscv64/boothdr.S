/*
 *  Copyright (C) 2022 Free Software Foundation
 *
 * This program is free software ; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation ; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY ; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the program ; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <mach/machine/asm.h>

#include <riscv64/riscv64asm.h>
#include <riscv64/riscv64/proc_reg.h>

	/* This section is placed first in the kernel image.  */
	.section .boot.text,"ax"
	.globl	_start
	.type	_start, @function
	.p2align 2
_start:
	/* Enter C with paging and supervisor interrupts disabled.  */
	csrw	satp, zero
	sfence.vma
	csrci	sstatus, SSTATUS_SIE
	csrw	sie, zero

	/* Initialize the global pointer without linker relaxation.  */
	.option push
	.option norelax
1:	auipc	gp, %pcrel_hi(_edata)
	addi	gp, gp, %pcrel_lo(1b)
	.option pop

	/* Clear uninitialized data before using the bootstrap stack.  */
	la	t0, _edata
	la	t1, _end
2:	bgeu	t0, t1, 3f
	sd	zero, 0(t0)
	addi	t0, t0, 8
	j	2b

	/* Establish an ABI-aligned stack and a clean frame chain.  */
3:	la	sp, boot_stack_top
	andi	sp, sp, -16
	mv	fp, zero

	/* OpenSBI leaves the hart ID in a0 and the DTB address in a1.  */
	call	EXT(c_boot_entry)

	/* c_boot_entry is not expected to return once bootstrap advances.  */
4:	wfi
	j	4b
	.size	_start, . - _start

	/* Temporary stack used until the VM system allocates kernel stacks.  */
	.section .bss.boot,"aw",@nobits
	.p2align 4
boot_stack:
	.skip	0x4000
boot_stack_top:
