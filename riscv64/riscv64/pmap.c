/*
 * Mach Operating System
 * Copyright (c) 1991,1990,1989,1988 Carnegie Mellon University
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
 *	File:	pmap.c
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young
 *	(These guys wrote the Vax version)
 *
 *	Physical Map management code for Intel i386, and i486.
 *
 *	Manages physical address maps.
 *
 *	In addition to hardware address maps, this
 *	module is called upon to provide software-use-only
 *	maps which may or may not be stored in the same
 *	form as hardware maps.  These pseudo-maps are
 *	used to store intermediate results from copy
 *	operations to and from address spaces.
 *
 *	Since the information managed by this module is
 *	also stored by the logical address mapping module,
 *	this module may throw away valid virtual-to-physical
 *	mappings at almost any time.  However, invalidations
 *	of virtual-to-physical mappings must be done as
 *	requested.
 *
 *	In order to cope with hardware architectures which
 *	make virtual-to-physical map invalidates expensive,
 *	this module may delay invalidate or reduced protection
 *	operations until such time as they are actually
 *	necessary.  This module is given full information as
 *	to which processors are currently using which maps,
 *	and to when physical maps must be made correct.
 */

#include <string.h>

#include <mach/machine/vm_types.h>

#include <mach/boolean.h>
#include <kern/debug.h>
#include <kern/printf.h>
#include <kern/thread.h>
#include <kern/slab.h>

#include <kern/lock.h>

#include <vm/pmap.h>
#include <vm/vm_map.h>
#include <vm/vm_kern.h>
#include <riscv64/vm_param.h>
#include <mach/vm_prot.h>
#include <vm/vm_object.h>
#include <vm/vm_page.h>
#include <vm/vm_user.h>

#include <mach/machine/vm_param.h>
#include <mach/xen.h>
#include <machine/thread.h>
#include <riscv64/cpu_number.h>
#include <riscv64/proc_reg.h>
#include <riscv64/locore.h>
#include <riscv64/model_dep.h>
#include <riscv64/spl.h>

#if	NCPUS > 1
#include <riscv64/mp_desc.h>
#endif

#include <ddb/db_output.h>
#include <machine/db_machdep.h>

#ifdef	MACH_PSEUDO_PHYS
#define	WRITE_PTE(pte_p, pte_entry)		*(pte_p) = pte_entry?pa_to_ma(pte_entry):0;
#else	/* MACH_PSEUDO_PHYS */
#define	WRITE_PTE(pte_p, pte_entry)		*(pte_p) = (pte_entry);
#endif	/* MACH_PSEUDO_PHYS */

/*
 *	Private data structures.
 */

/*
 *	For each vm_page_t, there is a list of all currently
 *	valid virtual mappings of that page.  An entry is
 *	a pv_entry_t; the list is the pv_table.
 */

typedef struct pv_entry {
	struct pv_entry	*next;		/* next pv_entry */
	pmap_t		pmap;		/* pmap where mapping lies */
	vm_offset_t	va;		/* virtual address for mapping */
} *pv_entry_t;

#define PV_ENTRY_NULL	((pv_entry_t) 0)

pv_entry_t	pv_head_table;		/* array of entries, one per page */

/*
 *	pv_list entries are kept on a list that can only be accessed
 *	with the pmap system locked (at SPLVM, not in the cpus_active set).
 *	The list is refilled from the pv_list_cache if it becomes empty.
 */
pv_entry_t	pv_free_list;		/* free list at SPLVM */
def_simple_lock_data(static, pv_free_list_lock)

#define	PV_ALLOC(pv_e) \
MACRO_BEGIN \
	simple_lock(&pv_free_list_lock); \
	if ((pv_e = pv_free_list) != 0) { \
	    pv_free_list = pv_e->next; \
	} \
	simple_unlock(&pv_free_list_lock); \
MACRO_END

#define	PV_FREE(pv_e) \
MACRO_BEGIN \
	simple_lock(&pv_free_list_lock); \
	pv_e->next = pv_free_list; \
	pv_free_list = pv_e; \
	simple_unlock(&pv_free_list_lock); \
MACRO_END

struct kmem_cache	pv_list_cache;		/* cache of pv_entry structures */

/*
 *	Each entry in the pv_head_table is locked by a bit in the
 *	pv_lock_table.  The lock bits are accessed by the physical
 *	address of the page they lock.
 */

char	*pv_lock_table;		/* pointer to array of bits */
#define pv_lock_table_size(n)	(((n)+BYTE_SIZE-1)/BYTE_SIZE)

/* Has pmap_init completed? */
boolean_t	pmap_initialized = FALSE;

/*
 *	Range of kernel virtual addresses available for kernel memory mapping.
 *	Does not include the virtual addresses used to map physical memory 1-1.
 *	Initialized by pmap_bootstrap.
 */
vm_offset_t kernel_virtual_start;
vm_offset_t kernel_virtual_end;

/*
 *	Index into pv_head table, its lock bits, and the modify/reference
 *	bits.
 */
#define pa_index(pa)	vm_page_table_index(pa)

#define pai_to_pvh(pai)		(&pv_head_table[pai])
#define lock_pvh_pai(pai)	(bit_lock(pai, pv_lock_table))
#define unlock_pvh_pai(pai)	(bit_unlock(pai, pv_lock_table))

/*
 *	Array of physical page attributes for managed pages.
 *	One byte per physical page.
 */
char	*pmap_phys_attributes;

/*
 *	Physical page attributes.  Copy bits from PTE definition.
 */
#define	PHYS_MODIFIED	INTEL_PTE_MOD	/* page modified */
#define	PHYS_REFERENCED	INTEL_PTE_REF	/* page referenced */

/*
 *	Amount of virtual memory mapped by one
 *	page-directory entry.
 */
#define	PDE_MAPPED_SIZE		(pdenum2lin(1))

/*
 *	We allocate page table pages directly from the VM system
 *	through this object.  It maps physical memory.
 */
vm_object_t	pmap_object = VM_OBJECT_NULL;

/*
 *	Locking and TLB invalidation
 */

