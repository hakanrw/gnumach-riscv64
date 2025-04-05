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
	/*
	 * This section will be put first into .boot.  See also	x86_64/ldscript.
	 */
	.section .boot.text,"ax"
	/* We should never be entered this way.	 */
	.globl	boot_start
boot_start:

	j	boot_entry

	/* MultiBoot header - see multiboot.h.	*/
#define	MULTIBOOT_MAGIC		0x1BADB002
#define MULTIBOOT_FLAGS		0x00000003
boot_hdr:
	.long	MULTIBOOT_MAGIC
	.long	MULTIBOOT_FLAGS
	/*
	* The next item here is the checksum.
	* XX this works OK until we need at least the 30th bit.
	*/
	.long	- (MULTIBOOT_MAGIC+MULTIBOOT_FLAGS)

	.global _start
_start:
boot_entry:
	/* TODO: implement */
.kernel_map:
#if KERNEL_MAP_BASE >= (1U << 39)
	/* TODO: implement */
#else
	/* TODO: implement */
#endif

switch64:
	/* TODO: implement (is this necessary in riscv64?) */
boot_entry64:

iplt_cont:

iplt_next:
iplt_done:

	/* Jump into C code.  */
	call	EXT(c_boot_entry)

	/* not reached */

p4table:	.space 4096
p3table:	.space 4096
p2table:	.space 4096
p2table1:	.space 4096
p2table2:	.space 4096
p2table3:	.space 4096
p3ktable:	.space 4096
p2ktable1:	 .space 4096
p2ktable2:	 .space 4096
