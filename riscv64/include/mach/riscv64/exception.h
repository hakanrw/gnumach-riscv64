/*
 * Copyright (c) 2023-2025 Free Software Foundation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/*
 *	Codes and subcodes for RISC-V exceptions.
 */
#ifndef	_MACH_RISCV64_EXCEPTION_H_
#define _MACH_RISCV64_EXCEPTION_H_

/*
 * EXC_BAD_INSTRUCTION
 */
#define EXC_RISCV64_ILLEGAL_INSTR   2   /* Illegal instruction exception */
#define EXC_RISCV64_BUS_FAULT       24  /* Bus fault */
#define EXC_RISCV64_PARITY_FAULT    25  /* Parity/Checksum fault */

/*
 * EXC_ARITHMETIC
 */

/*
 * EXC_SOFTWARE
 */
#define EXC_RISCV64_ECALL_U         8   /* ECALL from user mode */
#define EXC_RISCV64_ECALL_M         11  /* ECALL from machine mode */

/*
 * EXC_BAD_ACCESS
 */
#define EXC_RISCV64_ACCESS_FAULT    1   /* Access fault */
#define EXC_RISCV64_LOAD_FAULT      5   /* Load access fault */

/*
 * EXC_BREAKPOINT
 */
#define EXC_RISCV64_BREAKPOINT      3   /* Breakpoint exception (ebreak) */


#endif	/* _MACH_RISCV64_EXCEPTION_H_ */
