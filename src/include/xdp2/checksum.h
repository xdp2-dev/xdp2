/* SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2020,2021 Tom Herbert
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

#ifndef __XDP2_CHECKSUM_H__
#define __XDP2_CHECKSUM_H__

/* Checksum functions definitions for XDP2 */

#include <linux/types.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <byteswap.h>

#include "xdp2/utility.h"

static inline __u16 __xdp2_checksum_fold32(__u32 val)
{
	val = (val & 0xffffUL) + (val >> 16);
	val += (val >> 16);

	return (val & 0xffff);
}

static inline __u16 xdp2_checksum_fold32(__u32 val)
{
	return __xdp2_checksum_fold32(val);
}

static inline __u16 __xdp2_checksum_fold64(__u64 val)
{
	val = (val & 0xffffffffUL) + (val >> 32);
	val += (val >> 32);

	return __xdp2_checksum_fold32((__u32)val);
}

static inline __u16 xdp2_checksum_fold64(__u64 val)
{
	return __xdp2_checksum_fold64(val);
}

static inline __u64 __xdp2_checksum_add16(__u64 val1, __u16 val2)
{
	__u32 sum;

	sum = (val1 & 0xffff) + val2;
	sum += (sum >> 16);

	return (val1 & ~0xffff) | (sum & 0xffff);
}

static inline __u64 xdp2_checksum_add16(__u64 val1, __u16 val2)
{
	return __xdp2_checksum_add16(val1, val2);
}

static inline __u64 __xdp2_checksum_add32(__u64 val1, __u32 val2)
{
	__u64 sum;

	sum = (val1 & 0xffffffff) + val2;
	sum += (sum >> 32);

	sum = (val1 & ~0xffffffffULL) | (sum & 0xffffffffULL);

	return sum;
}

static inline __u64 xdp2_checksum_add32(__u64 val1, __u32 val2)
{
	return __xdp2_checksum_add32(val1, val2);
}

static inline __u64 __xdp2_checksum_add64(__u64 val1, __u64 val2)
{
	__u64 sum;

	sum = (val1 & 0xffffffffUL) + (val2 & 0xffffffffUL);
	sum += (val1 >> 32) + (val2 >> 32);

	return sum;
}

static inline __u64 xdp2_checksum_add64(__u64 val1, __u64 val2)
{
	return __xdp2_checksum_add64(val1, val2);
}

static inline __u16 __xdp2_checksum_compute(const void *src, size_t len)
{
	__u64 sum = 0;
	int i;

	XDP2_ASSERT(len < (1UL << 32), "Checksum length too big: %lu",
		     len);

	/* Sum thirty-two bit words */
	for (i = 0; i < len / sizeof(__u32); i++, src += sizeof(__u32))
		sum += *(__u32 *)src;

	if (len & 2) { /* Extra two bytes */
		sum += *(__u16 *)src;
		src += sizeof(__u16);
	}
	if (len & 1) /* Odd length */
		sum += *(__u8 *)src;

	/* Return sum folded to sixteen bits*/
	return __xdp2_checksum_fold64(sum);
}

static inline __u16 xdp2_checksum_compute(const void *src, size_t len)
{
	return __xdp2_checksum_compute(src, len);
}

#endif /* __XDP2_CHECKSUM_H__ */
