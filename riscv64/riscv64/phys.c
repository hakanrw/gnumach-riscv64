/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
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

#include <kern/debug.h>
#include <vm/vm_page.h>

#include <riscv64/pmap.h>


/*
 *	pmap_zero_page zeros the specified (machine independent) page.
 */
void
pmap_zero_page(phys_addr_t p)
{
	assert(p != vm_page_fictitious_addr);
	panic("riscv64: pmap_zero_page not implemented");
}

/*
 *	pmap_copy_page copies the specified (machine independent) pages.
 */
void
pmap_copy_page(
	phys_addr_t src,
	phys_addr_t dst)
{
	assert(src != vm_page_fictitious_addr);
	assert(dst != vm_page_fictitious_addr);

	panic("riscv64: pmap_copy_page not implemented");
}

/*
 *	copy_to_phys(src_addr_v, dst_addr_p, count)
 *
 *	Copy virtual memory to physical memory
 */
void
copy_to_phys(
	vm_offset_t 	src_addr_v, 
	phys_addr_t 	dst_addr_p,
	int 		count)
{
	(void) src_addr_v;
	(void) count;
	assert(dst_addr_p != vm_page_fictitious_addr);

	panic("riscv64: copy_to_phys not implemented");
}

/*
 *	copy_from_phys(src_addr_p, dst_addr_v, count)
 *
 *	Copy physical memory to virtual memory.  The virtual memory
 *	is assumed to be present (e.g. the buffer pool).
 */
void
copy_from_phys(
	phys_addr_t 	src_addr_p, 
	vm_offset_t 	dst_addr_v,
	int 		count)
{
	(void) dst_addr_v;
	(void) count;
	assert(src_addr_p != vm_page_fictitious_addr);

	panic("riscv64: copy_from_phys not implemented");
}

/*
 *	kvtophys(addr)
 *
 *	Convert a kernel virtual address to a physical address
 */
phys_addr_t
kvtophys(vm_offset_t addr)
{
	return _kvtophys(addr);
}