/*
 *	Locking Protocols:
 *
 *	There are two structures in the pmap module that need locking:
 *	the pmaps themselves, and the per-page pv_lists (which are locked
 *	by locking the pv_lock_table entry that corresponds to the pv_head
 *	for the list in question.)  Most routines want to lock a pmap and
 *	then do operations in it that require pv_list locking -- however
 *	pmap_remove_all and pmap_copy_on_write operate on a physical page
 *	basis and want to do the locking in the reverse order, i.e. lock
 *	a pv_list and then go through all the pmaps referenced by that list.
 *	To protect against deadlock between these two cases, the pmap_lock
 *	is used.  There are three different locking protocols as a result:
 *
 *  1.  pmap operations only (pmap_extract, pmap_access, ...)  Lock only
 *		the pmap.
 *
 *  2.  pmap-based operations (pmap_enter, pmap_remove, ...)  Get a read
 *		lock on the pmap_lock (shared read), then lock the pmap
 *		and finally the pv_lists as needed [i.e. pmap lock before
 *		pv_list lock.]
 *
 *  3.  pv_list-based operations (pmap_remove_all, pmap_copy_on_write, ...)
 *		Get a write lock on the pmap_lock (exclusive write); this
 *		also guaranteees exclusive access to the pv_lists.  Lock the
 *		pmaps as needed.
 *
 *	At no time may any routine hold more than one pmap lock or more than
 *	one pv_list lock.  Because interrupt level routines can allocate
 *	mbufs and cause pmap_enter's, the pmap_lock and the lock on the
 *	kernel_pmap can only be held at splvm.
 */

#if	NCPUS > 1
/*
 *	We raise the interrupt level to splvm, to block interprocessor
 *	interrupts during pmap operations.  We must take the CPU out of
 *	the cpus_active set while interrupts are blocked.
 */
#define SPLVM(spl)	\
MACRO_BEGIN \
	spl = splvm(); \
	i_bit_clear(cpu_number(), &cpus_active); \
MACRO_END

#define SPLX(spl)	\
MACRO_BEGIN \
	i_bit_set(cpu_number(), &cpus_active); \
	splx(spl); \
MACRO_END

/*
 *	Lock on pmap system
 */
lock_data_t	pmap_system_lock;

#define PMAP_READ_LOCK(pmap, spl) \
MACRO_BEGIN \
	SPLVM(spl); \
	lock_read(&pmap_system_lock); \
	simple_lock(&(pmap)->lock); \
MACRO_END

#define PMAP_WRITE_LOCK(spl) \
MACRO_BEGIN \
	SPLVM(spl); \
	lock_write(&pmap_system_lock); \
MACRO_END

#define PMAP_READ_UNLOCK(pmap, spl) \
MACRO_BEGIN \
	simple_unlock(&(pmap)->lock); \
	lock_read_done(&pmap_system_lock); \
	SPLX(spl); \
MACRO_END

#define PMAP_WRITE_UNLOCK(spl) \
MACRO_BEGIN \
	lock_write_done(&pmap_system_lock); \
	SPLX(spl); \
MACRO_END

#define PMAP_WRITE_TO_READ_LOCK(pmap) \
MACRO_BEGIN \
	simple_lock(&(pmap)->lock); \
	lock_write_to_read(&pmap_system_lock); \
MACRO_END

#define LOCK_PVH(index)		(lock_pvh_pai(index))

#define UNLOCK_PVH(index)	(unlock_pvh_pai(index))

#define PMAP_UPDATE_TLBS(pmap, s, e) \
MACRO_BEGIN \
	cpu_set	cpu_mask = 1 << cpu_number(); \
	cpu_set	users; \
 \
	/* Since the pmap is locked, other updates are locked */ \
	/* out, and any pmap_activate has finished. */ \
 \
	/* find other cpus using the pmap */ \
	users = (pmap)->cpus_using & ~cpu_mask; \
	if (users) { \
	    /* signal them, and wait for them to finish */ \
	    /* using the pmap */ \
	    signal_cpus(users, (pmap), (s), (e)); \
	    while ((pmap)->cpus_using & cpus_active & ~cpu_mask) \
		cpu_pause(); \
	} \
 \
	/* invalidate our own TLB if pmap is in use */ \
	if ((pmap)->cpus_using & cpu_mask) { \
	    INVALIDATE_TLB((pmap), (s), (e)); \
	} \
MACRO_END

#else	/* NCPUS > 1 */

#define SPLVM(spl) ((void)(spl))
#define SPLX(spl) ((void)(spl))

#define PMAP_READ_LOCK(pmap, spl)	SPLVM(spl)
#define PMAP_WRITE_LOCK(spl)		SPLVM(spl)
#define PMAP_READ_UNLOCK(pmap, spl)	SPLX(spl)
#define PMAP_WRITE_UNLOCK(spl)		SPLX(spl)
#define PMAP_WRITE_TO_READ_LOCK(pmap)

#define LOCK_PVH(index)
#define UNLOCK_PVH(index)

#define PMAP_UPDATE_TLBS(pmap, s, e) \
MACRO_BEGIN \
	/* invalidate our own TLB if pmap is in use */ \
	if ((pmap)->cpus_using) { \
	    INVALIDATE_TLB((pmap), (s), (e)); \
	} \
MACRO_END

#endif	/* NCPUS > 1 */

#ifdef	MACH_PV_PAGETABLES
#define INVALIDATE_TLB(pmap, s, e) \
MACRO_BEGIN \
	panic("TODO: Not implemented"); \
MACRO_END
#else	/* MACH_PV_PAGETABLES */
/* It is hard to know when a TLB flush becomes less expensive than a bunch of
 * invlpgs.  But it surely is more expensive than just one invlpg.  */
#define INVALIDATE_TLB(pmap, s, e) \
MACRO_BEGIN \
	panic("TODO: Not implemented"); \
MACRO_END
#endif	/* MACH_PV_PAGETABLES */


#if	NCPUS > 1
/*
 *	Structures to keep track of pending TLB invalidations
 */

#define UPDATE_LIST_SIZE	4

struct pmap_update_item {
	pmap_t		pmap;		/* pmap to invalidate */
	vm_offset_t	start;		/* start address to invalidate */
	vm_offset_t	end;		/* end address to invalidate */
} ;

typedef	struct pmap_update_item	*pmap_update_item_t;

/*
 *	List of pmap updates.  If the list overflows,
 *	the last entry is changed to invalidate all.
 */
struct pmap_update_list {
	decl_simple_lock_data(,	lock)
	int			count;
	struct pmap_update_item	item[UPDATE_LIST_SIZE];
} ;
typedef	struct pmap_update_list	*pmap_update_list_t;

struct pmap_update_list	cpu_update_list[NCPUS];

cpu_set		cpus_active;
cpu_set		cpus_idle;
volatile
boolean_t	cpu_update_needed[NCPUS];

#endif	/* NCPUS > 1 */

/*
 *	Other useful macros.
 */
#define current_pmap()		(vm_map_pmap(current_thread()->task->map))
#define pmap_in_use(pmap, cpu)	(((pmap)->cpus_using & (1 << (cpu))) != 0)

