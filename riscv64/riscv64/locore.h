/*
 * Copyright (C) 2006, 2011 Free Software Foundation.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _MACHINE_LOCORE_H_
#define _MACHINE_LOCORE_H_

#include <sys/types.h>

#include <kern/sched_prim.h>

/*
 * Fault recovery in copyin/copyout routines.
 */
struct recovery {
	vm_offset_t	fault_addr;
	vm_offset_t	recover_addr;
};

extern struct recovery recover_table[];
extern struct recovery recover_table_end[];

/*
 * Recovery from Successful fault in copyout does not
 * return directly - it retries the pte check, since
 * the 386 ignores write protection in kernel mode.
 */
extern struct recovery retry_table[];
extern struct recovery retry_table_end[];


extern int call_continuation (continuation_t continuation);

extern int copyin (const void *userbuf, void *kernelbuf, size_t cn);
extern int copyinmsg (const void *userbuf, void *kernelbuf, size_t cn, size_t kn);
extern int copyout (const void *kernelbuf, void *userbuf, size_t cn);
#ifdef USER32
extern int copyoutmsg (const void *kernelbuf, void *userbuf, size_t cn);
#else
static inline int copyoutmsg (const void *kernelbuf, void *userbuf, size_t cn) {
	return copyout (kernelbuf, userbuf, cn);
}
#endif

extern int inst_fetch (int eip, int cs);

extern void cpu_shutdown (void);

extern int syscall (void);
extern int syscall64 (void);

#endif /* _MACHINE__LOCORE_H_ */

