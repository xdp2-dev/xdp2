// SPDX-License-Identifier: BSD-2-Clause-FreeBSD
/*
 * Copyright (c) 2020, 2021 Tom Herbert
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

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "xdp2/pvpkt.h"

/* Call the user callback to complete the segmentation. The argument to the
 * callback routine is a pvbuf
 */
static bool __xdp2_pvpkt_iterate_do_cb(struct __xdp2_pvbuf_iter_segment *ist)
{
	struct xdp2_iovec *iovec = &ist->clone_iter.pvbuf->iovec[
						ist->clone_iter.index - 1];
	struct xdp2_paddr pvaddr = { .paddr = iovec->paddr };
	ssize_t ret;

	if (!ist->cb)
		return true;

	/* Do the user callback, argument is one segment in a pvbuf */
	ret = ist->cb(iovec->paddr, ist->priv);
	if (!ret)
		return true;

	if (ret < 0) {
		ist->clone_iter.error = ret;
		return false;
	}

	if (!pvaddr.pvbuf.data_length)
		return true;

	/* The callback may add data to the segment to make it a packet either
	 * by prepending or appending buffers. Adjust the pvbuf data length
	 * accordingly
	 */
	ret += pvaddr.pvbuf.data_length;
	pvaddr.pvbuf.data_length = ret < XDP2_PVBUF_MAX_LEN ? ret : 0;

	/* Set the adjusted paddr back into the iovec */
	iovec->paddr = pvaddr.paddr;

	return true;
}

/* Iterator callback function for segmenting a pvbuf. This is called
 * for each pbuf in the pvbuf being segmented.
 */
static bool __xdp2_pvpkt_iterate_segment_pvbuf(void *priv,
					       struct xdp2_iovec *iovec)
{
	struct __xdp2_pvbuf_iter_segment *ist = priv;
	struct __xdp2_pvbuf_iter_clone *cist = &ist->clone_iter;
	struct xdp2_paddr pbaddr_1, pbaddr_2;
	unsigned int num_slots, pvbuf_size;
	size_t data_len, size, orig_len;
	struct xdp2_paddr pvaddr;
	xdp2_paddr_t pvbuf_paddr;
	struct xdp2_pvbuf *pvbuf;
	bool is_long_addr;
	int ret;

	ret = __xdp2_pvbuf_iterate_iovec_clone_common(cist, iovec, &pbaddr_1,
						      &pbaddr_2, &data_len,
						      &num_slots);

	if (ret)
		return (ret == 1);

	is_long_addr = (xdp2_paddr_get_paddr_tag_from_paddr(pbaddr_1.paddr) ==
			 XDP2_PADDR_TAG_LONG_ADDR);
	orig_len = data_len;

