/*
 * Mach Operating System
 * Copyright (c) 1991,1990,1989, 1988 Carnegie Mellon University
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
 *	File:	model_dep.c
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Copyright (C) 1986, Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Basic initialization for I386 - ISA bus machines.
 */

#include <inttypes.h>
#include <string.h>

#include <device/cons.h>
#include <device/dtb.h>

#include <mach/vm_param.h>
#include <mach/vm_prot.h>
#include <mach/machine.h>
#include <mach/machine/multiboot.h>
#include <mach/boolean.h>
#include <mach/xen.h>

#include <kern/assert.h>
#include <kern/cpu_number.h>
#include <kern/debug.h>
#include <kern/mach_clock.h>
#include <kern/macros.h>
#include <kern/printf.h>
#include <kern/startup.h>
#include <kern/smp.h>
#include <sys/types.h>
#include <vm/vm_page.h>

#include <riscv64/model_dep.h>
#include <riscv64/db_interface.h>
#include <riscv64/uart.h>

#ifdef	MACH_XEN
#include <xen/console.h>
#include <xen/store.h>
#include <xen/evt.h>
#include <xen/xen.h>
#endif	/* MACH_XEN */

#if	ENABLE_IMMEDIATE_CONSOLE
#include "immc.h"
#endif	/* ENABLE_IMMEDIATE_CONSOLE */

/* Location of the kernel's symbol table.
   Both of these are 0 if none is available.  */
#if MACH_KDB
#include <ddb/db_sym.h>
#include <riscv64/db_interface.h>

/* ELF section header */
static unsigned elf_shdr_num;
static vm_size_t elf_shdr_size;
static vm_offset_t elf_shdr_addr;
static unsigned elf_shdr_shndx;

#endif /* MACH_KDB */

#define RESERVED_BIOS 0x10000

/* A copy of the multiboot info structure passed by the boot loader.  */
#ifdef MACH_XEN
struct start_info boot_info;
#ifdef MACH_PSEUDO_PHYS
unsigned long *mfn_list;
#if VM_MIN_KERNEL_ADDRESS != LINEAR_MIN_KERNEL_ADDRESS
unsigned long *pfn_list = (void*) PFN_LIST;
#endif
#endif	/* MACH_PSEUDO_PHYS */
#if VM_MIN_KERNEL_ADDRESS != LINEAR_MIN_KERNEL_ADDRESS
unsigned long la_shift = VM_MIN_KERNEL_ADDRESS;
#endif
#else	/* MACH_XEN */
struct multiboot_raw_info boot_info;
#endif	/* MACH_XEN */

/* Command line supplied to kernel.  */
char *kernel_cmdline = "";

extern char	version[];

/* Realmode relocated jmp */
extern uint32_t apboot_jmp_offset;

/* If set, reboot the system on ctrl-alt-delete.  */
boolean_t	rebootflag = FALSE;	/* exported to kdintr */

#ifdef LINUX_DEV
extern void linux_init(void);
#endif

/*
 * Find devices.  The system is alive.
 */
void machine_init(void)
{
	/* TODO: implement */
}

/* Conserve power on processor CPU.  */
void machine_idle (int cpu)
{
	/* TODO: implement */
}

void machine_relax (void)
{
	/* TODO: implement */
}

/*
 * Halt a cpu.
 */
void halt_cpu(void)
{
	/* TODO: implement */
}

/*
 * Halt the system or reboot.
 */
void halt_all_cpus(boolean_t reboot)
{
	/* TODO: implement */
}

void db_halt_cpu(void)
{
	halt_all_cpus(0);
}

void db_reset_cpu(void)
{
	halt_all_cpus(1);
}

#ifndef	MACH_HYP

static void
register_boot_data(const struct multiboot_raw_info *mbi)
{
	/* TODO: implement */
}

#endif /* MACH_HYP */

static void
padprint(unsigned int depth)
{
	for (unsigned int i = 0; i < depth; i++)
		printf("\t");
}

static boolean_t
maybeascii(dtb_prop_t prop)
{
	if (prop->length < 1)
		return FALSE;

	const char *data = (const char *)prop->data;

	for (vm_size_t i = 0; i < prop->length - 1; i++)
		if (data[i] < ' ' || data[i] > '~')
			return FALSE;

	return data[prop->length - 1] == '\0';
}

static void
early_dtb_print_node(dtb_node_t node,
		     unsigned int depth)
{
	struct dtb_prop prop;
	struct dtb_node child;

	padprint(depth); printf("%s {\n", node->name);

	dtb_for_each_prop (*node, prop) {

		padprint(depth + 1); printf("%s = ", prop.name);
		if (maybeascii(&prop)) {
			printf("\"%s\";\n", (const char*)prop.data);
		}
		else {
			const char *data = (const char *)prop.data;
			printf("<");
			for (vm_size_t i = 0; i < prop.length; i++)
				printf("%02x ", data[i]);
			printf(">;");
			printf("\n");
		}
	}

	dtb_for_each_child (*node, child) {
		early_dtb_print_node(&child, depth + 1);
	}

	padprint(depth); printf("};\n");
}

static void
early_dtb_walk_visit_node(dtb_node_t node,
			  dtb_ranges_map_t map)
{
	struct dtb_node child;
	struct dtb_ranges_map nmap;
	boolean_t have_nmap = FALSE;

	early_dtb_print_node(node, 0);
}

static void
early_dtb_walk(void)
{
	struct dtb_node node;
	struct dtb_prop prop;

	node = dtb_root_node();

	/*
	 *	Look at top-level nodes and their props.
	 */
	dtb_for_each_child (node, node) {
		if (!strcmp(node.name, "chosen") || !strncmp(node.name, "chosen@", 7)) {
			prop = dtb_node_find_prop(&node, "bootargs");
			if (!DTB_IS_SENTINEL(prop))
				kernel_cmdline = (const char *) prop.data;
			continue;
		}
		dtb_for_each_prop(node, prop) {
			if (!strcmp(prop.name, "device_type")
			    && !strcmp(prop.data, "memory"))
			{} /* TODO: discover physical memory */
		}
		early_dtb_walk_visit_node(&node, NULL);
	}
}

/*
 *	C boot entrypoint - called by _start in boothdr.S.
 *	Running in physical address space, without paging.
 */
unsigned long boot_hart_id;

void
c_boot_entry(unsigned long hart_id, dtb_t dtb)
{
	kern_return_t kr;

	boot_hart_id = hart_id;

	kr = dtb_load(dtb);
	assert(kr == KERN_SUCCESS);

	uart_init();
	romputc = uart_early_putc;
	printf("%s\n", version);

	early_dtb_walk();
}

#include <mach/vm_prot.h>
#include <vm/pmap.h>
#include <mach/time_value.h>

vm_offset_t
timemmap(dev_t dev, vm_offset_t off, vm_prot_t prot)
{
	extern time_value_t *mtime;

	if (prot & VM_PROT_WRITE) return (-1);
	/* TODO: implement */
}

void
startrtclock(void)
{
	/* TODO: implement */
}

void
inittodr(void)
{
	/* TODO: implement */
}

void
resettodr(void)
{
	/* TODO: implement */
}

boolean_t
init_alloc_aligned(vm_size_t size, vm_offset_t *addrp)
{
	/* TODO: implement */
	return FALSE;
}

/* Grab a physical page:
   the standard memory allocation mechanism
   during system initialization.  */
vm_offset_t
pmap_grab_page(void)
{
	vm_offset_t addr;
	if (!init_alloc_aligned(PAGE_SIZE, &addr))
		panic("Not enough memory to initialize Mach");
	return addr;
}
