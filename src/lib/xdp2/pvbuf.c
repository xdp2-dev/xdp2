// SPDX-License-Identifier: BSD-2-Clause-FreeBSD
/*
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

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "xdp2/checksum.h"
#include "xdp2/pvbuf.h"

struct xdp2_pvbuf_mgr xdp2_pvbuf_global_mgr;

/* PVbuf allocation and free functions */

/* Compute the size limit based from the size multiple. The fact argument is
 * either zero of a number from 1 through 65535 that serves as the numerator
 * of the multiplying fraction where 65536 is the denominator
 *
 * If fact argument is non-zero then return:
 *    (fact * input) / 65536
 *
 * Else if fact is zero then return zero
 *
 * The input is a pbuf allocation size being considered (e.g. 128, 1024, etc.)
 *
 * The size limit is used to determine how much slack is tolerable when
 * allocating a pvbuf. A higher size limit, means less tolerance for slack.
 * The higher size limit is produced by a higher fact number
 *
 * For example, if fact is the maximum value of 65536 and input is 1024, then
 * this function returns 1024. Unless the allocation is exactly 1024 bytes
 * then the smaller size is allocated. If fact were the minimum value of zero,
 * then the function returns zero meaning any amount of slack is tolerable
 * in order to minimize the number of pbufs allocated
 *
 * fact values between the minimum and maximum can be set to trade-off the
 * amount of wasted space in a pbufs versus the number of allocated
 *
 * As an example, suppose there are 1024 byte pbufs, 512 byte pbufs,
 * 256 byte pbufs are configured. Suppose fact is set to 49152 (75%). If an
 * allocation request is made for 900 bytes, then per the size limit a single
 * 1024 byte pbuf is allocated resulting in 124 bytes of slack. If an allocation
 * request is made for 700 bytes, then a 512 byte pbuf and a 256 byte pbuf are
 * allocated resulting in 68 bytes of slack; note that has a single 1024 byte
 * buffer been allocated that would have resulted in 324 bytes of slack
 */
static size_t __xdp2_pbuf_size_limit(unsigned int fact, size_t input)
{
	return ((unsigned long long)fact * (unsigned long long)input) >> 16;
}

/* Allocate a pvbuf of the requested size from the packet buffer manager in
 * the pvmgr argument. A paddr for the allocated pvbuf is returned or
 * XDP2_PADDR_NULL if the allocation failed. If the allocation succeeds a
 * pointer the pvbuf is returned in ret_pvbuf. The size argument gives the
 * aggregate size of the pbufs to allocate. pvbuf_off specifies the starting
 * index in the iovecs array to place the pbufs. fact specifies the numerator
 * in the calculation of whether to use small buffers to save slack in the
 * allocation, and fact is the input to the size limit function called to
 * determine a smaller allocations should be used
 */
xdp2_paddr_t __xdp2_pvbuf_alloc_params(struct xdp2_pvbuf_mgr *pvmgr,
				       size_t size_req, unsigned int fact,
				       unsigned int pvbuf_off,
				       struct xdp2_pvbuf **ret_pvbuf)
{
	unsigned int size_shift, zindex, num_iovecs, pvbuf_size;
	struct xdp2_pbuf_allocator_entry *entry;
	struct xdp2_pbuf_allocator *pallocator;
	xdp2_paddr_t ret_pvbuf_paddr, pvbuf_paddr;
	struct xdp2_pvbuf *pvbuf, *opvbuf;
	ssize_t size = size_req;
	void *pbuf_base = NULL;
	size_t alloc_size;
	void *pbuf;

	pvbuf_size = __xdp2_pvbuf_get_size(pvmgr, size);
	ret_pvbuf_paddr = ___xdp2_pvbuf_alloc(pvmgr, pvbuf_size,
					      &opvbuf, &pvbuf_size);
	if (!ret_pvbuf_paddr) {
		*ret_pvbuf = NULL;
		return XDP2_PADDR_NULL;
	}

	XDP2_ASSERT(opvbuf == __xdp2_pvbuf_paddr_to_addr(pvmgr,
							 ret_pvbuf_paddr),
		     "PVbuf: Mismatch between paddr and pvbuf: "
		     "%p != %p", opvbuf, __xdp2_pvbuf_paddr_to_addr(pvmgr,
							ret_pvbuf_paddr));

	num_iovecs = XDP2_PVBUF_NUM_IOVECS_FROM_PADDR(ret_pvbuf_paddr);

	XDP2_ASSERT(pvbuf_off < num_iovecs - 1, "In PVbuf allocate pvbuf "
		    "offset %u must be less than number of iovecs %u minus 1",
		    pvbuf_off, num_iovecs);

	pvbuf = opvbuf;
	while (size > 0) {
		/* Allocate as many pbufs as needed to satisfy request */
		size_shift = xdp2_get_log_round_up(size);

		/* Limit size shift to the support range */
		if (size_shift > XDP2_PBUF_MAX_SIZE_SHIFT)
			size_shift = XDP2_PBUF_MAX_SIZE_SHIFT;
		else if (size_shift < XDP2_PBUF_BASE_SIZE_SHIFT)
			size_shift = XDP2_PBUF_BASE_SIZE_SHIFT;

		entry = &pvmgr->pbuf_allocator_table[
			xdp2_pbuf_size_shift_to_buffer_tag(size_shift)];

		XDP2_ASSERT(entry->pallocator || entry->smaller_pallocator,
			     "pbuf allocator table does not have a non-NULL "
			     "allocator");

		if (entry->pallocator && (
				size > __xdp2_pbuf_size_limit(fact,
							entry->alloc_size) ||
				!entry->smaller_pallocator)) {
			/* Use bigger size since slack is tolerable */
			pallocator = XDP2_PVBUF_GET_ADDRESS(pvmgr,
						     entry->pallocator);
			alloc_size = entry->alloc_size;
			size_shift = entry->alloc_size_shift;
		} else {
			/* Use smaller size to reduce slack */
			pallocator = XDP2_PVBUF_GET_ADDRESS(pvmgr,
						     entry->smaller_pallocator);
			alloc_size = entry->smaller_size;
			size_shift = entry->smaller_size_shift;
		}

		/* Allocate a pbuf for the alloc_size */
		pbuf = ___xdp2_pbuf_alloc(pvmgr, pallocator, &zindex,
					   pvmgr->alloc_one_ref);
		if (!pbuf)
			goto alloc_fail;

		/* If length is precisely equal to 1 << 20 then a zero is
		 * written here and the buffer tag will be 15 from the above
		 * call to xdp2_pbuf_make_paddr everything's good
		 */
		xdp2_pvbuf_iovec_set_pbuf_make_paddr(pvbuf, pvbuf_off,
				xdp2_min(alloc_size, size),
				pvmgr->alloc_one_ref, size_shift, zindex, 0,
				alloc_size, pbuf_base, pbuf);

		size -= alloc_size;

		pvbuf_off++;
		if (pvbuf_off >= (num_iovecs - 2) &&
		    size > 0) {
			struct xdp2_pvbuf *new_pvbuf;

			/* We've exhausted the iovec slots except for the last
			 * one. Allocate another pvbuf, set in the last iovec
			 * slot, and then loop to allocate pbufs and put them
			 * in the iovec for that pvbuf
			 */
			pvbuf_paddr = ___xdp2_pvbuf_alloc(pvmgr, pvbuf_size,
							  &new_pvbuf, NULL);
			if (!pvbuf_paddr)
				goto alloc_fail;

			/* Proactively set the length under the assumption that
			 * the rest of the size will be allocated under the
			 * pvbuf we're adding here
			 */
			xdp2_pvbuf_iovec_set_pvbuf_ent(pvbuf, pvbuf_off,
					pvbuf_paddr,
					size < XDP2_PVBUF_MAX_LEN ? size : 0);

			pvbuf = new_pvbuf;
			pvbuf_off = 0;
			num_iovecs = XDP2_PVBUF_NUM_IOVECS_FROM_PADDR(
							pvbuf_paddr);
		}
	}

	*ret_pvbuf = opvbuf;
	return ret_pvbuf_paddr;

alloc_fail:
	/* Some part of the allocation failed, free and pvbufs and pbufs
	 * already allocated
	 */
	__xdp2_pvbuf_free(pvmgr, ret_pvbuf_paddr);

	return XDP2_PADDR_NULL;
}

/* Free a pvbuf given its paddr. The pvbuf is managed by the packet buffer
 * manager in the pvmgr argument
 */
void __xdp2_pvbuf_free(struct xdp2_pvbuf_mgr *pvmgr, xdp2_paddr_t pvbuf_paddr)
{
	unsigned int pvbuf_size = xdp2_pvbuf_get_size_from_paddr(pvbuf_paddr);
	struct xdp2_pvbuf *pvbuf = __xdp2_pvbuf_paddr_to_addr(pvmgr,
							      pvbuf_paddr);
	unsigned int i = 0;

	xdp2_pvbuf_iovec_map_foreach(pvbuf, i) {
		struct xdp2_iovec *iovec = &pvbuf->iovec[i];

		XDP2_ASSERT(iovec->paddr, "Address is NULL in iterate iovec");

		switch (xdp2_iovec_paddr_addr_tag(iovec)) {
		case XDP2_PADDR_TAG_PVBUF:
			__xdp2_pvbuf_free(pvmgr, iovec->paddr);
			break;
		case XDP2_PADDR_TAG_PBUF:
		case XDP2_PADDR_TAG_PBUF_1REF:
			__xdp2_pbuf_free(pvmgr, iovec->paddr);
			break;
		case XDP2_PADDR_TAG_ALL_SHORT_ADDR_CASES:
			__xdp2_paddr_short_addr_free(pvmgr, iovec->paddr);
			break;
		case XDP2_PADDR_TAG_LONG_ADDR:
			__xdp2_paddr_long_addr_free(pvmgr, iovec[0].paddr,
						    iovec[1].paddr);

			/* Long address paddrs use two iovec slot so skip
			 * one
			 */
			xdp2_pvbuf_iovec_unset_ent(pvbuf, i);
			i++;

			XDP2_ASSERT(xdp2_pvbuf_iovec_map_isset(pvbuf, i),
				    "Expected map bit %u of second word "
				    "of long addr set in pvbuf iterate", i);
			break;
		default:
			XDP2_IOVEC_PADDR_TYPE_DEFAULT_ERR(iovec, "Free pvbuf");
		}
		xdp2_pvbuf_iovec_unset_ent(pvbuf, i);
	}

	/* Now free the pvbuf object structure itself */
	xdp2_obj_alloc_free(XDP2_PVBUF_GET_ALLOCATOR(pvmgr, pvbuf_size),
			    pvbuf);
}

/* IOvec shift functions */

/* Shift the iovecs in a pvbuf to the tail if there are empty slots. This is a
 * form of compression for prepending pvbufs or pbufs
 */
static void __xdp2_pvbuf_shift_pvbuf_iovecs_to_tail(struct xdp2_pvbuf *pvbuf,
						    unsigned int num_iovecs)
{
	unsigned int i = num_iovecs - 1, j = num_iovecs - 1;

	xdp2_pvbuf_iovec_map_rev_foreach(pvbuf, i)
		xdp2_pvbuf_iovec_move_ent(pvbuf, i, j--);
}

/* Shift the iovecs in a pvbuf to the head if there are empty slots. This is a
 * form of compression for appending pvbufs or pbufs
 */
static void __xdp2_pvbuf_shift_pvbuf_iovecs_to_head(struct xdp2_pvbuf *pvbuf,
						     unsigned int num_iovecs)
{
	unsigned int i = 0, j = 0;

	xdp2_pvbuf_iovec_map_foreach(pvbuf, i)
		xdp2_pvbuf_iovec_move_ent(pvbuf, i, j++);
}

/* Prepend functions */

/* Compress a pvbuf that has less than two iovecs set as part of
 * prepending a PVbuf
 */
bool __xdp2_pvbuf_prepend_pvbuf_compress(struct xdp2_pvbuf_mgr *pvmgr,
					 xdp2_paddr_t pvbuf_paddr,
					 xdp2_paddr_t hdrs_paddr,
					 size_t length, unsigned int weight)
{
	unsigned int pvbuf_size = xdp2_pvbuf_get_size_from_paddr(hdrs_paddr);
	struct xdp2_pvbuf *pvbuf_headers =
				__xdp2_pvbuf_paddr_to_addr(pvmgr, hdrs_paddr);
	struct xdp2_iovec *iovec;
	unsigned int i;
	bool ret = true;

	/* Note that a pvbuf with a single long address paddr is not
	 * a candidate for compression since the long address takes
	 * up two slots in the iovec array
	 */
	if (weight) {
		i = xdp2_pvbuf_iovec_map_find(pvbuf_headers);
		iovec = &pvbuf_headers->iovec[i];

		switch (xdp2_iovec_paddr_addr_tag(iovec)) {
		case XDP2_PADDR_TAG_PBUF:
		case XDP2_PADDR_TAG_PBUF_1REF:
			ret = __xdp2_pvbuf_prepend_paddr(
				pvmgr, pvbuf_paddr, iovec->paddr,
				XDP2_PADDR_NULL,
				xdp2_pbuf_get_data_len_from_iovec(iovec),
				true);
			break;
		case XDP2_PADDR_TAG_ALL_SHORT_ADDR_CASES:
			ret = __xdp2_pvbuf_prepend_paddr(
				pvmgr, pvbuf_paddr, iovec->paddr,
				XDP2_PADDR_NULL,
				xdp2_paddr_short_addr_get_data_len_from_iovec(
								iovec),
				true);
			break;
		case XDP2_PADDR_TAG_PVBUF:
			ret = __xdp2_pvbuf_prepend_pvbuf(
				pvmgr, pvbuf_paddr, iovec->paddr,
				length, true);
			break;
		case XDP2_PADDR_TAG_LONG_ADDR:
			/* We checked that the weight is <= 1 so
			 * there's no way a long address can be
			 * set in the pvbuf
			 */
			XDP2_ERR(1, "Encountering long address in "
				    "pvbuf where weight is <= 1");
		default:
			XDP2_IOVEC_PADDR_TYPE_DEFAULT_ERR(iovec,
						"Prepend pvbuf");
		}
	}

	/* Empty pvbuf, just free it */
	if (ret)
		xdp2_obj_alloc_free(XDP2_PVBUF_GET_ALLOCATOR(pvmgr,
							pvbuf_size),
				    pvbuf_headers);
	return ret;
}