	while (data_len) {
		if (!ist->packet_need_length) {
			/* Starting a new packet */

			/* If we were previously working a packet call the
			 * callback to complete the segmentation. The callback
			 * could prepend a header to each segment for instance
			 */
			if (cist->index &&
			    !__xdp2_pvpkt_iterate_do_cb(ist))
				return false;

			if (cist->index >= cist->num_iovecs - num_slots -
								is_long_addr) {
				/* We're at the end of this pvbuf so extend it
				 * by setting the last iovec to another iovec
				 * that is a continuation of the packet list.
				 * The 2nd arg in __xdp2_check_clone_add_iovec
				 * is false, this indicates that we're not
				 * tracking the pvbuf length of the top level
				 * iovec. It's really not very interesting to
				 * set the real length since this is a packet
				 * vector
				 */
				if (!__xdp2_check_clone_add_iovec(
						cist, false,
						ist->pvbuf_list_size))
					return false;
			}

			/* Check if the whole segment fits in the paddr */
			if (!ist->pvbufs_only &&
			    (data_len >= ist->segment_length ||
			     data_len == cist->length)) {
				size = xdp2_min(ist->segment_length, data_len);

				/* The paddr has everything for a segment */
				xdp2_pvbuf_iovec_set_paddr_ent_and_bump_refcnt(
						cist->pvmgr, cist->pvbuf,
						cist->index, pbaddr_1.paddr,
						pbaddr_2.paddr, size);

				/* Move the offset in the input paddr over
				 * the portion set in the iovec
				 */
				__xdp2_paddr_add_to_offset(&pbaddr_1, &pbaddr_2,
							   size);
				data_len -= size;

				cist->index += 1 + is_long_addr;

				/* Note that we haven't touched "need packet
				 * length" so when this loops when come
				 * back into this block
				 */
				continue;
			}

			/* Create a pvbuf for the packet */
			pvbuf_size = __xdp2_pvbuf_get_size(cist->pvmgr, 0);
			pvbuf_paddr = ___xdp2_pvbuf_alloc(cist->pvmgr,
							  pvbuf_size, &pvbuf,
							  &pvbuf_size);
			if (!pvbuf_paddr)
				goto alloc_fail;

			size = xdp2_min(ist->segment_length, data_len);

			/* Clone and set the buffer as initial data for
			 * the new segment
			 */
			xdp2_pvbuf_iovec_set_paddr_ent_and_bump_refcnt(
					cist->pvmgr, pvbuf,
					2, pbaddr_1.paddr,
					pbaddr_2.paddr, size);

			/* Now set the pvbuf in the packet list */
			xdp2_pvbuf_iovec_set_pvbuf_ent(cist->pvbuf,
					cist->index, pvbuf_paddr, size);

			/* Compute the remaining length for the segment that
			 * we're looking for
			 */
			ist->packet_need_length =
				xdp2_min(ist->segment_length, cist->length) -
								size;
			cist->index++;

			__xdp2_paddr_add_to_offset(&pbaddr_1, &pbaddr_2,
						   size);
			data_len -= size;

			/* When we loop here "need length" is set so we
			 * shouldn't reenter this block
			 */

			continue;
		}

		/* Continuing to fill the current pvbuf */
		size = xdp2_min(ist->packet_need_length, data_len);
		iovec = &cist->pvbuf->iovec[cist->index - 1];

		/* Append the pbuf with appropriate offset and length to the
		 * pbuf under construction
		 */
		if (!__xdp2_pvbuf_append_paddr(cist->pvmgr, iovec->paddr,
					       pbaddr_1.paddr, pbaddr_2.paddr,
					       size, false))
			goto alloc_fail;

		/* Bump the ref count since we cloned the paddr */
		__xdp2_pvbuf_paddr_bump_refcnt(cist->pvmgr, pbaddr_1.paddr,
					       pbaddr_2.paddr);

		pvaddr.paddr = iovec->paddr;
		if (pvaddr.pvbuf.data_length) {
			/* We're still tracking length for this iovec.
			 * Set iovec length  per the pbuf we just appended
			 */
			if (size + pvaddr.pvbuf.data_length <
						XDP2_PVBUF_MAX_LEN)
				pvaddr.pvbuf.data_length += size;
			else
				pvaddr.pvbuf.data_length = 0;
			iovec->paddr = pvaddr.paddr;
		}

		/* Adjust counts and go to next iovec */
		__xdp2_paddr_add_to_offset(&pbaddr_1, &pbaddr_2,
					   size);
		data_len -= size;
		ist->packet_need_length -= size;
	}

	cist->length -= (orig_len - data_len);

	return true;

alloc_fail:
	cist->error = -ENOMEM;
	return false;
}

/* Frontend function to segment a pvbuf. Kick off an iovec iterator */
ssize_t ___xdp2_pvpkt_segment(xdp2_paddr_t pvbuf_paddr,
			       struct __xdp2_pvbuf_iter_clone *cist)
{
	struct __xdp2_pvbuf_iter_segment *ist =
				(struct __xdp2_pvbuf_iter_segment *)cist;
	size_t orig_length = cist->length;
	unsigned int num_segs;

	if (!ist->segment_length)
		return -1;

	/* Determine a good pvbuf_size for the packet list. Note that this
	 * might underestimate a bit since we don't take long address
	 * buffers into account (not a problem)
	 */
	num_segs = xdp2_round_up_div(cist->length, ist->segment_length);
	ist->pvbuf_list_size = XDP2_PVBUF_NUM_PVBUFS_NEEDED(num_segs);

	/* Iterate, this will populate the pvbuf with a packet vector for the
	 * segments
	 */
	__xdp2_pvbuf_iterate_iovec(cist->pvmgr, pvbuf_paddr,
				   __xdp2_pvpkt_iterate_segment_pvbuf, cist);

	if (cist->error < 0) {
		/* Iterator failed with negative error code. Free the
		 * incomplete pvbuf that was constructed
		 */
		return cist->error;
	}

	if (cist->index) {
		/* The last iovec was still under construction. Call the
		 * callback to close it
		 */
		if (!__xdp2_pvpkt_iterate_do_cb(ist))
			return false;

		/* Note that we don't need to worry about any compression here,
		 * if segments fit into one buffer then the iovec for them
		 * already contains a non-PVbuf paddr
		 */
	}

	return orig_length - cist->length;
}