struct pmap	kernel_pmap_store;
pmap_t		kernel_pmap;

struct kmem_cache pmap_cache;  /* cache of pmap structures */
struct kmem_cache pt_cache;    /* cache of page tables */
struct kmem_cache pd_cache;    /* cache of page directories */
#if PAE
struct kmem_cache pdpt_cache;  /* cache of page directory pointer tables */
#ifdef __x86_64__
struct kmem_cache l4_cache;    /* cache of L4 tables */
#endif /* __x86_64__ */
#endif /* PAE */

boolean_t		pmap_debug = FALSE;	/* flag for debugging prints */

#if 0
int		ptes_per_vm_page;	/* number of hardware ptes needed
					   to map one VM page. */
#else
#define		ptes_per_vm_page	1
#endif

unsigned int	inuse_ptepages_count = 0;	/* debugging */

/*
 * Pointer to the basic page directory for the kernel.
 * Initialized by pmap_bootstrap().
 */
pt_entry_t *kernel_page_dir;

/*
 * Two slots for temporary physical page mapping, to allow for
 * physical-to-physical transfers.
 */
static pmap_mapwindow_t mapwindows[PMAP_NMAPWINDOWS * NCPUS];
#define MAPWINDOW_SIZE (PMAP_NMAPWINDOWS * NCPUS * PAGE_SIZE)

static inline pt_entry_t *
pmap_pde(const pmap_t pmap, vm_offset_t addr)
{
	pt_entry_t *page_dir;
	if (pmap == kernel_pmap)
		addr = kvtolin(addr);

	panic("TODO: not implemented");
}
/*
 *	Given an offset and a map, compute the address of the
 *	pte.  If the address is invalid with respect to the map
 *	then PT_ENTRY_NULL is returned (and the map may need to grow).
 *
 *	This is only used internally.
 */
pt_entry_t *
pmap_pte(const pmap_t pmap, vm_offset_t addr)
{
	pt_entry_t	*ptp;
	pt_entry_t	pte;

	panic("TODO: not implemented");
}

#define DEBUG_PTE_PAGE	0

#if	DEBUG_PTE_PAGE
void ptep_check(ptep_t ptep)
{
	pt_entry_t		*pte, *epte;
	int			ctu, ctw;

	/* check the use and wired counts */
	if (ptep == PTE_PAGE_NULL)
		return;

	panic("TODO: not implemented");
}
#endif	/* DEBUG_PTE_PAGE */

/*
 *	Back-door routine for mapping kernel VM at initialization.
 * 	Useful for mapping memory outside the range of direct mapped
 *	physical memory (i.e., devices).
 */
vm_offset_t pmap_map_bd(
	vm_offset_t	virt,
	phys_addr_t	start,
	phys_addr_t	end,
	vm_prot_t	prot)
{
	pt_entry_t	template;
	pt_entry_t	*pte;
	int		spl;

	panic("TODO: not implemented");
}

#ifdef PAE
static void pmap_bootstrap_pae(void)
{
	vm_offset_t addr;
	pt_entry_t *pdp_kernel;

	panic("TODO: not implemented");
}
#endif /* PAE */

#ifdef	MACH_PV_PAGETABLES
#ifdef PAE
#define NSUP_L1 4
#else
#define NSUP_L1 1
#endif
static void pmap_bootstrap_xen(pt_entry_t *l1_map[NSUP_L1])
{
	panic("TODO: not implemented");

}
#endif	/* MACH_PV_PAGETABLES */

/*
 *	Bootstrap the system enough to run with virtual memory.
 *	Allocate the kernel page directory and page tables,
 *	and direct-map all physical memory.
 *	Called with mapping off.
 */
void pmap_bootstrap(void)
{
	/*
	 * Mapping is turned off; we must reference only physical addresses.
	 * The load image of the system is to be mapped 1-1 physical = virtual.
	 */

	/*
	 *	Set ptes_per_vm_page for general use.
	 */
#if 0
	ptes_per_vm_page = PAGE_SIZE / INTEL_PGBYTES;
#endif

	/*
	 *	The kernel's pmap is statically allocated so we don't
	 *	have to use pmap_create, which is unlikely to work
	 *	correctly at this part of the boot sequence.
	 */

	kernel_pmap = &kernel_pmap_store;

#if	NCPUS > 1
	lock_init(&pmap_system_lock, FALSE);	/* NOT a sleep lock */
#endif	/* NCPUS > 1 */

	simple_lock_init(&kernel_pmap->lock);

	kernel_pmap->ref_count = 1;

	/*
	 * Determine the kernel virtual address range.
	 * It starts at the end of the physical memory
	 * mapped into the kernel address space,
	 * and extends to a stupid arbitrary limit beyond that.
	 */

	panic("TODO: not implemented");
}

#ifdef	MACH_PV_PAGETABLES
/* These are only required because of Xen security policies */

/* Set back a page read write */
void pmap_set_page_readwrite(void *_vaddr) {
	vm_offset_t vaddr = (vm_offset_t) _vaddr;
	phys_addr_t paddr = kvtophys(vaddr);
	vm_offset_t canon_vaddr = phystokv(paddr);

	panic("TODO: not implemented");
}

/* Set a page read only (so as to pin it for instance) */
void pmap_set_page_readonly(void *_vaddr) {
	vm_offset_t vaddr = (vm_offset_t) _vaddr;
	phys_addr_t paddr = kvtophys(vaddr);
	vm_offset_t canon_vaddr = phystokv(paddr);

	panic("TODO: not implemented");
}

/* This needs to be called instead of pmap_set_page_readonly as long as RC3
 * still points to the bootstrap dirbase, to also fix the bootstrap table.  */
void pmap_set_page_readonly_init(void *_vaddr) {
	panic("TODO: not implemented");
}

void pmap_clear_bootstrap_pagetable(pt_entry_t *base) {
	unsigned i;
	pt_entry_t *dir;
	vm_offset_t va = 0;

	panic("TODO: not implemented");
}
#endif	/* MACH_PV_PAGETABLES */

/*
 * Create a temporary mapping for a given physical entry
 *
 * This can be used to access physical pages which are not mapped 1:1 by
 * phystokv().
 */