/* Prepend a paddr with a new pvbuf allocation. We need to prepend into a PVbuf
 * where the first slot contains a non-PVbuf paddr. Allocate a new PVbuf, copy
 * the original paddr to it, prepend the input paddr, and then set the original
 * index to the new PVbuf. Note that we need to handle the cases where a long
 * address paddr is being prepended or moved (we need two deal with 128 bit
 * paddrs in those cases
 */
static bool __xdp2_pvbuf_prepend_paddr_into_iovec_new_pvbuf(
		struct xdp2_pvbuf_mgr *pvmgr, struct xdp2_pvbuf *pvbuf,
		unsigned int index, xdp2_paddr_t paddr_1,
		xdp2_paddr_t paddr_2, size_t length)
{
	struct xdp2_iovec *iovec = &pvbuf->iovec[index];
	unsigned int pvbuf_size, num_iovecs, pos;
	xdp2_paddr_t new_pvbuf_paddr;
	struct xdp2_pvbuf *new_pvbuf;
	bool is_long_addr = false;
	size_t iovlen;

	/* There are no preceding empty slots and the zeroth slot (at least the
	 * one in index) is a pbuf. Allocate a new pvbuf. Set the pbuf iovec
	 * at the last slot of the new pvbuf, and set the prepended pvbuf
	 * to the second last slot. Then set the new pvbuf to index (i.e. the
	 * first slot) of the original iovec
	 */
	pvbuf_size = __xdp2_pvbuf_get_size(pvmgr, 0);
	new_pvbuf_paddr = ___xdp2_pvbuf_alloc(pvmgr, pvbuf_size,
					       &new_pvbuf, &pvbuf_size);
	if (!new_pvbuf_paddr)
		return false;

	num_iovecs = XDP2_PVBUF_NUM_IOVECS(pvbuf_size);
	pos = num_iovecs - 1;

	/* Note that we assume num_iovecs is at least four. In the case
	 * that the input paddr and the iovec is a long address paddr we
	 * need four slots in the new pvbuf
	 */
	XDP2_ASSERT(pos >= 4, "In prepend paddr into iovec, pos %u is "
			      "less than 4", pos);

	XDP2_ASSERT(iovec->paddr, "In pvbuf prepend pbuf pullup into iovec "
				  "iovec[%u] is NULL", index);

	switch (xdp2_iovec_paddr_addr_tag(iovec)) {
	case XDP2_PADDR_TAG_PVBUF:
		/* For an iovec containing a pvbuf with length zero the parent
		 * iovec also has to have a length of zero. Set the iovlen to
		 * XDP2_PVBUF_MAX_LEN so that the check below when setting the
		 * parent iovec length returns the correct value
		 */
		iovlen = iovec->pbaddr.pvbuf.data_length ? : XDP2_PVBUF_MAX_LEN;

		break;
	case XDP2_PADDR_TAG_PBUF:
	case XDP2_PADDR_TAG_PBUF_1REF:
		iovlen = xdp2_pbuf_get_data_len_from_iovec(iovec);
		break;
	case XDP2_PADDR_TAG_ALL_SHORT_ADDR_CASES:
		iovlen = xdp2_paddr_short_addr_get_data_len_from_iovec(iovec);
		break;
	case XDP2_PADDR_TAG_LONG_ADDR:
		XDP2_ASSERT(iovec->paddr, "Address is NULL instead of long "
					  "in prepend paddr in new pvbuf");
		iovlen = xdp2_paddr_long_addr_get_data_len_from_iovec(iovec);

		/* Set the second word of the long address */
		new_pvbuf->iovec[pos] = iovec[1];
		xdp2_pvbuf_iovec_map_set(new_pvbuf, pos);

		/* Decrement pos to make the position for the first word of
		 * the long paddr
		 */
		pos--;

		is_long_addr = true;

		break;
	default:
		XDP2_IOVEC_PADDR_TYPE_DEFAULT_ERR(iovec,
						  "Prepending into new pvbuf");
	}

	/* Move the original paddr */
	new_pvbuf->iovec[pos] = iovec[0];
	xdp2_pvbuf_iovec_map_set(new_pvbuf, pos);
	pos--;

	if (xdp2_paddr_get_paddr_tag_from_paddr(paddr_1) ==
	    XDP2_PADDR_TAG_LONG_ADDR)
		pos--;

	xdp2_pvbuf_iovec_set_paddr_ent(new_pvbuf, pos, paddr_1, paddr_2,
				       length);

	if (is_long_addr) {
		/* Clear the first slot occupied by a long paddr, we'll use the
		 * second slot of the new PVbuf
		 */
		xdp2_pvbuf_iovec_clear_ent(pvbuf, index);
		iovec++;
		index++;
	}

	XDP2_ASSERT(xdp2_pvbuf_iovec_map_isset(pvbuf, index),
		    "Expected map bit %u set in prepend_pvbuf_into_iovec",
		    index);

	/* If the input length is non-zero and the length plus the
	 * length of the pbuf being moved is in range (i.e. less than
	 * XDP2_PVBUF_MAX_LEN) then set the length for the new pvbuf
	 */
	length = length && (iovlen + length) < XDP2_PVBUF_MAX_LEN ?
		iovlen + length : 0;

	xdp2_pvbuf_set_pvbuf_iovec(iovec, new_pvbuf_paddr, length);

	return true;
}

/* Prepend a pvbuf into an iovec */
static bool __xdp2_pvbuf_prepend_pvbuf_into_iovec(
		struct xdp2_pvbuf_mgr *pvmgr, struct xdp2_pvbuf *pvbuf,
		unsigned int index, xdp2_paddr_t pvbuf_paddr, size_t length,
		bool compress)
{
	struct xdp2_iovec *iovec = &pvbuf->iovec[index];

	XDP2_ASSERT(iovec->paddr, "__xdp2_pvbuf_prepend_pbuf_pullup "
				  "iovec[%u] is NULL", index);

	XDP2_ASSERT(iovec->paddr, "In pvbuf prepend pvbuf into iovec "
				  "iovec[%u] is NULL", index);

	switch (xdp2_iovec_paddr_addr_tag(iovec)) {
	case XDP2_PADDR_TAG_PVBUF: {
		bool ret;

		/* There are no preceding empty slots, but the zeroth slot
		 * is already a pvbuf, recurse to push headers in that pvbuf
		 */
		ret = __xdp2_pvbuf_prepend_pvbuf(pvmgr, iovec->paddr,
						 pvbuf_paddr, length,
						 compress);
		if (!ret)
			return false;

		if (iovec->pbaddr.pvbuf.data_length) {
			/* The pvbuf at slot zero had a nonzero length, try to
			 * adjust it here
			 */
			if (length && iovec->pbaddr.pvbuf.data_length +
						length < XDP2_PVBUF_MAX_LEN)
				iovec->pbaddr.pvbuf.data_length += length;
			else
				iovec->pbaddr.pvbuf.data_length = 0;
		}
		if (compress)
			__xdp2_pvbuf_check_single(pvmgr, iovec);
		break;
	}
	case XDP2_PADDR_TAG_PBUF:
	case XDP2_PADDR_TAG_PBUF_1REF:
	case XDP2_PADDR_TAG_ALL_SHORT_ADDR_CASES:
	case XDP2_PADDR_TAG_LONG_ADDR:
		/* Need to prepend a new pvbuf at the head */
		return __xdp2_pvbuf_prepend_paddr_into_iovec_new_pvbuf(
				pvmgr, pvbuf, index, pvbuf_paddr,
				XDP2_PADDR_NULL, length);
	default:
		XDP2_IOVEC_PADDR_TYPE_DEFAULT_ERR(iovec,
						  "Prepending into iovec");
	}

	return true;
}

/* Compress iovecs array to try to find an prepend slot */
unsigned int __xdp2_pvbuf_find_prepend_slot_compress(
		struct xdp2_pvbuf *pvbuf, unsigned int num_iovecs)
{
	unsigned int index;

	if (xdp2_pvbuf_iovec_map_weight(pvbuf) == num_iovecs)
		return -1U;

	/* Shift the iovecs in the pvbuf to the tail to make room for
	 * prepending pvbufs
	 */
	__xdp2_pvbuf_shift_pvbuf_iovecs_to_tail(pvbuf, num_iovecs);

	index = xdp2_pvbuf_iovec_map_find(pvbuf);

	XDP2_ASSERT(index, "Shift prepend failed");

	XDP2_ASSERT(!xdp2_pvbuf_iovec_map_isset(pvbuf, index - 1),
		     "Shift prepend slot not empty");
	return index - 1;
}

/* Backend function prepend packet headers at the front of a pvbuf. This is
 * called when the zeroth slot in the pvbuf that the headers are being prepended
 * into is not NULL
 */
bool ___xdp2_pvbuf_prepend_pvbuf(struct xdp2_pvbuf_mgr *pvmgr,
				 xdp2_paddr_t pvbuf_paddr,
				 xdp2_paddr_t pvbuf_paddr_in,
				 size_t length, bool compress)
{
	struct xdp2_pvbuf *pvbuf =
			__xdp2_pvbuf_paddr_to_addr(pvmgr, pvbuf_paddr);
	unsigned int num_iovecs =
			XDP2_PVBUF_NUM_IOVECS_FROM_PADDR(pvbuf_paddr);
	unsigned int i;

	i = __xdp2_pvbuf_find_prepend_slot(pvbuf, pvbuf_paddr, compress);
	if (i < num_iovecs) {
		xdp2_pvbuf_iovec_set_pvbuf_ent(pvbuf, i, pvbuf_paddr_in,
					       length);
		return true;
	}

	return __xdp2_pvbuf_prepend_pvbuf_into_iovec(pvmgr, pvbuf, 0,
						     pvbuf_paddr_in, length,
						     compress);
}

/* Prepend headers from a non-pvbuf paddr into an iovec */
bool ___xdp2_pvbuf_prepend_paddr_into_iovec(
		struct xdp2_pvbuf_mgr *pvmgr, struct xdp2_pvbuf *pvbuf,
		unsigned int index, xdp2_paddr_t paddr_1,
		xdp2_paddr_t paddr_2, size_t length, bool compress)
{
	struct xdp2_iovec *iovec = &pvbuf->iovec[index];

	XDP2_ASSERT(iovec->paddr, "In pvbuf prepend paddr into iovec "
				  "iovec[%u] is NULL", index);

	switch (xdp2_iovec_paddr_addr_tag(iovec)) {
	case XDP2_PADDR_TAG_PVBUF: {
		bool ret;

		/* There are no preceding empty slots, but the zeroth slot
		 * is already a pvbuf, recurse to push headers in that pvbuf
		 */
		ret = __xdp2_pvbuf_prepend_paddr(pvmgr, iovec->paddr,
						 paddr_1, paddr_2, length,
						 compress);
		if (!ret)
			return false;

		if (iovec->pbaddr.pvbuf.data_length) {
			/* The pvbuf at slot zero had a nonzero length, try to
			 * adjust it here
			 */
			if (length &&
			    iovec->pbaddr.pvbuf.data_length + length <
							XDP2_PVBUF_MAX_LEN)
				iovec->pbaddr.pvbuf.data_length += length;
			else
				iovec->pbaddr.pvbuf.data_length = 0;
		}
		return true;
	}
	case XDP2_PADDR_TAG_PBUF:
	case XDP2_PADDR_TAG_PBUF_1REF:
	case XDP2_PADDR_TAG_ALL_SHORT_ADDR_CASES:
	case XDP2_PADDR_TAG_LONG_ADDR:
		/* Need to prepend a new pvbuf at the head */
		return __xdp2_pvbuf_prepend_paddr_into_iovec_new_pvbuf(
				pvmgr, pvbuf, index, paddr_1, paddr_2,
				length);
	default:
		XDP2_IOVEC_PADDR_TYPE_DEFAULT_ERR(iovec,
						"Prepending paddr into iovec");
	}
}

/* Prepend headers from a pbuf into pvbuf as part of the pullup function.
 * Note that the input paddr explicitly a pbuf for this function
 */
bool __xdp2_pvbuf_prepend_pbuf_pullup(struct xdp2_pvbuf_mgr *pvmgr,
				      xdp2_paddr_t pvbuf_paddr,
				      xdp2_paddr_t paddr,
				      size_t length, bool compress)
{
	struct xdp2_pvbuf *pvbuf =
			__xdp2_pvbuf_paddr_to_addr(pvmgr, pvbuf_paddr);
	unsigned int num_iovecs =
			XDP2_PVBUF_NUM_IOVECS_FROM_PADDR(pvbuf_paddr);
	struct xdp2_iovec *iovec = &pvbuf->iovec[0];
	unsigned int index;
	bool ret = true;

	XDP2_ASSERT(XDP2_PADDR_IS_PBUF(paddr),
		    "Prepend a paddr for pullup paddr but paddr type "
		    "%s (%u) is not a pbuf",
		    xdp2_paddr_print_paddr_type(paddr),
		    xdp2_paddr_get_paddr_tag_from_paddr(paddr));

	index = __xdp2_pvbuf_find_prepend_slot(pvbuf, pvbuf_paddr, compress);
	if (index < num_iovecs) {
		xdp2_pvbuf_iovec_set_pbuf_ent(pvbuf, index, paddr, length);
		return true;
	}

	if (!xdp2_pvbuf_iovec_map_isset(pvbuf, 1)) {
		/* If slot one is not set then slot zero cannot be a long
		 * address
		 */
		XDP2_ASSERT(!XDP2_IOVEC_IS_LONG_ADDR(iovec),
			    "Unexpected long addess in prepend pbuf pullup");
		pvbuf->iovec[1] = pvbuf->iovec[0];
		xdp2_pvbuf_iovec_clear_ent(pvbuf, 0);
		xdp2_pvbuf_iovec_map_set(pvbuf, 1);
		pvbuf->iovec[0].paddr = XDP2_PADDR_NULL;
	} else if (XDP2_IOVEC_IS_PVBUF(iovec)) {
		/* There are no preceding empty slots, but the zeroth slot
		 * is already a pvbuf, recurse to push headers in that pvbuf
		 */
		ret = __xdp2_pvbuf_prepend_pvbuf_into_iovec(
				pvmgr, pvbuf, 1, iovec->paddr,
				iovec->pbaddr.pvbuf.data_length, compress);
	} else if (xdp2_pvbuf_iovec_map_isset(pvbuf, 0)) {
		size_t data_len;
		void *data_ptr;

		__xdp2_pvbuf_get_ptr_len_from_iovec(pvmgr, iovec, &data_ptr,
						    &data_len, false);
		if (XDP2_IOVEC_IS_LONG_ADDR(iovec)) {
			ret = ___xdp2_pvbuf_prepend_paddr_into_iovec(
					pvmgr, pvbuf, 2, iovec[0].paddr,
					iovec[1].paddr, data_len, compress);
			xdp2_pvbuf_iovec_clear_ent(pvbuf, 1);
		} else {
			ret = ___xdp2_pvbuf_prepend_paddr_into_iovec(
					pvmgr, pvbuf, 1, iovec->paddr,
					XDP2_PADDR_NULL, data_len, compress);
		}
	} else {
		/* The iovec is empty, just set it */
	}

	if (ret) {
		xdp2_pbuf_set_iovec(iovec, paddr, length);
		xdp2_pvbuf_iovec_map_set(pvbuf, 0);
	}

	return ret;
}

