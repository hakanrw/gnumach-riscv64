/*
 * Copyright (C) 2007 Free Software Foundation, Inc.
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
 *
 * Author: Barry deFreese.
 */

#ifndef _RISCV64_DB_INTERFACE_H_
#define _RISCV64_DB_INTERFACE_H_

#include <sys/types.h>
#include <kern/task.h>
#include <machine/thread.h>
#include <ddb/db_watch.h>
#include <ddb/db_variables.h>

extern boolean_t kdb_trap (
	int 			type,
	int			code,
	struct riscv64_saved_state *regs);

struct int_regs;

extern void kdb_kentry(struct int_regs	*int_regs);

extern boolean_t db_read_bytes (
	vm_offset_t	addr,
	int		size,
	char		*data,
	task_t		task);

extern void db_write_bytes (
	vm_offset_t	addr,
	int		size,
	char		*data,
	task_t		task);

extern boolean_t db_check_access (
	vm_offset_t	addr,
	int		size,
	task_t		task);

extern boolean_t db_phys_eq (
	task_t		task1,
	vm_offset_t	addr1,
	task_t		task2,
	vm_offset_t	addr2);

extern int db_user_to_kernel_address(
	task_t		task,
	vm_offset_t	addr,
	vm_offset_t	*kaddr,
	int		flag);

extern void db_task_name (task_t task);

extern void cpu_interrupt_to_db(int i);

#define RISCV64_DB_TYPE_X 0
#define RISCV64_DB_TYPE_W 1
#define RISCV64_DB_TYPE_RW 3

#define RISCV64_DB_LEN_1 0
#define RISCV64_DB_LEN_2 1
#define RISCV64_DB_LEN_4 3
#define RISCV64_DB_LEN_8 2

#define RISCV64_DB_LOCAL 1
#define RISCV64_DB_GLOBAL 2

#if MACH_KDB
extern boolean_t db_set_hw_watchpoint(
	db_watchpoint_t	watch,
	unsigned	num);

extern boolean_t db_clear_hw_watchpoint(
	unsigned	num);

extern void db_dr (
	int		num,
	vm_offset_t	linear_addr,
	int		type,
	int		len,
	int		persistence);

extern void
db_stack_trace_cmd(
	db_expr_t	addr,
	boolean_t	have_addr,
	db_expr_t	count,
	const char	*modif);

extern void
db_halt_cpu(void);
extern void
db_reset_cpu(void);

void
db_riscv64_reg_value(
	struct db_variable	*vp,
	db_expr_t		*valuep,
	int			flag,
	struct db_var_aux_param	*ap);

void feep(void);

/*
 * Put a debugging character on the screen.
 * LOC=0 means put it in the bottom right corner, LOC=1 means put it
 * one column to the left, etc.
 */
void kd_debug_put(int loc, char c);

#endif

extern void db_get_debug_state(
	pcb_t pcb,
	struct riscv64_debug_state *state);
extern kern_return_t db_set_debug_state(
	pcb_t pcb,
	const struct riscv64_debug_state *state);

extern void db_load_context(pcb_t pcb);

extern void cnpollc(boolean_t on);

void
db_write_bytes_user_space(
	vm_offset_t	addr,
	int		size,
	char		*data,
	task_t		task);

void db_debug_all_traps (boolean_t enable);

#endif /* _RISCV64_DB_INTERFACE_H_ */
