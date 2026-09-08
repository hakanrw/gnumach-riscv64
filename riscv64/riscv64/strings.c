/*
 * Copyright (c) 2014 Richard Braun.
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

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define ARCH_STRING_MEMCPY
#define ARCH_STRING_MEMMOVE
#define ARCH_STRING_MEMSET
#define ARCH_STRING_MEMCMP

#ifdef ARCH_STRING_MEMCPY
void *
memcpy(void *dest, const void *src, size_t n)
{
	unsigned char *d = dest;
	const unsigned char *s = src;
	void *orig_dest = dest;

	while (n-- != 0)
		*d++ = *s++;

	return orig_dest;
}
#endif /* ARCH_STRING_MEMCPY */

#ifdef ARCH_STRING_MEMMOVE
void *
memmove(void *dest, const void *src, size_t n)
{
	unsigned char *d = dest;
	const unsigned char *s = src;
	void *orig_dest = dest;

	if ((uintptr_t) d < (uintptr_t) s) {
		while (n-- != 0)
			*d++ = *s++;
	} else if ((uintptr_t) d > (uintptr_t) s) {
		d += n;
		s += n;
		while (n-- != 0)
			*--d = *--s;
	}

	return orig_dest;
}
#endif /* ARCH_STRING_MEMMOVE */

#ifdef ARCH_STRING_MEMSET
void *
memset(void *s, int c, size_t n)
{
	unsigned char *p = s;
	void *orig_s = s;

	while (n-- != 0)
		*p++ = (unsigned char) c;

	return orig_s;
}
#endif /* ARCH_STRING_MEMSET */

#ifdef ARCH_STRING_MEMCMP
int
memcmp(const void *s1, const void *s2, size_t n)
{
	const unsigned char *p1 = s1;
	const unsigned char *p2 = s2;

	while (n-- != 0) {
		if (*p1 != *p2)
			return (int) *p1 - (int) *p2;
		p1++;
		p2++;
	}

	return 0;
}
#endif /* ARCH_STRING_MEMCMP */