/* Append functions */

/* Compress a pvbuf that has less than two iovecs set as part of
 * prepending a PVbuf
 */
bool __xdp2_pvbuf_append_pvbuf_compress(struct xdp2_pvbuf_mgr *pvmgr,
					xdp2_paddr_t pvbuf_paddr,
					xdp2_paddr_t trls_paddr,
					size_t length, unsigned int weight)
{
	unsigned int pvbuf_size = xdp2_pvbuf_get_size_from_paddr(trls_paddr);
	struct xdp2_pvbuf *pvbuf_trailers =
			__xdp2_pvbuf_paddr_to_addr(pvmgr, trls_paddr);
	struct xdp2_iovec *iovec;
	unsigned int i;
	bool ret = true;

	if (weight) {
		i = xdp2_pvbuf_iovec_map_find(pvbuf_trailers);
		iovec = &pvbuf_trailers->iovec[i];

		switch (xdp2_iovec_paddr_addr_tag(iovec)) {
		case XDP2_PADDR_TAG_PBUF:
		case XDP2_PADDR_TAG_PBUF_1REF:
			ret = __xdp2_pvbuf_append_paddr(
				pvmgr, pvbuf_paddr, iovec->paddr,
				XDP2_PADDR_NULL,
				xdp2_pbuf_get_data_len_from_iovec(iovec),
				true);
			break;
		case XDP2_PADDR_TAG_ALL_SHORT_ADDR_CASES:
			ret = __xdp2_pvbuf_append_paddr(
				pvmgr, pvbuf_paddr, iovec->paddr,
				XDP2_PADDR_NULL,
				xdp2_paddr_short_addr_get_data_len_from_iovec(
								iovec),
				true);
			break;
		case XDP2_PADDR_TAG_PVBUF:
			ret = __xdp2_pvbuf_append_pvbuf(pvmgr, pvbuf_paddr,
							iovec->paddr, length,
							true);
			break;
		case XDP2_PADDR_TAG_LONG_ADDR:
			/* We checked that the weight is <= 1 so
			 * there's no way a long address can be
			 * set in the pvbuf
			 */
			XDP2_ERR(1, "Encountering long address in "
				    "pvbuf where weight is <= 1");
		default:
			XDP2_IOVEC_PADDR_TYPE_DEFAULT_ERR(iovec,
						"Append pvbuf");
		}
	}

	/* Empty pvbuf, just free it */
	if (ret)
		xdp2_obj_alloc_free(XDP2_PVBUF_GET_ALLOCATOR(pvmgr,
						  pvbuf_size),
			pvbuf_trailers);
	return ret;
}

/* Append a pvbuf with a new pvbuf allocation. We need to append into a PVbuf
 * where the first slot contains a non-PVbuf paddr. Allocate a new PVbuf, copy
 * the original paddr to it, append the input paddr, and then set the original
 * index to the new PVbuf. Note that we need to handle the cases where a long
 * address paddr is being appended or moved (we need two deal with 128-bit
 * paddrs in those cases
 */
static bool __xdp2_pvbuf_append_paddr_into_iovec_new_pvbuf(
		struct xdp2_pvbuf_mgr *pvmgr, struct xdp2_pvbuf *pvbuf,
		unsigned int index, xdp2_paddr_t paddr_1,
		xdp2_paddr_t paddr_2, size_t length, bool index_set,
		bool set_pvbuf)
{
	struct xdp2_iovec *iovec = &pvbuf->iovec[index];
	unsigned int pvbuf_size, pos = 0;
	xdp2_paddr_t new_pvbuf_paddr;
	struct xdp2_pvbuf *new_pvbuf;
	size_t iovlen = 0;

	/* There are no trailing empty slots and the last slot (at least the
	 * one in index) is a pbuf. Allocate a new pvbuf. Set the pbuf iovec
	 * at the first slot of the new pvbuf, and set the prepended pvbuf to
	 * the second slot. Then set the new pvbuf to index (i.e. the last
	 * slot) of the original iovec
	 */
	pvbuf_size = __xdp2_pvbuf_get_size(pvmgr, 0);
	new_pvbuf_paddr = ___xdp2_pvbuf_alloc(pvmgr, pvbuf_size, &new_pvbuf,
					      &pvbuf_size);
	if (!new_pvbuf_paddr)
		return false;

	/* Note that we assume num_iovecs is at least four. In the case
	 * that the input paddr and the iovec is a long address paddr we
	 * need four slots in the new pvbuf
	 */
	XDP2_ASSERT(XDP2_PVBUF_NUM_IOVECS(pvbuf_size) >= 4,
		    "In append paddr into iovec, num_iovecs %lu is less than 4",
		    XDP2_PVBUF_NUM_IOVECS(pvbuf_size));

	if (!index_set) {
		/* Preemptively set the map bit since we're setting the iovec
		 * by hand (in the common case we're overriding and non-zero
		 * iovec
		 */
		xdp2_pvbuf_iovec_map_set(pvbuf, index);
		goto after_length;
	}

	/* Move the original paddr */

	XDP2_ASSERT(iovec->paddr, "Append paddr into new pvbuf iovec "
				  "iovec[%u] is NULL", index);

	switch (xdp2_iovec_paddr_addr_tag(iovec)) {
	case XDP2_PADDR_TAG_PVBUF:
		/* For an iovec containing a pvbuf with length zero the parent
		 * iovec also has to have a length of zero. Set the iovlen to
		 * XDP2_PVBUF_MAX_LEN so that the check below when setting the
		 * parent iovec length returns the correct value
		 */
		iovlen = iovec->pbaddr.pvbuf.data_length ? : XDP2_PVBUF_MAX_LEN;

		new_pvbuf->iovec[pos] = iovec[0];
		xdp2_pvbuf_iovec_map_set(new_pvbuf, pos);
		pos++;

		break;
	case XDP2_PADDR_TAG_PBUF:
	case XDP2_PADDR_TAG_PBUF_1REF:
		iovlen = xdp2_pbuf_get_data_len_from_iovec(iovec);

		new_pvbuf->iovec[pos] = iovec[0];
		xdp2_pvbuf_iovec_map_set(new_pvbuf, pos);
		pos++;

		break;
	case XDP2_PADDR_TAG_ALL_SHORT_ADDR_CASES:
		iovlen = xdp2_paddr_short_addr_get_data_len_from_iovec(iovec);

		new_pvbuf->iovec[pos] = iovec[0];
		xdp2_pvbuf_iovec_map_set(new_pvbuf, pos);
		pos++;

		break;
	case XDP2_PADDR_TAG_LONG_ADDR:
		if (iovec->pbaddr.long_addr.word_num) {
			/* We're inserting at the second word of a long
			 * address, back up iovec by on the get to the
			 * first word.
			 */
			iovec--;
			index--;
		}
		__xdp2_paddr_long_addr_check(iovec[0].paddr, iovec[1].paddr,
					     "Append in new iovec");

		iovlen = xdp2_paddr_long_addr_get_data_len_from_iovec(iovec);

		/* Set the second word of the long address */
		new_pvbuf->iovec[pos] = iovec[0];
		xdp2_pvbuf_iovec_map_set(new_pvbuf, pos);
		pos++;
		new_pvbuf->iovec[pos] = iovec[1];
		xdp2_pvbuf_iovec_map_set(new_pvbuf, pos);
		pos++;

		break;
	default:
		XDP2_IOVEC_PADDR_TYPE_DEFAULT_ERR(iovec,
						"Prepending into new pvbuf");
	}

after_length:
	/* Set input paddr(s) in the new pvbuf */
	switch (xdp2_paddr_get_paddr_tag_from_paddr(paddr_1)) {
	case XDP2_PADDR_TAG_PVBUF:
		xdp2_pvbuf_iovec_set_pvbuf_ent(new_pvbuf, pos, paddr_1, length);

		break;
	case XDP2_PADDR_TAG_PBUF:
	case XDP2_PADDR_TAG_PBUF_1REF:
		xdp2_pvbuf_iovec_set_pbuf_ent(new_pvbuf, pos, paddr_1, length);

		break;
	case XDP2_PADDR_TAG_ALL_SHORT_ADDR_CASES:
		xdp2_pvbuf_iovec_set_short_addr_ent(new_pvbuf, pos, paddr_1,
						    length);
		break;
	case XDP2_PADDR_TAG_LONG_ADDR:
		xdp2_pvbuf_iovec_set_long_addr_ent(new_pvbuf, pos,
						   paddr_1, paddr_2, length);
		break;
	default:
		XDP2_IOVEC_PADDR_TYPE_DEFAULT_ERR(iovec,
						"Prepending into new pvbuf");
	}

	if (index_set && xdp2_paddr_get_paddr_tag_from_paddr(iovec->paddr) ==
	    XDP2_PADDR_TAG_LONG_ADDR) {
		/* Clear the first slot occupied by a long paddr, we'll use the
		 * second slot of the new PVbuf
		 */
		xdp2_pvbuf_iovec_clear_ent(pvbuf, index);
		iovec++;
		index++;
	}

	XDP2_ASSERT(!index_set || xdp2_pvbuf_iovec_map_isset(pvbuf, index),
		     "Expected map bit %u set in "
		     "prepend_pvbuf_into_iovec", index);

	/* If the input length is non-zero and the length plus the length of
	 * the pbuf being moved is in range (i.e. less than XDP2_PVBUF_MAX_LEN)
	 * then set the length for the new pvbuf
	 */
	length = length && (iovlen + length) < XDP2_PVBUF_MAX_LEN ?
		iovlen + length : 0;

	xdp2_pvbuf_set_pvbuf_iovec(iovec, new_pvbuf_paddr, length);

	return true;
}

/* Append trailers from a pvbuf into an iovec */
static bool __xdp2_pvbuf_append_pvbuf_into_iovec(
		struct xdp2_pvbuf_mgr *pvmgr, struct xdp2_pvbuf *pvbuf,
		unsigned int index, xdp2_paddr_t paddr,
		size_t length, bool index_set, bool compress) {
	struct xdp2_iovec *iovec = &pvbuf->iovec[index];

	XDP2_ASSERT(XDP2_PADDR_IS_PVBUF(paddr), "paddr is not a PVBuf in "
						"append pvbuf into iovec");
	if (!index_set) {
		xdp2_pvbuf_iovec_set_pvbuf_ent(pvbuf, index, paddr, length);
	} else if (XDP2_IOVEC_IS_PVBUF(iovec)) {
		bool ret;

		/* There are no preceding empty slots, but the last slot
		 * is already a pvbuf, recurse to push headers in that pvbuf
		 */
		ret = __xdp2_pvbuf_append_pvbuf(pvmgr, iovec->paddr,
			     paddr, length, compress);
		if (!ret)
			return false;

		if (iovec->pbaddr.pvbuf.data_length) {
			/* The pvbuf at slot seven had a nonzero length, try to
			 * adjust it here
			 */
			if (length &&
			    iovec->pbaddr.pvbuf.data_length + length <
							XDP2_PVBUF_MAX_LEN)
				iovec->pbaddr.pvbuf.data_length += length;
			else
				iovec->pbaddr.pvbuf.data_length = 0;

		}
	} else { /* Non-PVbuf paddr */
		/* Need to create a new pvbuf at the tail */
		return __xdp2_pvbuf_append_paddr_into_iovec_new_pvbuf(pvmgr,
				pvbuf, index, paddr, XDP2_PADDR_NULL,
				length, index_set, true);
	}

	return true;
}

/* Compress iovecs array to try to find an append slot */
unsigned int __xdp2_pvbuf_find_append_slot_compress(struct xdp2_pvbuf *pvbuf,
						    unsigned int num_iovecs)
{
	unsigned int index;

	if (xdp2_pvbuf_iovec_map_weight(pvbuf) >= num_iovecs - 1)
		return -1U;

	/* Shift the iovecs in the pvbuf to the tail to make room for
	 * prepending pvbufs
	 */
	__xdp2_pvbuf_shift_pvbuf_iovecs_to_head(pvbuf, num_iovecs);

	index = xdp2_pvbuf_iovec_map_rev_find(pvbuf);

	XDP2_ASSERT(index < num_iovecs - 1, "Shift append failed");

	XDP2_ASSERT(!xdp2_pvbuf_iovec_map_isset(pvbuf, index + 1),
		     "Shift append slot not empty");
	return index + 1;
}

/* Backend function append packet trailers at the end of a pvbuf. This is
 * called when the last slot in the pvbuf that the trailers are being prepend
 * into is not NULL
 */
