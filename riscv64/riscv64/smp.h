/* smp.h - RISC-V SMP controller for Mach. Header file
   Copyright (C) 2020 Free Software Foundation, Inc.
   Written by Almudena Garcia Jurado-Centurion

   This file is part of GNU Mach.

   GNU Mach is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mach is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA. */

#ifndef _SMP_H_
#define _SMP_H_

#include <mach/machine/vm_types.h>

int smp_init(void);
void smp_remote_ast(unsigned logical_id);
void smp_pmap_update(unsigned logical_id);
int smp_startup_cpus(unsigned bsp_apic_id, phys_addr_t start_eip);

/*
 * pause instruction is a NOP on cores that do not implement it,
 * so this is safe.  The reason we use a hand-coded value instead
 * of the explicit assembler instruction is because the latter
 * is only available for _Zipause targets (binutils).  That
 * behaviour is undesirable as it unnecessarily blocks the build
 * for a NOP operation.
 */
#define cpu_pause() asm volatile (".word 0x0100000f" : : : "memory")
#define STARTUP_VECTOR_SHIFT	(20 - 8)

#endif
