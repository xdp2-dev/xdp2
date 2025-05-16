/* SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2020 Tom Herbert
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
#ifndef __XDP2_BSWAP_H__
#define __XDP2_BSWAP_H__

#include <byteswap.h>

#include "xdp2/utility.h"

#define __XDP2_MAKE_ENDIAN_SWAP(NUM)					\
	static inline XDP2_JOIN2(__u, NUM) XDP2_JOIN2(			\
			xdp2_bswap, NUM)(XDP2_JOIN2(__u, NUM) val) {	\
		return XDP2_JOIN2(bswap_, NUM)(val);			\
	}

__XDP2_MAKE_ENDIAN_SWAP(16)
__XDP2_MAKE_ENDIAN_SWAP(32)
__XDP2_MAKE_ENDIAN_SWAP(64)

#define __XDP_SWAP_ENDIAN(NUM, NAME)					\
	XDP2_JOIN2(__u, NUM) XDP2_JOIN3(xdp2_, NAME, NUM)(		\
				XDP2_JOIN2(__u, NUM) val) {		\
		return XDP2_JOIN(bswap_, NUM)(val);			\
	}

#define __XDP_NULL_ENDIAN(NUM, NAME)					\
	static inline XDP2_JOIN2(__u, NUM) XDP2_JOIN3(xdp2_,		\
						      NAME, NUM)(	\
				XDP2_JOIN2(__u, NUM) val) {		\
		return val;						\
	}

#if defined(__BIG_ENDIAN)
	__XDP_NULL_ENDIAN(16, ntoh)
	__XDP_NULL_ENDIAN(32, ntoh)
	__XDP_NULL_ENDIAN(64, ntoh)
	__XDP_NULL_ENDIAN(16, hton)
	__XDP_NULL_ENDIAN(32, hton)
	__XDP_NULL_ENDIAN(64, hton)
#elif defined(__LITTLE_ENDIAN)
	__XDP_SWAP_ENDIAN(16, ntoh)
	__XDP_SWAP_ENDIAN(32, ntoh)
	__XDP_SWAP_ENDIAN(64, ntoh)
	__XDP_SWAP_ENDIAN(16, hton)
	__XDP_SWAP_ENDIAN(32, hton)
	__XDP_SWAP_ENDIAN(64, hton)
#else
	"Cannot determine endianness"
#endif

#endif /* __XDP2_BWSAP_H__ */