bool ___xdp2_pvbuf_append_pvbuf(struct xdp2_pvbuf_mgr *pvmgr,
				xdp2_paddr_t pvbuf_paddr, xdp2_paddr_t paddr,
				size_t length, bool compress)
{
	struct xdp2_pvbuf *pvbuf =
			__xdp2_pvbuf_paddr_to_addr(pvmgr, pvbuf_paddr);
	unsigned int num_iovecs =
			XDP2_PVBUF_NUM_IOVECS_FROM_PADDR(pvbuf_paddr);
	unsigned int index;
	bool last_set;

	XDP2_ASSERT(XDP2_PADDR_IS_PVBUF(paddr), "paddr is not a PVBuf in "
						"append pvbuf");
	index = __xdp2_pvbuf_find_append_slot(pvbuf, pvbuf_paddr, compress);
	if (index < num_iovecs) {
		xdp2_pvbuf_iovec_set_pvbuf_ent(pvbuf, index, paddr, length);
		return true;
	}

	last_set = xdp2_pvbuf_iovec_map_isset(pvbuf, num_iovecs - 1);

	return __xdp2_pvbuf_append_pvbuf_into_iovec(pvmgr, pvbuf,
			num_iovecs - 1, paddr, length, last_set, compress);
}

/* Backend function append packet trailers at the end of a pvbuf. This is
 * called when the last slot in the pvbuf that the trailers are being appended
 * into is not NULL
 */
bool ___xdp2_pvbuf_append_paddr_into_iovec(
		struct xdp2_pvbuf_mgr *pvmgr, struct xdp2_pvbuf *pvbuf,
		unsigned int index, xdp2_paddr_t paddr_1,
		xdp2_paddr_t paddr_2, size_t length, bool index_set,
		bool compress)
{
	struct xdp2_iovec *iovec = &pvbuf->iovec[index];

	if (index_set && xdp2_paddr_get_paddr_tag_from_paddr(iovec->paddr) ==
						XDP2_PADDR_TAG_PVBUF) {
		bool ret;

		/* There are no preceding empty slots, but the zeroth slot
		 * is already a pvbuf, recurse to push headers in that pvbuf
		 */
		ret = __xdp2_pvbuf_append_paddr(pvmgr, iovec->paddr, paddr_1,
						paddr_2, length, compress);
		if (!ret)
			return false;

		if (iovec->pbaddr.pvbuf.data_length) {
			/* The pvbuf at slot zero had a nonzero length, try to
			 * adjust it here
			 */
			if (length &&
			    iovec->pbaddr.pvbuf.data_length + length <
							XDP2_PVBUF_MAX_LEN)
				iovec->pbaddr.pvbuf.data_length += length;
			else
				iovec->pbaddr.pvbuf.data_length = 0;
		}
		return true;
	}

	/* Need to create a new pvbuf at the tail */
	return __xdp2_pvbuf_append_paddr_into_iovec_new_pvbuf(pvmgr,
			pvbuf, index, paddr_1, paddr_2, length,
			index_set, false);
}

/* Append trailers from a pbuf into a pvbuf */
bool ___xdp2_pvbuf_append_paddr(struct xdp2_pvbuf_mgr *pvmgr,
				xdp2_paddr_t pvbuf_paddr, xdp2_paddr_t paddr_1,
				xdp2_paddr_t paddr_2, size_t length,
				bool compress)
{
	struct xdp2_pvbuf *pvbuf =
			__xdp2_pvbuf_paddr_to_addr(pvmgr, pvbuf_paddr);
	unsigned int num_iovecs =
			XDP2_PVBUF_NUM_IOVECS_FROM_PADDR(pvbuf_paddr);
	bool is_long_addr = xdp2_paddr_get_paddr_tag_from_paddr(paddr_1) ==
					XDP2_PADDR_TAG_LONG_ADDR;
	unsigned int index;
	bool last_set;

	index = __xdp2_pvbuf_find_append_slot(pvbuf, pvbuf_paddr, compress);
	if (index < num_iovecs && (index != num_iovecs - 1 || !is_long_addr)) {
		xdp2_pvbuf_iovec_set_paddr_ent(pvbuf, index,
					       paddr_1, paddr_2, length);
		return true;
	}

	last_set = xdp2_pvbuf_iovec_map_isset(pvbuf, num_iovecs - 1);

	return ___xdp2_pvbuf_append_paddr_into_iovec(pvmgr, pvbuf,
						     num_iovecs - 1, paddr_1,
						     paddr_2, length,
						     last_set, compress);
}

/* Append trailers from a pbuf into pvbuf as part of the pulltail function
 * Note that the input paddr explicitly a pbuf for this function
 */
bool __xdp2_pvbuf_append_pbuf_pulltail(struct xdp2_pvbuf_mgr *pvmgr,
					xdp2_paddr_t pvbuf_paddr,
					xdp2_paddr_t paddr,
					size_t length)
{
	struct xdp2_pvbuf *pvbuf =
			__xdp2_pvbuf_paddr_to_addr(pvmgr, pvbuf_paddr);
	unsigned int num_iovecs =
			XDP2_PVBUF_NUM_IOVECS_FROM_PADDR(pvbuf_paddr);
	struct xdp2_iovec *iovec = &pvbuf->iovec[num_iovecs - 1];
	bool ret = true;
	unsigned int i;
	bool last_set;
	size_t iovlen;

	XDP2_ASSERT(XDP2_PADDR_IS_PBUF(paddr),
		    "Prepend a paddr for pullup paddr but paddr type "
		    "%s (%u) is not a pbuf",
		    xdp2_paddr_print_paddr_type(paddr),
		    xdp2_paddr_get_paddr_tag_from_paddr(paddr));

	i = __xdp2_pvbuf_find_append_slot(pvbuf, pvbuf_paddr, false);
	if (i < num_iovecs) {
		xdp2_pvbuf_iovec_set_pbuf_ent(pvbuf, i, paddr, length);
		return true;
	}

	last_set = xdp2_pvbuf_iovec_map_isset(pvbuf, num_iovecs - 2);

	if (!xdp2_pvbuf_iovec_map_isset(pvbuf, num_iovecs - 1))
		goto not_set;

	if (XDP2_IOVEC_IS_PVBUF(iovec)) {
		/* There are no tailing slots, but the last slot is already a
		 * pvbuf, recurse to append trailers in that pvbuf
		 */
		iovlen = iovec->pbaddr.pvbuf.data_length;
		ret = __xdp2_pvbuf_append_pvbuf_into_iovec(pvmgr, pvbuf,
							   num_iovecs - 2,
							   iovec->paddr, iovlen,
							   last_set, false);
	} else { /* Non-PVbuf paddr */
		size_t data_len;
		void *data_ptr;

		/* Need to create a new pvbuf at the tail */

		__xdp2_pvbuf_get_ptr_len_from_iovec(pvmgr, iovec, &data_ptr,
						    &data_len, false);
		return __xdp2_pvbuf_append_paddr_into_iovec_new_pvbuf(pvmgr,
				pvbuf, num_iovecs - 2, paddr, XDP2_PADDR_NULL,
				length, last_set, true);
	}

not_set:
	if (ret) {
		xdp2_pbuf_set_iovec(iovec, paddr, length);
		xdp2_pvbuf_iovec_map_set(pvbuf, num_iovecs - 1);
	}

	return ret;
}

/* Pop functions */
static inline void __xdp2_pvbuf_paddr_free_iovec(struct xdp2_pvbuf_mgr *pvmgr,
						 struct xdp2_pvbuf *pvbuf,
						 struct xdp2_iovec *iovec,
						 unsigned int index)
{
	XDP2_ASSERT(iovec->paddr, "Free iovec iovec[%u] is NULL", index);

	switch (xdp2_iovec_paddr_addr_tag(iovec)) {
	case XDP2_PADDR_TAG_PBUF:
	case XDP2_PADDR_TAG_PBUF_1REF:
		__xdp2_pbuf_free(pvmgr, iovec->paddr);
		break;
	case XDP2_PADDR_TAG_ALL_SHORT_ADDR_CASES:
		__xdp2_paddr_short_addr_free(pvmgr, iovec->paddr);
		break;
	case XDP2_PADDR_TAG_LONG_ADDR:
		__xdp2_paddr_long_addr_free(pvmgr, iovec[0].paddr,
					    iovec[1].paddr);
		/* Unset second iovec for long address */
		xdp2_pvbuf_iovec_unset_ent(pvbuf, index + 1);
		break;
	default:
		XDP2_IOVEC_PADDR_TYPE_DEFAULT_ERR(iovec, "Free iovec");
	}

	xdp2_pvbuf_iovec_unset_ent(pvbuf, index);
}

/* Pop header bytes from a non-pvbuf paddr iovec */
static size_t __xdp2_pvbuf_pop_from_paddr_iovec(struct xdp2_pvbuf_mgr *pvmgr,
						struct xdp2_pvbuf *pvbuf,
						unsigned int index,
						size_t num_drop_bytes,
						void **copyptr)
{
	struct xdp2_iovec *iovec = &pvbuf->iovec[index];
	size_t data_len;
	void *data_ptr;

	__xdp2_pvbuf_get_ptr_len_from_iovec(pvmgr, iovec, &data_ptr,
					    &data_len, false);

	if (num_drop_bytes >= data_len) {
		/* The number of bytes to drop is greater than or equal to the
		 * length in the iovec, pbuf and remove it from the iovec
		 */
		if (copyptr) {
			memcpy(*copyptr, data_ptr, data_len);
			*copyptr += data_len;
		}
		num_drop_bytes -= data_len;

		__xdp2_pvbuf_paddr_free_iovec(pvmgr, pvbuf, iovec, index);
	} else {
		if (copyptr) {
			memcpy(*copyptr, data_ptr, num_drop_bytes);
			*copyptr += num_drop_bytes;
		}
		/* The number of bytes to drop is less than the length of the
		 * iovec. Remove the number of bytes to drop from the iovec by
		 * advancing the address by the number to drop and subtracting
		 * the number to drop from the iovec length
		 */
		XDP2_ASSERT(iovec->paddr, "Pop from paddr iovec[%u] is NULL",
			    index);

		switch (xdp2_iovec_paddr_addr_tag(iovec)) {
		case XDP2_PADDR_TAG_PBUF:
		case XDP2_PADDR_TAG_PBUF_1REF:
			/* iovec->length may be zero and the buffer tag is
			 * address may be 15 such that the real length is
			 * 1 << 20. This subtraction yields the correct result
			 * in that case. Also, note that the result of this
			 * subtraction is guaranteed to be greater than zero so
			 * we don't need to worry about setting the field to
			 * zero if the buffer tag is 15
			 */
			iovec->pbaddr.pbuf.offset += num_drop_bytes;
			iovec->pbaddr.pbuf.data_length -= num_drop_bytes;
			break;
		case XDP2_PADDR_TAG_ALL_SHORT_ADDR_CASES:
			iovec->pbaddr.short_addr.offset += num_drop_bytes;
			iovec->pbaddr.short_addr.data_length -= num_drop_bytes;
			break;
		case XDP2_PADDR_TAG_LONG_ADDR: {
			size_t offset;

			offset = xdp2_paddr_long_addr_get_offset_from_paddr(
					iovec[0].paddr, iovec[1].paddr);
			offset += num_drop_bytes;
			xdp2_paddr_long_addr_set_offset_from_paddr(
					&iovec[0].paddr, &iovec[1].paddr,
					offset);
			iovec->pbaddr.long_addr.data_length -= num_drop_bytes;
			break;
		default:
			XDP2_IOVEC_PADDR_TYPE_DEFAULT_ERR(iovec,
							  "Pop from paddr");
		}
		}

		num_drop_bytes = 0;
	}

	return num_drop_bytes;
}

/* Pop the headers as some number of bytes from one iovec holding a pvbuf */
static size_t __xdp2_pvbuf_pop_from_pvbuf_iovec(struct xdp2_pvbuf_mgr *pvmgr,
						struct xdp2_pvbuf *pvbuf,
						unsigned int index,
						size_t num_drop_bytes,
						bool compress, void **copyptr)
{
	struct xdp2_iovec *iovec = &pvbuf->iovec[index];
	xdp2_paddr_t paddr = iovec->paddr;
	struct xdp2_pvbuf *pvbuf1 = __xdp2_pvbuf_paddr_to_addr(pvmgr, paddr);
	unsigned int num_iovecs = XDP2_PVBUF_NUM_IOVECS_FROM_PADDR(paddr);
	size_t pvbuf_data_len = xdp2_pvbuf_get_data_len_from_iovec(iovec);
	size_t dropped_bytes;
	int i;

	if (pvbuf_data_len) {
		if (num_drop_bytes >= pvbuf_data_len) {
			if (!copyptr) {
				/* We know length of the pvbuf in the iovec and
				 * it's less than or equal to the number of
				 * bytes to drop, so just free the pvbuf and
				 * set the iovec to NULL
				 */
				__xdp2_pvbuf_free(pvmgr, paddr);
			} else {
				dropped_bytes = __xdp2_pvbuf_pop_hdrs(
					pvmgr, paddr, num_drop_bytes,
					compress, copyptr);
				XDP2_ASSERT(dropped_bytes ==
					iovec->pbaddr.pvbuf.data_length,
					     "Dropped bytes does not equal "
					     "iovec->length: %lu != %u",
					     dropped_bytes,
					     iovec->pbaddr.pvbuf.data_length);

				__xdp2_pvbuf_free(pvmgr, paddr);
			}

			num_drop_bytes -= pvbuf_data_len;
			xdp2_pvbuf_iovec_unset_ent(pvbuf, index);
		} else {
			/* The iovec length is non-zero and the number of bytes
			 * being dropped is less than the length in the iovec.
			 * Recursively call the pvbuf pop headers function
			 * to drop the headers in the pvbuf. The return value,
			 * i.e. the number of bytes actually dropped, must
			 * be equal to the number of bytes being dropped (if
			 * their no equal then the length in the iovec for the
			 * pvbuf was corrupted
			 */
			dropped_bytes = __xdp2_pvbuf_pop_hdrs(pvmgr, paddr,
							       num_drop_bytes,
							       compress,
							       copyptr);

			XDP2_ASSERT(dropped_bytes == num_drop_bytes,
				     "Removing bytes from a pvbuf iovec with "
				     "an iovec length greater then the number "
				     "of bytes being dropped, but the number "
				     "of bytes actually dropped doesn't equal "
				     "the number being dropped: %lu != %lu",
				     dropped_bytes, num_drop_bytes);

			iovec->pbaddr.pvbuf.data_length -= dropped_bytes;
			num_drop_bytes -= dropped_bytes;

			if (compress)
				__xdp2_pvbuf_check_single(pvmgr, iovec);
		}

		return num_drop_bytes;
	}

	/* pvbuf_data_len is zero */

	dropped_bytes = __xdp2_pvbuf_pop_hdrs(pvmgr, paddr, num_drop_bytes,
					      compress, copyptr);

	/* The embedded pvbuf might be empty now, but we don’t know for
	 * sure so we can’t free it
	 */
	if (dropped_bytes < num_drop_bytes) {
		/* The number of bytes dropped from the pvbuf is less than the
		 * number requested, this means that the total length of the
		 * pvbuf is now zero so we can free it
		 */
		xdp2_pvbuf_iovec_unset_ent(pvbuf, index);
		__xdp2_pvbuf_free(pvmgr, paddr);
	} else {
		XDP2_ASSERT(dropped_bytes == num_drop_bytes,
			    "Number of dropped bytes is greater than the "
			    "number of bytes to drop: %lu != %lu",
			     dropped_bytes, num_drop_bytes);

		/* All the requested bytes have been dropped from the pvbuf,
		 * but it can only be freed if its total length is no zero.
		 * Check this by scanning the iovecs and see if all the
		 * addresses are now zero
		 */
		i = xdp2_pvbuf_iovec_map_find(pvbuf1);
		if (i >= num_iovecs) {
			xdp2_pvbuf_iovec_unset_ent(pvbuf, index);
			__xdp2_pvbuf_free(pvmgr, paddr);
		}
	}

	num_drop_bytes -= dropped_bytes;

	return num_drop_bytes;
}

