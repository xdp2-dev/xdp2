/* SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2026 XDPnet Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef __XDP2_ADDR_XLAT_H__
#define __XDP2_ADDR_XLAT_H__

/* Address translation. Translate a relative address to a fully qualified
 * address form some base address. This is used, for instance, to handle
 * pointers in shared memory. If the relative address is NULL this signifies
 * a NULL address and no translation is done (hence, the user of this facility
 * should ensure that all valid possible relative addresses are non-zero)
 */

#include <stdint.h>

#include "xdp2/utility.h"

#define XDP2_ADDR_XLAT_MAX_TRANSLATORS 10

/* Address translation structure. An instance of this defined as a global
 * for the XDP2 library. It is referenced in various address translation
 * functions to retrieve the base memory address for translation. Each
 * translator in the array can translate addresses in a different memory
 * space
 *
 * The base address can be expressed in one of two ways: a base address or a
 * function that returns the base address
 *
 * - The base address can be used if it is the same base address for the
 *   translation across all the process and threads of a process
 * - The function to get the base is used if the base for the translation
 *   differs in the process-- for instance, the process may have multiple
 *   threads that have their own memory space and so each thread would
 *   have it's own concept of a base. In that case the base function can
 *   be used and that function might get thread a thread specific base for
 *   the translation (for instance by pthread_getspecific which could return
 *   a per thread base address)
 */
struct xdp2_addr_xlat {
	void *base;
	void *(*get_base)(void);
};

#define XDP2_ADDR_XLAT_NO_XLAT (-1)

extern struct xdp2_addr_xlat
		xdp2_addr_xlat_translators[XDP2_ADDR_XLAT_MAX_TRANSLATORS];

/* Translate a relative address to an absolute address */
static inline void *xdp2_addr_xlat_translate(int num, void *addr)
{
	void *fbase = NULL;

	if (num < 0 || !addr)
		return addr;

	XDP2_ASSERT(num < XDP2_ADDR_XLAT_MAX_TRANSLATORS,
		     "Invalid address translator number");

	if (xdp2_addr_xlat_translators[num].get_base)
		fbase = xdp2_addr_xlat_translators[num].get_base();

	if (!fbase)
		fbase = xdp2_addr_xlat_translators[num].base;

	return fbase + (uintptr_t)addr;
}

/* Macro's to translate a relative address to an absolute address */
#define __XDP2_ADDR_XLAT(num, addr, TYPE)				\
	((TYPE)xdp2_addr_xlat_translate(num, addr))

#define XDP2_ADDR_XLAT(num, addr)					\
	__XDP2_ADDR_XLAT(num, addr, typeof(addr))

/* Translate an absolute address to its relative address */
static inline void *xdp2_addr_xlat_reverse_translate(int num, void *addr)
{
	uintptr_t fbase = 0;

	if (num < 0 || !addr)
		return addr;

	if (xdp2_addr_xlat_translators[num].get_base)
		fbase = (uintptr_t)
			xdp2_addr_xlat_translators[num].get_base();

	if (!fbase)
		fbase = (uintptr_t)xdp2_addr_xlat_translators[num].base;

	XDP2_ASSERT(num < XDP2_ADDR_XLAT_MAX_TRANSLATORS,
		     "Invalid address translator number");

	return addr - fbase;
}

#define __XDP2_ADDR_REV_XLAT(num, addr, TYPE)				\
	((TYPE)xdp2_addr_xlat_reverse_translate(num, addr))

#define XDP2_ADDR_REV_XLAT(num, addr)					\
	__XDP2_ADDR_REV_XLAT(num, addr, typeof(addr))

/* Set the base address for an address translator. num indicates which
 * translator, i.e. the index in the translators array
 */
void xdp2_addr_xlat_set_base(unsigned int num, void *base);

/* Set the function to get the base address for an address translator. num
 * indicates which translator, i.e. the index in the translators array
 */
void xdp2_addr_xlat_set_base_func(unsigned int num,
				   void *(*base_func)(void));

#endif /* __XDP2_ADDR_XLAT_H__ */
