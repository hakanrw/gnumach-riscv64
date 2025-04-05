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

#ifndef _MACH_MACH_RISCV64_TYPES_H_
#define _MACH_MACH_RISCV64_TYPES_H_

/* The currently defined number of hwcap values.
   More ones could be added in future versions.  */
#define HWCAPS_COUNT	2

#ifndef __ASSEMBLER__
#include <stdint.h>
typedef uint64_t *hwcaps_t;
#endif

/* These definitions are meant to match those in
   linux:arch/riscv/include/uapi/asm/hwcap.h and
   glibc:sysdeps/unix/sysv/linux/riscv/bits/hwcap.h,
   but this is not strictly required for anything.  */

#define COMPAT_HWCAP_ISA_I	(1 << ('I' - 'A'))
#define COMPAT_HWCAP_ISA_M	(1 << ('M' - 'A'))
#define COMPAT_HWCAP_ISA_A	(1 << ('A' - 'A'))
#define COMPAT_HWCAP_ISA_F	(1 << ('F' - 'A'))
#define COMPAT_HWCAP_ISA_D	(1 << ('D' - 'A'))
#define COMPAT_HWCAP_ISA_C	(1 << ('C' - 'A'))

#endif