/* If a sub-iovec only contains one entry then it can be raised up a level to
 * replace its parent iovec as a form of compression
 */
static void __xdp2_pvbuf_try_iovec_uplevel(struct xdp2_pvbuf_mgr *pvmgr,
					   struct xdp2_pvbuf *pvbuf,
					   unsigned int num_iovecs)
{
	unsigned int pvbuf_size, old_num_iovecs, i, j = 0;
	struct xdp2_pvbuf *old_pvbuf;

	if (xdp2_pvbuf_iovec_map_weight(pvbuf) != 1)
		return;

	i = xdp2_pvbuf_iovec_map_find(pvbuf);

	XDP2_ASSERT(i < num_iovecs, "Bad index %u try iovec uplevel", i);

	if (!XDP2_IOVEC_IS_PVBUF(&pvbuf->iovec[i]))
		return;

	old_num_iovecs = XDP2_PVBUF_NUM_IOVECS_FROM_PADDR(
						pvbuf->iovec[i].paddr);
	if (old_num_iovecs > num_iovecs)
		return;

	old_pvbuf = __xdp2_pvbuf_paddr_to_addr(pvmgr, pvbuf->iovec[i].paddr);
	pvbuf_size = xdp2_pvbuf_get_size_from_paddr(pvbuf->iovec[i].paddr);

	xdp2_pvbuf_iovec_clear_ent(pvbuf, i);

	i = 0;
	xdp2_pvbuf_iovec_map_foreach(old_pvbuf, i) {
		pvbuf->iovec[j] = old_pvbuf->iovec[i];
		xdp2_pvbuf_iovec_map_set(pvbuf, j);
		xdp2_pvbuf_iovec_clear_ent(old_pvbuf, i);
		j++;
	}

	/* Free the old pvbuf structure, but not the
	 * contents
	 */
	xdp2_obj_alloc_free(XDP2_PVBUF_GET_ALLOCATOR(pvmgr, pvbuf_size),
			    old_pvbuf);
}

/* Pop headers from the front of a pvbuf. The target pvbuf is in the pvbuf
 * paddr argument, and number of bytes to drop is in num_drop_bytes. The pvbuf
 * is managed by the packet buffer manager in the pvmgr argument
 *
 * Return the number of bytes actually dropped
 */
size_t __xdp2_pvbuf_pop_hdrs(struct xdp2_pvbuf_mgr *pvmgr,
			     xdp2_paddr_t paddr, size_t num_drop_bytes,
			     bool compress, void **copyptr)
{
	struct xdp2_pvbuf *pvbuf = __xdp2_pvbuf_paddr_to_addr(pvmgr, paddr);
	unsigned int num_iovecs = XDP2_PVBUF_NUM_IOVECS_FROM_PADDR(paddr);
	size_t orig_num_drop_bytes = num_drop_bytes;
	unsigned int i = 0;

	if (num_drop_bytes == 0)
		return 0;

	xdp2_pvbuf_iovec_map_foreach(pvbuf, i) {
		struct xdp2_iovec *iovec = &pvbuf->iovec[i];

		XDP2_ASSERT(iovec->paddr, "Pop hdrs iovec[%u] is NULL", i);

		switch (xdp2_iovec_paddr_addr_tag(iovec)) {
		case XDP2_PADDR_TAG_PVBUF:
			num_drop_bytes = __xdp2_pvbuf_pop_from_pvbuf_iovec(
				pvmgr, pvbuf, i, num_drop_bytes,
				compress, copyptr);
			if (compress && XDP2_IOVEC_IS_PVBUF(iovec))
				__xdp2_pvbuf_check_single(pvmgr, iovec);
			break;
		case XDP2_PADDR_TAG_LONG_ADDR:
			num_drop_bytes = __xdp2_pvbuf_pop_from_paddr_iovec(
				pvmgr, pvbuf, i, num_drop_bytes, copyptr);
			i++;
			break;
		case XDP2_PADDR_TAG_PBUF:
		case XDP2_PADDR_TAG_PBUF_1REF:
		case XDP2_PADDR_TAG_ALL_SHORT_ADDR_CASES:
			num_drop_bytes = __xdp2_pvbuf_pop_from_paddr_iovec(
				pvmgr, pvbuf, i, num_drop_bytes, copyptr);
			break;
		default:
			XDP2_IOVEC_PADDR_TYPE_DEFAULT_ERR(iovec,
							  "Pop headers");
		}

		if (num_drop_bytes <= 0)
			break;
	}

	if (compress)
		__xdp2_pvbuf_try_iovec_uplevel(pvmgr, pvbuf, num_iovecs);

	return orig_num_drop_bytes - num_drop_bytes;
}

/* Pop the trailers as some number of bytes from one iovec holding a pbuf */
static size_t __xdp2_pvbuf_pop_trailers_from_paddr_iovec(
		struct xdp2_pvbuf_mgr *pvmgr, struct xdp2_pvbuf *pvbuf,
		unsigned int index, size_t num_drop_bytes, void **copyptr)
{
	struct xdp2_iovec *iovec = &pvbuf->iovec[index];
	size_t data_len;
	void *data_ptr;

	__xdp2_pvbuf_get_ptr_len_from_iovec(pvmgr, iovec, &data_ptr,
					    &data_len, false);

	if (num_drop_bytes >= data_len) {
		/* The number of bytes to drop is greater than or equal to the
		 * length in the iovec, pbuf and remove it from the iovec
		 */
		if (copyptr) {
			*copyptr -= data_len;
			memcpy(*copyptr, data_ptr, data_len);
		}
		num_drop_bytes -= data_len;

		__xdp2_pvbuf_paddr_free_iovec(pvmgr, pvbuf, iovec, index);

		return num_drop_bytes;
	}

	/* Else num_drop_bytes < data_len */

	if (copyptr) {
		*copyptr -= num_drop_bytes;
		memcpy(*copyptr, data_ptr + data_len -
					num_drop_bytes, num_drop_bytes);
	}

	XDP2_ASSERT(iovec->paddr, "Pop trailers for paddrd  iovec[%u] is NULL",
		    index);

	switch (xdp2_iovec_paddr_addr_tag(iovec)) {
	case XDP2_PADDR_TAG_PBUF:
	case XDP2_PADDR_TAG_PBUF_1REF:
		/* iovec->length may be zero and the buffer tag is address may
		 * be 15 such that the real length is 1 << 20. This subtraction
		 * yields the correct result in that case. Also, note that the
		 * result of this subtraction is guaranteed to be greater than
		 * zero so we don't need to worry about setting the field to
		 * zero if the buffer tag is 15
		 */
		iovec->pbaddr.pbuf.data_length -= num_drop_bytes;
		break;
	case XDP2_PADDR_TAG_ALL_SHORT_ADDR_CASES:
		iovec->pbaddr.short_addr.data_length -= num_drop_bytes;
		break;
	case XDP2_PADDR_TAG_LONG_ADDR:
		iovec->pbaddr.long_addr.data_length -= num_drop_bytes;
		break;
	default:
		XDP2_IOVEC_PADDR_TYPE_DEFAULT_ERR(iovec,
						  "Pop trailers from paddr");
	}

	return 0;
}

/* Pop the trailers as some number of bytes from one iovec holding a pvbuf */
static size_t __xdp2_pvbuf_pop_trailers_from_pvbuf_iovec(
		struct xdp2_pvbuf_mgr *pvmgr, struct xdp2_pvbuf *pvbuf,
		unsigned int index, size_t num_drop_bytes, bool compress,
		void **copyptr)
{
	struct xdp2_iovec *iovec = &pvbuf->iovec[index];
	xdp2_paddr_t paddr = iovec->paddr;
	struct xdp2_pvbuf *pvbuf1 = __xdp2_pvbuf_paddr_to_addr(pvmgr, paddr);
	unsigned int num_iovecs = XDP2_PVBUF_NUM_IOVECS_FROM_PADDR(paddr);
	size_t pvbuf_data_len = xdp2_pvbuf_get_data_len_from_iovec(iovec);
	size_t dropped_bytes;

	if (pvbuf_data_len) {
		if (num_drop_bytes >= pvbuf_data_len) {
			if (!copyptr) {
				/* We know length of the pvbuf in the iovec and
				 * it's less than or equal to the number of
				 * bytes to drop, so just free the pvbuf and
				 * set the iovec to NULL
				 */
				__xdp2_pvbuf_free(pvmgr, paddr);
				dropped_bytes = iovec->pbaddr.pvbuf.data_length;
			} else {
				dropped_bytes = __xdp2_pvbuf_pop_trailers(
					pvmgr, paddr, num_drop_bytes,
					compress, copyptr);
				XDP2_ASSERT(dropped_bytes ==
						iovec->pbaddr.pvbuf.data_length,
					     "Dropped bytes in pop trailers "
					     "does not equal iovec->length, "
					     "%lu != %u\n",
					     dropped_bytes,
					     iovec->pbaddr.pvbuf.data_length);

				__xdp2_pvbuf_free(pvmgr, paddr);
			}

			num_drop_bytes -= dropped_bytes;
			xdp2_pvbuf_iovec_unset_ent(pvbuf, index);
		} else {
			/* The iovec length is non-zero and the number of bytes
			 * being dropped is less than the length in the iovec.
			 * Recursively call the pvbuf pop headers function
			 * to drop the headers in the pvbuf. The return value,
			 * i.e. the number of bytes actually dropped, must
			 * be equal to the number of bytes being dropped (if
			 * their no equal then the length in the iovec for the
			 * pvbuf was corrupted
			 */
			dropped_bytes = __xdp2_pvbuf_pop_trailers(
					pvmgr, paddr, num_drop_bytes,
					compress, copyptr);

			XDP2_ASSERT(dropped_bytes == num_drop_bytes,
				     "Removing bytes from a pvbuf iovec with "
				     "an iovec length greater then the number "
				     "of bytes being dropped, but the number "
				     "of bytes actually dropped doesn't equal "
				     "the number being dropped: %lu != %lu",
				     dropped_bytes, num_drop_bytes);

			iovec->pbaddr.pvbuf.data_length -= dropped_bytes;
			num_drop_bytes -= dropped_bytes;

			if (compress)
				__xdp2_pvbuf_check_single(pvmgr, iovec);
		}

		return num_drop_bytes;
	}

	/* Else PVbuf data length is zero */

	dropped_bytes = __xdp2_pvbuf_pop_trailers(pvmgr, paddr,
						  num_drop_bytes,
						  compress, copyptr);

	/* The embedded pvbuf might be empty now, but we don’t know for sure
	 * so we can’t free it
	 */
	if (dropped_bytes < num_drop_bytes) {
		/* The number of bytes dropped from the pvbuf is less than the
		 * number requested, this means that the total length of the
		 * pvbuf is now zero so we can free it
		 */
		xdp2_pvbuf_iovec_unset_ent(pvbuf, index);
		__xdp2_pvbuf_free(pvmgr, paddr);
	} else {
		unsigned int index2;

		XDP2_ASSERT(dropped_bytes == num_drop_bytes,
			    "Number of dropped bytes is greater than the "
			    "number of bytes to drop: %lu != %lu",
			    dropped_bytes, num_drop_bytes);

		/* All the requested bytes have been dropped from the pvbuf,
		 * but it can only be freed if its total length is not zero.
		 * Check this by scanning the iovecs and see if all the
		 * addresses are now zero
		 */
		index2 = xdp2_pvbuf_iovec_map_find(pvbuf1);
		if (index2 >= num_iovecs) {
			__xdp2_pvbuf_free(pvmgr, paddr);
			xdp2_pvbuf_iovec_unset_ent(pvbuf, index);
		}
	}

	num_drop_bytes -= dropped_bytes;

	return num_drop_bytes;
}

/* Clone functions */

/* Walk the iovecs of a pvbuf and fix lengths of pvbufs to be correct. This
 * is called when performing a clone and the requested number of bytes is
 * less than the actual number in the pvbuf that can be cloned. We need
 * to do this since the iterator functions proactively set iovec lengths
 * under the assumption that all the requested number of bytes will be cloned
 */