pmap_mapwindow_t *pmap_get_mapwindow(pt_entry_t entry)
{
	pmap_mapwindow_t *map;
	int cpu = cpu_number();

	assert(entry != 0);

	/* Find an empty one.  */
	for (map = &mapwindows[cpu * PMAP_NMAPWINDOWS]; map < &mapwindows[(cpu+1) * PMAP_NMAPWINDOWS]; map++)
		if (!(*map->entry))
			break;
	assert(map < &mapwindows[(cpu+1) * PMAP_NMAPWINDOWS]);

#ifdef MACH_PV_PAGETABLES
	if (!hyp_mmu_update_pte(kv_to_ma(map->entry), pa_to_ma(entry)))
		panic("pmap_get_mapwindow");
#else /* MACH_PV_PAGETABLES */
	WRITE_PTE(map->entry, entry);
#endif /* MACH_PV_PAGETABLES */
	INVALIDATE_TLB(kernel_pmap, map->vaddr, map->vaddr + PAGE_SIZE);
	return map;
}

/*
 * Destroy a temporary mapping for a physical entry
 */
void pmap_put_mapwindow(pmap_mapwindow_t *map)
{
#ifdef MACH_PV_PAGETABLES
	if (!hyp_mmu_update_pte(kv_to_ma(map->entry), 0))
		panic("pmap_put_mapwindow");
#else /* MACH_PV_PAGETABLES */
	WRITE_PTE(map->entry, 0);
#endif /* MACH_PV_PAGETABLES */
	INVALIDATE_TLB(kernel_pmap, map->vaddr, map->vaddr + PAGE_SIZE);
}

void pmap_virtual_space(
	vm_offset_t *startp,
	vm_offset_t *endp)
{
	*startp = kernel_virtual_start;
	*endp = kernel_virtual_end - MAPWINDOW_SIZE;
}

/*
 *	Initialize the pmap module.
 *	Called by vm_init, to initialize any structures that the pmap
 *	system needs to map virtual memory.
 */
void pmap_init(void)
{
	unsigned long		npages;
	vm_offset_t		addr;
	vm_size_t		s;
#if	NCPUS > 1
	int			i;
#endif	/* NCPUS > 1 */

	/*
	 *	Allocate memory for the pv_head_table and its lock bits,
	 *	the modify bit array, and the pte_page table.
	 */

	npages = vm_page_table_size();
	s = (vm_size_t) (sizeof(struct pv_entry) * npages
				+ pv_lock_table_size(npages)
				+ npages);

	s = round_page(s);
	if (kmem_alloc_wired(kernel_map, &addr, s) != KERN_SUCCESS)
		panic("pmap_init");
	memset((void *) addr, 0, s);

	/*
	 *	Allocate the structures first to preserve word-alignment.
	 */
	pv_head_table = (pv_entry_t) addr;
	addr = (vm_offset_t) (pv_head_table + npages);

	pv_lock_table = (char *) addr;
	addr = (vm_offset_t) (pv_lock_table + pv_lock_table_size(npages));

	pmap_phys_attributes = (char *) addr;

	panic("TODO: not implemented");
}

static inline boolean_t
valid_page(phys_addr_t addr)
{
	struct vm_page *p;

	if (!pmap_initialized)
		return FALSE;

	p = vm_page_lookup_pa(addr);
	return (p != NULL);
}

/*
 *	Routine:	pmap_page_table_page_alloc
 *
 *	Allocates a new physical page to be used as a page-table page.
 *
 *	Must be called with the pmap system and the pmap unlocked,
 *	since these must be unlocked to use vm_page_grab.
 */
#ifdef	MACH_XEN
static vm_offset_t
pmap_page_table_page_alloc(void)
{
	vm_page_t	m;
	phys_addr_t	pa;

	check_simple_locks();

	/*
	 *	We cannot allocate the pmap_object in pmap_init,
	 *	because it is called before the cache package is up.
	 *	Allocate it now if it is missing.
	 */
	if (pmap_object == VM_OBJECT_NULL)
	    pmap_object = vm_object_allocate(vm_page_table_size() * PAGE_SIZE);

	/*
	 *	Allocate a VM page for the level 2 page table entries.
	 */
	while ((m = vm_page_grab(VM_PAGE_DIRECTMAP)) == VM_PAGE_NULL)
		VM_PAGE_WAIT((void (*)()) 0);

	/*
	 *	Map the page to its physical address so that it
	 *	can be found later.
	 */
	pa = m->phys_addr;
	assert(pa == (vm_offset_t) pa);
	vm_object_lock(pmap_object);
	vm_page_insert(m, pmap_object, pa);
	vm_page_lock_queues();
	vm_page_wire(m);
	inuse_ptepages_count++;
	vm_page_unlock_queues();
	vm_object_unlock(pmap_object);

	/*
	 *	Zero the page.
	 */
	memset((void *)phystokv(pa), 0, PAGE_SIZE);

	return pa;
}
#endif

#ifdef	MACH_XEN
void pmap_map_mfn(void *_addr, unsigned long mfn) {
	vm_offset_t addr = (vm_offset_t) _addr;
	pt_entry_t	*pte, *pdp;
	vm_offset_t	ptp;
	pt_entry_t ma = ((pt_entry_t) mfn) << PAGE_SHIFT;

	panic("TODO: not implemented");
}
#endif	/* MACH_XEN */

/*
 *	Deallocate a page-table page.
 *	The page-table page must have all mappings removed,
 *	and be removed from its page directory.
 */
#ifdef	MACH_XEN
static void
pmap_page_table_page_dealloc(vm_offset_t pa)
{
	vm_page_t	m;

	vm_object_lock(pmap_object);
	m = vm_page_lookup(pmap_object, pa);
	vm_page_lock_queues();
#ifdef	MACH_PV_PAGETABLES
        if (!hyp_mmuext_op_mfn (MMUEXT_UNPIN_TABLE, pa_to_mfn(pa)))
	        panic("couldn't unpin page %llx(%lx)\n", (uint64_t) pa, (unsigned long) kv_to_ma(pa));
        pmap_set_page_readwrite((void*) phystokv(pa));
#endif	/* MACH_PV_PAGETABLES */
	vm_page_free(m);
	inuse_ptepages_count--;
	vm_page_unlock_queues();
	vm_object_unlock(pmap_object);
}
#endif

/*
 *	Create and return a physical map.
 *
 *	If the size specified for the map
 *	is zero, the map is an actual physical
 *	map, and may be referenced by the
 *	hardware.
 *
 *	If the size specified is non-zero,
 *	the map will be used in software only, and
 *	is bounded by that size.
 */
pmap_t pmap_create(vm_size_t size)
{
	pt_entry_t		*page_dir[PDPNUM];
	int			i;
	pmap_t			p;
	pmap_statistics_t	stats;

	/*
	 *	A software use-only map doesn't even need a map.
	 */

	if (size != 0) {
		return(PMAP_NULL);
	}

/*
 *	Allocate a pmap struct from the pmap_cache.  Then allocate
 *	the page descriptor table.
 */

	panic("TODO: not implemented");

	return(p);
}

