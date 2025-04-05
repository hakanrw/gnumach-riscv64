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
#include <string.h>

#include <kern/debug.h>

#define ARCH_STRING_MEMCPY
#define ARCH_STRING_MEMMOVE
#define ARCH_STRING_MEMSET
#define ARCH_STRING_MEMCMP

#ifdef ARCH_STRING_MEMCPY
void *
memcpy(void *dest, const void *src, size_t n)
{
    void *orig_dest;

    panic("TODO: not implemented");

    return orig_dest;
}
#endif /* ARCH_STRING_MEMCPY */

#ifdef ARCH_STRING_MEMMOVE
void *
memmove(void *dest, const void *src, size_t n)
{
    void *orig_dest;

    orig_dest = dest;

    panic("TODO: not implemented");

    return orig_dest;
}
#endif /* ARCH_STRING_MEMMOVE */

#ifdef ARCH_STRING_MEMSET
void *
memset(void *s, int c, size_t n)
{
    void *orig_s;

    panic("TODO: not implemented");

    return orig_s;
}
#endif /* ARCH_STRING_MEMSET */

#ifdef ARCH_STRING_MEMCMP
int
memcmp(const void *s1, const void *s2, size_t n)
{
    unsigned char c1, c2;

    if (n == 0)
        return 0;

    panic("TODO: not implemented");

    return (int)c1 - (int)c2;
}
#endif /* ARCH_STRING_MEMCMP */