static size_t __xdp2_pvbuf_fixup_iovec_length(struct xdp2_pvbuf_mgr *pvmgr,
					       xdp2_paddr_t paddr)
{
	struct xdp2_pvbuf *pvbuf = __xdp2_pvbuf_paddr_to_addr(pvmgr, paddr);
	size_t len = 0, rlen;
	int i = 0;

	xdp2_pvbuf_iovec_map_foreach(pvbuf, i) {
		struct xdp2_iovec *iovec = &pvbuf->iovec[i];

		XDP2_ASSERT(iovec->paddr, "Fixup iovec length iovec[%u] "
					  "is NULL", i);

		switch (xdp2_iovec_paddr_addr_tag(iovec)) {
		case XDP2_PADDR_TAG_PVBUF:
			rlen = __xdp2_pvbuf_fixup_iovec_length(pvmgr,
								iovec->paddr);
			len += rlen;

			rlen = rlen < XDP2_PVBUF_MAX_LEN ? rlen : 0;
			xdp2_pbuf_set_iovec(iovec, paddr, rlen);
		case XDP2_PADDR_TAG_PBUF:
		case XDP2_PADDR_TAG_PBUF_1REF:
			len += xdp2_pbuf_get_data_len_from_iovec(iovec);
			break;
		case XDP2_PADDR_TAG_ALL_SHORT_ADDR_CASES:
			len += xdp2_paddr_short_addr_get_data_len_from_iovec(
									iovec);
			break;
		case XDP2_PADDR_TAG_LONG_ADDR:
			len +=
			    xdp2_paddr_long_addr_get_data_len_from_iovec(iovec);
			break;
		default:
			XDP2_IOVEC_PADDR_TYPE_DEFAULT_ERR(iovec,
							"Fixup iovec length");
		}
	}
	return len;
}

bool __xdp2_check_clone_add_iovec(struct __xdp2_pvbuf_iter_clone *ist,
				  bool set_length, unsigned int pvbuf_size)
{
	struct xdp2_pvbuf *new_pvbuf;
	xdp2_paddr_t new_pvbuf_paddr;
	size_t len;

	/* Need to create another pvbuf. Link it into the current one as the
	 * last entry
	 */

	new_pvbuf_paddr = ___xdp2_pvbuf_alloc(ist->pvmgr, pvbuf_size,
					       &new_pvbuf, &pvbuf_size);
	if (!new_pvbuf_paddr) {
		ist->error = -ENOMEM;
		return false;
	}

	len = (set_length && ist->length < XDP2_PVBUF_MAX_LEN) ?
							ist->length : 0;

	xdp2_pvbuf_iovec_set_pvbuf_ent(ist->pvbuf, ist->num_iovecs - 1,
					new_pvbuf_paddr, len);

	ist->index = 0;
	ist->pvbuf = new_pvbuf;
	ist->num_iovecs = XDP2_PVBUF_NUM_IOVECS(pvbuf_size);

	return true;
}

/* Common helper function for a clone iterator. This does the following:
 *	- Check is the iterator is done (i.e. ist->length == 0)
 *	- Determine the data length from the iovec. If there's no
 *	  data length then return to skip
 *	- Check if ist->offset is greater or equal to data length. If it is
 *	  then subtract data length from ist->offset and return to skip
 *	- If the iovec is a single reference pbuf then convert it to
 *	  a multi-reference one for cloning
 *	- If there is residual offset (i.e. ist->offset is greater than
 *	  zero but less than data length) then advance the offset in the
 *	  paddr to be returned (not the original iovec)
 *	- If ass_pvbuf is true an there's no more space in the iovec array
 *	  then add a new pvbuf to the pvbuf under construction
 *
 *	Returns 0 if iovec is not skipped, 1 if the iovec is skipped but
 *	the iterator should continue, and -1 if the iovec is skipped and
 *	the iterator should abort
 *
 *	If 0 is returned then *pbaddr_1 and *pbaddr_2 contain the adjust
 *	paddrs with ist->offset applied, *data_len contains the data length
 *	for cloning, and num_slots contains the number of iovecs for the
 *	paddr (2 for long address paddrs, 1 for the others)
 */
int __xdp2_pvbuf_iterate_iovec_clone_common(
		struct __xdp2_pvbuf_iter_clone *ist,
		struct xdp2_iovec *iovec, struct xdp2_paddr *pbaddr_1,
		struct xdp2_paddr *pbaddr_2, size_t *data_len,
		unsigned int *num_slots)
{
	*num_slots = 1;

	if (!ist->length)
		return -1;

	pbaddr_2->paddr = XDP2_PADDR_NULL;

	XDP2_ASSERT(iovec->paddr, "Iovec clone common iovec is NULL");

	switch (xdp2_iovec_paddr_addr_tag(iovec)) {
	case XDP2_PADDR_TAG_PBUF:
	case XDP2_PADDR_TAG_PBUF_1REF:
		*data_len = xdp2_pbuf_get_data_len_from_iovec(iovec);
		break;
	case XDP2_PADDR_TAG_ALL_SHORT_ADDR_CASES:
		*data_len = xdp2_paddr_short_addr_get_data_len_from_iovec(
									iovec);
		break;
	case XDP2_PADDR_TAG_LONG_ADDR:
		*data_len = xdp2_paddr_long_addr_get_data_len_from_iovec(iovec);
		pbaddr_2->paddr = iovec[1].paddr;
		(*num_slots)++;
		break;
	default:
		XDP2_IOVEC_PADDR_TYPE_DEFAULT_ERR(iovec,
						  "Iterate clone common");
		break;
	}

	if (!*data_len)
		return 1;

	if (ist->offset && ist->offset >= *data_len) {
		/* Skipping over offset */
		ist->offset -= *data_len;
		return 1;
	}

	if (xdp2_iovec_paddr_addr_tag(iovec) == XDP2_PADDR_TAG_PBUF_1REF) {
		/* Cloning as pbuf marked as having a single reference.
		 * We need to change the pbuf type on the input iovec
		 */
		XDP2_ASSERT(!xdp2_pbuf_get_refcnt(ist->pvmgr, iovec->paddr),
			    "Cloning pbuf with single refcnt, "
			    "but refcnt is %u",
			    xdp2_pbuf_get_refcnt(ist->pvmgr, iovec->paddr));

		iovec->pbaddr.paddr_tag = XDP2_PADDR_TAG_PBUF;

		/* Give pbuf an initial reference count */
		__xdp2_pbuf_bump_refcnt(ist->pvmgr, iovec->paddr);
		ist->pvmgr->conversions++;
	}

	pbaddr_1->paddr = iovec->paddr;

	if (ist->offset) {
		__xdp2_paddr_add_to_offset(pbaddr_1, pbaddr_2, ist->offset);
		*data_len -= ist->offset;
		ist->offset = 0;
	}

	/* data_len will not be zero here */
	*data_len = xdp2_min(ist->length, *data_len);

	return 0;
}

/* Iterator callback function for cloning a pvbuf */
static bool __xdp2_pvbuf_iterate_clone_pvbuf(void *priv,
					     struct xdp2_iovec *iovec)
{
	struct __xdp2_pvbuf_iter_clone *ist =
				(struct __xdp2_pvbuf_iter_clone *)priv;
	struct xdp2_paddr pbaddr_1, pbaddr_2;
	unsigned int num_slots, max_index;
	bool is_long_addr;
	size_t data_len;
	int ret;

	ret = __xdp2_pvbuf_iterate_iovec_clone_common(ist, iovec,
			&pbaddr_1, &pbaddr_2, &data_len, &num_slots);
	if (ret)
		return (ret == 1);

	is_long_addr = xdp2_paddr_get_paddr_tag_from_paddr(pbaddr_1.paddr) ==
						XDP2_PADDR_TAG_LONG_ADDR;

	max_index = ist->num_iovecs - num_slots;
	if (ist->index > max_index ||
	   (ist->index == max_index && data_len != ist->length)) {
		/* Proactively set the length, second argument to
		 * __xdp2_check_clone_add_iovec is true,  under the assumption
		 * that the rest of the length will be cloned under the pvbuf
		 * we're adding here. If it's wrong we'll fix all the lengths
		 * when returning to the top level clone call
		 */
		if (!__xdp2_check_clone_add_iovec(ist, true,
					__xdp2_pvbuf_get_size(ist->pvmgr, 0)))
			return -1;
	}
	/* Time to clone paddr. Set in the pvbuf being build and take a
	 * reference for the cloned buffer
	 */
	xdp2_pvbuf_iovec_set_paddr_ent_and_bump_refcnt(ist->pvmgr,
						       ist->pvbuf, ist->index,
						       pbaddr_1.paddr,
						       pbaddr_2.paddr,
						       data_len);

	ist->length -= data_len;
	ist->index += 1 + is_long_addr;

	return true;
}

/* Frontend function to clone a pvbuf. Kick off a paddr iterator */
xdp2_paddr_t __xdp2_pvbuf_clone(struct xdp2_pvbuf_mgr *pvmgr,
				xdp2_paddr_t pvbuf_paddr,
				size_t offset, size_t length, size_t *retlen)
{
	unsigned int pvbuf_size = __xdp2_pvbuf_get_size(pvmgr, 0);
	struct __xdp2_pvbuf_iter_clone ist;
	struct xdp2_pvbuf *pvbuf;
	xdp2_paddr_t paddr;

	/* Allocate initial pvbuf */
	paddr = ___xdp2_pvbuf_alloc(pvmgr, pvbuf_size, &pvbuf, &pvbuf_size);
	if (!paddr)
		return XDP2_PADDR_NULL;

	ist.pvmgr = pvmgr;
	ist.length = length;
	ist.offset = offset;
	ist.num_iovecs = XDP2_PVBUF_NUM_IOVECS(pvbuf_size);
	ist.pvbuf = pvbuf;
	ist.index = 0;
	ist.error = 0;

	/* Iterate, this will populate the pvbuf and create linked pvbufs as
	 * needed
	 */
	__xdp2_pvbuf_iterate_iovec(pvmgr, pvbuf_paddr,
				    __xdp2_pvbuf_iterate_clone_pvbuf, &ist);

	if (ist.error) {
		/* Iterator failed with negative error code. Free the
		 * incomplete pvbuf that was constructed
		 */
		__xdp2_pvbuf_free(pvmgr, paddr);
		*retlen = 0;
		return XDP2_PADDR_NULL;
	}

	*retlen = length - ist.length;

	if (ist.length) {
		/* There were fewer bytes in the pvbuf than the number
		 * requested to clone. Fix up the lengths of any iovecs
		 * that were set under the assumption that all the requested
		 * length would be cloned
		 */
		__xdp2_pvbuf_fixup_iovec_length(pvmgr, paddr);
	}

	return paddr;
}

/* Pop trailers from the end of a pvbuf. The target pvbuf is in the pvbuf
 * argument, and number of bytes to drop is in num_drop_bytes. The pvbuf is
 * managed by the packet buffer manager in the pvmgr argument
 *
 * Return the number of bytes actually dropped
 */
size_t __xdp2_pvbuf_pop_trailers(struct xdp2_pvbuf_mgr *pvmgr,
				 xdp2_paddr_t paddr, size_t num_drop_bytes,
				 bool compress, void **copyptr)
{
	struct xdp2_pvbuf *pvbuf = __xdp2_pvbuf_paddr_to_addr(pvmgr, paddr);
	unsigned int num_iovecs = XDP2_PVBUF_NUM_IOVECS_FROM_PADDR(paddr);
	size_t orig_num_drop_bytes = num_drop_bytes;
	unsigned int i = num_iovecs - 1;

	if (num_drop_bytes == 0)
		return 0;

	xdp2_pvbuf_iovec_map_rev_foreach(pvbuf, i) {
		struct xdp2_iovec *iovec = &pvbuf->iovec[i];

		XDP2_ASSERT(iovec->paddr, "Bitmap is set for %u but slot is "
					  "NULL", i);
		switch (xdp2_iovec_paddr_addr_tag(iovec)) {
		case XDP2_PADDR_TAG_PVBUF:
			num_drop_bytes =
				__xdp2_pvbuf_pop_trailers_from_pvbuf_iovec(
					pvmgr, pvbuf, i, num_drop_bytes,
					compress, copyptr);
			break;
		case XDP2_PADDR_TAG_LONG_ADDR:
			i--;

			/* Fallthrough */
		case XDP2_PADDR_TAG_PBUF:
		case XDP2_PADDR_TAG_PBUF_1REF:
		case XDP2_PADDR_TAG_ALL_SHORT_ADDR_CASES:
			num_drop_bytes =
				__xdp2_pvbuf_pop_trailers_from_paddr_iovec(
					pvmgr, pvbuf, i, num_drop_bytes,
					copyptr);
			break;
		default:
			XDP2_IOVEC_PADDR_TYPE_DEFAULT_ERR(iovec,
							  "Pop trailers");
		}

		if (!num_drop_bytes)
			break;;
	}

	if (compress)
		__xdp2_pvbuf_try_iovec_uplevel(pvmgr, pvbuf, num_iovecs);

	return orig_num_drop_bytes - num_drop_bytes;
}

/* Initialization functions */

/* Clear all the resources assigned to a pvmgr. This is primarily used to
 * cleanup a pvmgr that is partially initialized but then there is a failure
 * in initialization
 */
static void __xdp2_clear_pvmgr(struct xdp2_pvbuf_mgr *pvmgr)
{
	struct xdp2_pbuf_allocator_entry *entry;
	struct xdp2_pbuf_allocator *pallocator;
	int i;

	for (i = 0; i < XDP2_PVBUF_NUM_SIZES; i++) {
		struct xdp2_obj_allocator *allocator =
				pvmgr->pvbuf_allocator_table[i].allocator;

		if (!allocator)
			continue;

		if (allocator->base &&
		    pvmgr->alloced_pvbufs_base) {
			/* The base of the allocator points to the object
			 * created for the allocator in the call to
			 * xdp2_obj_alloc_fifo_init. There is currently no
			 * function to destroy a fifo, so free them here
			 */
			free(allocator->base);
		}
		if (pvmgr->alloced_pvbuf_allocator[i])
			free(allocator);
	}

	for (i = 0; i < XDP2_PBUF_NUM_SIZE_SHIFTS; i++) {
		entry = &pvmgr->pbuf_allocator_table[i];
		if (entry->alloc_size_shift !=
				xdp2_pbuf_buffer_tag_to_size_shift(i) ||
		    !entry->pallocator)
			continue;

		/* Free the entries in the table that "own" and allocator */

		pallocator = XDP2_PVBUF_GET_ADDRESS(pvmgr,
						     entry->pallocator);

		if (pallocator->allocator) {
			if (pallocator->allocator->base &&
			    pallocator->alloced_base) {
				/* The base of the allocator points to the
				 * object created for the allocator in the
				 * call to xdp2_obj_alloc_fifo_init. There is
				 * currently no function to destroy a fifo,
				 * so free them here
				 */
				free(pallocator->allocator->base);
			}
			if (pallocator->alloced_allocator)
				free(pallocator->allocator);
		}
		if (pallocator->alloced_this_pallocator)
			free(pallocator);
	}
}

