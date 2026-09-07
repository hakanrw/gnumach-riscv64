/*
 * Copyright (c) 2023-2026 Free Software Foundation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
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

#ifndef _RISCV64_PMAP_H_
#define _RISCV64_PMAP_H_

#ifndef __ASSEMBLER__

#include <kern/lock.h>
#include <mach/boolean.h>
#include <mach/kern_return.h>
#include <mach/machine/vm_param.h>
#include <mach/vm_statistics.h>

typedef phys_addr_t pt_entry_t;

struct pmap {
	pt_entry_t		*root_table;
	int			ref_count;
	decl_simple_lock_data(, lock)
	struct pmap_statistics	stats;
};

typedef struct pmap *pmap_t;

#define PMAP_NULL	((pmap_t) 0)

typedef struct {
	pt_entry_t	*entry;
	vm_offset_t	vaddr;
} pmap_mapwindow_t;

#define PMAP_NMAPWINDOWS	2

extern pmap_t kernel_pmap;
extern vm_offset_t kernel_virtual_start;
extern vm_offset_t kernel_virtual_end;

#define PMAP_ACTIVATE_KERNEL(cpu)	((void) (cpu))
#define PMAP_DEACTIVATE_KERNEL(cpu)	((void) (cpu))
#define PMAP_ACTIVATE_USER(pmap, thread, cpu)			\
	pmap_activate((pmap), (thread), (cpu))
#define PMAP_DEACTIVATE_USER(pmap, thread, cpu)			\
	pmap_deactivate((pmap), (thread), (cpu))
#define PMAP_CONTEXT(pmap, thread)

#define pmap_kernel()			(kernel_pmap)
#define pmap_resident_count(pmap)	((pmap)->stats.resident_count)
#define pmap_phys_address(frame)	((phys_addr_t) (frame) << PAGE_SHIFT)
#define pmap_phys_to_frame(phys)	((phys_addr_t) (phys) >> PAGE_SHIFT)
#define pmap_copy(dst, src, dst_addr, len, src_addr)
#define pmap_attribute(pmap, addr, size, attr, value)		\
	(KERN_INVALID_ADDRESS)

struct dtb_node;
extern void pmap_discover_physical_memory(struct dtb_node *node);
extern void pmap_bootstrap(void);

extern pmap_mapwindow_t *pmap_get_mapwindow(pt_entry_t entry);
extern void pmap_put_mapwindow(pmap_mapwindow_t *map);

extern void pmap_zero_page(phys_addr_t phys);
extern void pmap_copy_page(phys_addr_t src, phys_addr_t dst);
extern void copy_to_phys(vm_offset_t src, phys_addr_t dst, int count);
extern void copy_from_phys(phys_addr_t src, vm_offset_t dst, int count);
extern phys_addr_t kvtophys(vm_offset_t addr);

#endif /* __ASSEMBLER__ */

#endif /* _RISCV64_PMAP_H_ */
