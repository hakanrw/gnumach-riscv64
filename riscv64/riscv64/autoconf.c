/*
 * Mach Operating System
 * Copyright (c) 1993,1992,1991,1990,1989 Carnegie Mellon University
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

#include <kern/printf.h>
#include <mach/std_types.h>
#include <chips/busses.h>

#include <riscv64/autoconf.h>

#if NNS16550 > 0
extern struct bus_driver ns16550_driver;
#include <device/ns16550.h>

/*
 * NS16550 base addresses for known RISC-V platforms.
 * QEMU virt machine: 0x10000000
 * Milk-V Mars (JH7110): 0x10000000 (UART0)
 * Allwinner D1 (C906): 0x02500000 (UART0)
 */
#define NS16550_BASE_QEMU_VIRT	0x10000000UL
#define NS16550_BASE_ALLWINNER_D1	0x02500000UL

/* Default to QEMU virt for development */
#ifndef NS16550_BASE
#define NS16550_BASE	NS16550_BASE_QEMU_VIRT
#endif

static const struct ns16550_config nsuart0_config = {
 /* reg_shift    reg_io_width  clock_freq   baud_rate  irq */
            2,              4,          0,          0,  32
};
#endif /* NNS16550 */

struct	bus_ctlr	bus_master_init[] = {

/* driver    name unit intr    address        len phys_address
     adaptor alive flags spl    pic				 */

  {0}
};


struct	bus_device	bus_device_init[] = {

  /* driver     name unit intr    address       am   phys_address
     adaptor alive ctlr slave flags *mi       *next  sysdep sysdep */

#if NNS16550 > 0
  {&ns16550_driver, "nsuart", 0, ns16550_intr, NS16550_BASE, 0, NS16550_BASE,
     '?',    0,   -1,    -1,    0,   0,        0,         (vm_offset_t)&nsuart0_config, 0},
#endif /* NNS16550 */
  {0}
};