/* Initialize the pbuf allocators */
static int __xdp2_pvbuf_init_pbuf_allocators(struct xdp2_pvbuf_mgr *pvmgr,
		struct xdp2_pbuf_init_allocator *pbuf_allocs,
		const char *name)
{
	struct xdp2_pbuf_allocator *pallocator, *smaller_pallocator;
	char pname[XDP2_OBJ_ALLOC_NAME_LEN + 1];
	struct xdp2_pbuf_allocator_entry *entry;
	struct xdp2_obj_allocator *allocator;
	unsigned int smaller_size_shift;
	int i, j, last = -1;
	size_t smaller_size;
	void *pbufs_base;

	/* Allocate a pbuf allocator for another entries in the shift array
	 * that are nonzero
	 */
	for (i = 0; i < XDP2_PBUF_NUM_SIZE_SHIFTS; i++) {
		if (!pbuf_allocs->obj[i].num_objs)
			continue;

		if (!pbuf_allocs->obj[i].pallocator) {
			/* Allocate a pbuf allocator plus an atomic uint as a
			 * reference count for each object
			 */
			pallocator = calloc(1,
				sizeof(struct xdp2_pbuf_allocator) +
						sizeof(atomic_uint) *
						pbuf_allocs->obj[i].num_objs);
			if (!pallocator)
				return -ENOMEM;
			pallocator->alloced_this_pallocator = true;
		} else {
			pallocator = pbuf_allocs->obj[i].pallocator;
			pallocator->alloced_this_pallocator = false;
		}

		if (!pbuf_allocs->obj[i].allocator) {
			/* Allocate the allocator with and object allocator
			 * FIFO
			 */
			allocator = malloc(XDP2_OBJ_ALLOC_SIZE(
						pbuf_allocs->obj[i].num_objs));
			if (!allocator)
				return -ENOMEM;
			pallocator->alloced_allocator = true;
		} else {
			allocator = pbuf_allocs->obj[i].allocator;
			pallocator->alloced_allocator = false;
		}

		pallocator->allocator = allocator;

		if (!pbuf_allocs->obj[i].base) {
			pbufs_base = calloc(pbuf_allocs->obj[i].num_objs,
					xdp2_pbuf_buffer_tag_to_size(i));
			if (!pbufs_base)
				return -ENOMEM;
			pallocator->alloced_base = true;
		} else {
			pbufs_base = pbuf_allocs->obj[i].base;
			pallocator->alloced_base = false;
		}

		snprintf(pname, XDP2_OBJ_ALLOC_NAME_LEN + 1,
			 "%s-pbuf-alloc-%u", name, i);
		__xdp2_obj_alloc_free_list_init(allocator,
				pbuf_allocs->obj[i].num_objs,
				xdp2_pbuf_buffer_tag_to_size(i),
				pbufs_base, pname, XDP2_OBJ_ALLOC_BASE_INDEX);

		pvmgr->pbuf_allocator_table[i].pbuf_base = pbufs_base;
		pvmgr->pbuf_allocator_table[i].offset_mask =
			xdp2_pbuf_buffer_tag_to_size(i) - 1;

		/* Determine if there's a smaller size for this one */
		if (last < 0) {
			/* No previous allocator was created which means this
			 * one is the first and smallest allocator size
			 * configured for the packet buffer manager
			 */
			smaller_size = 0;
			smaller_size_shift = 0;
			smaller_pallocator = 0;
		} else {
			/* There is a smaller allocator already configured,
			 * use that one
			 */
			smaller_size = xdp2_pbuf_buffer_tag_to_size(last);
			smaller_size_shift =
				xdp2_pbuf_buffer_tag_to_size_shift(last);

			smaller_pallocator =
				pvmgr->pbuf_allocator_table[last].pallocator;
		}

		/* Set the allocator table starting from the last one set plus
		 * one through the current entry. For each entry set the
		 * allocator which is the minimum size allocation for
		 * allocate the object size, and the smaller allocation size
		 * to the last one if it was defined
		 */
		for (j = last + 1; j <= i; j++) {
			entry = &pvmgr->pbuf_allocator_table[j];

			entry->alloc_size = xdp2_pbuf_buffer_tag_to_size(i);
			entry->alloc_size_shift =
					xdp2_pbuf_buffer_tag_to_size_shift(i);
			entry->pallocator = pallocator;
			entry->smaller_size = smaller_size;
			entry->smaller_size_shift = smaller_size_shift;
			entry->smaller_pallocator = smaller_pallocator;
		}
		last = i;
	}

	if (last < 0) {
		/* Looks like all the sizes in the input configuration table
		 * were zero. That's very odd, but technically I suppose it's
		 * not a bug
		 */
		XDP2_WARN("No object sizes were allocated in packet buffer "
			   "init");
		return 0;
	}
	/* Set the last entries in the allocator that had zero size
	 * configured in the input table. For each entry set the allocator
	 * is set NULL and the smaller allocation size is set the last one
	 * that was defined
	 */
	smaller_size = xdp2_pbuf_buffer_tag_to_size(last);
	smaller_size_shift =
			xdp2_pbuf_buffer_tag_to_size_shift(last);
	smaller_pallocator = pvmgr->pbuf_allocator_table[last].pallocator;

	for (j = last + 1; j < XDP2_PBUF_NUM_SIZE_SHIFTS; j++) {
		entry = &pvmgr->pbuf_allocator_table[j];

		entry->alloc_size = 0;
		entry->alloc_size_shift = 0;
		entry->pallocator = NULL;
		entry->smaller_size = smaller_size;
		entry->smaller_size_shift = smaller_size_shift;
		entry->smaller_pallocator = smaller_pallocator;
	}

	return 0;
}

/* Initialize the pvbuf allocators */
static int __xdp2_pvbuf_init_pvbuf_allocators(struct xdp2_pvbuf_mgr *pvmgr,
		struct xdp2_pvbuf_init_allocator *pvbuf_allocs,
		const char *name)
{
	struct xdp2_obj_allocator *smaller_allocator = NULL;
	struct xdp2_pvbuf_allocator_entry *entry;
	char pname[XDP2_OBJ_ALLOC_NAME_LEN + 1];
	size_t smaller_size = 0;
	int i, j, last = -1;

	/* Allocate a pbuf allocator for another entries in the shift array
	 * that are nonzero
	 */
	for (i = 0; i < XDP2_PVBUF_NUM_SIZES; i++) {
		unsigned int num_pvbufs = pvbuf_allocs->obj[i].num_pvbufs;
		struct xdp2_obj_allocator *allocator =
				pvbuf_allocs->obj[i].allocator;
		void *base = pvbuf_allocs->obj[i].base;

		if (!num_pvbufs)
			continue;

		entry = &pvmgr->pvbuf_allocator_table[i];

		if (!allocator) {
			allocator = malloc(XDP2_OBJ_ALLOC_SIZE(num_pvbufs));
			if (!allocator)
				return -ENOMEM;

			entry->alloced_pvbuf_allocator = true;
		} else {
			entry->alloced_pvbuf_allocator = false;
		}

		entry->allocator = allocator;

		if (!base) {
			base = calloc(num_pvbufs, (i + 1) * 64);
			if (!base) {
				if (!entry->alloced_pvbufs_base)
					return -ENOMEM;
			}
			entry->alloced_pvbufs_base = true;
		} else {
			entry->alloced_pvbufs_base = false;
		}

		entry->pvbufs_base = base;

		/* Initialize the object allocator FIFO, the memory objects are
		 * allocated in the call
		 */
		snprintf(pname, XDP2_OBJ_ALLOC_NAME_LEN + 1,
			 "%s-pvbuf-alloc-%u", name, i);

		__xdp2_obj_alloc_free_list_init(allocator, num_pvbufs,
					    (i + 1) * 64,
					    entry->pvbufs_base, pname,
					    XDP2_OBJ_ALLOC_BASE_INDEX);

		/* Determine if there's a smaller size for this one */
		if (last >= 0) {
			/* There is a smaller allocator already configured,
			 * use that one
			 */
			smaller_size = last;

			smaller_allocator =
				pvmgr->pvbuf_allocator_table[last].allocator;
		}

		/* Set the allocator table starting from the last one set plus
		 * one through the current entry. For each entry set the
		 * allocator which is the minimum size allocation for
		 * allocate the object size, and the smaller allocation size
		 * to the last one if it was defined
		 */
		for (j = last + 1; j <= i; j++) {
			entry = &pvmgr->pvbuf_allocator_table[j];

			entry->smaller_size = smaller_size;
			entry->smaller_allocator = smaller_allocator;
		}
		last = i;
	}

	if (last < 0) {
		/* Looks like all the sizes in the input configuration table
		 * were zero. That's very odd, but technically I suppose it's
		 * not a bug
		 */
		XDP2_WARN("No object sizes were allocated in packet buffer "
			   "init");
		return 0;
	}

	pvmgr->default_pvbuf_size = last;

	/* Set the last entries in the allocator that had zero size
	 * configured in the input table. For each entry set the allocator
	 * is set NULL and the smaller allocation size is set the last one
	 * that was defined
	 */
	for (j = last + 1; j < XDP2_PVBUF_NUM_SIZES; j++) {
		entry = &pvmgr->pvbuf_allocator_table[j];

		entry->smaller_size = smaller_size;
		entry->smaller_allocator = smaller_allocator;
	}

	return 0;
}

/* Initialize a packet buffer manager */
int __xdp2_pvbuf_init(struct xdp2_pvbuf_mgr *pvmgr,
		      struct xdp2_pbuf_init_allocator *pbuf_allocs,
		      struct xdp2_pvbuf_init_allocator *pvbuf_allocs,
		      const char *name, uintptr_t external_base_offset,
		      bool random_pvbuf_size, bool alloc_one_ref,
		      struct xdp2_short_addr_config *short_addr_config,
		      struct xdp2_long_addr_config *long_addr_config)
{
	int i, ret;

	XDP2_ASSERT(XDP2_PVBUF_INDEX_BITS >= 10 &&
		     XDP2_PVBUF_INDEX_BITS <= 36,
		     "Number of index bits %u for a pvbuf address must "
		     "be greater than 9 and less than or equal to 36",
		     XDP2_PVBUF_INDEX_BITS);

	memset(pvmgr, 0, sizeof(*pvmgr));

	pvmgr->external_base_offset = external_base_offset;
	pvmgr->random_pvbuf_size = random_pvbuf_size;
	pvmgr->alloc_one_ref = alloc_one_ref;

	ret = __xdp2_pvbuf_init_pvbuf_allocators(pvmgr, pvbuf_allocs, name);
	if (ret)
		goto out_err;

	ret = __xdp2_pvbuf_init_pbuf_allocators(pvmgr, pbuf_allocs, name);
	if (ret)
		goto out_err;

	if (short_addr_config) {
		for (i = 0; i < 3; i++) {
			pvmgr->short_addr_base[i + 1] =
					short_addr_config->mem_region[i].base;
			pvmgr->short_addr_ops[i + 1] =
					short_addr_config->mem_region[i].ops;
		}
	}

	if (long_addr_config) {
		for (i = 0; i < 64; i++) {
			pvmgr->long_addr_base[i] =
					long_addr_config->mem_region[i].base;
			pvmgr->long_addr_ops[i] =
					long_addr_config->mem_region[i].ops;
		}
	}

	return 0;

out_err:
	/* Cleanup including freeing any resources that were allocated
	 * above
	 */
	__xdp2_clear_pvmgr(pvmgr);

	return ret;
}

/* Check functions */

/* Check information for one pvbuf managed by the packet buffer manager in the
 * pvmgr argument
 */
void __xdp2_pvbuf_check(struct xdp2_pvbuf_mgr *pvmgr,
			xdp2_paddr_t paddr, const char *where)
{
	struct xdp2_pvbuf *pvbuf =
			__xdp2_pvbuf_paddr_to_addr(pvmgr, paddr);
	unsigned int num_iovecs =
			XDP2_PVBUF_NUM_IOVECS_FROM_PADDR(paddr);
	int i;

	for (i = 0; i < num_iovecs; i++) {
		struct xdp2_iovec *iovec = &pvbuf->iovec[i];

		if (!iovec->paddr) {
			XDP2_ASSERT(!xdp2_pvbuf_iovec_map_isset(pvbuf, i),
				     "Expected map bit %u to be unset at: %s",
				     i, where);
			continue;
		}

		XDP2_ASSERT(xdp2_pvbuf_iovec_map_isset(pvbuf, i),
			     "Expected map bit %u set with address %llx at: %s",
			     i, iovec->paddr, where);

		if (XDP2_PADDR_TAG_IS_PVBUF(iovec->pbaddr.paddr_tag))
			__xdp2_pvbuf_check(pvmgr, iovec->paddr, where);
	}
}

void __xdp2_pvbuf_check_empty(struct xdp2_pvbuf_mgr *pvmgr,
			       xdp2_paddr_t paddr, const char *where)
{
	struct xdp2_pvbuf *pvbuf =
			__xdp2_pvbuf_paddr_to_addr(pvmgr, paddr);
	unsigned int num_iovecs =
			XDP2_PVBUF_NUM_IOVECS_FROM_PADDR(paddr);
	int i;

	for (i = 0; i < num_iovecs; i++) {
		XDP2_ASSERT(!pvbuf->iovec[i].paddr, "Check empty address "
						     "is set at index %u "
						     "with value %llx: at %s",
			i, pvbuf->iovec[i].paddr, where);
		XDP2_ASSERT(!xdp2_pvbuf_iovec_map_isset(pvbuf, i),
				"Expected map bit %u in check empty to be "
				"unset at: %s", i, where);
	}
}

/* Print info and CLI functions */

/* Print information for one pvbuf managed by the packet buffer manager in the
 * pvmgr argument
 */