/*
 *	Retire the given physical map from service.
 *	Should only be called if the map contains
 *	no valid mappings.
 */

void pmap_destroy(pmap_t p)
{
	int		c, s;

	if (p == PMAP_NULL)
		return;

	SPLVM(s);
	simple_lock(&p->lock);
	c = --p->ref_count;
	simple_unlock(&p->lock);
	SPLX(s);

	if (c != 0) {
	    return;	/* still in use */
	}

        /*
         * Free the page table tree.
         */
	panic("TODO: not implemented");
}

/*
 *	Add a reference to the specified pmap.
 */

void pmap_reference(pmap_t p)
{
	int	s;
	if (p != PMAP_NULL) {
		SPLVM(s);
		simple_lock(&p->lock);
		p->ref_count++;
		simple_unlock(&p->lock);
		SPLX(s);
	}
}

/*
 *	Remove a range of hardware page-table entries.
 *	The entries given are the first (inclusive)
 *	and last (exclusive) entries for the VM pages.
 *	The virtual address is the va for the first pte.
 *
 *	The pmap must be locked.
 *	If the pmap is not the kernel pmap, the range must lie
 *	entirely within one pte-page.  This is NOT checked.
 *	Assumes that the pte-page exists.
 */

static
void pmap_remove_range(
	pmap_t			pmap,
	vm_offset_t		va,
	pt_entry_t		*spte,
	pt_entry_t		*epte)
{
	pt_entry_t		*cpte;
	unsigned long		num_removed, num_unwired;
	unsigned long		pai;
	phys_addr_t		pa;

	panic("TODO: not implemented");
}

/*
 *	Remove the given range of addresses
 *	from the specified map.
 *
 *	It is assumed that the start and end are properly
 *	rounded to the hardware page size.
 */

void pmap_remove(
	pmap_t		map,
	vm_offset_t	s, 
	vm_offset_t	e)
{
	int			spl;
	pt_entry_t		*spte, *epte;
	vm_offset_t		l;
	vm_offset_t		_s = s;

	if (map == PMAP_NULL)
		return;

	panic("TODO: not implemented");
}

/*
 *	Routine:	pmap_page_protect
 *
 *	Function:
 *		Lower the permission for all mappings to a given
 *		page.
 */
void pmap_page_protect(
	phys_addr_t	phys,
	vm_prot_t	prot)
{
	pv_entry_t		pv_h, prev;
	pv_entry_t		pv_e;
	pt_entry_t		*pte;
	unsigned long		pai;
	pmap_t			pmap;
	int			spl;
	boolean_t		remove;

	assert(phys != vm_page_fictitious_addr);
	if (!valid_page(phys)) {
	    /*
	     *	Not a managed page.
	     */
	    return;
	}

	/*
	 * Determine the new protection.
	 */
	switch (prot) {
	    case VM_PROT_READ:
	    case VM_PROT_READ|VM_PROT_EXECUTE:
		remove = FALSE;
		break;
	    case VM_PROT_ALL:
		return;	/* nothing to do */
	    default:
		remove = TRUE;
		break;
	}

	/*
	 *	Lock the pmap system first, since we will be changing
	 *	several pmaps.
	 */

	PMAP_WRITE_LOCK(spl);

	pai = pa_index(phys);
	pv_h = pai_to_pvh(pai);

	/*
	 * Walk down PV list, changing or removing all mappings.
	 * We do not have to lock the pv_list because we have
	 * the entire pmap system locked.
	 */
	if (pv_h->pmap != PMAP_NULL) {

	    prev = pv_e = pv_h;
	    do {
		vm_offset_t va;

		pmap = pv_e->pmap;
		/*
		 * Lock the pmap to block pmap_extract and similar routines.
		 */
		simple_lock(&pmap->lock);

		va = pv_e->va;
		pte = pmap_pte(pmap, va);

		/*
		 * Consistency checks.
		 */
		assert(*pte & INTEL_PTE_VALID);
		assert(pte_to_pa(*pte) == phys);

		/*
		 * Remove the mapping if new protection is NONE
		 * or if write-protecting a kernel mapping.
		 */
		if (remove || pmap == kernel_pmap) {
		    /*
		     * Remove the mapping, collecting any modify bits.
		     */

		    if (*pte & INTEL_PTE_WIRED) {
			pmap->stats.wired_count--;
		    }

		    {
			int	i = ptes_per_vm_page;

			do {
			    pmap_phys_attributes[pai] |=
				*pte & (PHYS_MODIFIED|PHYS_REFERENCED);
#ifdef	MACH_PV_PAGETABLES
			    if (!hyp_mmu_update_pte(kv_to_ma(pte++), 0))
			    	panic("%s:%d could not clear pte %p\n",__FILE__,__LINE__,pte-1);
#else	/* MACH_PV_PAGETABLES */
			    *pte++ = 0;
#endif	/* MACH_PV_PAGETABLES */
			} while (--i > 0);
		    }

		    pmap->stats.resident_count--;

		    /*
		     * Remove the pv_entry.
		     */
		    if (pv_e == pv_h) {
			/*
			 * Fix up head later.
			 */
			pv_h->pmap = PMAP_NULL;
		    }
		    else {
			/*
			 * Delete this entry.
			 */
			prev->next = pv_e->next;
			PV_FREE(pv_e);
		    }
		}
		else {
		    /*
		     * Write-protect.
		     */
		    int i = ptes_per_vm_page;

		    do {
#ifdef	MACH_PV_PAGETABLES
			if (!hyp_mmu_update_pte(kv_to_ma(pte), *pte & ~INTEL_PTE_WRITE))
			    	panic("%s:%d could not disable write on pte %p\n",__FILE__,__LINE__,pte);
#else	/* MACH_PV_PAGETABLES */
			*pte &= ~INTEL_PTE_WRITE;
#endif	/* MACH_PV_PAGETABLES */
			pte++;
		    } while (--i > 0);

		    /*
		     * Advance prev.
		     */
		    prev = pv_e;
		}
		PMAP_UPDATE_TLBS(pmap, va, va + PAGE_SIZE);

		simple_unlock(&pmap->lock);

	    } while ((pv_e = prev->next) != PV_ENTRY_NULL);

	    /*
	     * If pv_head mapping was removed, fix it up.
	     */
	    if (pv_h->pmap == PMAP_NULL) {
		pv_e = pv_h->next;
		if (pv_e != PV_ENTRY_NULL) {
		    *pv_h = *pv_e;
		    PV_FREE(pv_e);
		}
	    }
	}

	PMAP_WRITE_UNLOCK(spl);
}

