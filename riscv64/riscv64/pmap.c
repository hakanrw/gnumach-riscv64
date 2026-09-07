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

#include <device/dtb.h>
#include <kern/debug.h>
#include <mach/vm_prot.h>
#include <vm/pmap.h>
#include <vm/vm_page.h>

static struct pmap kernel_pmap_store;
pmap_t kernel_pmap;

vm_offset_t kernel_virtual_start;
vm_offset_t kernel_virtual_end;

/* Largest usable RAM region reported by the device tree.  */
static phys_addr_t phys_mem_start;
static vm_size_t phys_mem_size;

extern const void __text_start;
extern const void _end;

static vm_offset_t heap_start;
static vm_offset_t heap_end;

static void
pmap_exclude_from_bootstrap_heap(phys_addr_t start, phys_addr_t end)
{
	phys_addr_t lower_end, upper_start;
	vm_size_t lower_size, upper_size;

	if (end <= heap_start || start >= heap_end)
		return;

	lower_end = trunc_page(start);
	upper_start = round_page(end);
	lower_size = lower_end > heap_start ? lower_end - heap_start : 0;
	upper_size = heap_end > upper_start ? heap_end - upper_start : 0;

	if (lower_size >= upper_size)
		heap_end = lower_end;
	else
		heap_start = upper_start;

	if (heap_start >= heap_end)
		panic("No physical memory available for bootstrap");
}

vm_offset_t
pmap_grab_page(void)
{
	vm_offset_t page;

	if (heap_end - heap_start < PAGE_SIZE)
		panic("Not enough memory to initialize Mach");

	page = heap_start;
	heap_start += PAGE_SIZE;
	return page;
}

void
pmap_discover_physical_memory(struct dtb_node *node)
{
	struct dtb_prop prop;
	dtb_t dtb;
	phys_addr_t start;
	phys_addr_t kernel_start, kernel_end;
	phys_addr_t dtb_start, dtb_end;
	vm_size_t size, dtb_size;
	vm_size_t off = 0;

	prop = dtb_node_find_prop(node, "reg");
	assert(!DTB_IS_SENTINEL(prop));

	/*
	 *	TODO: We currently only consider a single largest
	 *	region of memory.  It appears to be a limitation
	 *	of the vm_page module, it can only handle a single
	 *	region at the given "seg_index", of which there are
	 *	only 4?
	 */
	while (off < prop.length) {
		start = dtb_prop_read_cells(&prop, node->address_cells, &off);
		size = dtb_prop_read_cells(&prop, node->size_cells, &off);
		if (size > phys_mem_size) {
			phys_mem_start = start;
			phys_mem_size = size;
		}
	}

	assert(phys_mem_size != 0);
	/* TODO: is VM_PAGE_SEG_DMA appropriate here? */
	vm_page_load(VM_PAGE_SEG_DMA, phys_mem_start,
	             phys_mem_start + phys_mem_size);

	kernel_start = (phys_addr_t) &__text_start;
	kernel_end = (phys_addr_t) &_end;
	dtb_get_location(&dtb, &dtb_size);
	dtb_start = (phys_addr_t) dtb;
	dtb_end = dtb_start + dtb_size;

	heap_start = round_page(phys_mem_start);
	heap_end = trunc_page(phys_mem_start + phys_mem_size);
	pmap_exclude_from_bootstrap_heap(kernel_start, kernel_end);
	pmap_exclude_from_bootstrap_heap(dtb_start, dtb_end);
}

void
pmap_bootstrap(void)
{
	kernel_pmap = &kernel_pmap_store;
	panic("riscv64: pmap_bootstrap not implemented");
}

void
pmap_virtual_space(vm_offset_t *start, vm_offset_t *end)
{
	*start = kernel_virtual_start;
	*end = kernel_virtual_end;
}

void
pmap_init(void)
{
	panic("riscv64: pmap_init not implemented");
}

pmap_t
pmap_create(vm_size_t size)
{
	(void) size;
	panic("riscv64: pmap_create not implemented");
}

void
pmap_destroy(pmap_t pmap)
{
	(void) pmap;
	panic("riscv64: pmap_destroy not implemented");
}

void
pmap_reference(pmap_t pmap)
{
	if (pmap != PMAP_NULL)
		pmap->ref_count++;
}

void
pmap_enter(pmap_t pmap, vm_offset_t va, phys_addr_t pa,
	   vm_prot_t prot, boolean_t wired)
{
	(void) pmap;
	(void) va;
	(void) pa;
	(void) prot;
	(void) wired;
	panic("riscv64: pmap_enter not implemented");
}

void
pmap_remove(pmap_t pmap, vm_offset_t start, vm_offset_t end)
{
	(void) pmap;
	(void) start;
	(void) end;
	panic("riscv64: pmap_remove not implemented");
}

void
pmap_protect(pmap_t pmap, vm_offset_t start, vm_offset_t end,
	     vm_prot_t prot)
{
	(void) pmap;
	(void) start;
	(void) end;
	(void) prot;
	panic("riscv64: pmap_protect not implemented");
}

void
pmap_page_protect(phys_addr_t pa, vm_prot_t prot)
{
	(void) pa;
	(void) prot;
	panic("riscv64: pmap_page_protect not implemented");
}

void
pmap_change_wiring(pmap_t pmap, vm_offset_t va, boolean_t wired)
{
	(void) pmap;
	(void) va;
	(void) wired;
	panic("riscv64: pmap_change_wiring not implemented");
}

phys_addr_t
pmap_extract(pmap_t pmap, vm_offset_t va)
{
	(void) pmap;
	(void) va;
	panic("riscv64: pmap_extract not implemented");
}

void
pmap_collect(pmap_t pmap)
{
	(void) pmap;
}

int
pmap_whatis(pmap_t pmap, vm_offset_t va)
{
	(void) pmap;
	(void) va;
	return 0;
}

void
pmap_activate(pmap_t pmap, thread_t thread, int cpu)
{
	(void) pmap;
	(void) thread;
	(void) cpu;
	panic("riscv64: pmap_activate not implemented");
}

void
pmap_deactivate(pmap_t pmap, thread_t thread, int cpu)
{
	(void) pmap;
	(void) thread;
	(void) cpu;
}

void
pmap_pageable(pmap_t pmap, vm_offset_t start, vm_offset_t end,
	      boolean_t pageable)
{
	(void) pmap;
	(void) start;
	(void) end;
	(void) pageable;
}

vm_offset_t
pmap_map_bd(vm_offset_t virt, phys_addr_t start, phys_addr_t end,
	    vm_prot_t prot)
{
	(void) virt;
	(void) start;
	(void) end;
	(void) prot;
	panic("riscv64: pmap_map_bd not implemented");
}

pmap_mapwindow_t *
pmap_get_mapwindow(pt_entry_t entry)
{
	(void) entry;
	panic("riscv64: pmap_get_mapwindow not implemented");
}

void
pmap_put_mapwindow(pmap_mapwindow_t *map)
{
	(void) map;
	panic("riscv64: pmap_put_mapwindow not implemented");
}

void
pmap_clear_modify(phys_addr_t pa)
{
	(void) pa;
}

boolean_t
pmap_is_modified(phys_addr_t pa)
{
	(void) pa;
	return FALSE;
}

void
pmap_clear_reference(phys_addr_t pa)
{
	(void) pa;
}

boolean_t
pmap_is_referenced(phys_addr_t pa)
{
	(void) pa;
	return FALSE;
}
