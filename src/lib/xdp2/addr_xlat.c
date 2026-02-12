// SPDX-License-Identifier: BSD-2-Clause-FreeBSD
/*
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

/* Address translation. Translate a relative address to a fully qualified
 * address form some base address. This is used, for instance, to handle
 * pointers in shared memory
 */

#include "xdp2/addr_xlat.h"

struct xdp2_addr_xlat xdp2_addr_xlat_translators[
					XDP2_ADDR_XLAT_MAX_TRANSLATORS];

/* Set the base address for an address translator. num indicates which
 * translator, i.e. the index in the translators array
 */
void xdp2_addr_xlat_set_base(unsigned int num, void *base)
{
	XDP2_ASSERT(num < XDP2_ADDR_XLAT_MAX_TRANSLATORS,
		     "Invalid address translator number in set base");

	xdp2_addr_xlat_translators[num].base = base;
}

/* Set the function to get the base address for an address translator. num
 * indicates which translator, i.e. the index in the translators array
 */
void xdp2_addr_xlat_set_base_func(unsigned int num, void *(*base_func)(void))
{
	XDP2_ASSERT(num < XDP2_ADDR_XLAT_MAX_TRANSLATORS,
		     "Invalid address translator number in set base function");

	xdp2_addr_xlat_translators[num].get_base = base_func;
}
