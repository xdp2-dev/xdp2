/* SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2025 Tom Herbert
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
#ifndef __XDP2_PVPKT_H__
#define __XDP2_PVPKT_H__

#include "xdp2/pvbuf.h"

/* pvbuf segment via iterate */

ssize_t ___xdp2_pvpkt_segment(xdp2_paddr_t paddr,
			      struct __xdp2_pvbuf_iter_clone *ist);

/* Iterator struct for pvbuf segment operation */

struct __xdp2_pvbuf_iter_segment {
	/* Must be first for casting */
	struct __xdp2_pvbuf_iter_clone clone_iter;

	size_t segment_length;
	unsigned int pvbuf_list_size;
	xdp2_paddr_t packet_pvbuf_paddr;
	size_t packet_need_length;
	void *priv;
	ssize_t (*cb)(xdp2_paddr_t pvbuf_paddr, void *priv);
	bool compress;
	bool pvbufs_only;
};

/* Function to segment a pvbuf. Returns a packet vector pvbuf where
 * each iovec pvbuf has the segment size expect for the last one
 */
static inline xdp2_paddr_t __xdp2_pvpkt_segment(
		struct xdp2_pvbuf_mgr *pvmgr, xdp2_paddr_t paddr,
		size_t offset, size_t length, size_t segment_size,
		size_t *retlen)
{
	unsigned int pvbuf_size = __xdp2_pvbuf_get_size(pvmgr, 0);
	struct __xdp2_pvbuf_iter_segment ist = {};
	struct xdp2_pvbuf *pvbuf;
	xdp2_paddr_t pvbuf_paddr;
	ssize_t ret;

	/* Allocate initial pvbuf */
	pvbuf_paddr = ___xdp2_pvbuf_alloc(pvmgr, pvbuf_size, &pvbuf,
					   &pvbuf_size);
	if (!pvbuf_paddr)
		return XDP2_PADDR_NULL;

	ist.clone_iter.pvmgr = pvmgr;
	ist.clone_iter.length = length;
	ist.clone_iter.offset = offset;
	ist.clone_iter.num_iovecs = XDP2_PVBUF_NUM_IOVECS(pvbuf_size);
	ist.clone_iter.pvbuf = pvbuf;

	ist.segment_length = segment_size;

	if ((ret = ___xdp2_pvpkt_segment(paddr, &ist.clone_iter)) < 0) {
		__xdp2_pvbuf_free(pvmgr, pvbuf_paddr);
		return XDP2_PADDR_NULL;
	}

	*retlen = ret;

	return pvbuf_paddr;
}

static inline xdp2_paddr_t xdp2_pvpkt_segment(xdp2_paddr_t paddr,
						size_t offset, size_t length,
						size_t segment_size,
						size_t *retlen)
{
	return __xdp2_pvpkt_segment(&xdp2_pvbuf_global_mgr, paddr, offset,
				     length, segment_size, retlen);
}

/* Segment a pvbuf with a callback for each segment. The callback can be
 * used to prepend a pbuf to each segment for instance
 */
static inline xdp2_paddr_t __xdp2_pvpkt_segment_cb(
		struct xdp2_pvbuf_mgr *pvmgr, xdp2_paddr_t paddr,
		size_t offset, size_t length, size_t segment_size,
		ssize_t (*cb)(xdp2_paddr_t, void *priv), void *priv,
		size_t *retlen)
{
	unsigned int pvbuf_size = __xdp2_pvbuf_get_size(pvmgr, 0);
	struct __xdp2_pvbuf_iter_segment ist = {};
	struct xdp2_pvbuf *pvbuf;
	xdp2_paddr_t pvbuf_paddr;
	ssize_t ret;

	/* Allocate initial pvbuf */
	pvbuf_paddr = ___xdp2_pvbuf_alloc(pvmgr, pvbuf_size, &pvbuf,
					   &pvbuf_size);
	if (!pvbuf_paddr)
		return XDP2_PADDR_NULL;

	ist.clone_iter.pvmgr = pvmgr;
	ist.clone_iter.length = length;
	ist.clone_iter.offset = offset;
	ist.clone_iter.num_iovecs = XDP2_PVBUF_NUM_IOVECS(pvbuf_size);
	ist.clone_iter.pvbuf = pvbuf;
	ist.clone_iter.error = 0;

	ist.segment_length = segment_size;
	ist.pvbufs_only = true;
	ist.cb = cb;
	ist.priv = priv;

	if ((ret = ___xdp2_pvpkt_segment(paddr, &ist.clone_iter)) < 0) {
		__xdp2_pvbuf_free(pvmgr, pvbuf_paddr);
		return XDP2_PADDR_NULL;
	}

	*retlen = ret;

	return pvbuf_paddr;
}

static inline xdp2_paddr_t xdp2_pvpkt_segment_cb(
		xdp2_paddr_t paddr, size_t offset, size_t length,
		size_t segment_size, ssize_t (*cb)(xdp2_paddr_t, void *priv),
		void *priv, size_t *retlen)
{
	return __xdp2_pvpkt_segment_cb(&xdp2_pvbuf_global_mgr, paddr, offset,
					length, segment_size, cb, priv, retlen);
}

typedef bool (*xdp2_pvpkt_iterate_t)(void *priv, struct xdp2_iovec *iovec);

/* Iterator over a packet vector, Call the function for each packet in
 * the vector
 */
static inline void __xdp2_pvpkt_iterate(struct xdp2_pvbuf_mgr *pvmgr,
					 xdp2_paddr_t paddr,
					 xdp2_pvpkt_iterate_t func,
					 void *priv)
{
	struct xdp2_pvbuf *pvbuf;
	unsigned int num_iovecs;
	unsigned int i = 0;

	XDP2_ASSERT(paddr, "paddr is null");
	XDP2_ASSERT(XDP2_PADDR_IS_PVBUF(paddr),
		    "Not an iovec: paddr is %p", (void *)paddr);

	do {
		pvbuf = xdp2_pvbuf_paddr_to_addr(paddr);
		num_iovecs =
			XDP2_PVBUF_NUM_IOVECS_FROM_PADDR(paddr);
		i = 0;

		xdp2_pvbuf_iovec_map_foreach(pvbuf, i) {
			struct xdp2_iovec *iovec = &pvbuf->iovec[i];

			if (i == num_iovecs - 1) {
				XDP2_ASSERT(XDP2_IOVEC_IS_PVBUF(iovec),
					    "iovec in pvpkt is not a pvbuf");
				paddr = iovec->paddr;
				pvbuf = xdp2_pvbuf_paddr_to_addr(paddr);
				i = 0;

				break;
			}

			if (!func(priv, iovec))
				return;
		}
	} while (i < num_iovecs);
}

static inline void xdp2_pvpkt_iterate(xdp2_paddr_t paddr,
				       xdp2_pvpkt_iterate_t func,
				       void *priv)
{
	__xdp2_pvpkt_iterate(&xdp2_pvbuf_global_mgr, paddr, func, priv);
}

#endif /* __XDP2_PVBUF_PKT_H__ */