/*
 *	Set the physical protection on the
 *	specified range of this map as requested.
 *	Will not increase permissions.
 */
void pmap_protect(
	pmap_t		map,
	vm_offset_t	s, 
	vm_offset_t	e,
	vm_prot_t	prot)
{
	pt_entry_t	*spte, *epte;
	vm_offset_t	l;
	int		spl;
	vm_offset_t	_s = s;

	if (map == PMAP_NULL)
		return;

	/*
	 * Determine the new protection.
	 */
	switch (prot) {
	    case VM_PROT_READ:
	    case VM_PROT_READ|VM_PROT_EXECUTE:
		break;
	    case VM_PROT_READ|VM_PROT_WRITE:
	    case VM_PROT_ALL:
		return;	/* nothing to do */
	    default:
		pmap_remove(map, s, e);
		return;
	}

	simple_lock(&map->lock);

	panic("TODO: not implemented");

	simple_unlock(&map->lock);
	SPLX(spl);
}

typedef	pt_entry_t* (*pmap_level_getter_t)(const pmap_t pmap, vm_offset_t addr);
/*
* Expand one single level of the page table tree
*/
static inline pt_entry_t* pmap_expand_level(pmap_t pmap, vm_offset_t v, int spl,
                                            pmap_level_getter_t pmap_level,
                                            pmap_level_getter_t pmap_level_upper,
                                            int n_per_vm_page,
                                            struct kmem_cache *cache)
{
	pt_entry_t		*pte;

	/*
	 *	Expand pmap to include this pte.  Assume that
	 *	pmap is always expanded to include enough hardware
	 *	pages to map one VM page.
	 */
	while ((pte = pmap_level(pmap, v)) == PT_ENTRY_NULL) {
	    /*
	     * Need to allocate a new page-table page.
	     */
	    vm_offset_t	ptp;
	    pt_entry_t	*pdp;
	    int		i;

	    if (pmap == kernel_pmap) {
		/*
		 * Would have to enter the new page-table page in
		 * EVERY pmap.
		 */
		panic("pmap_expand kernel pmap to %#zx", v);
	    }

	    /*
	     * Unlock the pmap and allocate a new page-table page.
	     */
	    PMAP_READ_UNLOCK(pmap, spl);

	    while (!(ptp = kmem_cache_alloc(cache)))
		VM_PAGE_WAIT((void (*)()) 0);
	    memset((void *)ptp, 0, PAGE_SIZE);

	    /*
	     * Re-lock the pmap and check that another thread has
	     * not already allocated the page-table page.  If it
	     * has, discard the new page-table page (and try
	     * again to make sure).
	     */
	    PMAP_READ_LOCK(pmap, spl);

	    if (pmap_level(pmap, v) != PT_ENTRY_NULL) {
		/*
		 * Oops...
		 */
		PMAP_READ_UNLOCK(pmap, spl);
		kmem_cache_free(cache, ptp);
		PMAP_READ_LOCK(pmap, spl);
		continue;
	    }

	    /*
	     * Enter the new page table page in the page directory.
	     */
	panic("TODO: not implemented");

	}
        return pte;
}

/*
 * Expand, if required, the PMAP to include the virtual address V.
 * PMAP needs to be locked, and it will be still locked on return. It
 * can temporarily unlock the PMAP, during allocation or deallocation
 * of physical pages.
 */
static inline pt_entry_t* pmap_expand(pmap_t pmap, vm_offset_t v, int spl)
{
#ifdef PAE
#ifdef __x86_64__
	pmap_expand_level(pmap, v, spl, pmap_ptp, pmap_l4base, 1, &pdpt_cache);
#endif /* __x86_64__ */
	pmap_expand_level(pmap, v, spl, pmap_pde, pmap_ptp, 1, &pd_cache);
#endif /* PAE */
	return pmap_expand_level(pmap, v, spl, pmap_pte, pmap_pde, ptes_per_vm_page, &pt_cache);
}

/*
 *	Insert the given physical page (p) at
 *	the specified virtual address (v) in the
 *	target physical map with the protection requested.
 *
 *	If specified, the page will be wired down, meaning
 *	that the related pte can not be reclaimed.
 *
 *	NB:  This is the only routine which MAY NOT lazy-evaluate
 *	or lose information.  That is, this routine must actually
 *	insert this page into the given map NOW.
 */
void pmap_enter(
	pmap_t			pmap,
	vm_offset_t		v,
	phys_addr_t		pa,
	vm_prot_t		prot,
	boolean_t		wired)
{
	boolean_t		is_physmem;
	pt_entry_t		*pte;
	pv_entry_t		pv_h;
	unsigned long		i, pai;
	pv_entry_t		pv_e;
	pt_entry_t		template;
	int			spl;
	phys_addr_t		old_pa;

	assert(pa != vm_page_fictitious_addr);
	if (pmap_debug) printf("pmap(%zx, %llx)\n", v, (unsigned long long) pa);
	if (pmap == PMAP_NULL)
		return;

	panic("TODO: not implemented");
}

/*
 *	Routine:	pmap_change_wiring
 *	Function:	Change the wiring attribute for a map/virtual-address
 *			pair.
 *	In/out conditions:
 *			The mapping must already exist in the pmap.
 */
void pmap_change_wiring(
	pmap_t		map,
	vm_offset_t	v,
	boolean_t	wired)
{
	pt_entry_t	*pte;
	int		i;
	int		spl;

	panic("TODO: not implemented");
}

/*
 *	Routine:	pmap_extract
 *	Function:
 *		Extract the physical page address associated
 *		with the given map/virtual_address pair.
 */

phys_addr_t pmap_extract(
	pmap_t		pmap,
	vm_offset_t	va)
{
	pt_entry_t	*pte;
	phys_addr_t	pa;
	int		spl;

	SPLVM(spl);
	simple_lock(&pmap->lock);
	if ((pte = pmap_pte(pmap, va)) == PT_ENTRY_NULL)
	    pa = 0;
	else if (!(*pte & INTEL_PTE_VALID))
	    pa = 0;
	else
	    pa = pte_to_pa(*pte) + (va & INTEL_OFFMASK);
	simple_unlock(&pmap->lock);
	SPLX(spl);
	return(pa);
}

/*
 *	Copy the range specified by src_addr/len
 *	from the source map to the range dst_addr/len
 *	in the destination map.
 *
 *	This routine is only advisory and need not do anything.
 */
#if	0
void pmap_copy(
	pmap_t		dst_pmap,
	pmap_t		src_pmap,
	vm_offset_t	dst_addr,
	vm_size_t	len,
	vm_offset_t	src_addr)
{
}
#endif	/* 0 */

