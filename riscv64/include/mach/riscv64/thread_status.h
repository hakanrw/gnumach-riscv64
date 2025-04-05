/*
 * Copyright (c) 2023-2024 Free Software Foundation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
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

#ifndef	_MACH_RISCV64_THREAD_STATUS_H_
#define _MACH_RISCV64_THREAD_STATUS_H_

#define RISCV64_THREAD_STATE	1
#define RISCV64_FLOAT_STATE	2

/* TODO: fix */
struct riscv64_thread_state {
	uint64_t x[32];
	uint64_t sp;
	uint64_t pc;
	uint64_t tp;
	uint64_t sstatus;
};
#define RISCV64_THREAD_STATE_COUNT	(sizeof(struct riscv64_thread_state) / sizeof(unsigned int))

/* TODO: check */
struct riscv64_float_state {
	uint64_t f[32];
	uint64_t fflags;
	uint64_t frm;
	uint64_t fcsr;
};
#define RISCV64_FLOAT_STATE_COUNT	(sizeof(struct riscv64_float_state) / sizeof(unsigned int))

struct riscv64_debug_state {
	/* TODO: implement */
};
#define RISCV64_DEBUG_STATE_COUNT \
            (sizeof(struct riscv64_debug_state)/sizeof(unsigned int))


#endif	/* _MACH_RISCV64_THREAD_STATUS_H_ */
