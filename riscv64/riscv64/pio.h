/*
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University
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

#ifndef _RISCV64_PIO_H_
#define _RISCV64_PIO_H_

#include <kern/debug.h>

#ifndef	__GNUC__
#error	You do not stand a chance.  This file is gcc only.
#endif	/* __GNUC__ */

#define inl(y) \
	panic("TODO: Not implemented");

#define inw(y) \
	panic("TODO: Not implemented");

#define inb(y) \
	panic("TODO: Not implemented");


#define outl(x, y) \
MACRO_BEGIN panic("TODO: not implemented"); MACRO_END


#define outw(x, y) \
MACRO_BEGIN panic("TODO: not implemented"); MACRO_END

#define outb(x, y) \
MACRO_BEGIN panic("TODO: not implemented"); MACRO_END

#endif /* _RISCV64_PIO_H_ */