static void __xdp2_pvbuf_print_one(struct xdp2_pvbuf_mgr *pvmgr,
				    unsigned int level, xdp2_paddr_t paddr)
{
	struct xdp2_pvbuf *pvbuf =
			__xdp2_pvbuf_paddr_to_addr(pvmgr, paddr);
	unsigned int num_iovecs =
			XDP2_PVBUF_NUM_IOVECS_FROM_PADDR(paddr);
	char indent[10 + 1];
	int i;

	for (i = 0; i < xdp2_min(level, 10); i++)
		indent[i] = '\t';
	indent[i] = 0;

	for (i = 0; i < num_iovecs; i++) {
		struct xdp2_iovec *iovec = &pvbuf->iovec[i];

		if (!iovec->paddr) {
			XDP2_ASSERT(!xdp2_pvbuf_iovec_map_isset(pvbuf, i),
				     "Expected map bit %u to be unset in "
				     "__xdp2_pvbuf_print_one", i);
			continue;
		}

		XDP2_ASSERT(xdp2_pvbuf_iovec_map_isset(pvbuf, i),
			     "Expected map bit %u set in "
			     "__xdp2_pvbuf_print_one, address is %llx",
			     i, iovec->paddr);

		switch (xdp2_iovec_paddr_addr_tag(iovec)) {
		case XDP2_PADDR_TAG_PVBUF:
			printf("%s#%u: pvbuf, length %u, num iovecs %lu\n",
			       indent, i, iovec->pbaddr.pvbuf.data_length,
				XDP2_PVBUF_NUM_IOVECS_FROM_PADDR(
						iovec->paddr));
			__xdp2_pvbuf_print_one(pvmgr, level + 1, iovec->paddr);
			break;
		case XDP2_PADDR_TAG_PBUF:
		case XDP2_PADDR_TAG_PBUF_1REF: {
			unsigned int size_shift, zindex, length;
			__u64 addroff;

			zindex = __xdp2_pbuf_zindex_from_paddr(iovec->paddr);
			size_shift = xdp2_pbuf_size_shift_from_paddr(
								iovec->paddr);
			length = xdp2_pbuf_get_data_len_from_iovec(iovec);
			addroff = xdp2_pbuf_get_offset_from_paddr(iovec->paddr);
			printf("%s#%u: Pbuf** Size:%u Index:%u Offset: %llu "
			       "Length: %u Address: %llx (%p) Refcnt: %u\n",
			       indent, i, 1 << size_shift, zindex, addroff,
			       length, iovec->paddr,
			       __xdp2_pbuf_paddr_to_addr(pvmgr,
						iovec->paddr),
			       xdp2_pbuf_get_refcnt(pvmgr, iovec->paddr));
			break;
		}
		case XDP2_PADDR_TAG_ALL_SHORT_ADDR_CASES:
			printf("%s#%u: Short address** Memory region: %u "
			       "Data length:%lu Offset: %lu "
			       "Address: %llx (%p)\n", indent, i,
			       iovec->pbaddr.short_addr.short_paddr_tag,
			       xdp2_paddr_short_addr_get_data_len_from_iovec(
								iovec),
			       xdp2_paddr_short_addr_get_offset_from_paddr(
								iovec->paddr),
			       iovec->paddr,
			       __xdp2_paddr_short_addr_paddr_to_addr(pvmgr,
								iovec->paddr));
			break;
		case XDP2_PADDR_TAG_LONG_ADDR:
			__xdp2_paddr_long_addr_check(iovec[0].paddr,
						     iovec[1].paddr,
						     "Print PVbufs");

			printf("%s#%u-%u: Long address** Memory region: %u "
			       "Data length:%lu Offset: %lu "
			       "Address: %llx (%p)\n", indent, i, i + 1,
			       iovec->pbaddr.long_addr.memory_region,
			       xdp2_paddr_long_addr_get_data_len_from_iovec(
								iovec),
			       xdp2_paddr_long_addr_get_offset_from_paddr(
								iovec[0].paddr,
								iovec[1].paddr),
			       iovec->paddr,
			       __xdp2_paddr_long_addr_to_addr(pvmgr,
							      iovec[0].paddr,
							      iovec[1].paddr));
			i++;

			break;
		default:
			printf("Unknown tag: %s (%u)\n",
			       xdp2_paddr_print_paddr_type(iovec->paddr),
			       xdp2_paddr_get_paddr_tag_from_paddr(
							iovec->paddr));
			break;
		}
	}
}

/* Print information for one pvbuf managed by the packet buffer manager in the
 * pvmgr argument
 */
void __xdp2_pvbuf_print(struct xdp2_pvbuf_mgr *pvmgr,
			 unsigned int level, xdp2_paddr_t paddr)
{
	printf("PVBUF ********** Length %lu\n",
		__xdp2_pvbuf_calc_length(pvmgr, paddr, false));
	__xdp2_pvbuf_print_one(pvmgr, level, paddr);
}

/* Print information for a packet buffer manager */
void ___xdp2_pvbuf_show_buffer_manager(struct xdp2_pvbuf_mgr *pvmgr,
		void *cli, void (*cb)(struct xdp2_obj_allocator *allocator,
				      void *cli, const char *arg),
				      const char *arg)

{
	struct xdp2_pbuf_allocator_entry *entry;
	struct xdp2_pbuf_allocator *pallocator;
	int i;

	XDP2_CLI_PRINT(cli, "Stats: allocs %lu, 1ref_allocs %lu, "
			     "conversions %lu, frees %lu, 1ref_frees %lu\n",
			pvmgr->allocs, pvmgr->allocs_1ref,
			pvmgr->conversions, pvmgr->frees, pvmgr->frees_1ref);

	XDP2_CLI_PRINT(cli, "\n");
	for (i = 0; i < XDP2_PVBUF_NUM_SIZES; i++) {
		struct xdp2_obj_allocator *allocator =
					XDP2_PVBUF_GET_ALLOCATOR(pvmgr, i);

		if (allocator)
			__xdp2_obj_alloc_show_allocator(allocator,
							 cli, cb, arg);
	}

	XDP2_CLI_PRINT(cli, "\n");
	for (i = 0; i < XDP2_PBUF_NUM_SIZE_SHIFTS; i++) {
		entry = &pvmgr->pbuf_allocator_table[i];
		if (entry->alloc_size_shift ==
				xdp2_pbuf_buffer_tag_to_size_shift(i) &&
		    entry->pallocator) {
			pallocator = XDP2_PVBUF_GET_ADDRESS(pvmgr,
							     entry->pallocator);
			__xdp2_obj_alloc_show_allocator(
					XDP2_PVBUF_GET_ADDRESS(pvmgr,
						pallocator->allocator), cli,
						cb, arg);
		}
	}
}

/* Print detailed information for a packet buffer manager */
void __xdp2_pvbuf_show_buffer_manager_details(struct xdp2_pvbuf_mgr *pvmgr,
					       void *cli)
{
	int i;

	for (i = 0; i < XDP2_PVBUF_NUM_SIZES; i++) {
		struct xdp2_pvbuf_allocator_entry *entry =
					&pvmgr->pvbuf_allocator_table[i];

		XDP2_CLI_PRINT(cli, "%u: smaller_size %u\n", i + 1,
				entry->smaller_size + 1);
	}
	for (i = 0; i < XDP2_PBUF_NUM_SIZE_SHIFTS; i++) {
		struct xdp2_pbuf_allocator_entry *entry =
					&pvmgr->pbuf_allocator_table[i];

		XDP2_CLI_PRINT(cli, "%u (%lu): alloc_size %lu, "
				     "smaller_size %lu\n", i,
				xdp2_pbuf_buffer_tag_to_size(i),
				entry->alloc_size, entry->smaller_size);
	}
}

void xdp2_pvbuf_show_buffer_manager_from_cli(void *cli,
					 struct xdp2_cli_thread_info *info,
					const char *arg)
{
		xdp2_pvbuf_show_buffer_manager(cli);
}

void xdp2_pvbuf_show_buffer_manager_details_from_cli(void *cli,
					struct xdp2_cli_thread_info *info,
					const char *arg)
{
	xdp2_pvbuf_show_buffer_manager_details(cli);
}

/* Iterator struct for copy operation */
struct __xdp2_pvbuf_iter_copy {
	void *data;
	size_t len;
	size_t offset;
};

static bool __xdp2_pvbuf_iterate_copy_to_pvbuf(void *priv, __u8 *data,
					       size_t len)
{
	struct __xdp2_pvbuf_iter_copy *ist = priv;

	if (ist->offset) {
		if (len <= ist->offset) {
			ist->offset -= len;
			return true;
		}
		data += ist->offset;
		len -= ist->offset;

		ist->offset = 0;
	}

	if (ist->len)
		len = xdp2_min(ist->len, len);

	memcpy(data, ist->data, len);
	ist->data += len;

	if (ist->len) {
		ist->len -= len;
		if (!ist->len)
			return false;
	}

	return true;
}

/* Copy data from a linear buffer to a PVBuf */
size_t __xdp2_pvbuf_copy_data_to_pvbuf(struct xdp2_pvbuf_mgr *pvmgr,
					xdp2_paddr_t paddr,
					void *data, size_t len, size_t offset)
{
	struct __xdp2_pvbuf_iter_copy ist = {
		.data = data,
		.len = len,
		.offset = offset,
	};

	__xdp2_pvbuf_iterate(pvmgr, paddr,
			      __xdp2_pvbuf_iterate_copy_to_pvbuf, &ist);

	/* Return number of bytes that were copied */
	return len - ist.len;
}

static bool __xdp2_pvbuf_iterate_copy_to_data(void *priv, __u8 *data,
					       size_t len)
{
	struct __xdp2_pvbuf_iter_copy *ist = priv;

	if (ist->offset) {
		if (len <= ist->offset) {
			ist->offset -= len;
			return true;
		}
		data += ist->offset;
		len -= ist->offset;

		ist->offset = 0;
	}

	if (ist->len)
		len = xdp2_min(ist->len, len);

	memcpy(ist->data, data, len);
	ist->data += len;

	if (ist->len) {
		ist->len -= len;
		if (!ist->len)
			return false;
	}

	return true;
}

/* Copy data from a PVBuf to a linear buffer */
size_t __xdp2_pvbuf_copy_pvbuf_to_data(struct xdp2_pvbuf_mgr *pvmgr,
					xdp2_paddr_t paddr, void *data,
					size_t len, size_t offset)
{
	struct __xdp2_pvbuf_iter_copy ist = {
		.data = data,
		.len = len,
		.offset = offset,
	};

	__xdp2_pvbuf_iterate(pvmgr, paddr,
			      __xdp2_pvbuf_iterate_copy_to_data, &ist);

	/* Return number of bytes that were copied */
	return len - ist.len;
}

struct __xdp2_pvbuf_iter_checksum {
	size_t len;
	size_t offset;
	__u64 csum_total;
	bool odd_byte;
};

static bool __xdp2_pvbuf_iterate_checksum(void *priv, __u8 *data, size_t len)
{
	struct __xdp2_pvbuf_iter_checksum *ist = priv;
	__u16 csum;

	if (ist->offset) {
		if (len <= ist->offset) {
			ist->offset -= len;
			return true;
		}
		data += ist->offset;
		len -= ist->offset;

		ist->offset = 0;
	}

	if (ist->len)
		len = xdp2_min(ist->len, len);

	if (ist->odd_byte && len) {
		ist->csum_total = xdp2_checksum_add16(ist->csum_total,
						       data[0] << 8);
		len--;
		data++;
		ist->odd_byte = false;
	}

	csum = __xdp2_checksum_compute(data, len);
	ist->csum_total = xdp2_checksum_add16(ist->csum_total, csum);

	if (len & 1)
		ist->odd_byte = true;

	if (ist->len) {
		ist->len -= len;
		if (!ist->len)
			return false;
	}

	return true;
}

/* Compute the checksum over a PVbuf */
__u16 __xdp2_pvbuf_checksum(struct xdp2_pvbuf_mgr *pvmgr,
			     xdp2_paddr_t paddr, size_t len, size_t offset)
{
	struct __xdp2_pvbuf_iter_checksum ist = {
		.len = len,
		.offset = offset,
	};

	__xdp2_pvbuf_iterate(pvmgr, paddr,
			      __xdp2_pvbuf_iterate_checksum, &ist);

	return __xdp2_checksum_fold64(ist.csum_total);
}

/* Iterator struct for creating an iovec */
struct __xdp2_pvbuf_iter_make_iovecs {
	struct iovec *iovecs;
	unsigned int num_vec;
	unsigned int max_vecs;
	size_t len;
	size_t offset;
};

static bool __xdp2_pvbuf_iterate_make_iovecs(void *priv, __u8 *data,
					     size_t len)
{
	struct __xdp2_pvbuf_iter_make_iovecs *ist = priv;

	if (ist->num_vec >= ist->max_vecs) {
		ist->num_vec++;

		/* Test overflow by num_vec being greater than max_vecs */
		return false;
	}

	if (ist->offset) {
		if (len <= ist->offset) {
			ist->offset -= len;
			return true;
		}
		data += ist->offset;
		len -= ist->offset;

		ist->offset = 0;
	}

	if (ist->len)
		len = xdp2_min(ist->len, len);

	ist->iovecs[ist->num_vec].iov_base = data;
	ist->iovecs[ist->num_vec].iov_len = len;

	ist->num_vec++;

	if (!ist->len) {
		ist->len -= len;
		if (!ist->len)
			return false;
	}

	return true;
}

/* Make a standard iovec array from a PVbuf */
int __xdp2_pvbuf_make_iovecs(struct xdp2_pvbuf_mgr *pvmgr,
			     xdp2_paddr_t paddr, struct iovec *iovecs,
			     unsigned int max_vecs, size_t len, size_t offset)
{
	struct __xdp2_pvbuf_iter_make_iovecs ist = {
		.iovecs = iovecs,
		.num_vec = 0,
		.max_vecs = max_vecs,
		.len = len,
		.offset = offset,
	};

	__xdp2_pvbuf_iterate(pvmgr, paddr,
			     __xdp2_pvbuf_iterate_make_iovecs, &ist);

	return ist.num_vec <= max_vecs ? ist.num_vec : -1;
}
