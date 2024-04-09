/*
 * Copyright (c) 2024 Free Software Foundation.
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

#ifndef _DEVICE_DTB_H_
#define _DEVICE_DTB_H_

#include <mach/boolean.h>
#include <mach/kern_return.h>
#include <mach/machine/vm_types.h>
#include <device/device_types.h>

#define DTB_SENTINEL_OFFSET		((vm_offset_t) -1)
#define DTB_IS_SENTINEL(s)		((s).offset == DTB_SENTINEL_OFFSET)

/*
 *	Big endian 4-byte integer.
 */
typedef struct {
	unsigned char	bytes[4];
} dtb_uint32_t;

struct dtb_header {
	dtb_uint32_t	magic;
	dtb_uint32_t	total_size;
	dtb_uint32_t	offset_dt_struct;
	dtb_uint32_t	offset_dt_strings;
	dtb_uint32_t	offset_mem_rsvmap;
	dtb_uint32_t	version;
	dtb_uint32_t	last_compatible_version;
	dtb_uint32_t	boot_cpuid_phys;
	dtb_uint32_t	sizeof_dt_strings;
	dtb_uint32_t	sizeof_dt_struct;
};

typedef const struct dtb_header *dtb_t;

extern kern_return_t dtb_load(dtb_t dtb);
extern void dtb_get_location(dtb_t *out_dtb, vm_size_t *out_dtb_size);

struct dtb_node {
	vm_offset_t	offset;
	const char	*name;
	unsigned short	address_cells;
	unsigned short	size_cells;
};

typedef const struct dtb_node *dtb_node_t;

struct dtb_prop {
	vm_offset_t	offset;
	const char	*name;
	const void	*data;
	vm_size_t	length;
};

typedef const struct dtb_prop *dtb_prop_t;

struct dtb_ranges_map {
	unsigned short	child_address_cells;
	unsigned short	parent_address_cells;
	unsigned short	child_size_cells;
	const void	*ranges;
	vm_size_t	ranges_length;

	const struct dtb_ranges_map *next;
};

typedef const struct dtb_ranges_map *dtb_ranges_map_t;

extern struct dtb_node dtb_root_node(void);
extern struct dtb_prop dtb_node_first_prop(dtb_node_t node);
extern struct dtb_prop dtb_node_next_prop(dtb_prop_t prop);
extern struct dtb_node dtb_node_first_child(dtb_node_t node);
extern struct dtb_node dtb_node_next_sibling(dtb_node_t node);

#define dtb_for_each_child(parent, child)				\
	for (child = dtb_node_first_child(&(parent));			\
	     !DTB_IS_SENTINEL(child);					\
	     child = dtb_node_next_sibling(&child))

#define dtb_for_each_prop(node, prop)					\
	for (prop = dtb_node_first_prop(&(node));			\
	     !DTB_IS_SENTINEL(prop);					\
	     prop = dtb_node_next_prop(&prop))

extern struct dtb_node dtb_node_by_path(const char *node_path);

extern struct dtb_prop dtb_node_find_prop(
	dtb_node_t	node,
	const char 	*prop_name);

extern boolean_t dtb_node_is_compatible(
	dtb_node_t	node,
	const char	*model);

extern uint64_t dtb_prop_read_cells(
	dtb_prop_t	prop,
	unsigned short	size,
	vm_size_t	*off);

extern struct dtb_ranges_map dtb_node_make_ranges_map(
	dtb_node_t	node);

extern vm_offset_t dtb_map_address(
	dtb_ranges_map_t	map,
	vm_offset_t		address);

#endif /* _DEVICE_DTB_H_ */