/*
 *	Routine:	pmap_collect
 *	Function:
 *		Garbage collects the physical map system for
 *		pages which are no longer used.
 *		Success need not be guaranteed -- that is, there
 *		may well be pages which are not referenced, but
 *		others may be collected.
 *	Usage:
 *		Called by the pageout daemon when pages are scarce.
 */
void pmap_collect(pmap_t p)
{
	pt_entry_t	        *ptp;
	pt_entry_t		*eptp;
	phys_addr_t		pa;
	int			spl, wired;

	if (p == PMAP_NULL)
		return;

	if (p == kernel_pmap)
		return;

	panic("TODO: not implemented");
}

#if	MACH_KDB
/*
 *	Routine:	pmap_whatis
 *	Function:
 *		Check whether this address is within a pmap
 *	Usage:
 *		Called from debugger
 */
int pmap_whatis(pmap_t p, vm_offset_t a)
{
	pt_entry_t	        *ptp;
	phys_addr_t		pa;
	int			spl;
	int			ret = 0;

	if (p == PMAP_NULL)
		return 0;

	panic("TODO: not implemented");

	return ret;
}
#endif /* MACH_KDB */

/*
 *	Routine:	pmap_activate
 *	Function:
 *		Binds the given physical map to the given
 *		processor, and returns a hardware map description.
 */
#if	0
void pmap_activate(pmap_t my_pmap, thread_t th, int my_cpu)
{
	PMAP_ACTIVATE(my_pmap, th, my_cpu);
}
#endif	/* 0 */

/*
 *	Routine:	pmap_deactivate
 *	Function:
 *		Indicates that the given physical map is no longer
 *		in use on the specified processor.  (This is a macro
 *		in pmap.h)
 */
#if	0
void pmap_deactivate(pmap_t pmap, thread_t th, int which_cpu)
{
	PMAP_DEACTIVATE(pmap, th, which_cpu);
}
#endif	/* 0 */

/*
 *	Routine:	pmap_kernel
 *	Function:
 *		Returns the physical map handle for the kernel.
 */
#if	0
pmap_t pmap_kernel()
{
    	return (kernel_pmap);
}
#endif	/* 0 */

/*
 *	pmap_zero_page zeros the specified (machine independent) page.
 *	See machine/phys.c or machine/phys.s for implementation.
 */
#if	0
pmap_zero_page(vm_offset_t phys)
{
	int	i;

	panic("TODO: not implemented");
}
#endif	/* 0 */

/*
 *	pmap_copy_page copies the specified (machine independent) page.
 *	See machine/phys.c or machine/phys.s for implementation.
 */
#if	0
pmap_copy_page(vm_offset_t src, vm_offset_t dst)
{
	int	i;
	panic("TODO: not implemented");
}
#endif	/* 0 */

/*
 *	Routine:	pmap_pageable
 *	Function:
 *		Make the specified pages (by pmap, offset)
 *		pageable (or not) as requested.
 *
 *		A page which is not pageable may not take
 *		a fault; therefore, its page table entry
 *		must remain valid for the duration.
 *
 *		This routine is merely advisory; pmap_enter
 *		will specify that these pages are to be wired
 *		down (or not) as appropriate.
 */
void
pmap_pageable(
	pmap_t		pmap,
	vm_offset_t	start,
	vm_offset_t	end,
	boolean_t	pageable)
{
}

/*
 *	Clear specified attribute bits.
 */
static void
phys_attribute_clear(
	phys_addr_t	phys,
	int		bits)
{
	pv_entry_t		pv_h;
	pv_entry_t		pv_e;
	pt_entry_t		*pte;
	unsigned long		pai;
	pmap_t			pmap;
	int			spl;

	assert(phys != vm_page_fictitious_addr);
	if (!valid_page(phys)) {
	    /*
	     *	Not a managed page.
	     */
	    return;
	}

	/*
	 *	Lock the pmap system first, since we will be changing
	 *	several pmaps.
	 */

	panic("TODO: not implemented");
}

/*
 *	Check specified attribute bits.
 */
static boolean_t
phys_attribute_test(
	phys_addr_t	phys,
	int		bits)
{
	pv_entry_t		pv_h;
	pv_entry_t		pv_e;
	pt_entry_t		*pte;
	unsigned long		pai;
	pmap_t			pmap;
	int			spl;

	assert(phys != vm_page_fictitious_addr);
	if (!valid_page(phys)) {
	    /*
	     *	Not a managed page.
	     */
	    return (FALSE);
	}

	/*
	 *	Lock the pmap system first, since we will be checking
	 *	several pmaps.
	 */

	panic("TODO: not implemented");

	return (FALSE);
}

/*
 *	Clear the modify bits on the specified physical page.
 */

void pmap_clear_modify(phys_addr_t phys)
{
	phys_attribute_clear(phys, PHYS_MODIFIED);
}

/*
 *	pmap_is_modified:
 *
 *	Return whether or not the specified physical page is modified
 *	by any physical maps.
 */

boolean_t pmap_is_modified(phys_addr_t phys)
{
	return (phys_attribute_test(phys, PHYS_MODIFIED));
}

/*
 *	pmap_clear_reference:
 *
 *	Clear the reference bit on the specified physical page.
 */

void pmap_clear_reference(phys_addr_t phys)
{
	phys_attribute_clear(phys, PHYS_REFERENCED);
}

/*
 *	pmap_is_referenced:
 *
 *	Return whether or not the specified physical page is referenced
 *	by any physical maps.
 */

boolean_t pmap_is_referenced(phys_addr_t phys)
{
	return (phys_attribute_test(phys, PHYS_REFERENCED));
}

