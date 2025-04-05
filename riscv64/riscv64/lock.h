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
/*
 * Machine-dependent simple locks for the RISC-V.
 */
#ifndef	_RISCV64_LOCK_H_
#define	_RISCV64_LOCK_H_

#define	SIMPLE_LOCK_INITIALIZER(l) \
	{.lock_data = 0}

#if NCPUS > 1
#include <riscv64/smp.h>

/*
 *	All of the locking routines are built from calls on
 *	a locked-exchange operation.  Values of the lock are
 *	0 for unlocked, 1 for locked.
 */

#ifdef	__GNUC__

/*
 *	The code here depends on the GNU C compiler.
 */

#define	_simple_lock_xchg_(lock, new_val) \
    ({  natural_t _old_val_; \
        asm volatile( \
            "   amoswap.w.aq t0, %0, (%2)  \n" \
            : "=r" (_old_val) \
            : "0" ((natural_t)(new_val)), "r" (lock) \
            : "t0", "memory" \
        ); \
        old_val; \
    })

#define	simple_lock_init(l) \
	((l)->lock_data = 0)

#define	_simple_lock(l) \
    ({ \
	while(_simple_lock_xchg_(l, 1)) \
	    while (*(volatile natural_t *)&(l)->lock_data) \
		cpu_pause(); \
	0; \
    })

#define	_simple_unlock(l) \
	(_simple_lock_xchg_(l, 0))

#define	_simple_lock_try(l) \
	(!_simple_lock_xchg_(l, 1))

/* TODO check the accuracy of the below assemblies */
/* including clobbered values */

/*
 *	General bit-lock routines.
 */
#define bit_lock(bit, l) \
    ({ \
        int bit_val = (int)(bit); \
        long mask, old, new; \
        asm volatile( \
            "   li      %0, 1           \n" \
            "   sll     %0, %0, %3      \n" /* Create bit mask (1 << bit) */ \
            "1: \n" \
            "2: lw      %1, (%2)        \n" /* Load current value (not atomic, just for test) */ \
            "   and     %1, %1, %0      \n" /* Test if bit is set */ \
            "   bnez    %1, 2b          \n" /* If bit is set, keep spinning */ \
            "   lr.w    %1, (%2)        \n" /* Start atomic sequence: load and reserve */ \
            "   and     %4, %1, %0      \n" /* Test if bit is set */ \
            "   bnez    %4, 2b          \n" /* If bit is set, go back to spin loop */ \
            "   or      %4, %1, %0      \n" /* Set the bit */ \
            "   sc.w.aq %4, %4, (%2)    \n" /* Try to store new value with bit set */ \
            "   bnez    %4, 1b          \n" /* If store failed, retry atomic sequence */ \
            : "=&r" (mask), "=&r" (old), "+r" (l), "+r" (bit_val), "=&r" (new) \
            : \
            : "memory"); \
        0; \
    })

#define bit_unlock(bit, l) \
    ({ \
        int bit_val = (int)(bit); \
        long mask, old, new; \
        asm volatile( \
            "   li      %0, 1           \n" \
            "   sll     %0, %0, %3      \n" /* Create bit mask (1 << bit) */ \
            "   not     %0, %0          \n" /* Invert mask to clear bit */ \
            "1: lr.w    %1, (%2)        \n" /* Load and reserve */ \
            "   and     %4, %1, %0      \n" /* Clear the bit */ \
            "   sc.w.rl %4, %4, (%2)    \n" /* Store conditionally with release ordering */ \
            "   bnez    %4, 1b          \n" /* If store failed, retry */ \
            : "=&r" (mask), "=&r" (old), "+r" (l), "+r" (bit_val), "=&r" (new) \
            : \
            : "memory"); \
        0; \
    })

/*
 *	Set or clear individual bits in a long word.
 *	The locked access is needed only to lock access
 *	to the word, not to individual bits.
 */
#define i_bit_set(bit, l) \
    ({ \
        long old, neu; \
        int bit_val = (int)(bit); \
        asm volatile( \
            "1: lr.d %0, (%2)        \n" /* Load value atomically */ \
            "   ori %1, %0, (1<<%3)   \n" /* Set the bit */ \
            "   sc.d.aq %1, %1, (%2)  \n" /* Store conditionally */ \
            "   bnez %1, 1b          \n" /* Retry if failed */ \
            : "=&r" (old), "=&r" (neu) \
            : "r" (l), "r" (bit_val) \
            : "memory"); \
        0; \
    })

#define i_bit_clear(bit, l) \
    ({ \
        long old, neu; \
        int bit_val = (int)(bit); \
        asm volatile( \
            "1: lr.d %0, (%2)        \n" /* Load value atomically */ \
            "   li %1, 1             \n" \
            "   sll %1, %1, %3       \n" /* Create mask with only the bit position set */ \
            "   not %1, %1           \n" /* Invert to clear that bit position */ \
            "   and %1, %0, %1       \n" /* Clear the bit */ \
            "   sc.d.aq %1, %1, (%2)  \n" /* Store conditionally */ \
            "   bnez %1, 1b          \n" /* Retry if failed */ \
            : "=&r" (old), "=&r" (neu) \
            : "r" (l), "r" (bit_val) \
            : "memory"); \
        0; \
    })

#endif	/* __GNUC__ */

extern void simple_lock_pause(void);

#endif /* NCPUS > 1 */



#endif	/* _RISCV64_LOCK_H_ */