#if	NCPUS > 1
/*
*	    TLB Coherence Code (TLB "shootdown" code)
*
* Threads that belong to the same task share the same address space and
* hence share a pmap.  However, they  may run on distinct cpus and thus
* have distinct TLBs that cache page table entries. In order to guarantee
* the TLBs are consistent, whenever a pmap is changed, all threads that
* are active in that pmap must have their TLB updated. To keep track of
* this information, the set of cpus that are currently using a pmap is
* maintained within each pmap structure (cpus_using). Pmap_activate() and
* pmap_deactivate add and remove, respectively, a cpu from this set.
* Since the TLBs are not addressable over the bus, each processor must
* flush its own TLB; a processor that needs to invalidate another TLB
* needs to interrupt the processor that owns that TLB to signal the
* update.
*
* Whenever a pmap is updated, the lock on that pmap is locked, and all
* cpus using the pmap are signaled to invalidate. All threads that need
* to activate a pmap must wait for the lock to clear to await any updates
* in progress before using the pmap. They must ACQUIRE the lock to add
* their cpu to the cpus_using set. An implicit assumption made
* throughout the TLB code is that all kernel code that runs at or higher
* than splvm blocks out update interrupts, and that such code does not
* touch pageable pages.
*
* A shootdown interrupt serves another function besides signaling a
* processor to invalidate. The interrupt routine (pmap_update_interrupt)
* waits for the both the pmap lock (and the kernel pmap lock) to clear,
* preventing user code from making implicit pmap updates while the
* sending processor is performing its update. (This could happen via a
* user data write reference that turns on the modify bit in the page
* table). It must wait for any kernel updates that may have started
* concurrently with a user pmap update because the IPC code
* changes mappings.
* Spinning on the VALUES of the locks is sufficient (rather than
* having to acquire the locks) because any updates that occur subsequent
* to finding the lock unlocked will be signaled via another interrupt.
* (This assumes the interrupt is cleared before the low level interrupt code
* calls pmap_update_interrupt()).
*
* The signaling processor must wait for any implicit updates in progress
* to terminate before continuing with its update. Thus it must wait for an
* acknowledgement of the interrupt from each processor for which such
* references could be made. For maintaining this information, a set
* cpus_active is used. A cpu is in this set if and only if it can
* use a pmap. When pmap_update_interrupt() is entered, a cpu is removed from
* this set; when all such cpus are removed, it is safe to update.
*
* Before attempting to acquire the update lock on a pmap, a cpu (A) must
* be at least at the priority of the interprocessor interrupt
* (splip<=splvm). Otherwise, A could grab a lock and be interrupted by a
* kernel update; it would spin forever in pmap_update_interrupt() trying
* to acquire the user pmap lock it had already acquired. Furthermore A
* must remove itself from cpus_active.  Otherwise, another cpu holding
* the lock (B) could be in the process of sending an update signal to A,
* and thus be waiting for A to remove itself from cpus_active. If A is
* spinning on the lock at priority this will never happen and a deadlock
* will result.
*/

/*
 *	Signal another CPU that it must flush its TLB
 */
void    signal_cpus(
	cpu_set		use_list,
	pmap_t		pmap,
	vm_offset_t	start, 
	vm_offset_t	end)
{
	int			which_cpu, j;
	pmap_update_list_t	update_list_p;

	while ((which_cpu = __builtin_ffs(use_list)) != 0) {
	    which_cpu -= 1;	/* convert to 0 origin */

	    update_list_p = &cpu_update_list[which_cpu];
	    simple_lock(&update_list_p->lock);

	    j = update_list_p->count;
	    if (j >= UPDATE_LIST_SIZE) {
		/*
		 *	list overflowed.  Change last item to
		 *	indicate overflow.
		 */
		update_list_p->item[UPDATE_LIST_SIZE-1].pmap  = kernel_pmap;
		update_list_p->item[UPDATE_LIST_SIZE-1].start = VM_MIN_USER_ADDRESS;
		update_list_p->item[UPDATE_LIST_SIZE-1].end   = VM_MAX_KERNEL_ADDRESS;
	    }
	    else {
		update_list_p->item[j].pmap  = pmap;
		update_list_p->item[j].start = start;
		update_list_p->item[j].end   = end;
		update_list_p->count = j+1;
	    }
	    cpu_update_needed[which_cpu] = TRUE;
	    simple_unlock(&update_list_p->lock);

	    __sync_synchronize();
	    if (((cpus_idle & (1 << which_cpu)) == 0))
		interrupt_processor(which_cpu);
	    use_list &= ~(1 << which_cpu);
	}
}

/*
 *	This is called at splvm
 */
void process_pmap_updates(pmap_t my_pmap)
{
	int			my_cpu = cpu_number();
	pmap_update_list_t	update_list_p;
	int			j;
	pmap_t			pmap;

	update_list_p = &cpu_update_list[my_cpu];
	assert_splvm();
	simple_lock_nocheck(&update_list_p->lock);

	for (j = 0; j < update_list_p->count; j++) {
	    pmap = update_list_p->item[j].pmap;
	    if (pmap == my_pmap ||
		pmap == kernel_pmap) {

		INVALIDATE_TLB(pmap,
				update_list_p->item[j].start,
				update_list_p->item[j].end);
	    }
	}
	update_list_p->count = 0;
	cpu_update_needed[my_cpu] = FALSE;
	simple_unlock_nocheck(&update_list_p->lock);
}

/*
 *	Interrupt routine for TBIA requested from other processor.
 */
void pmap_update_interrupt(void)
{
	int		my_cpu;
	pmap_t		my_pmap;
	int		s;

	my_cpu = cpu_number();

	/*
	 *	Exit now if we're idle.  We'll pick up the update request
	 *	when we go active, and we must not put ourselves back in
	 *	the active set because we'll never process the interrupt
	 *	while we're idle (thus hanging the system).
	 */
	if (cpus_idle & (1 << my_cpu))
	    return;

	if (current_thread() == THREAD_NULL)
	    my_pmap = kernel_pmap;
	else {
	    my_pmap = current_pmap();
	    if (!pmap_in_use(my_pmap, my_cpu))
		my_pmap = kernel_pmap;
	}

	/*
	 *	Raise spl to splvm (above splip) to block out pmap_extract
	 *	from IO code (which would put this cpu back in the active
	 *	set).
	 */
	s = splvm();

	do {

	    /*
	     *	Indicate that we're not using either user or kernel
	     *	pmap.
	     */
	    i_bit_clear(my_cpu, &cpus_active);

	    /*
	     *	Wait for any pmap updates in progress, on either user
	     *	or kernel pmap.
	     */
	    while (my_pmap->lock.lock_data ||
		   kernel_pmap->lock.lock_data)
		cpu_pause();

	    process_pmap_updates(my_pmap);

	    i_bit_set(my_cpu, &cpus_active);

	} while (cpu_update_needed[my_cpu]);

	splx(s);
}
#else	/* NCPUS > 1 */
/*
 *	Dummy routine to satisfy external reference.
 */
void pmap_update_interrupt(void)
{
	/* should never be called. */
}
#endif	/* NCPUS > 1 */

/* Unmap page 0 to trap NULL references.  */
void
pmap_unmap_page_zero (void)
{
	panic("TODO: not implemented");
}

void
pmap_make_temporary_mapping(void)
{
	panic("TODO: not implemented");
}

void
pmap_set_page_dir(void)
{
	panic("TODO: not implemented");
}

void
pmap_remove_temporary_mapping(void)
{
	panic("TODO: not implemented");
}
