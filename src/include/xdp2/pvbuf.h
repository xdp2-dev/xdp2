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
#ifndef __XDP2_PVBUF_H__
#define __XDP2_PVBUF_H__

/* Definitions and API for XDP2 pbufs, pvbufs, and paddrs */

#include <linux/types.h>
#include <stdatomic.h>

#include "xdp2/bitmap_word.h"
#include "xdp2/cli.h"
#include "xdp2/obj_allocator.h"
#include "xdp2/utility.h"

/* paddrs encode information about buffers (an address and data length).
 * They are concise sixty-four bit values that encode an paddr type, an offset
 * relative to a base address for the paddr type, and a data length of the
 * buffer (when set in an iovec). While the data length isn't technically part
 * of the address, it's convenient to encode it with an address
 */
typedef __u64 xdp2_paddr_t;

/* paddr tags. Tags are four bit values. Note that the short address tags
 * are expanded from two bits for convenience to use with a switch statement
 */
enum xdp2_paddr_tags {
	XDP2_PADDR_TAG_LONG_ADDR =		0b0000,
	XDP2_PADDR_TAG_PVBUF =			0b0001,
	XDP2_PADDR_TAG_PBUF =			0b0010,
	XDP2_PADDR_TAG_PBUF_1REF =		0b0011,
	XDP2_PADDR_TAG_SHORT_ADDR_1_00 =	0b0100,
	XDP2_PADDR_TAG_SHORT_ADDR_1_01 =	0b0101,
	XDP2_PADDR_TAG_SHORT_ADDR_1_10 =	0b0110,
	XDP2_PADDR_TAG_SHORT_ADDR_1_11 =	0b0111,
	XDP2_PADDR_TAG_SHORT_ADDR_2_00 =	0b1000,
	XDP2_PADDR_TAG_SHORT_ADDR_2_01 =	0b1001,
	XDP2_PADDR_TAG_SHORT_ADDR_2_10 =	0b1010,
	XDP2_PADDR_TAG_SHORT_ADDR_2_11 =	0b1011,
	XDP2_PADDR_TAG_SHORT_ADDR_3_00 =	0b1100,
	XDP2_PADDR_TAG_SHORT_ADDR_3_01 =	0b1101,
	XDP2_PADDR_TAG_SHORT_ADDR_3_10 =	0b1110,
	XDP2_PADDR_TAG_SHORT_ADDR_3_11 =	0b1111,
};

/* Short tags for short addresses */
enum xdp2_paddr_short_addr_short_tag {
	XDP2_PADDR_TAG_SHORT_ADDR_1 = 0b01,
	XDP2_PADDR_TAG_SHORT_ADDR_2 = 0b10,
	XDP2_PADDR_TAG_SHORT_ADDR_3 = 0b11,
};

/* paddr NULL address (i.e. (__u64)0). "if (!paddr)" works as expected */
#define XDP2_PADDR_NULL ((xdp2_paddr_t)0)

/* Constants for pbuf sizes
 *
 * The largest size pbuf is 1 << 20 or 1,048,576 bytes (this can be overridden
 * at compile time). This maps to buffer tags 14 and 15. The object index and
 * object offset are encoded in 36 bits where the size of each depends on the
 * pbuf object size given in the pbuf size field
 */
#ifndef XDP2_PBUF_DATA_LEN_BITS
#define XDP2_PBUF_DATA_LEN_BITS	20
#endif

#define XDP2_PBUF_OFFSET_BITS		(64 - 8 - XDP2_PBUF_DATA_LEN_BITS)
#define XDP2_PBUF_MAX_DATA_LEN		(1 << XDP2_PBUF_DATA_LEN_BITS)

/* Constants for pvbuf sizes
 *
 * The largest pvbuf index is 1 << 28 (this may be set with a compile time
 * constant). The maximum data length in a pvbuf address structure then
 * defaults to (1 << 32) - 1
 */
#ifndef XDP2_PVBUF_INDEX_BITS
#define XDP2_PVBUF_INDEX_BITS	24
#endif

#define XDP2_PVBUF_DATA_LEN_BITS	(64 - 8 - XDP2_PVBUF_INDEX_BITS)
#define XDP2_PVBUF_MAX_LEN		(1UL << XDP2_PVBUF_DATA_LEN_BITS)
#define XDP2_PVBUF_MAX_INDEX		(1UL << XDP2_PVBUF_INDEX_BITS)

/* Constants for short addresses
 *
 * The default offset for a short address offset is 44 bit which allows
 * 16 TBytes to be address for each of the three types. The maximum data length
 * is 1 << 18 bytes or 262,144 bytes
 */
#ifndef XDP2_PADDR_SHORT_ADDR_OFFSET_BITS
#define XDP2_PADDR_SHORT_ADDR_OFFSET_BITS 44
#endif

#define XDP2_PADDR_SHORT_ADDR_DATA_LENGTH_BITS (64 - 2 -		\
			XDP2_PADDR_SHORT_ADDR_OFFSET_BITS)
#define XDP2_PADDR_SHORT_ADDR_MAX_DATA_LENGTH				\
			(1UL << XDP2_PADDR_SHORT_ADDR_DATA_LENGTH_BITS)
#define XDP2_PADDR_SHORT_ADDR_MAX_OFFSET				\
			(1UL << XDP2_PADDR_SHORT_ADDR_OFFSET_BITS)

#define XDP2_PADDR_LONG_ADDR_MAX_MEM_REGION (1 << 7)

/* paddr address encoding. There are four basic encodings:
 *
 *	* pbuf
 *	* pvbuf
 *	* short addresses
 *	* long addresses
 *
 * A paddr is sixty-four bits in size and encodes the offset and length of
 * a buffer. The offset is relative to some address as indicated by the type
 * information of the paddr. Long addresses are actually in two paddrs that
 * is 128 bits long where * the address is split into two parts
 */
struct xdp2_paddr {
	union {
		/* pvbuf address encoding */
		struct {
			/* Aggregate length of all pbufs in the sub-hierarchy
			 * of pvbufs. If this is set to zero then that
			 * conveys no length information (i.e. a value of
			 * zero doesn't mean zero length)
			 */
			__u64 data_length: XDP2_PVBUF_DATA_LEN_BITS;

			/* Index of the pvbuf for the particular size. The
			 * offset is computed by multiplying this value by
			 * the byte size of the pvbuf structure (given by
			 * pvbuf_size)
			 */
			__u64 index: XDP2_PVBUF_INDEX_BITS;

			/* Size of the pvbuf structure. The real byte size is
			 * (1 << pvbuf_size) * CACHE_LINE SIZE (presumably
			 * cache-line size is 64 bytes)
			 */
			__u64 pvbuf_size: 4;

			/* Set paddr_tag to XDP2_PADDR_TAG_PVBUF for a PVbuf */
			__u64 short_paddr_tag: 4;
		} pvbuf;

		/* pbuf address encoding */
		struct {
			__u64 data_length: XDP2_PBUF_DATA_LEN_BITS;

			/* Encodes the pointer offset of data in a pbuf, the
			 * "address offset" is relative to the base address of
			 * the pbuf for its particular size allocator
			 */
			__u64 offset: XDP2_PBUF_OFFSET_BITS;

			/* Shift size minus six. E.g. if buffer tag is equal
			 * to 2, then the shift size is 8 and so the size of
			 * the pbuf is 1 << 8 = 256 bytes
			 */
			__u64 buffer_tag: 4;

			/* paddr_tag is either XDP2_PADDR_TAG_PBUF or
			 * XDP2_PADDR_TAG_PBUF_1REF
			 */
			__u64 paddr_tag: 4;
		} pbuf;

		/* Short addresses encoding */
		struct {
			/* 18 bits of length. Add one to get real length.
			 * Zero length buffers cannot be represented
			 */
			__u64 data_length:
					XDP2_PADDR_SHORT_ADDR_DATA_LENGTH_BITS;

			/* 44 bits of offset relative to the base address
			 * of the memory region indicated by the tag
			 */
			__u64 offset: XDP2_PADDR_SHORT_ADDR_OFFSET_BITS;

			/* Tag is XDP2_PADDR_TAG_SHORT_ADDR_{0, 1, 2}
			 * This refers to one of three short address memory
			 * regions that can be pooled together to make a single
			 * large region of 48TB
			 */
			__u64 short_paddr_tag: 2;
		} short_addr;

		/* First word of long address encoding */
		struct {
			/* Data length is full thirty-two bits */
			__u64 data_length: 32;

			/* High order bits of the offset */
			__u64 high_offset: 16;

			/* Memory region for deducing the base address */
			__u64 memory_region: 6;

			__u64 rsvd2: 4;

			/* Set to zero for first word of a long address paddr */
			__u64 word_num: 1;

			/* Always set to 1 to distinguish from NULL paddr */
			__u64 one_bit: 1;

			/* paddr_tag is XDP2_PADDR_TAG_PBUF_LONG_ADDR for
			 * long address
			 */
			__u64 paddr_tag: 4;
		} long_addr;

		/* Second word of long address encoding */
		struct {
			/* Low order bits of the offset */
			__u64 low_offset: 48;

			__u64 rsvd: 10;

			/* Set to one for second word of a long address paddr */
			__u64 word_num: 1;

			/* Always set to 1 to distinguish from NULL paddr */
			__u64 one_bit: 1;

			/* paddr_tag is XDP2_PADDR_TAG_PBUF_LONG_ADDR for
			 * long address
			 */
			__u64 paddr_tag: 4;
		} long_addr_2;

		/* Define four-bit paddr_tag field */
		struct {
			__u64 data_anon: 60;
			__u64 paddr_tag: 4;
		};

		xdp2_paddr_t paddr;
	};
} __packed;

/* paddr type check functions */

#define XDP2_PADDR_TAG_IS_PBUF(TAG)					\
	((TAG) == XDP2_PADDR_TAG_PBUF || (TAG) == XDP2_PADDR_TAG_PBUF_1REF)

#define XDP2_PADDR_TAG_IS_PVBUF(TAG) ((TAG) == XDP2_PADDR_TAG_PVBUF)

#define XDP2_PADDR_TAG_IS_SHORT_ADDR(TAG) (((TAG) & 0b1100) != 0b0000)

#define XDP2_PADDR_TAG_IS_LONG_ADDR(TAG) ((TAG) == XDP2_PADDR_TAG_LONG_ADDR)

#define XDP2_PADDR_IS_PBUF(PADDR)					\
	XDP2_PADDR_TAG_IS_PBUF(xdp2_paddr_get_paddr_tag_from_paddr(PADDR))

#define XDP2_PADDR_IS_PVBUF(PADDR)					\
	XDP2_PADDR_TAG_IS_PVBUF(xdp2_paddr_get_paddr_tag_from_paddr(PADDR))

#define XDP2_PADDR_IS_SHORT_ADDR(PADDR)					\
	XDP2_PADDR_TAG_IS_SHORT_ADDR(xdp2_paddr_get_paddr_tag_from_paddr(PADDR))

#define XDP2_PADDR_IS_LONG_ADDR(PADDR)					\
	XDP2_PADDR_TAG_IS_LONG_ADDR(xdp2_paddr_get_paddr_tag_from_paddr(PADDR))

/* Get the paddr tag from a paddr */
static inline unsigned int xdp2_paddr_get_paddr_tag_from_paddr(
						xdp2_paddr_t paddr)
{
	struct xdp2_paddr pbaddr = { .paddr = paddr };

	return pbaddr.paddr_tag;
}

#define XDP2_PADDR_TAG_ALL_SHORT_ADDR_CASES			\
	XDP2_PADDR_TAG_SHORT_ADDR_1_00:				\
	case XDP2_PADDR_TAG_SHORT_ADDR_1_01:			\
	case XDP2_PADDR_TAG_SHORT_ADDR_1_10:			\
	case XDP2_PADDR_TAG_SHORT_ADDR_1_11:			\
	case XDP2_PADDR_TAG_SHORT_ADDR_2_00:			\
	case XDP2_PADDR_TAG_SHORT_ADDR_2_01:			\
	case XDP2_PADDR_TAG_SHORT_ADDR_2_10:			\
	case XDP2_PADDR_TAG_SHORT_ADDR_2_11:			\
	case XDP2_PADDR_TAG_SHORT_ADDR_3_00:			\
	case XDP2_PADDR_TAG_SHORT_ADDR_3_01:			\
	case XDP2_PADDR_TAG_SHORT_ADDR_3_10:			\
	case XDP2_PADDR_TAG_SHORT_ADDR_3_11

static inline const char *xdp2_paddr_print_paddr_type(xdp2_paddr_t paddr)
{
	switch (xdp2_paddr_get_paddr_tag_from_paddr(paddr)) {
	case XDP2_PADDR_TAG_PVBUF:
		return "pvbuf";
	case XDP2_PADDR_TAG_PBUF:
	case XDP2_PADDR_TAG_PBUF_1REF:
		return "pbuf";
	case XDP2_PADDR_TAG_ALL_SHORT_ADDR_CASES:
		return "short address";
	case XDP2_PADDR_TAG_LONG_ADDR:
		return "long address";
	default:
		return "unknown";
	}
}

/* XDP2 IOvec definitions */

/* An iovec structure holds a paddr. Each iovec contains a sixty-four bit
 * paddr. The paddr may be NULL (0 value), a pbuf, pvbuf, short address, or a
 * long address (the first word and second word). A paddr contains both a
 * length and offset relative to some base that can be used to derive an
 * absolute address from the iovec. Note that the length is technically not
 * part of the address. In various utility functions that operate on paddrs for
 * pbufs or pvbufs, the length bits are ignored (i.e. the length bits are only
 * relevant when the paddr is in an iovec)
 */
struct xdp2_iovec {
	union {
		xdp2_paddr_t paddr;
		struct xdp2_paddr pbaddr;
	};
};

/* Macro to check if an iovec address is a pbuf paddr */
#define XDP2_IOVEC_IS_PBUF(iovec)					\
	XDP2_PADDR_TAG_IS_PBUF(xdp2_iovec_paddr_addr_tag(iovec))

/* Macro to check if an iovec address is a pvbuf paddr */
#define XDP2_IOVEC_IS_PVBUF(iovec)					\
	XDP2_PADDR_TAG_IS_PVBUF(xdp2_iovec_paddr_addr_tag(iovec))

/* Macro to check if an iovec address is a short address paddr */
#define XDP2_IOVEC_IS_SHORT_ADDR(iovec)					\
	XDP2_PADDR_TAG_IS_SHORT_ADDR(xdp2_iovec_paddr_addr_tag(iovec))

/* Macro to check if an iovec address is a long address paddr */
#define XDP2_IOVEC_IS_LONG_ADDR(iovec)					\
	XDP2_PADDR_TAG_IS_LONG_ADDR(xdp2_iovec_paddr_addr_tag(iovec))

/* Clear an iovec */
static inline void xdp2_iovec_zero_iovec(struct xdp2_iovec *iovec)
{
	iovec->paddr = 0;
}

/* Get the paddr tag from an iovec */
static inline unsigned int xdp2_iovec_paddr_addr_tag(struct xdp2_iovec *iovec)
{
	return xdp2_paddr_get_paddr_tag_from_paddr(iovec->paddr);
}

static inline const char *xdp2_iovec_print_paddr_type(struct xdp2_iovec *iovec)
{
	return xdp2_paddr_print_paddr_type(iovec->paddr);
}

/* Number of iovecs in a pvbuf of some size (in number of cache-lines) */
#define XDP2_PVBUF_NUM_IOVECS(SIZE)					\
	xdp2_min(((((SIZE) + 1) * XDP2_CACHELINE_SIZE) -		\
		sizeof(struct xdp2_pvbuf)) /				\
			sizeof(struct xdp2_iovec), XDP2_PVBUF_MAX_IOVECS)

#define XDP2_PVBUF_NUM_PVBUFS_NEEDED(NUM_SEGS)				\
	xdp2_min(NUM_SEGS / (XDP2_CACHELINE_SIZE /			\
					sizeof(struct xdp2_iovec)),	\
		 XDP2_PVBUF_NUM_SIZES - 1)

/* Get the number of iovecs in a pvbuf (called must ensure argument is a
 * paddr for a pvbuf
 */
#define XDP2_PVBUF_NUM_IOVECS_FROM_PADDR(PVBUF_PADDR)			\
	XDP2_PVBUF_NUM_IOVECS(xdp2_pvbuf_get_size_from_paddr(PVBUF_PADDR))


/* Helper macro used in switch statements on iovec paddr type */
#define XDP2_IOVEC_PADDR_TYPE_DEFAULT_ERR(IOVEC, STRING)		\
	XDP2_ERR(1, STRING ": unexpected paddr type "			\
		    "%s (%u)", xdp2_iovec_print_paddr_type(iovec),	\
		 xdp2_iovec_paddr_addr_tag(IOVEC))

#define XDP2_PADDR_TYPE_DEFAULT_ERR(PADDR, STRING)			\
	XDP2_ERR(1, STRING ": unexpected paddr type "			\
		    "%s (%u)", xdp2_paddr_print_paddr_type(PADDR),	\
		xdp2_paddr_get_paddr_tag_from_paddr(PADDR))

/* PVbufs */

static inline void __xdp2_pvbuf_check_paddr(xdp2_paddr_t paddr,
					    const char *label)
{
	XDP2_ASSERT(XDP2_PADDR_IS_PVBUF(paddr),
		    " %s but paddr type %s (%u) is not a pvbuf",
		    label, xdp2_paddr_print_paddr_type(paddr),
		    xdp2_paddr_get_paddr_tag_from_paddr(paddr));
}

/* The maximum number of iovecs in a pvbuf is sixty-four (note that this
 * matches the number of bits in the iovec_map)
 */
#define XDP2_PVBUF_MAX_IOVECS 64

/* Definition of a pvbuf. iovec_map contains a bitmap of non-NULL iovecs
 * in the iovec array. The pvbuf structure is variable size.
 */
struct xdp2_pvbuf {
	__u64 iovec_map;
	struct xdp2_iovec iovec[];
};

/* Maximum number of pvbuf sizes. pvbuf sizes are in number of cache-lines,
 * where size 0 is one cache-line, and size 15 is sixteen cache-lines. Note
 * that this is four bits to fit in the size field of struct xdp2_pvbuf_addr
 */
#define XDP2_PVBUF_NUM_SIZES 16

/* Return data length of a pvbuf in a pvbuf paddr. Caller must ensure that the
 * paddr is a pvbuf
 */
static inline size_t xdp2_pvbuf_get_data_len_from_paddr(xdp2_paddr_t paddr)
{
	struct xdp2_paddr pvaddr = { .paddr = paddr };

	__xdp2_pvbuf_check_paddr(paddr, "Getting length from a pvbuf paddr");

	return pvaddr.pvbuf.data_length;
}

/* Get the index from a pvbuf address */
static inline unsigned int xdp2_pvbuf_index_from_paddr(xdp2_paddr_t paddr)
{
	struct xdp2_paddr pvaddr = { .paddr = paddr };

	__xdp2_pvbuf_check_paddr(paddr, "Getting pvbuf index from a "
					"pvbuf paddr");

	return pvaddr.pvbuf.index;
}

/* Get the pvbuf size from a paddr */
static inline unsigned int xdp2_pvbuf_get_size_from_paddr(xdp2_paddr_t paddr)
{
	struct xdp2_paddr pvaddr = { .paddr = paddr };

	__xdp2_pvbuf_check_paddr(paddr, "Getting pvbuf size from a "
					"pvbuf paddr");

	return pvaddr.pvbuf.pvbuf_size;
}

/* Make a paddr for a pvbuf given a pvbuf size, zindex, and base */
static inline xdp2_paddr_t xdp2_pvbuf_make_paddr(unsigned int size,
						 unsigned int zindex,
						 void *base)
{
	struct xdp2_paddr pvaddr = {};

	XDP2_ASSERT(zindex < (1 << XDP2_PVBUF_INDEX_BITS),
		    "Make pvbuf paddr but index %u is greater then "
		    "maximum index %u", zindex,
		    (1 << XDP2_PVBUF_INDEX_BITS) - 1);

	XDP2_ASSERT(size < XDP2_PVBUF_NUM_SIZES,
		    "Make pvbuf paddr but PVbuf size %u  greater "
		    "than %u\n", size, XDP2_PVBUF_NUM_SIZES - 1);

	pvaddr.paddr_tag = XDP2_PADDR_TAG_PVBUF;
	pvaddr.pvbuf.index = zindex;
	pvaddr.pvbuf.pvbuf_size = size;

	return pvaddr.paddr;
}

/* Set a pvbuf in an iovec */
static inline void xdp2_pvbuf_set_pvbuf_iovec(struct xdp2_iovec *iovec,
					       xdp2_paddr_t paddr,
					       size_t len)
{
	struct xdp2_paddr pbaddr = { .paddr = paddr };

	__xdp2_pvbuf_check_paddr(paddr, "Setting pvbuf in an iovec");

	XDP2_ASSERT(len < XDP2_PVBUF_MAX_LEN,
		    "Setting pvbuf in an iovec but length %lu is greater "
		    "than maximum length %lu", len, XDP2_PVBUF_MAX_LEN - 1);

	pbaddr.pvbuf.data_length = len;
	iovec->paddr = pbaddr.paddr;
}

/* Return data length of a pvbuf in an iovec. Caller must ensure that the
 * paddr in the iovec is for a pvbuf
 */
static inline size_t xdp2_pvbuf_get_data_len_from_iovec(
		struct xdp2_iovec *iovec)
{
	return xdp2_pvbuf_get_data_len_from_paddr(iovec->paddr);
}

/* Set a bit in the iovec_map for a pvbuf */
static inline void xdp2_pvbuf_iovec_map_set(struct xdp2_pvbuf *pvbuf,
					     unsigned int bitnum)
{
	XDP2_ASSERT(bitnum < 64, "Setting bad bit index %u in PVbuf "
		    "bitmap", bitnum);

	xdp2_bitmap_word64_set(pvbuf->iovec_map, bitnum);
}

/* Unset a bit in the iovec_map for a pvbuf */
static inline void xdp2_pvbuf_iovec_map_unset(struct xdp2_pvbuf *pvbuf,
					       unsigned int bitnum)
{
	XDP2_ASSERT(bitnum < 64, "Unsetting bad bit index %u in PVbuf "
		    "bitmap", bitnum);

	xdp2_bitmap_word64_unset(pvbuf->iovec_map, bitnum);
}

/* Check if a bit is set in the iovec_map for a pvbuf (i.e. check if the
 * iovec entry in the array is non-NULL)
 */
static inline bool xdp2_pvbuf_iovec_map_isset(struct xdp2_pvbuf *pvbuf,
					      unsigned int bitnum)
{
	XDP2_ASSERT(bitnum < 64, "Doing isset on  bad bit index %u in PVbuf "
		    "bitmap", bitnum);

	return xdp2_bitmap_word64_isset(pvbuf->iovec_map, bitnum);
}

/* Find the first set bit in an iovec_map of a pvbuf (that is, find the first
 * non-NULL iovec in the iovec array)
 */
static inline unsigned int xdp2_pvbuf_iovec_map_find(struct xdp2_pvbuf *pvbuf)
{
	return xdp2_bitmap_word64_find(pvbuf->iovec_map);
}

/* Find the last set bit in an iovec_map of a pvbuf (that is, find the last
 * non-NULL iovec in the iovec array)
 */
static inline unsigned int xdp2_pvbuf_iovec_map_rev_find(
						struct xdp2_pvbuf *pvbuf)
{
	return xdp2_bitmap_word64_rev_find(pvbuf->iovec_map);
}

/* Find the next set bit in an iovec_map of a pvbuf from some staring position
 * (that is, find the next * non-NULL iovec in the iovec array)
 */
static inline unsigned int xdp2_pvbuf_iovec_map_find_next(
		struct xdp2_pvbuf *pvbuf, unsigned int pos)
{
	return xdp2_bitmap_word64_find_next(pvbuf->iovec_map, pos);
}

/* Return the number of non-NULL iovecs in a pvbuf (the weight of the
 * iovec_map in the pvbuf
 */
static inline unsigned int xdp2_pvbuf_iovec_map_weight(
		struct xdp2_pvbuf *pvbuf)
{
	return xdp2_bitmap_word64_weight(pvbuf->iovec_map);
}

/* "foreach" macros for the non-NULL iovecs in a pvbuf. Does a foreach over
 * the iovec in a pvbuf
 */

#define xdp2_pvbuf_iovec_map_foreach(PVBUF, POS)			\
	xdp2_bitmap_word64_foreach_bit((PVBUF)->iovec_map, POS)

#define xdp2_pvbuf_iovec_map_rev_foreach(PVBUF, POS)			\
	xdp2_bitmap_word64_rev_foreach_bit((PVBUF)->iovec_map, POS)

static inline void __xdp2_pvbuf_iovec_check_empty(struct xdp2_pvbuf *pvbuf,
						  unsigned int index)
{
	struct xdp2_iovec *iovec = &pvbuf->iovec[index];

	XDP2_ASSERT(!iovec->paddr,
		     "Setting iovec %u in pvbuf but entry is non-null: "
		     "address is %llx and bitmap is %s", index,
		     (__u64)iovec->paddr,
		     !!xdp2_pvbuf_iovec_map_isset(pvbuf, index) ? "yes" : "no");
	XDP2_ASSERT(!xdp2_pvbuf_iovec_map_isset(pvbuf, index),
		     "Setting iovec in pvbuf but bitmap %u is set", index);

}

/* Set a pvbuf in an iovec and set the corresponding bit in the iovec_map of
 * a pvbuf
 */
static inline void xdp2_pvbuf_iovec_set_pvbuf_ent(struct xdp2_pvbuf *pvbuf,
						  unsigned int index,
						  xdp2_paddr_t paddr,
						  size_t len)
{
	__xdp2_pvbuf_iovec_check_empty(pvbuf, index);
	xdp2_pvbuf_set_pvbuf_iovec(&pvbuf->iovec[index], paddr, len);
	xdp2_pvbuf_iovec_map_set(pvbuf, index);
}

/* Make a paddr for a pvbuf and set it in an iovec of a pvbuf */
static inline void xdp2_pvbuf_iovec_set_pvbuf_make_paddr(
		struct xdp2_pvbuf *pvbuf, unsigned int index,
		size_t len, unsigned int ptag, unsigned int size,
		unsigned int zindex, void *base)
{
	xdp2_paddr_t paddr = xdp2_pvbuf_make_paddr(size, zindex, base);

	xdp2_pvbuf_iovec_set_pvbuf_ent(pvbuf, index, paddr, len);
}

/* Clear an iovec and unset the corresponding bit in the iovec_map of a pvbuf */
static inline void xdp2_pvbuf_iovec_clear_ent(struct xdp2_pvbuf *pvbuf,
					       unsigned int index)
{
	xdp2_iovec_zero_iovec(&pvbuf->iovec[index]);
	xdp2_pvbuf_iovec_map_unset(pvbuf, index);
}

/* Move an iovec from one index in a pvbuf to another */
static inline void xdp2_pvbuf_iovec_move_ent(struct xdp2_pvbuf *pvbuf,
					      unsigned int old,
					      unsigned int new)
{
	if (old == new)
		return;

	XDP2_ASSERT(!pvbuf->iovec[new].paddr,
		     "Setting iovec in pvbuf but entry %u is non-null with "
		     "value %llx", new, (__u64)pvbuf->iovec[new].paddr);
	XDP2_ASSERT(!xdp2_pvbuf_iovec_map_isset(pvbuf, new),
		     "Setting iovec in pvbuf but bitmap %u is set", new);
	XDP2_ASSERT(pvbuf->iovec[old].paddr,
		     "Unsetting iovec in pvbuf but entry %u is NULL", old);
	XDP2_ASSERT(xdp2_pvbuf_iovec_map_isset(pvbuf, old),
		     "Unsetting iovec in pvbuf but bitmap %u is unset", old);

	pvbuf->iovec[new] = pvbuf->iovec[old];
	xdp2_pvbuf_iovec_clear_ent(pvbuf, old);
	xdp2_pvbuf_iovec_map_set(pvbuf, new);
}

/* Unset an iovec in a pvbuf with checks */
static inline void xdp2_pvbuf_iovec_unset_ent(struct xdp2_pvbuf *pvbuf,
					       unsigned int index)
{
	XDP2_ASSERT(pvbuf->iovec[index].paddr,
		     "Unsetting iovec %u in pvbuf but entry is NULL", index);
	XDP2_ASSERT(xdp2_pvbuf_iovec_map_isset(pvbuf, index),
		     "Unsetting iovec %u in pvbuf but bitmap is unset",
		     index);

	xdp2_pvbuf_iovec_clear_ent(pvbuf, index);
}

/* Pbufs */

static inline void __xdp2_pbuf_check_paddr(xdp2_paddr_t paddr,
					   const char *label)
{
	XDP2_ASSERT(XDP2_PADDR_IS_PBUF(paddr),
		    " %s but paddr type %s (%u) is not a pbuf",
		    label, xdp2_paddr_print_paddr_type(paddr),
		    xdp2_paddr_get_paddr_tag_from_paddr(paddr));
}

/* Possible pbuf allocation sizes are 2^N where N is value in the range 6 to
 * twenty inclusive
 */
#define XDP2_PBUF_NUM_SIZE_SHIFTS	15
#define XDP2_PBUF_BASE_SIZE_SHIFT	6
#define XDP2_PBUF_MAX_SIZE_SHIFT	XDP2_PBUF_DATA_LEN_BITS

/* Return data length of a pbuf in a pbuf paddr. Caller must ensure that the
 * paddr is a pbuf
 */
static inline size_t xdp2_pbuf_get_data_len_from_paddr(xdp2_paddr_t paddr)
{
	struct xdp2_paddr pbaddr = { .paddr = paddr };

	__xdp2_pbuf_check_paddr(paddr, "Getting length from a pbuf paddr");

	return pbaddr.pbuf.data_length +
		(pbaddr.pbuf.buffer_tag == XDP2_PBUF_NUM_SIZE_SHIFTS);
}

/* Get the offset from a pbuf address */
static inline size_t xdp2_pbuf_get_offset_from_paddr(xdp2_paddr_t paddr)
{
	struct xdp2_paddr pbaddr = { .paddr = paddr };

	__xdp2_pbuf_check_paddr(paddr, "Getting offset from a pbuf paddr");

	return pbaddr.pbuf.offset;
}

/* Determine the size shift from a pbuf buffer tag */
static inline unsigned int xdp2_pbuf_buffer_tag_to_size_shift(
		unsigned int btag)
{
	if (btag == XDP2_PBUF_NUM_SIZE_SHIFTS)
		btag--;

	return btag + XDP2_PBUF_BASE_SIZE_SHIFT;
}

/* Convert is size shift to a pbuf buffer tag. This is done by subtracting six
 * from the size shift
 */
static inline unsigned int xdp2_pbuf_size_shift_to_buffer_tag(
							unsigned int size_shift)
{
	return size_shift - XDP2_PBUF_BASE_SIZE_SHIFT;
}

/* Get the real size from a pbuf size shift */
static inline size_t xdp2_pbuf_size_shift_to_size(unsigned int size_shift)
{
	return 1UL << size_shift;
}

/* Determine the size from a pbuf buffer tag */
static inline size_t xdp2_pbuf_buffer_tag_to_size(unsigned int btag)
{
	return xdp2_pbuf_size_shift_to_size(
			xdp2_pbuf_buffer_tag_to_size_shift(btag));
}

/* Determine the pbuf buffer tag for size */
static inline size_t xdp2_pbuf_size_to_buffer_tag(size_t size)
{
	return xdp2_pbuf_size_shift_to_buffer_tag(xdp2_get_log(size));
}

/* Get the buffer tag from a pbuf address */
static inline unsigned int xdp2_pbuf_buffer_tag_from_paddr(
						xdp2_paddr_t paddr)
{
	struct xdp2_paddr pbaddr = { .paddr = paddr };

	__xdp2_pbuf_check_paddr(paddr, "Getting buffer tag from a pbuf paddr");

	return pbaddr.pbuf.buffer_tag;
}

/* Get the size shift from a pbuf address */
static inline unsigned int xdp2_pbuf_size_shift_from_paddr(
						xdp2_paddr_t paddr)
{
	__xdp2_pbuf_check_paddr(paddr, "Getting size shift from a pbuf paddr");

	return xdp2_pbuf_buffer_tag_to_size_shift(
			xdp2_pbuf_buffer_tag_from_paddr(paddr));
}

/* Get the size of a pbuf from a paddr */
static inline size_t xdp2_pbuf_size_from_paddr(xdp2_paddr_t paddr)
{
	return xdp2_pbuf_size_shift_to_size(
		xdp2_pbuf_buffer_tag_to_size_shift(
			xdp2_pbuf_buffer_tag_from_paddr(paddr)));
}

/* Get the zindex (object allocator index) of a pbuf from a paddr */
static inline unsigned int __xdp2_pbuf_zindex_from_paddr(xdp2_paddr_t paddr)
{
	unsigned int btag = xdp2_pbuf_buffer_tag_from_paddr(paddr);
	size_t offset = xdp2_pbuf_get_offset_from_paddr(paddr);

	btag -= (btag == XDP2_PBUF_NUM_SIZE_SHIFTS);

	offset >>= xdp2_pbuf_buffer_tag_to_size_shift(btag);

	return (unsigned int)offset;
}

/* Make a paddr for a pvbuf given a size_shift, index, offset, and base. The
 * length that is being set in the iovec is also an argument and is used here
 * to determine if the buffer tag should be 15 (i.e. length is * equal to
 * 1 << 20)
 */
static inline xdp2_paddr_t xdp2_pbuf_make_paddr(
		bool make_1ref, unsigned int size_shift,
		unsigned int zindex, size_t offset, size_t length, void *base,
		void *addr)
{
	struct xdp2_paddr pbaddr = {};
	__u16 paddr_tag = make_1ref ? XDP2_PADDR_TAG_PBUF_1REF :
				      XDP2_PADDR_TAG_PBUF;

	XDP2_ASSERT(size_shift <= XDP2_PBUF_MAX_SIZE_SHIFT &&
		     size_shift >= XDP2_PBUF_BASE_SIZE_SHIFT,
		     "Make pbuf address: size %u must between %u and %u "
		     "inclusive", size_shift, XDP2_PBUF_BASE_SIZE_SHIFT,
		     XDP2_PBUF_MAX_SIZE_SHIFT);

	XDP2_ASSERT(zindex < (1 << (36 - size_shift)),
		     "Make pbuf address: index is too big for size shift: "
		     "%u >= %u\n", zindex, (1 << size_shift));

	XDP2_ASSERT(offset + length <=
			xdp2_pbuf_size_shift_to_size(size_shift),
		     "Make pbuf address: offset %lu plus length %lu "
		     "equals %lu is greater then size shift %lu\n",
		     offset, length, offset + length,
		     xdp2_pbuf_size_shift_to_size(size_shift));

	/* Check if the input length is precisely equal to 1 << 20
	 * (XDP2_MAX_PBUF_SIZE) and that the size shift is 20. If this is
	 * true, then use buffer tag 15 in the address so that the iovec can
	 * have a length of 1 << 20
	 */
	if (size_shift == XDP2_PBUF_MAX_SIZE_SHIFT &&
	    length == xdp2_pbuf_size_shift_to_size(
						XDP2_PBUF_MAX_SIZE_SHIFT))
		pbaddr.pbuf.buffer_tag = XDP2_PBUF_NUM_SIZE_SHIFTS;
	else
		pbaddr.pbuf.buffer_tag =
				xdp2_pbuf_size_shift_to_buffer_tag(size_shift);

	pbaddr.pbuf.paddr_tag = paddr_tag;
	pbaddr.pbuf.offset =
			((uintptr_t)base + ((__u64)zindex << size_shift)) |
									offset;
	return pbaddr.paddr;
}

/* Set a pbuf in an iovec with the length set per buffer tag in paddr */
static inline void xdp2_pbuf_set_iovec(struct xdp2_iovec *iovec,
				       xdp2_paddr_t paddr, size_t len)
{
	struct xdp2_paddr pbaddr = { .paddr = paddr };

	__xdp2_pbuf_check_paddr(paddr, "Setting pbuf in an iovec");

	pbaddr.pbuf.data_length = len -
			(pbaddr.pbuf.buffer_tag == XDP2_PBUF_NUM_SIZE_SHIFTS);

	iovec->paddr = pbaddr.paddr;
}

/* Return data length of a pbuf in an iovec. Caller must ensure that the paddr
 * in the iovec is for a pbuf
 */
static inline size_t xdp2_pbuf_get_data_len_from_iovec(
		struct xdp2_iovec *iovec)
{
	return xdp2_pbuf_get_data_len_from_paddr(iovec->paddr);
}

/* Set a pbuf in an iovec and set the corresponding bit in the iovec_map of
 * a pvbuf
 */
static inline void xdp2_pvbuf_iovec_set_pbuf_ent(struct xdp2_pvbuf *pvbuf,
						 unsigned int index,
						 xdp2_paddr_t paddr,
						 size_t len)
{
	__xdp2_pvbuf_iovec_check_empty(pvbuf, index);
	xdp2_pbuf_set_iovec(&pvbuf->iovec[index], paddr, len);
	xdp2_pvbuf_iovec_map_set(pvbuf, index);
}

/* Make a paddr for a pbuf and set it in an iovec of a pvbuf */
static inline void xdp2_pvbuf_iovec_set_pbuf_make_paddr(
		struct xdp2_pvbuf *pvbuf,
		unsigned int index, size_t len, bool make_1ref,
		unsigned int size_shift, unsigned int zindex,
		unsigned int offset, size_t length, void *base, void *addr)
{
	xdp2_paddr_t paddr = xdp2_pbuf_make_paddr(make_1ref, size_shift,
						  zindex, offset, len, base,
						  addr);

	xdp2_pvbuf_iovec_set_pbuf_ent(pvbuf, index, paddr, len);
}

/* Short address paddrs */

static inline void __xdp2_paddr_short_addr_check(xdp2_paddr_t paddr,
						 const char *label)
{
	XDP2_ASSERT(XDP2_PADDR_IS_SHORT_ADDR(paddr),
		    " %s but paddr type %s (%u) is not a pbuf", label,
		    xdp2_paddr_print_paddr_type(paddr),
		    xdp2_paddr_get_paddr_tag_from_paddr(paddr));
}

/* Get the data length from a short address paddr */
static inline unsigned int xdp2_paddr_short_addr_get_data_len_from_paddr(
							xdp2_paddr_t paddr)
{
	struct xdp2_paddr pvaddr = { .paddr = paddr };

	__xdp2_paddr_short_addr_check(paddr, "Getting data length from a "
					     "short address paddr");

	return pvaddr.short_addr.data_length + 1;
}

/* Get the offset from a short address paddr */
static inline size_t xdp2_paddr_short_addr_get_offset_from_paddr(
							xdp2_paddr_t paddr)
{
	struct xdp2_paddr pvaddr = { .paddr = paddr };

	__xdp2_paddr_short_addr_check(paddr, "Getting offset from a short "
					     "address paddr");

	return pvaddr.short_addr.offset;
}

/* Get the offset from a short address paddr */
static inline size_t xdp2_paddr_short_addr_get_mem_region_from_paddr(
							xdp2_paddr_t paddr)
{
	struct xdp2_paddr pvaddr = { .paddr = paddr };

	__xdp2_paddr_short_addr_check(paddr, "Getting offset from a short "
					     "address paddr");

	return pvaddr.short_addr.short_paddr_tag;
}

/* Make a paddr for a short address given a short tag and offset */
static inline xdp2_paddr_t xdp2_paddr_short_addr_make_paddr(
		unsigned int short_tag, size_t offset)
{
	struct xdp2_paddr pvaddr = {};

	XDP2_ASSERT(short_tag == XDP2_PADDR_TAG_SHORT_ADDR_1 ||
		    short_tag == XDP2_PADDR_TAG_SHORT_ADDR_2 ||
		    short_tag == XDP2_PADDR_TAG_SHORT_ADDR_3,
		    "Make short address: short tag %u not "
		    "%u, %u, or %u", short_tag, XDP2_PADDR_TAG_SHORT_ADDR_1,
		    XDP2_PADDR_TAG_SHORT_ADDR_2,
		    XDP2_PADDR_TAG_SHORT_ADDR_3);

	XDP2_ASSERT(offset < XDP2_PADDR_SHORT_ADDR_MAX_OFFSET,
		    "Make short address offset %lu is greater "
		    "than %lu", offset, XDP2_PADDR_SHORT_ADDR_MAX_OFFSET);

	pvaddr.short_addr.short_paddr_tag = short_tag;
	pvaddr.short_addr.offset = offset;

	return pvaddr.paddr;
}

/* Set a short address in an iovec */
static inline void xdp2_paddr_short_addr_set_iovec(struct xdp2_iovec *iovec,
						   xdp2_paddr_t paddr,
						  size_t len)
{
	struct xdp2_paddr pbaddr = { .paddr = paddr };

	__xdp2_paddr_short_addr_check(paddr, "Setting short address "
					     "paddr in an iovec");

	XDP2_ASSERT(len, "Setting short address in an iovec but "
			 "length is zero");

	XDP2_ASSERT(len <= XDP2_PADDR_SHORT_ADDR_MAX_DATA_LENGTH,
		    "Setting short address in an iovec but "
		    "length %lu is greater than maximum length %lu",
		    len, XDP2_PADDR_SHORT_ADDR_MAX_DATA_LENGTH);

	/* Subtract one since saved length is minus one of real length */
	pbaddr.short_addr.data_length = len - 1;

	iovec->paddr = pbaddr.paddr;
}

/* Return data length of a short address paddr in an iovec. Caller must
 * ensure that the paddr in the iovec is for a short address
 */
static inline size_t xdp2_paddr_short_addr_get_data_len_from_iovec(
						struct xdp2_iovec *iovec)
{
	return xdp2_paddr_short_addr_get_data_len_from_paddr(iovec->paddr);
}

/* Set a short address  in an iovec and set the corresponding bit in
 * the iovec_map of a pvbuf
 */
static inline void xdp2_pvbuf_iovec_set_short_addr_ent(struct xdp2_pvbuf *pvbuf,
						       unsigned int index,
						       xdp2_paddr_t paddr,
						       size_t len)
{
	__xdp2_pvbuf_iovec_check_empty(pvbuf, index);
	xdp2_paddr_short_addr_set_iovec(&pvbuf->iovec[index], paddr, len);
	xdp2_pvbuf_iovec_map_set(pvbuf, index);
}

/* Make a paddr for a short address and set it in an iovec of a pvbuf */
static inline void xdp2_pvbuf_iovec_set_short_addr_make_paddr(
		struct xdp2_pvbuf *pvbuf, unsigned int index, size_t len,
		unsigned int short_tag, size_t offset)
{
	xdp2_paddr_t paddr = xdp2_paddr_short_addr_make_paddr(short_tag,
							      offset);

	xdp2_pvbuf_iovec_set_short_addr_ent(pvbuf, index, paddr, len);
}

/* Long address paddrs */

/* Validate a long address paddr */
static inline void __xdp2_paddr_long_addr_check(xdp2_paddr_t paddr_1,
						xdp2_paddr_t paddr_2,
						const char *label)
{
	struct xdp2_paddr pbaddr_1 = { .paddr = paddr_1 };
	struct xdp2_paddr pbaddr_2 = { .paddr = paddr_2 };

	XDP2_ASSERT(paddr_1, "Address is NULL and not long address");

	XDP2_ASSERT(XDP2_PADDR_IS_LONG_ADDR(paddr_1),
		    "%s for first word but paddr type %s (%u) is not a "
		    "long address paddr", label,
			xdp2_paddr_print_paddr_type(paddr_1),
			xdp2_paddr_get_paddr_tag_from_paddr(paddr_1));

	XDP2_ASSERT(pbaddr_1.long_addr.word_num == 0 &&
		    pbaddr_1.long_addr.one_bit,
		    "%s for first word but word_num %u is not zero or "
		    "one_bit %u is not one", label,
			pbaddr_1.long_addr.word_num,
			pbaddr_1.long_addr.one_bit);

	XDP2_ASSERT(XDP2_PADDR_IS_LONG_ADDR(paddr_2),
		    "%s for second word but paddr type %s (%u) is not a "
		    "long address paddr", label,
			xdp2_paddr_print_paddr_type(paddr_2),
			xdp2_paddr_get_paddr_tag_from_paddr(paddr_2));

	XDP2_ASSERT(pbaddr_2.long_addr.word_num == 1 &&
		    pbaddr_2.long_addr.one_bit,
		    "%s for second word but word_num %u is not zero or "
		    "one_bit %u is not one", label,
			pbaddr_2.long_addr.word_num,
			pbaddr_2.long_addr.one_bit);
}

/* Get the data length from a long address paddr */
static inline size_t xdp2_paddr_long_addr_get_data_len_from_paddr(
							xdp2_paddr_t paddr_1,
							xdp2_paddr_t paddr_2)
{
	struct xdp2_paddr pvaddr = { .paddr = paddr_1 };

	__xdp2_paddr_long_addr_check(paddr_1, paddr_2,
				"Getting data length for long address paddr");

	return pvaddr.long_addr.data_length;
}

/* Get the offset from a long address paddr */
static inline size_t xdp2_paddr_long_addr_get_offset_from_paddr(
							xdp2_paddr_t paddr_1,
							xdp2_paddr_t paddr_2)
{
	struct xdp2_paddr pvaddr_1 = { .paddr = paddr_1 };
	struct xdp2_paddr pvaddr_2 = { .paddr = paddr_2 };

	__xdp2_paddr_long_addr_check(paddr_1, paddr_2,
			"Getting data length for long address paddr");

	return ((size_t)pvaddr_1.long_addr.high_offset << 48) |
	       pvaddr_2.long_addr_2.low_offset;
}

/* Get the offset from a long address paddr */
static inline void xdp2_paddr_long_addr_set_offset_from_paddr(
							xdp2_paddr_t *paddr_1,
							xdp2_paddr_t *paddr_2,
							size_t offset)
{
	struct xdp2_paddr pvaddr_1 = { .paddr = *paddr_1 };
	struct xdp2_paddr pvaddr_2 = { .paddr = *paddr_2 };

	__xdp2_paddr_long_addr_check(*paddr_1, *paddr_2,
			"Getting data length for long address paddr");

	pvaddr_1.long_addr.high_offset = offset >> 48;
	pvaddr_2.long_addr_2.low_offset = offset & ((1ULL << 48) - 1);

	*paddr_1 = pvaddr_1.paddr;
	*paddr_2 = pvaddr_2.paddr;
}

/* Get the offset from a short address paddr */
static inline unsigned int xdp2_paddr_long_addr_get_mem_region_from_paddr(
							xdp2_paddr_t paddr_1,
							xdp2_paddr_t paddr_2)
{
	struct xdp2_paddr pvaddr = { .paddr = paddr_1 };

	__xdp2_paddr_long_addr_check(paddr_1, paddr_2,
				     "Getting mem region from a long "
				     "address paddr");

	return pvaddr.long_addr.memory_region;
}

/* Make paddrs for a long address given a memory region and offset, Note
 * the two paddrs are returned in pointer arguments
 */
static inline void xdp2_paddr_long_addr_make_paddr(unsigned int mem_region,
						   xdp2_paddr_t *paddr_1,
						   xdp2_paddr_t *paddr_2,
						   size_t offset)
{
	struct xdp2_paddr pvaddr_1 = {};
	struct xdp2_paddr pvaddr_2 = {};

	XDP2_ASSERT(mem_region < XDP2_PADDR_LONG_ADDR_MAX_MEM_REGION,
		    "Make long address paddr: memory region %u is greater "
		    "than maximum %u", mem_region,
		    XDP2_PADDR_LONG_ADDR_MAX_MEM_REGION - 1);

	/* We don't check the offset size since the field is sixty-four bits */

	pvaddr_1.long_addr.paddr_tag = XDP2_PADDR_TAG_LONG_ADDR;
	pvaddr_1.long_addr.one_bit = 1;
	pvaddr_1.long_addr.word_num = 0;

	pvaddr_1.long_addr.memory_region = mem_region;
	pvaddr_1.long_addr.high_offset = offset >> (64 - 16);

	pvaddr_2.long_addr_2.paddr_tag = XDP2_PADDR_TAG_LONG_ADDR;
	pvaddr_2.long_addr_2.one_bit = 1;
	pvaddr_2.long_addr_2.word_num = 1;

	pvaddr_2.long_addr_2.low_offset = offset & ((1ULL >> (64 - 16)) - 1);

	*paddr_1 = pvaddr_1.paddr;
	*paddr_2 = pvaddr_2.paddr;
}

/* Set a long address in an iovec, Note that a long address is composed
 * of two paddrs and requires two slots iovecs in the array (the iovec input
 * must point to an array of at least two iovecs)
 */
static inline void xdp2_paddr_long_addr_set_iovec(struct xdp2_iovec *iovec,
						  xdp2_paddr_t paddr_1,
						  xdp2_paddr_t paddr_2,
						  size_t len)
{
	struct xdp2_paddr pbaddr_1 = { .paddr = paddr_1 };

	__xdp2_paddr_long_addr_check(paddr_1, paddr_2,
			"Setting long address in an iovec");

	XDP2_ASSERT(len < (1ULL << 32),
		    "Setting long address in an iovec but "
		    "length %lu is greater than maximum length %llu",
		    len, 1ULL << 32);

	pbaddr_1.long_addr.data_length = len;
	iovec[0].paddr = pbaddr_1.paddr;
	iovec[1].paddr = paddr_2;

	__xdp2_paddr_long_addr_check(iovec[0].paddr, iovec[1].paddr,
			"Setting long address in an iovec");
}

/* Return data length of a long address paddr in an iovec. Caller must
 * ensure that the iovec is an array of two paddrs where the first element
 * is the first word of a long address paddr and the second element is the
 * second word of the long address paddr
 */
static inline size_t xdp2_paddr_long_addr_get_data_len_from_iovec(
		struct xdp2_iovec *iovec)
{
	return xdp2_paddr_long_addr_get_data_len_from_paddr(iovec[0].paddr,
							    iovec[1].paddr);
}

/* Set a long address paddrs in an iovec and set the corresponding bits in
 * the iovec_map of a pvbuf. Note we need to deal with two paddrs and
 * two indices in the iovecs array
 */
static inline void xdp2_pvbuf_iovec_set_long_addr_ent(
		struct xdp2_pvbuf *pvbuf, unsigned int index,
		xdp2_paddr_t paddr_1, xdp2_paddr_t paddr_2, size_t len)
{
	__xdp2_pvbuf_iovec_check_empty(pvbuf, index);
	__xdp2_pvbuf_iovec_check_empty(pvbuf, index + 1);
	xdp2_paddr_long_addr_set_iovec(&pvbuf->iovec[index],
					  paddr_1, paddr_2, len);
	xdp2_pvbuf_iovec_map_set(pvbuf, index);
	xdp2_pvbuf_iovec_map_set(pvbuf, index + 1);
}

static inline void xdp2_pvbuf_iovec_set_paddr_ent(
		struct xdp2_pvbuf *pvbuf, unsigned int index,
		xdp2_paddr_t paddr_1, xdp2_paddr_t paddr_2, size_t length)
{
	/* Set input paddr(s) in the new pvbuf */
	switch (xdp2_paddr_get_paddr_tag_from_paddr(paddr_1)) {
	case XDP2_PADDR_TAG_PVBUF:
		xdp2_pvbuf_iovec_set_pvbuf_ent(pvbuf, index, paddr_1, length);
		break;
	case XDP2_PADDR_TAG_PBUF:
	case XDP2_PADDR_TAG_PBUF_1REF:
		xdp2_pvbuf_iovec_set_pbuf_ent(pvbuf, index, paddr_1, length);
		break;
	case XDP2_PADDR_TAG_ALL_SHORT_ADDR_CASES:
		xdp2_pvbuf_iovec_set_short_addr_ent(pvbuf, index, paddr_1,
						    length);
		break;
	case XDP2_PADDR_TAG_LONG_ADDR:
		xdp2_pvbuf_iovec_set_long_addr_ent(pvbuf, index, paddr_1,
						   paddr_2, length);
		break;
	default:
		XDP2_PADDR_TYPE_DEFAULT_ERR(paddr_1, "Set paddr iovec ent");
	}
}

/* Pbuf and PVbuf allocators and buffer manager definitions */

/* An entry in the pvbuf allocator table. The pbuf allocator is an array of
 * possible pvbufs sizes (a size is index plus one number of cache-lines)
 * index plus six.
 *
 * Each entry also contains a smaller alloc_size, and the allocator for the
 * smaller alloc size. The "smaller" allocator indicates the next smallest
 * supported size allocation. This is used in cases where it's preferable to
 * allocate a number of smaller objects rather than allocating a larger
 * object with a lot of unused slack space
 */
struct xdp2_pvbuf_allocator_entry {
	struct xdp2_obj_allocator *allocator;
	bool alloced_pvbuf_allocator;

	void *pvbufs_base;
	bool alloced_pvbufs_base;

	unsigned int smaller_size;
	struct xdp2_obj_allocator *smaller_allocator;
};

/* pbuf allocator: Consists of a XDP2 object allocator and array of atomics
 * to hold pbuf reference counts
 */
struct xdp2_pbuf_allocator {
	bool alloced_this_pallocator;
	bool alloced_allocator;
	bool alloced_base;
	struct xdp2_obj_allocator *allocator;
	void *pbufs_base;
	atomic_uint refcnt[0];
};

/* Helper structure for initializing pbuf allocators */
struct xdp2_pbuf_init_allocator {
	struct {
		unsigned int num_objs;
		void *base;
		struct xdp2_obj_allocator *allocator;
		struct xdp2_pbuf_allocator *pallocator;
		struct xdp2_fifo_stats *fifo_stats;
	} obj[XDP2_PBUF_NUM_SIZE_SHIFTS];
};

/* Helper structure for initializing pvbuf allocators */
struct xdp2_pvbuf_init_allocator {
	struct {
		unsigned int num_pvbufs;
		void *base;
		struct xdp2_obj_allocator *allocator;
		struct xdp2_fifo_stats *fifo_stats;
	} obj[XDP2_PVBUF_NUM_SIZES];
};

/* An entry in the pbuf allocator table. The pbuf allocator is an array of
 * possible shift sizes (a 15 element array where the shift size is the
 * index plus six)
 *
 * Each entry contains a proper alloc_size, log2 of the alloc size (the size
 * shift), and the allocator for the alloc size. This describes the allocator
 * and size for the supported size minimum allocation that is greater than or
 * equal to the size derived from the array index element. Note that if there
 * no such allocator that can satisfy this size allocation then the allocator
 * field will be NULL and alloc_size is zero
 *
 * Each entry also contains a smaller alloc_size, log2 of the smaller size (the
 * size * shift), and the allocator for the smaller alloc size. The "smaller"
 * allocator indicates the next smallest supported size allocation. This is
 * used in cases where it's preferable to allocate a number of smaller objects
 * rather than allocating a larger object with a lot of unused slack space
 */
struct xdp2_pbuf_allocator_entry {
	size_t alloc_size;
	size_t smaller_size;

	size_t alloc_size_shift;
	size_t smaller_size_shift;

	void *pbuf_base;
	unsigned int offset_mask;

	struct xdp2_pbuf_allocator *pallocator;
	struct xdp2_pbuf_allocator *smaller_pallocator;
};

struct xdp2_pvbuf_mgr;

struct xdp2_pvbuf_ops {
	void (*free)(struct xdp2_pvbuf_mgr *pvmgr, xdp2_paddr_t paddr);
	void (*bump_refcnt)(struct xdp2_pvbuf_mgr *pvmgr, xdp2_paddr_t paddr);
};

struct xdp2_pvbuf_long_ops {
	void (*free)(struct xdp2_pvbuf_mgr *pvmgr, xdp2_paddr_t paddr_1,
		     xdp2_paddr_t paddr_2);
	void (*bump_refcnt)(struct xdp2_pvbuf_mgr *pvmgr, xdp2_paddr_t paddr_1,
			    xdp2_paddr_t paddr_2);
};

struct xdp2_short_addr_config_one {
	void *base;
	struct xdp2_pvbuf_ops ops;
};

struct xdp2_short_addr_config {
	struct xdp2_short_addr_config_one mem_region[3];
};

struct xdp2_long_addr_config_one {
	void *base;
	struct xdp2_pvbuf_long_ops ops;
};

struct xdp2_long_addr_config {
	struct xdp2_long_addr_config_one mem_region[64];
};

/* Structure for a pvbuf buffer manager instance. Includes object allocators
 * for pbufs and pvbufs for various sizes
 */
struct xdp2_pvbuf_mgr {
	int addr_xlat_num;
	bool alloced_pvbuf_allocator[XDP2_PVBUF_NUM_SIZES];
	bool alloced_pvbufs_base;
	bool random_pvbuf_size;
	bool alloc_one_ref;
	struct xdp2_pbuf_allocator_entry pbuf_allocator_table[
			XDP2_PBUF_NUM_SIZE_SHIFTS];
	struct xdp2_pvbuf_allocator_entry pvbuf_allocator_table[
			XDP2_PVBUF_NUM_SIZES];
	uintptr_t external_base_offset;

	struct xdp2_pbuf_allocator *ext_buff_allocator;

	void *short_addr_base[4];
	void *long_addr_base[64];

	struct xdp2_pvbuf_ops short_addr_ops[4];
	struct xdp2_pvbuf_long_ops long_addr_ops[64];

	unsigned int default_pvbuf_size;
	unsigned long allocs;
	unsigned long ext_buf_allocs;
	unsigned long allocs_1ref;
	unsigned long conversions;
	unsigned long frees;
	unsigned long frees_1ref;
};

/* Extern to the global or default pvbuf manager */
extern struct xdp2_pvbuf_mgr xdp2_pvbuf_global_mgr;

#define XDP2_PVBUF_GET_ALLOCATOR(PVMGR, NUM)			\
	((PVMGR)->pvbuf_allocator_table[NUM].allocator)

#define XDP2_PVBUF_GET_SMALLER_ALLOCATOR_SIZE(PVMGR, NUM)	\
	((PVMGR)->pvbuf_allocator_table[NUM].smaller_size)

#define XDP2_PVBUF_GET_ADDRESS(PVMGR, ADDRESS) ADDRESS

/* paddr to address conversion */

/* Convert a paddr for a pbuf to a fully qualified address */
static inline void *__xdp2_pbuf_paddr_to_addr(struct xdp2_pvbuf_mgr *pvmgr,
					       xdp2_paddr_t paddr)
{
	unsigned int btag = xdp2_pbuf_buffer_tag_from_paddr(paddr);
	size_t offset = xdp2_pbuf_get_offset_from_paddr(paddr);

	btag -= (btag == XDP2_PBUF_NUM_SIZE_SHIFTS);

	return pvmgr->pbuf_allocator_table[btag].pbuf_base + offset;
}

static inline void *xdp2_pbuf_paddr_to_addr(xdp2_paddr_t paddr)
{
	/* XXXTH There should be a more here that does not require a pvmgr,
	 * just get the base from address translation
	 */
	return __xdp2_pbuf_paddr_to_addr(&xdp2_pvbuf_global_mgr, paddr);
}

/* Get the offset within a pbuf from a paddr */
static inline unsigned int __xdp2_pbuf_paddr_to_pbuf_offset(
		struct xdp2_pvbuf_mgr *pvmgr, xdp2_paddr_t paddr)
{
	unsigned int btag = xdp2_pbuf_buffer_tag_from_paddr(paddr);
	size_t offset = xdp2_pbuf_get_offset_from_paddr(paddr);

	btag -= (btag == XDP2_PBUF_NUM_SIZE_SHIFTS);

	return pvmgr->pbuf_allocator_table[btag].offset_mask & offset;
}

/* Return headroom in a pbuf */
static inline size_t __xdp2_pbuf_get_headroom_from_paddr(
		struct xdp2_pvbuf_mgr *pvmgr, xdp2_paddr_t paddr)
{
	XDP2_ASSERT(XDP2_PADDR_IS_PBUF(paddr),
		    "Getting headroom from a pbuf paddr but paddr type "
		    "%s (%u) is not a pbuf",
			xdp2_paddr_print_paddr_type(paddr),
			xdp2_paddr_get_paddr_tag_from_paddr(paddr));

	return __xdp2_pbuf_paddr_to_pbuf_offset(pvmgr, paddr);
}

static inline size_t xdp2_pbuf_get_headroom_from_paddr(xdp2_paddr_t paddr)
{
	return __xdp2_pbuf_get_headroom_from_paddr(&xdp2_pvbuf_global_mgr,
						   paddr);
}

/* Return tailroom in a pbuf */
static inline size_t __xdp2_pbuf_get_tailroom_from_paddr(
		struct xdp2_pvbuf_mgr *pvmgr, xdp2_paddr_t paddr)
{
	XDP2_ASSERT(xdp2_pbuf_size_from_paddr(paddr) >=
		    __xdp2_pbuf_paddr_to_pbuf_offset(pvmgr, paddr) +
		    xdp2_pbuf_get_data_len_from_paddr(paddr),
		    "Offset pluse data length greater then pbuf size");

	XDP2_ASSERT(XDP2_PADDR_IS_PBUF(paddr),
		    "Getting tailroom from a pbuf paddr but paddr type "
		    "%s (%u) is not a pbuf",
			xdp2_paddr_print_paddr_type(paddr),
			xdp2_paddr_get_paddr_tag_from_paddr(paddr));

	XDP2_ASSERT(xdp2_pbuf_size_from_paddr(paddr) >=
		    __xdp2_pbuf_paddr_to_pbuf_offset(pvmgr, paddr) +
		    xdp2_pbuf_get_data_len_from_paddr(paddr),
		    "Offset pluse data length greater then pbuf size");

	return xdp2_pbuf_size_from_paddr(paddr) -
		(__xdp2_pbuf_paddr_to_pbuf_offset(pvmgr, paddr) +
		 xdp2_pbuf_get_data_len_from_paddr(paddr));
}

static inline size_t xdp2_pbuf_get_tailroom_from_paddr(xdp2_paddr_t paddr)
{
	return __xdp2_pbuf_get_tailroom_from_paddr(&xdp2_pvbuf_global_mgr,
						   paddr);
}

/* Return suggested size of pvbuf allocation. If RANDOM_PVBUF_SIZE is set
 * the that can be used for testing
 */
static inline unsigned int __xdp2_pvbuf_get_size(struct xdp2_pvbuf_mgr *pvmgr,
						 size_t size)
{
	return pvmgr->random_pvbuf_size ? random() % XDP2_PVBUF_NUM_SIZES :
		pvmgr->default_pvbuf_size;
}

static inline unsigned int xdp2_pvbuf_get_size(size_t size)
{
	return __xdp2_pvbuf_get_size(&xdp2_pvbuf_global_mgr, size);

}

/* Convert a paddr for a pvbuf fully qualified address for a pvbuf managed by
 * the packet buffer manager in the pvmgr argument
 */
static inline struct xdp2_pvbuf *__xdp2_pvbuf_paddr_to_addr(
		struct xdp2_pvbuf_mgr *pvmgr, xdp2_paddr_t paddr)
{
	unsigned int pvbuf_size = xdp2_pvbuf_get_size_from_paddr(paddr);
	unsigned int index  = xdp2_pvbuf_index_from_paddr(paddr);

	return xdp2_obj_alloc_index_to_obj(
			XDP2_PVBUF_GET_ALLOCATOR(pvmgr, pvbuf_size),
			index + XDP2_OBJ_ALLOC_BASE_INDEX);
}

/* Convert a paddr for a pvbuf fully qualified address for a pvbuf managed by
 * the global packet buffer manager
 */
static inline struct xdp2_pvbuf *xdp2_pvbuf_paddr_to_addr(
						xdp2_paddr_t paddr)
{
	/* XXXTH There should be a more here that does not require a pvmgr,
	 * just get the base from address translation
	 */
	return __xdp2_pvbuf_paddr_to_addr(&xdp2_pvbuf_global_mgr,
					   paddr);
}

/* Convert a short address paddr to a fully qualified address */
static inline void *__xdp2_paddr_short_addr_paddr_to_addr(
		struct xdp2_pvbuf_mgr *pvmgr, xdp2_paddr_t paddr)
{
	struct xdp2_paddr pbaddr = { .paddr = paddr };
	void *base = pvmgr->short_addr_base[pbaddr.short_addr.short_paddr_tag];

	return base + pbaddr.short_addr.offset;
}

static inline void *xdp2_paddr_short_addr_paddr_to_addr(xdp2_paddr_t paddr)
{
	/* XXXTH There should be a more here that does not require a pvmgr,
	 * just get the base from address translation
	 */
	return __xdp2_paddr_short_addr_paddr_to_addr(&xdp2_pvbuf_global_mgr,
						     paddr);
}

/* Convert a long address paddr to a fully qualified address */
static inline void *__xdp2_paddr_long_addr_to_addr(
		struct xdp2_pvbuf_mgr *pvmgr, xdp2_paddr_t paddr_1,
		xdp2_paddr_t paddr_2)
{
	struct xdp2_paddr pbaddr = { .paddr = paddr_1 };

	void *base = pvmgr->long_addr_base[pbaddr.long_addr.memory_region];

	return base + xdp2_paddr_long_addr_get_offset_from_paddr(paddr_1,
								 paddr_2);
}

static inline void *xdp2_paddr_long_paddr_to_addr(xdp2_paddr_t paddr_1,
						  xdp2_paddr_t paddr_2)
{
	/* XXXTH There should be a more here that does not require a pvmgr,
	 * just get the base from address translation
	 */
	return __xdp2_paddr_long_addr_to_addr(&xdp2_pvbuf_global_mgr,
					       paddr_1, paddr_2);
}

/* Check if an iovec contains only one non-NULL address. If it does then
 * that may replace the parent pvbuf in its iovec as a form of compression
 */
static inline void __xdp2_pvbuf_check_single(struct xdp2_pvbuf_mgr *pvmgr,
					     struct xdp2_iovec *iovec)
{
	unsigned int num_iovecs =
			XDP2_PVBUF_NUM_IOVECS_FROM_PADDR(iovec->paddr);
	unsigned int pvbuf_size, index;
	struct xdp2_pvbuf *pvbuf;

	XDP2_ASSERT(XDP2_IOVEC_IS_PVBUF(iovec),
		    "Checking for a single buf iovec but the iovec type "
		    "%s (%u) is not a pvbuf",
			xdp2_paddr_print_paddr_type(iovec->paddr),
			xdp2_paddr_get_paddr_tag_from_paddr(iovec->paddr));

	pvbuf = __xdp2_pvbuf_paddr_to_addr(pvmgr, iovec->paddr);
	if (xdp2_pvbuf_iovec_map_weight(pvbuf) != 1)
		return;

	pvbuf_size = xdp2_pvbuf_get_size_from_paddr(iovec->paddr);

	index = xdp2_pvbuf_iovec_map_find(pvbuf);

	XDP2_ASSERT(index < num_iovecs, "Bad index %u in pvbuf check single",
		    index);

	*iovec = pvbuf->iovec[index];

	/* Empty pvbuf, just free it */
	xdp2_obj_alloc_free(XDP2_PVBUF_GET_ALLOCATOR(pvmgr, pvbuf_size),
			     pvbuf);
}

/* Allocate, free, and other pvbuf helpers */

/* Backend function to allocate a pvbuf from the provided pvbuf manager */
static inline xdp2_paddr_t ___xdp2_pvbuf_alloc(
		struct xdp2_pvbuf_mgr *pvmgr, unsigned int pvbuf_size,
		struct xdp2_pvbuf **ret_pvbuf,
		unsigned int *smaller_pvbuf_size)
{
	struct xdp2_obj_allocator *allocator;
	struct xdp2_pvbuf *pvbuf = NULL;
	unsigned int zindex;

	XDP2_ASSERT(pvbuf_size < XDP2_PVBUF_NUM_SIZES, "Bad pvbuf alloc "
		     "size: %u >= %u", pvbuf_size, XDP2_PVBUF_NUM_SIZES);

	allocator = XDP2_PVBUF_GET_ALLOCATOR(pvmgr, pvbuf_size);

	if (allocator) {
		/* Get an object from the object allocator */
		pvbuf = xdp2_obj_alloc_alloc(allocator, &zindex);
	}

	if (!pvbuf) {
		if (!smaller_pvbuf_size) {
			pvbuf_size = XDP2_PVBUF_GET_SMALLER_ALLOCATOR_SIZE(
					pvmgr, pvbuf_size);
			allocator = XDP2_PVBUF_GET_ALLOCATOR(pvmgr,
					pvbuf_size);

			while (allocator) {
				pvbuf = xdp2_obj_alloc_alloc(allocator,
							      &zindex);
				if (pvbuf)
					break;

				pvbuf_size =
				    XDP2_PVBUF_GET_SMALLER_ALLOCATOR_SIZE(
							pvmgr, pvbuf_size);
				allocator = XDP2_PVBUF_GET_ALLOCATOR(pvmgr,
							      pvbuf_size);
			}
		}

		if (!pvbuf) {
			*ret_pvbuf = NULL;
			return XDP2_PADDR_NULL;
		}
	}

	/* Zero the pvbuf */
	memset(pvbuf, 0, (pvbuf_size + 1) * 64);

	/* Normalize index for use in pbuf addresses */
	zindex -= XDP2_OBJ_ALLOC_BASE_INDEX;

	if (smaller_pvbuf_size)
		*smaller_pvbuf_size = pvbuf_size;

	*ret_pvbuf = pvbuf;

	return xdp2_pvbuf_make_paddr(pvbuf_size, zindex, NULL);
}

static inline xdp2_paddr_t __xdp2_pvbuf_alloc_empty(
		struct xdp2_pvbuf_mgr *pvmgr, unsigned int pvbuf_size,
		struct xdp2_pvbuf **ret_pvbuf)
{
	return ___xdp2_pvbuf_alloc(pvmgr, pvbuf_size, ret_pvbuf, NULL);
}

static inline xdp2_paddr_t xdp2_pvbuf_alloc_empty(
		unsigned int pvbuf_size,
		struct xdp2_pvbuf **ret_pvbuf)
{
	return __xdp2_pvbuf_alloc_empty(&xdp2_pvbuf_global_mgr,
					pvbuf_size, ret_pvbuf);
}

/* Allocate a pvbuf of the requested size from the packet buffer manager in
 * the pvmgr argument. A paddr is returned. The fract argument specifies the
 * numerator in the calculation of whether to use small buffers to save slack
 * in the allocation, and fact is the input to the size limit function called
 * to determine a smaller allocations should be used. A fully qualified address
 * to the pvbuf is returned in the pvbuf argument
 */
xdp2_paddr_t __xdp2_pvbuf_alloc_params(struct xdp2_pvbuf_mgr *pvmgr,
				       size_t size, unsigned int fract,
				       unsigned int pvbuf_off,
				       struct xdp2_pvbuf **pvbuf);

/* Allocate a pvbuf of the requested size from the global packet buffer manager
 * paddr is returned. The fract argument specifies the numerator in the
 * calculation of whether to use small buffers to save slack in the allocation,
 * and fact is the input to the size limit function called to determine a
 * smaller allocations should be used. A fully qualified address to the pvbuf
 * is returned in the pvbuf argument
 */
static inline xdp2_paddr_t xdp2_pvbuf_alloc_params(size_t size,
						   unsigned int fract,
						   unsigned int pvbuf_off,
						   struct xdp2_pvbuf **pvbuf)
{
	return __xdp2_pvbuf_alloc_params(&xdp2_pvbuf_global_mgr, size,
					 fract, pvbuf_off, pvbuf);
}

#define XDP2_PVBUF_DEFAULT_FRACT 32768
#define XDP2_PVBUF_DEFAULT_PVBUF_OFF 2

static inline xdp2_paddr_t __xdp2_pvbuf_alloc(struct xdp2_pvbuf_mgr *pvmgr,
					      size_t size,
					      struct xdp2_pvbuf **pvbuf)
{
	return  __xdp2_pvbuf_alloc_params(pvmgr, size, XDP2_PVBUF_DEFAULT_FRACT,
					  XDP2_PVBUF_DEFAULT_PVBUF_OFF, pvbuf);
}

static inline xdp2_paddr_t xdp2_pvbuf_alloc(size_t size,
					    struct xdp2_pvbuf **pvbuf)
{
	return  __xdp2_pvbuf_alloc(&xdp2_pvbuf_global_mgr, size, pvbuf);
}

/* Free a pvbuf given its paddr. The pvbuf is managed by the packet buffer
 * manager in the pvmgr argument
 */
void __xdp2_pvbuf_free(struct xdp2_pvbuf_mgr *pvmgr, xdp2_paddr_t paddr);

/* Free a pvbuf given its paddr. The pvbuf is managed by the global packet
 * buffer manager
 */
static inline void xdp2_pvbuf_free(xdp2_paddr_t paddr)
{
	__xdp2_pvbuf_free(&xdp2_pvbuf_global_mgr, paddr);
}

static inline void xdp2_pvbuf_print(xdp2_paddr_t paddr);

/* Calculate the total length of data in a pvbuf. This walks the pvbuf
 * and any embedded pvbufs. If the deep_dig argument is false then non-zero
 * lengths on iovecs with pvbufs are used, else lengths for iovecs with pvbufs
 * are ignored and only lengths of iovecs containing pbufs are used
 */
static inline size_t __xdp2_pvbuf_calc_length(struct xdp2_pvbuf_mgr *pvmgr,
					      xdp2_paddr_t paddr,
					      bool deep_dig)
{
	struct xdp2_pvbuf *pvbuf = __xdp2_pvbuf_paddr_to_addr(pvmgr, paddr);
	size_t size = 0, len;
	unsigned int i = 0;

	xdp2_pvbuf_iovec_map_foreach(pvbuf, i) {
		struct xdp2_iovec *iovec = &pvbuf->iovec[i];

		switch (xdp2_iovec_paddr_addr_tag(iovec)) {
		case XDP2_PADDR_TAG_PVBUF:
			if (iovec->pbaddr.pvbuf.data_length && !deep_dig) {
				size += iovec->pbaddr.pvbuf.data_length;
				continue;
			}

			len = __xdp2_pvbuf_calc_length(pvmgr,
						iovec->paddr, deep_dig);
			if (iovec->pbaddr.pvbuf.data_length) {
				if (iovec->pbaddr.pvbuf.data_length != len)
					xdp2_pvbuf_print(paddr);
				XDP2_ASSERT(iovec->pbaddr.pvbuf.data_length ==
									len,
					    "Deep dig length check failed: "
					    "%u != %lu\n",
					    iovec->pbaddr.pvbuf.data_length,
					    len);
			}

			size += len;
			break;
		case XDP2_PADDR_TAG_PBUF:
		case XDP2_PADDR_TAG_PBUF_1REF:
			size += xdp2_pbuf_get_data_len_from_iovec(iovec);
			break;
		case XDP2_PADDR_TAG_ALL_SHORT_ADDR_CASES:
			size += xdp2_paddr_short_addr_get_data_len_from_iovec(
									iovec);
			break;
		case XDP2_PADDR_TAG_LONG_ADDR:
			size += xdp2_paddr_long_addr_get_data_len_from_iovec(
									iovec);
			/* Increment index to skip second word of long address
			 */
			i++;
		}
	}

	return size;
}

/* Front end function to calculate the total length of a pvbuf by its
 * paddr without deep dig
 */
static inline size_t xdp2_pvbuf_calc_length(xdp2_paddr_t paddr)
{
	return __xdp2_pvbuf_calc_length(&xdp2_pvbuf_global_mgr, paddr, false);
}

/* Front end function to calculate the total length of a pvbuf by its
 * paddr with deep dig
 */
static inline size_t xdp2_pvbuf_calc_length_deep(xdp2_paddr_t paddr)
{
	return __xdp2_pvbuf_calc_length(&xdp2_pvbuf_global_mgr, paddr, true);
}

/* Validate the length of pvbuf. Calculates the length with and without deep
 * dig and compares the results
 */
static inline size_t xdp2_pvbuf_validate_length(xdp2_paddr_t paddr)
{
	size_t len = xdp2_pvbuf_calc_length(paddr);

	XDP2_ASSERT(len == xdp2_pvbuf_calc_length_deep(paddr),
		     "Length mismatch for pvbuf: %lu != %lu",
		     len, xdp2_pvbuf_calc_length_deep(paddr));

	return len;
}

/* Allocate, free, refcnt, and other pbuf helpers */

static inline void *___xdp2_pbuf_alloc(struct xdp2_pvbuf_mgr *pvmgr,
					struct xdp2_pbuf_allocator *pallocator,
					unsigned int *zindex,
					bool alloc_one_ref)
{
	unsigned int index;
	void *pbuf;

	/* Get an object from the object allocator */
	pbuf = xdp2_obj_alloc_alloc(XDP2_PVBUF_GET_ADDRESS(pvmgr,
					pallocator->allocator), &index);
	if (!pbuf)
		return NULL;

	/* Normalize index for use in pbuf addresses */
	*zindex = index - XDP2_OBJ_ALLOC_BASE_INDEX;

	/* Reference count for new pbuf is initialized to one */
	if (alloc_one_ref)
		pvmgr->allocs_1ref++;
	else
		atomic_store(&pallocator->refcnt[*zindex], 1);

	pvmgr->allocs++;

	return pbuf;
}

/* Allocate a pbuf of the requested size from the packet buffer manager in
 * the pvmgr argument. Return a paddr and a pointer the data in pbuf pointer
 * argument
 */
static inline xdp2_paddr_t __xdp2_pbuf_alloc(struct xdp2_pvbuf_mgr *pvmgr,
					       size_t size, bool tail_alloc,
					       void **pbuf)
{
	unsigned int size_shift = xdp2_get_log_round_up(size);
	struct xdp2_pbuf_allocator_entry *entry;
	struct xdp2_pbuf_allocator *pallocator;
	bool one_ref = pvmgr->alloc_one_ref;
	unsigned int zindex, offset;
	void *addr, *base = NULL;

	if (size_shift > XDP2_PBUF_MAX_SIZE_SHIFT)
		size_shift = XDP2_PBUF_MAX_SIZE_SHIFT;
	else if (size_shift < XDP2_PBUF_BASE_SIZE_SHIFT)
		size_shift = XDP2_PBUF_BASE_SIZE_SHIFT;

	entry = &pvmgr->pbuf_allocator_table[
			xdp2_pbuf_size_shift_to_buffer_tag(size_shift)];

	if (size > entry->alloc_size) {
		*pbuf = NULL;
		return XDP2_PADDR_NULL;
	}

	pallocator = XDP2_PVBUF_GET_ADDRESS(pvmgr, entry->pallocator);

	addr = ___xdp2_pbuf_alloc(pvmgr, pallocator, &zindex, one_ref);
	if (!addr) {
		*pbuf = NULL;
		return XDP2_PADDR_NULL;
	}

	offset = tail_alloc ? entry->alloc_size - size : 0;

	*pbuf = addr + offset;

	return xdp2_pbuf_make_paddr(one_ref, entry->alloc_size_shift,
				    zindex, offset, size, base, *pbuf);
}

/* Allocate a pbuf of the requested size from the global packet buffer
 * manager
 */
static inline xdp2_paddr_t xdp2_pbuf_alloc(size_t size, void **pbuf)
{
	return __xdp2_pbuf_alloc(&xdp2_pvbuf_global_mgr, size, false, pbuf);
}

/* Allocate a pbuf of the requested size from the global packet buffer
 * manager. Set offset a tail of pbuf
 */
static inline xdp2_paddr_t xdp2_pbuf_alloc_at_tail(size_t size, void **pbuf)
{
	return __xdp2_pbuf_alloc(&xdp2_pvbuf_global_mgr, size, true, pbuf);
}

/* Get the pbuf allocator and object allocator for a pbuf by its paddr */
static inline unsigned int __xdp2_pbuf_get_avals(struct xdp2_pvbuf_mgr *pvmgr,
		xdp2_paddr_t paddr, struct xdp2_pbuf_allocator **pallocator,
		struct xdp2_obj_allocator **allocator)
{
	unsigned int btag = xdp2_pbuf_buffer_tag_from_paddr(paddr);
	size_t offset = xdp2_pbuf_get_offset_from_paddr(paddr);
	struct xdp2_pbuf_allocator_entry *entry;

	btag -= (btag == XDP2_PBUF_NUM_SIZE_SHIFTS);

	entry = &pvmgr->pbuf_allocator_table[btag];
	*pallocator = XDP2_PVBUF_GET_ADDRESS(pvmgr, entry->pallocator);
	*allocator = XDP2_PVBUF_GET_ADDRESS(pvmgr, (*pallocator)->allocator);

	offset >>= xdp2_pbuf_buffer_tag_to_size_shift(btag);

	return (unsigned int)offset;
}

/* Get the reference count for a pbuf. Note that this returns the refcnt from
 * the pallocator refcnt array (that should be zero in the case of a single
 * reference pbuf)
 */
static inline unsigned int xdp2_pbuf_get_refcnt(struct xdp2_pvbuf_mgr *pvmgr,
						xdp2_paddr_t paddr)
{
	unsigned int ptag = xdp2_paddr_get_paddr_tag_from_paddr(paddr);
	struct xdp2_pbuf_allocator *pallocator;
	struct xdp2_obj_allocator *allocator;
	unsigned int zindex, refcnt;

	XDP2_ASSERT(ptag == XDP2_PADDR_TAG_PBUF ||
		    ptag == XDP2_PADDR_TAG_PBUF_1REF,
		    "Getting reference count from a non-pbuf paddr");

	zindex = __xdp2_pbuf_get_avals(pvmgr, paddr, &pallocator, &allocator);

	refcnt =  atomic_load(&pallocator->refcnt[zindex]);

	return refcnt;
}

/* Increment the reference count for a pbuf */
static inline void __xdp2_pbuf_bump_refcnt(struct xdp2_pvbuf_mgr *pvmgr,
					   xdp2_paddr_t paddr)
{
	unsigned int ptag = xdp2_paddr_get_paddr_tag_from_paddr(paddr);
	struct xdp2_pbuf_allocator *pallocator;
	struct xdp2_obj_allocator *allocator;
	unsigned int zindex;

	XDP2_ASSERT(ptag == XDP2_PADDR_TAG_PBUF,
		    "Can't bump refcnt for single refcnt pbuf");

	zindex = __xdp2_pbuf_get_avals(pvmgr, paddr, &pallocator, &allocator);

	atomic_fetch_add(&pallocator->refcnt[zindex], 1);
}

static inline void xdp2_pbuf_bump_refcnt(xdp2_paddr_t paddr)
{
	__xdp2_pbuf_bump_refcnt(&xdp2_pvbuf_global_mgr, paddr);
}

/* Free a pbuf given its paddr. The pbuf is managed by the packet buffer
 * manager in the pvmgr argument
 */
static inline void __xdp2_pbuf_free(struct xdp2_pvbuf_mgr *pvmgr,
				      xdp2_paddr_t paddr)
{
	unsigned int ptag = xdp2_paddr_get_paddr_tag_from_paddr(paddr);
	struct xdp2_pbuf_allocator *pallocator;
	struct xdp2_obj_allocator *allocator;
	unsigned int zindex;

	zindex = __xdp2_pbuf_get_avals(pvmgr, paddr, &pallocator, &allocator);

	if (ptag == XDP2_PADDR_TAG_PBUF_1REF) {
		unsigned int refcnt = xdp2_pbuf_get_refcnt(pvmgr, paddr);

		XDP2_ASSERT(!refcnt,
			     "Freeing single refcnt pbuf has refcnt %u",
			     refcnt);
		pvmgr->frees_1ref++;
	} else if (atomic_fetch_sub(&pallocator->refcnt[zindex], 1) != 1) {
		return;
	}

	xdp2_obj_alloc_free_by_index(allocator,
				      zindex + XDP2_OBJ_ALLOC_BASE_INDEX);
	pvmgr->frees++;
}

/* Free a pbuf given its paddr. The pbuf is managed by the global packet
 * buffer manager
 */
static inline void xdp2_pbuf_free(xdp2_paddr_t paddr)
{
	__xdp2_pbuf_free(&xdp2_pvbuf_global_mgr, paddr);
}

/* Free and reference count functions for short address paddrs */

static inline void __xdp2_paddr_short_addr_free(struct xdp2_pvbuf_mgr *pvmgr,
						xdp2_paddr_t paddr)
{
	struct xdp2_paddr pbaddr = { .paddr = paddr };

	__xdp2_paddr_short_addr_check(paddr, "Free short address paddr ");

	pvmgr->short_addr_ops[pbaddr.short_addr.short_paddr_tag].free(
								pvmgr, paddr);
}

static inline void xdp2_paddr_short_addr_free(xdp2_paddr_t paddr)
{
	__xdp2_paddr_short_addr_free(&xdp2_pvbuf_global_mgr, paddr);
}

static inline void __xdp2_paddr_short_addr_bump_refcnt(
		struct xdp2_pvbuf_mgr *pvmgr, xdp2_paddr_t paddr)
{
	struct xdp2_paddr pbaddr = { .paddr = paddr };

	__xdp2_paddr_short_addr_check(paddr, "Bump refcnt for short address "
					     "paddr ");

	pvmgr->short_addr_ops[pbaddr.short_addr.short_paddr_tag].bump_refcnt(
								pvmgr, paddr);
}

static inline void xdp2_paddr_short_addr_bump_refcnt(xdp2_paddr_t paddr)
{
	__xdp2_paddr_short_addr_bump_refcnt(&xdp2_pvbuf_global_mgr, paddr);
}

/* Free and reference count functions for long address paddrs */

static inline void __xdp2_paddr_long_addr_free(struct xdp2_pvbuf_mgr *pvmgr,
					       xdp2_paddr_t paddr_1,
					       xdp2_paddr_t paddr_2)
{
	struct xdp2_paddr pbaddr = { .paddr = paddr_1 };

	__xdp2_paddr_long_addr_check(paddr_1, paddr_2,
				     "Free long address paddr ");

	pvmgr->long_addr_ops[pbaddr.long_addr.memory_region].free(
		pvmgr, paddr_1, paddr_2);
}

static inline void xdp2_paddr_long_addr_free(xdp2_paddr_t paddr_1,
					     xdp2_paddr_t paddr_2)
{
	__xdp2_paddr_long_addr_free(&xdp2_pvbuf_global_mgr, paddr_1, paddr_2);
}

static inline void __xdp2_paddr_long_addr_bump_refcnt(
		struct xdp2_pvbuf_mgr *pvmgr, xdp2_paddr_t paddr_1,
		xdp2_paddr_t paddr_2)
{
	struct xdp2_paddr pbaddr = { .paddr = paddr_1 };

	__xdp2_paddr_long_addr_check(paddr_1, paddr_2,
				     "Bump refcnt for long address paddr ");

	pvmgr->long_addr_ops[pbaddr.long_addr.memory_region].bump_refcnt(
		pvmgr, paddr_1, paddr_2);
}

static inline void xdp2_paddr_long_addr_bump_refcnt(xdp2_paddr_t paddr_1,
						    xdp2_paddr_t paddr_2)
{
	__xdp2_paddr_long_addr_bump_refcnt(&xdp2_pvbuf_global_mgr, paddr_1,
					   paddr_2);
}

/* Functions that act on different paddr types */

static inline void __xdp2_pvbuf_paddr_bump_refcnt(
		struct xdp2_pvbuf_mgr *pvmgr, xdp2_paddr_t paddr_1,
		xdp2_paddr_t paddr_2)
{
	switch (xdp2_paddr_get_paddr_tag_from_paddr(paddr_1)) {
	case XDP2_PADDR_TAG_PBUF:
		__xdp2_pbuf_bump_refcnt(pvmgr, paddr_1);
		break;
	case XDP2_PADDR_TAG_ALL_SHORT_ADDR_CASES:
		__xdp2_paddr_short_addr_bump_refcnt(pvmgr, paddr_1);
		break;
	case XDP2_PADDR_TAG_LONG_ADDR:
		__xdp2_paddr_long_addr_bump_refcnt(pvmgr, paddr_1, paddr_2);
		break;
	default:
		XDP2_PADDR_TYPE_DEFAULT_ERR(paddr_1,
				"Set paddr iovec ent and bump refcnt");
	}
}

static inline void xdp2_pvbuf_paddr_bump_refcnt(
		xdp2_paddr_t paddr_1, xdp2_paddr_t paddr_2)
{
	__xdp2_pvbuf_paddr_bump_refcnt(&xdp2_pvbuf_global_mgr, paddr_1,
				       paddr_2);
}

static inline void __xdp2_paddr_free(
		struct xdp2_pvbuf_mgr *pvmgr, xdp2_paddr_t paddr_1,
		xdp2_paddr_t paddr_2)
{
	switch (xdp2_paddr_get_paddr_tag_from_paddr(paddr_1)) {
	case XDP2_PADDR_TAG_PBUF:
		__xdp2_pbuf_free(pvmgr, paddr_1);
		break;
	case XDP2_PADDR_TAG_ALL_SHORT_ADDR_CASES:
		__xdp2_paddr_short_addr_free(pvmgr, paddr_1);
		break;
	case XDP2_PADDR_TAG_LONG_ADDR:
		__xdp2_paddr_long_addr_free(pvmgr, paddr_1, paddr_2);
		break;
	default:
		XDP2_PADDR_TYPE_DEFAULT_ERR(paddr_1, "Free paddr");
	}
}

static inline void xdp2_paddr_free(xdp2_paddr_t paddr_1,
				   xdp2_paddr_t paddr_2)
{
	__xdp2_paddr_free(&xdp2_pvbuf_global_mgr, paddr_1, paddr_2);
}

static inline void xdp2_pvbuf_iovec_set_paddr_ent_and_bump_refcnt(
		struct xdp2_pvbuf_mgr *pvmgr, struct xdp2_pvbuf *pvbuf,
		unsigned int index, xdp2_paddr_t paddr_1,
		xdp2_paddr_t paddr_2, size_t length)
{
	/* Set input paddr(s) in the new pvbuf */
	switch (xdp2_paddr_get_paddr_tag_from_paddr(paddr_1)) {
	case XDP2_PADDR_TAG_PBUF:
		xdp2_pvbuf_iovec_set_pbuf_ent(pvbuf, index, paddr_1, length);
		__xdp2_pbuf_bump_refcnt(pvmgr, paddr_1);
		break;
	case XDP2_PADDR_TAG_ALL_SHORT_ADDR_CASES:
		xdp2_pvbuf_iovec_set_short_addr_ent(pvbuf, index, paddr_1,
						    length);
		__xdp2_paddr_short_addr_bump_refcnt(pvmgr, paddr_1);
		break;
	case XDP2_PADDR_TAG_LONG_ADDR:
		xdp2_pvbuf_iovec_set_long_addr_ent(pvbuf, index, paddr_1,
						   paddr_2, length);
		__xdp2_paddr_long_addr_bump_refcnt(pvmgr, paddr_1, paddr_2);
		break;
	default:
		XDP2_PADDR_TYPE_DEFAULT_ERR(paddr_1,
				"Set paddr iovec ent and bump refcnt");
	}
}

static inline void __xdp2_paddr_get_addr_len(
		struct xdp2_pvbuf_mgr *pvmgr, xdp2_paddr_t paddr_1,
		xdp2_paddr_t paddr_2, void **data_ptr, size_t *data_len)
{
	switch (xdp2_paddr_get_paddr_tag_from_paddr(paddr_1)) {
	case XDP2_PADDR_TAG_PBUF:
	case XDP2_PADDR_TAG_PBUF_1REF:
		*data_ptr = __xdp2_pbuf_paddr_to_addr(pvmgr, paddr_1),
		*data_len = xdp2_pbuf_get_data_len_from_paddr(paddr_1);
		break;
	case XDP2_PADDR_TAG_ALL_SHORT_ADDR_CASES:
		*data_ptr = __xdp2_paddr_short_addr_paddr_to_addr(pvmgr,
								  paddr_1);
		*data_len = xdp2_paddr_short_addr_get_data_len_from_paddr(
								paddr_1);
		break;
	case XDP2_PADDR_TAG_LONG_ADDR:
		XDP2_ASSERT(paddr_1, "Address is NULL instead of long "
			    "in paddr get addr len");
		*data_ptr = __xdp2_paddr_long_addr_to_addr(pvmgr,
				paddr_1, paddr_2);
		*data_len = xdp2_paddr_long_addr_get_data_len_from_paddr(
				paddr_1, paddr_2);
		break;
	default:
		XDP2_PADDR_TYPE_DEFAULT_ERR(paddr_1, "Get ptr/len");
	}
}

static inline void __xdp2_pvbuf_get_ptr_len_from_iovec(
		struct xdp2_pvbuf_mgr *pvmgr, struct xdp2_iovec *iovec,
		void **data_ptr, size_t *data_len, bool last_iovec)
{
	switch (xdp2_iovec_paddr_addr_tag(iovec)) {
	case XDP2_PADDR_TAG_LONG_ADDR:
		XDP2_ASSERT(iovec->paddr, "Address is NULL instead of long "
					  "in paddr get ptr len from iovec");
		/* For long addr if last_iovec is set then iovec is actually
		 * the second word of the paddr so subtract one from iovec
		 */
		if (last_iovec)
			iovec--;
		/* Fallthrough */

	case XDP2_PADDR_TAG_PBUF:
	case XDP2_PADDR_TAG_PBUF_1REF:
	case XDP2_PADDR_TAG_ALL_SHORT_ADDR_CASES:
		__xdp2_paddr_get_addr_len(pvmgr, iovec[0].paddr,
					  iovec[1].paddr,  data_ptr,
					  data_len);
		break;
	default:
		XDP2_IOVEC_PADDR_TYPE_DEFAULT_ERR(iovec,
						"Get ptr/len from iovec");
	}
}

static inline void xdp2_pvbuf_get_ptr_len_from_iovec(
		struct xdp2_iovec *iovec, void **data_ptr,
		size_t *data_len, bool last_iovec)
{
	__xdp2_pvbuf_get_ptr_len_from_iovec(
		&xdp2_pvbuf_global_mgr, iovec, data_ptr, data_len,
		last_iovec);
}

static inline size_t __xdp2_pvbuf_get_len_num_slots_from_iovec(
		struct xdp2_iovec *iovec, unsigned int *num_slots,
		struct xdp2_paddr *pbaddr2)
{
	size_t data_len;

	switch (xdp2_iovec_paddr_addr_tag(iovec)) {
	case XDP2_PADDR_TAG_PBUF:
	case XDP2_PADDR_TAG_PBUF_1REF:
		data_len = xdp2_pbuf_get_data_len_from_iovec(iovec);
		break;
	case XDP2_PADDR_TAG_ALL_SHORT_ADDR_CASES:
		data_len = xdp2_paddr_short_addr_get_data_len_from_iovec(iovec);
		break;
	case XDP2_PADDR_TAG_LONG_ADDR:
		XDP2_ASSERT(iovec->paddr, "Address is NULL instead of long "
					  "in get len num slots from iovec");
		data_len = xdp2_paddr_long_addr_get_data_len_from_iovec(iovec);
		(*num_slots)++;
		pbaddr2->paddr = iovec[1].paddr;
		break;
	default:
		XDP2_IOVEC_PADDR_TYPE_DEFAULT_ERR(iovec,
				"Get length and num_slots from iovec");
	}

	return data_len;
}

static inline void __xdp2_paddr_add_to_offset(struct xdp2_paddr *pbaddr_1,
					      struct xdp2_paddr *pbaddr_2,
					      size_t offset)
{
	switch (xdp2_paddr_get_paddr_tag_from_paddr(pbaddr_1->paddr)) {
	case XDP2_PADDR_TAG_PBUF:
	case XDP2_PADDR_TAG_PBUF_1REF:
		pbaddr_1->pbuf.offset += offset;
		break;
	case XDP2_PADDR_TAG_ALL_SHORT_ADDR_CASES:
		pbaddr_1->short_addr.offset += offset;
		break;
	case XDP2_PADDR_TAG_LONG_ADDR: {
		xdp2_paddr_t paddr1 = pbaddr_1->paddr;
		xdp2_paddr_t paddr2 = pbaddr_2->paddr;
		size_t toffset;

		XDP2_ASSERT(pbaddr_1->paddr, "Address is NULL instead of long "
					     "in add to offset");

		toffset = xdp2_paddr_long_addr_get_offset_from_paddr(
				pbaddr_1->paddr, pbaddr_2->paddr);
		toffset += offset;
		xdp2_paddr_long_addr_set_offset_from_paddr(
				&paddr1, &paddr2, toffset);

		toffset = xdp2_paddr_long_addr_get_offset_from_paddr(
				paddr1, paddr2);
		pbaddr_1->paddr = paddr1;
		pbaddr_2->paddr = paddr2;

		toffset = xdp2_paddr_long_addr_get_offset_from_paddr(
				pbaddr_1->paddr, pbaddr_2->paddr);
		break;
	}
	default:
		XDP2_ERR(1, "Add offset to paddr unexpected paddr type %s (%u)",
			 xdp2_paddr_print_paddr_type(pbaddr_1->paddr),
			 xdp2_paddr_get_paddr_tag_from_paddr(pbaddr_1->paddr));
	}
}

/* Prepend functions */

/* Backend function that is called when prepending headers and the zeroth slot
 * in the iovec of the pvbuf being prepended into is non-NULL
 *
 * Return true on success or false otherwise due to an object allocation failure
 */
bool ___xdp2_pvbuf_prepend_pvbuf(struct xdp2_pvbuf_mgr *pvmgr,
				 xdp2_paddr_t pvbuf_paddr,
				 xdp2_paddr_t paddr,
				 size_t length, bool compress);

static inline bool __xdp2_pvbuf_prepend_paddr(struct xdp2_pvbuf_mgr *pvmgr,
					      xdp2_paddr_t pvbuf_paddr,
					      xdp2_paddr_t paddr_1,
					      xdp2_paddr_t paddr_2,
					      size_t length, bool compress);

static inline bool xdp2_pvbuf_prepend_paddr(xdp2_paddr_t pvbuf_paddr,
					    xdp2_paddr_t paddr_1,
					    xdp2_paddr_t paddr_2,
					    size_t length, bool compress)
{
	return __xdp2_pvbuf_prepend_paddr(&xdp2_pvbuf_global_mgr, pvbuf_paddr,
					  paddr_1, paddr_2, length, compress);
}

bool __xdp2_pvbuf_prepend_pvbuf_compress(struct xdp2_pvbuf_mgr *pvmgr,
					 xdp2_paddr_t paddr,
					 xdp2_paddr_t hdrs_paddr,
					 size_t length, unsigned int weight);

/* Insert packet headers at the front of a pvbuf. The target pvbuf is in the
 * pvbuf argument, and the pvbuf with the headers to be prepended is in the
 * pvbuf_headers argument. The length argument is the length of data in the
 * pvbuf_headers, it may set to zero if the length is unknown. Both of the
 * pvbufs are managed by the packet buffer manager in the pvmgr argument
 *
 * Return true on success or false otherwise due to an object allocation failure
 */
static inline bool __xdp2_pvbuf_prepend_pvbuf(struct xdp2_pvbuf_mgr *pvmgr,
					       xdp2_paddr_t pvbuf_paddr,
					       xdp2_paddr_t hdrs_paddr,
					       size_t length, bool compress)
{
	struct xdp2_pvbuf *pvbuf =
			__xdp2_pvbuf_paddr_to_addr(pvmgr, pvbuf_paddr);
	struct xdp2_pvbuf *pvbuf_headers =
			__xdp2_pvbuf_paddr_to_addr(pvmgr, hdrs_paddr);
	unsigned int i, weight;

	if (compress && (weight =
			 xdp2_pvbuf_iovec_map_weight(pvbuf_headers) <= 1))
		return __xdp2_pvbuf_prepend_pvbuf_compress(pvmgr, pvbuf_paddr,
							   hdrs_paddr,
							   length, weight);

	/* Try to locate an empty slot */
	i = xdp2_pvbuf_iovec_map_find(pvbuf);

	if (i > 0) {
		/* Hurray! Found an empty slot. Just set the pvbuf at that
		 * location
		 */

		/* If the pvbuf is empty then 64 is returned so cap to the
		 * number of iovecs for the pvbuf
		 */
		i = xdp2_min(i, XDP2_PVBUF_NUM_IOVECS_FROM_PADDR(pvbuf_paddr));
		xdp2_pvbuf_iovec_set_pvbuf_ent(pvbuf, i - 1, hdrs_paddr,
						length);
		return true;
	}

	return ___xdp2_pvbuf_prepend_pvbuf(pvmgr, pvbuf_paddr, hdrs_paddr,
					   length,
					    compress);
}

/* Insert packet headers at the front of a pvbuf. The target pvbuf is in the
 * pvbuf argument, and the pvbuf with the headers to be prepended is in the
 * pvbuf_headers argument. The length argument is the length of data in the
 * pvbuf_headers, it may set to zero if the length is unknown. Both of the
 * pvbufs are managed by the global packet buffer manager
 *
 * Return true on success or false otherwise due to an object allocation failure
 */
static inline bool xdp2_pvbuf_prepend_pvbuf(xdp2_paddr_t paddr,
					     xdp2_paddr_t hdrs_paddr,
					     size_t length, bool compress)
{
	return __xdp2_pvbuf_prepend_pvbuf(&xdp2_pvbuf_global_mgr,
					   paddr, hdrs_paddr,
					   length < XDP2_PVBUF_MAX_LEN ?
								length : 0,
					   compress);
}

/* Backed function that is called when prepending headers from a pbuf and the
 * zeroth slot in the iovec of the pvbuf being prepended into is non-NULL
 */
bool ___xdp2_pvbuf_prepend_paddr(struct xdp2_pvbuf_mgr *pvmgr,
				 xdp2_paddr_t pvbuf_paddr,
				 xdp2_paddr_t paddr_1,
				 xdp2_paddr_t paddr_2,
				 size_t length, bool compress);

bool ___xdp2_pvbuf_prepend_paddr_into_iovec(
		struct xdp2_pvbuf_mgr *pvmgr, struct xdp2_pvbuf *pvbuf,
		unsigned int index, xdp2_paddr_t paddr_1,
		xdp2_paddr_t paddr_2, size_t length, bool compress);

unsigned int __xdp2_pvbuf_find_prepend_slot_compress(
		struct xdp2_pvbuf *pvbuf, unsigned int num_iovecs);

static inline unsigned int __xdp2_pvbuf_find_prepend_slot(
		struct xdp2_pvbuf *pvbuf, xdp2_paddr_t paddr,
		bool compress)
{
	unsigned int num_iovecs =
			XDP2_PVBUF_NUM_IOVECS_FROM_PADDR(paddr);
	unsigned int i;

	i = xdp2_pvbuf_iovec_map_find(pvbuf);
	if (i != 0) {
		/* Hurray! We found an available slot */

		i = (i < num_iovecs) ? i - 1 : num_iovecs - 1;
		return i;
	}

	if (compress)
		return __xdp2_pvbuf_find_prepend_slot_compress(pvbuf,
							       num_iovecs);

	return -1U;
}

/* Insert packet headers at the front of a pvbuf from a pbuf. The target pvbuf
 * is in the pvbuf argument, and the pbuf with the headers to be prepended is in
 * the pbuf_headers argument as a forty-four bit address. The length argument
 * is the length of data in the pbuf_headers (it may be zero where the
 * pbuf_headers has tag 15 in the address to indicate the size is 1 << 20)
 *
 * Return true on success or false otherwise due to an object allocation failure
 */
static inline bool __xdp2_pvbuf_prepend_paddr(struct xdp2_pvbuf_mgr *pvmgr,
					      xdp2_paddr_t pvbuf_paddr,
					      xdp2_paddr_t paddr_1,
					      xdp2_paddr_t paddr_2,
					      size_t length, bool compress)
{
	unsigned int num_iovecs = XDP2_PVBUF_NUM_IOVECS_FROM_PADDR(pvbuf_paddr);
	struct xdp2_pvbuf *pvbuf =
			__xdp2_pvbuf_paddr_to_addr(pvmgr, pvbuf_paddr);
	bool is_long_addr = xdp2_paddr_get_paddr_tag_from_paddr(paddr_1) ==
					XDP2_PADDR_TAG_LONG_ADDR;
	unsigned int index;

	index = __xdp2_pvbuf_find_prepend_slot(pvbuf, pvbuf_paddr, compress);
	if (index < num_iovecs && (index > 0 || !is_long_addr)) {
		xdp2_pvbuf_iovec_set_paddr_ent(pvbuf, index - is_long_addr,
					       paddr_1, paddr_2, length);
		return true;
	}

	/* There's two possibilities for index. It can be greater then 64 in
	 * which case slot zero must be set so use that. Otherwise the index
	 * is zero but it's a long address which doesn't fit in one slot so
	 * insert at index one which must be set of it would have been
	 * returned by find prepend slot
	 */
	index = index || xdp2_pvbuf_iovec_map_isset(pvbuf, index) ? 0 : 1;

	return ___xdp2_pvbuf_prepend_paddr_into_iovec(
		pvmgr, pvbuf, index, paddr_1, paddr_2, length, compress);
}

/* Insert packet headers at the front of a pvbuf. The target pvbuf is in the
 * pvbuf argument, and the pvbuf with the headers to be prepended is in the
 * pvbuf_headers argument. The length argument is the length of data in the
 * pvbuf_headers, it may set to zero if the length is unknown. Both of the
 * pvbufs are managed by the global packet buffer manager
 *
 * Return true on success or false otherwise due to an object allocation failure
 * The operation is potentially destructive on the input pvbuf so if false is
 * returned the pvbuf should be freed without further processing
 */
static inline bool xdp2_pvbuf_prepend_pbuf(xdp2_paddr_t paddr,
					    xdp2_paddr_t pbuf_paddr,
					    size_t length, bool compress)
{
	return __xdp2_pvbuf_prepend_paddr(&xdp2_pvbuf_global_mgr,
					  paddr, pbuf_paddr, XDP2_PADDR_NULL,
					  length, compress);
}

/* Append functions */

/* Backed function that is called when append trailers and the last slot
 * in the iovec of the pvbuf being prepended into is non-NULL
 */
bool ___xdp2_pvbuf_append_pvbuf(struct xdp2_pvbuf_mgr *pvmgr,
				 xdp2_paddr_t paddr, xdp2_paddr_t trls_paddr,
				 size_t length, bool compress);

static inline bool __xdp2_pvbuf_append_paddr(struct xdp2_pvbuf_mgr *pvmgr,
					     xdp2_paddr_t pvbuf_paddr,
					     xdp2_paddr_t paddr_1,
					     xdp2_paddr_t paddr_2,
					     size_t length, bool compress);


bool __xdp2_pvbuf_append_pvbuf_compress(struct xdp2_pvbuf_mgr *pvmgr,
					xdp2_paddr_t pvbuf_paddr,
					xdp2_paddr_t trls_paddr,
					size_t length, unsigned int weight);

/* Append packet trailers at the end of a pvbuf. The target pvbuf is in the
 * pvbuf argument, and the pvbuf with the trailers to be prepended is in the
 * pvbuf_trailers argument. The length argument is the length of data in the
 * pvbuf_headers, it may set to zero if the length is unknown. Both of the
 * pvbufs are managed by the packet buffer manager in the pvmgr argument
 *
 * Return true on success or false otherwise due to an object allocation failure
 */
static inline bool __xdp2_pvbuf_append_pvbuf(struct xdp2_pvbuf_mgr *pvmgr,
					      xdp2_paddr_t pvbuf_paddr,
					      xdp2_paddr_t trls_paddr,
					      size_t length, bool compress)
{
	struct xdp2_pvbuf *pvbuf =
			__xdp2_pvbuf_paddr_to_addr(pvmgr, pvbuf_paddr);
	struct xdp2_pvbuf *pvbuf_trailers =
			__xdp2_pvbuf_paddr_to_addr(pvmgr, trls_paddr);
	unsigned int i, weight;

	if (compress && (weight =
			 xdp2_pvbuf_iovec_map_weight(pvbuf_trailers) <= 1))
		return __xdp2_pvbuf_append_pvbuf_compress(pvmgr, pvbuf_paddr,
							  trls_paddr, length,
							  weight);

	/* Try to locate an empty slot */
	i = xdp2_pvbuf_iovec_map_rev_find(pvbuf);

	if (i < XDP2_PVBUF_NUM_IOVECS_FROM_PADDR(pvbuf_paddr) - 1) {
		/* Hurray! Found an empty slot. Just set the pvbuf at that
		 * location
		 */
		xdp2_pvbuf_iovec_set_pvbuf_ent(pvbuf, i + 1, trls_paddr,
						length);
		return true;
	}

	return ___xdp2_pvbuf_append_pvbuf(pvmgr, pvbuf_paddr, trls_paddr,
					  length, compress);
}

/* Append packet trailers at the end of a pvbuf. The target pvbuf is in the
 * pvbuf argument, and the pvbuf with the trailers to be appended is in the
 * pvbuf_trailers argument. The length argument is the length of data in the
 * pvbuf_headers, it may set to zero if the length is unknown. Both of the
 * pvbufs are managed by the global packet buffer manager
 *
 * Return true on success or false otherwise due to an object allocation failure
 */
static inline bool xdp2_pvbuf_append_pvbuf(xdp2_paddr_t pvbuf_paddr,
					   xdp2_paddr_t paddr,
					   size_t length, bool compress)
{
	return __xdp2_pvbuf_append_pvbuf(&xdp2_pvbuf_global_mgr,
					  pvbuf_paddr, paddr,
					  length < XDP2_PVBUF_MAX_LEN ?
								length : 0,
					  compress);
}

/* Backed function that is called when appending trailers from a pbuf and the
 * zeroth slot in the iovec of the pvbuf being appended into is non-NULL
 *
 * Return true on success or false otherwise due to an object allocation failure
 */
bool ___xdp2_pvbuf_append_paddr(struct xdp2_pvbuf_mgr *pvmgr,
				xdp2_paddr_t pvbuf_paddr,
				xdp2_paddr_t paddr_1,
				xdp2_paddr_t paddr_2,
				size_t length, bool compress);

bool ___xdp2_pvbuf_append_paddr_into_iovec(struct xdp2_pvbuf_mgr *pvmgr,
					   struct xdp2_pvbuf *pvbuf,
					   unsigned int index,
					   xdp2_paddr_t paddr_1,
					   xdp2_paddr_t paddr_2,
					   size_t length, bool index_set,
					   bool compress);

unsigned int __xdp2_pvbuf_find_append_slot_compress(struct xdp2_pvbuf *pvbuf,
						    unsigned int num_iovecs);

static unsigned int __xdp2_pvbuf_find_append_slot(struct xdp2_pvbuf *pvbuf,
						  xdp2_paddr_t paddr,
						  bool compress)
{
	unsigned int num_iovecs = XDP2_PVBUF_NUM_IOVECS_FROM_PADDR(paddr);
	unsigned int i;

	i = xdp2_pvbuf_iovec_map_rev_find(pvbuf);
	if (i != num_iovecs - 1) {
		/* Hurray! We found an available slot */

		i = (i < num_iovecs) ? i + 1 : 0;
		return i;
	}

	if (compress)
		return __xdp2_pvbuf_find_append_slot_compress(pvbuf,
							      num_iovecs);

	return -1U;
}

/* Append packet trailers at the end of a pvbuf from a pbuf. The target pvbuf
 * is in the pvbuf argument, and the pbuf with the trailers to be appended is in
 * the pbuf_trailers argument as a forty-four bit address. The length argument
 * is the length of data in the pbuf_headers (it may be zero where the
 * pbuf_headers has tag 15 in the address to indicate the size is 1 << 20)
 *
 * Return true on success or false otherwise due to an object allocation failure
 */
static inline bool __xdp2_pvbuf_append_paddr(struct xdp2_pvbuf_mgr *pvmgr,
					     xdp2_paddr_t pvbuf_paddr,
					     xdp2_paddr_t paddr_1,
					     xdp2_paddr_t paddr_2,
					     size_t length, bool compress)
{
	struct xdp2_pvbuf *pvbuf =
			__xdp2_pvbuf_paddr_to_addr(pvmgr, pvbuf_paddr);
	unsigned int num_iovecs =
			XDP2_PVBUF_NUM_IOVECS_FROM_PADDR(pvbuf_paddr);
	bool is_long_addr = xdp2_paddr_get_paddr_tag_from_paddr(paddr_1) ==
					XDP2_PADDR_TAG_LONG_ADDR;
	unsigned int i;
	bool last_set;

	i = __xdp2_pvbuf_find_append_slot(pvbuf, pvbuf_paddr, compress);
	if (i < num_iovecs && (i != num_iovecs - 1 || !is_long_addr)) {
		xdp2_pvbuf_iovec_set_paddr_ent(pvbuf, i, paddr_1, paddr_2,
					       length);
		return true;
	}

	last_set = xdp2_pvbuf_iovec_map_isset(pvbuf, num_iovecs - 1);
	return ___xdp2_pvbuf_append_paddr_into_iovec(pvmgr, pvbuf,
						     num_iovecs - 1, paddr_1,
						     paddr_2, length,
						     last_set, compress);
}

/* Append packet trailers at the front of a pvbuf. The target pvbuf is in the
 * pvbuf argument, and the pvbuf with the trailers to be appended is in the
 * pvbuf_trailers argument. The length argument is the length of data in the
 * pvbuf_trailers, it may set to zero if the length is unknown. Both of the
 * pvbufs are managed by the global packet buffer manager
 *
 * Return true on success or false otherwise due to an object allocation failure
 * The operation is potentially destructive on the input pvbuf so if false is
 * returned the pvbuf should be freed without further processing
 */
static inline bool xdp2_pvbuf_append_paddr(xdp2_paddr_t pvbuf_paddr,
					   xdp2_paddr_t paddr_1,
					   xdp2_paddr_t paddr_2,
					   size_t length, bool compress)
{
	return __xdp2_pvbuf_append_paddr(&xdp2_pvbuf_global_mgr, pvbuf_paddr,
					 paddr_1, paddr_2, length,
					 compress);
}

/* Pop headers from the front of a pvbuf. The target pvbuf is in the
 * pvbuf argument, and number of bytes to drop is in num_drop_bytes.
 * The pvbuf is managed by the packet buffer manager in the pvmgr argument
 *
 * Returns the number of bytes actually dropped
 */
size_t __xdp2_pvbuf_pop_hdrs(struct xdp2_pvbuf_mgr *pvmgr,
			      xdp2_paddr_t paddr, size_t num_drop_bytes,
			      bool compress, void **copyptr);

/* Pop functions */

/* Pop headers from the front of a pvbuf. The target pvbuf is in the
 * pvbuf argument, and number of bytes to drop is in num_drop_bytes.
 * The pvbuf is managed by the global packet buffer manager
 *
 * Returns the number of bytes actually dropped
 */
static inline size_t xdp2_pvbuf_pop_hdrs(xdp2_paddr_t paddr,
					  size_t num_drop_bytes,
					  bool compress) {
	return __xdp2_pvbuf_pop_hdrs(&xdp2_pvbuf_global_mgr, paddr,
				      num_drop_bytes, compress, NULL);
}

/* Pop trailers from the end of a pvbuf. The target pvbuf is in the
 * pvbuf argument, and number of bytes to drop is in num_drop_bytes.
 * The pvbuf is managed by the packet buffer manager in the pvmgr argument
 *
 * Returns the number of bytes actually dropped
 */
size_t __xdp2_pvbuf_pop_trailers(struct xdp2_pvbuf_mgr *pvmgr,
				  xdp2_paddr_t paddr, size_t num_drop_bytes,
				  bool compress, void **copyptr);

/* Pop trailers from the end of a pvbuf. The target pvbuf is in the
 * pvbuf argument, and number of bytes to drop is in num_drop_bytes.
 * The pvbuf is managed by the global packet buffer manager
 *
 * Returns the number of bytes actually dropped
 */
static inline size_t xdp2_pvbuf_pop_trailers(xdp2_paddr_t paddr,
					      size_t num_drop_bytes,
					      bool compress) {
	return __xdp2_pvbuf_pop_trailers(&xdp2_pvbuf_global_mgr, paddr,
					  num_drop_bytes, compress, NULL);
}

/* Pullup and pulltail functions */

/* Helper function to prepend a pbuf when doing a pullup */
bool __xdp2_pvbuf_prepend_pbuf_pullup(struct xdp2_pvbuf_mgr *pvmgr,
				       xdp2_paddr_t paddr,
				       xdp2_paddr_t pbuf_paddr,
				       size_t length, bool compress);

/* Pull up a number of bytes into a contiguous buffer. length is a pointer:
 * on input this is the number of bytes to be pulled up, and on output it
 * equals the number of bytes actually pulled up
 *
 * Return a pointer the data with the pulled up data or NULL in the case of
 * pbuf or pvbuf allocation failure. Note that the operation is potentially
 * destructive to the so if NULL is returned then the caller should free the
 * pvbuf with no further processing on it
 *
 * The compress argument indicates that the data should be pulled up into
 * the tailroom of the first pbuf if there is space
 */
static inline void *__xdp2_pvbuf_pullup(struct xdp2_pvbuf_mgr *pvmgr,
					 xdp2_paddr_t pvbuf_paddr,
					 size_t *length, bool compress)
{
	unsigned int num_iovecs =
			XDP2_PVBUF_NUM_IOVECS_FROM_PADDR(pvbuf_paddr);
	struct xdp2_pvbuf *pvbuf =
			__xdp2_pvbuf_paddr_to_addr(pvmgr, pvbuf_paddr);
	size_t start_length, copy_length, data_len;
	void *data_ptr = NULL, *copyptr;
	struct xdp2_iovec *iovec;
	xdp2_paddr_t paddr;
	unsigned int index;

	index = xdp2_pvbuf_iovec_map_find(pvbuf);
	if (index >= num_iovecs)
		return NULL;

	iovec = &pvbuf->iovec[index];
	if (!XDP2_IOVEC_IS_PVBUF(iovec)) {
		__xdp2_pvbuf_get_ptr_len_from_iovec(pvmgr, iovec, &data_ptr,
						    &data_len, false);

		if (data_len >= *length) {
			/* We have all the data we need in the first buffer */
			*length = data_len;
			return data_ptr;
		}

		if (XDP2_IOVEC_IS_PBUF(iovec) && compress &&
		    xdp2_pbuf_get_refcnt(pvmgr, iovec->paddr) == 1 &&
		    __xdp2_pbuf_get_tailroom_from_paddr(pvmgr, iovec->paddr) >=
								*length) {
			/* The iovec is a single reference paddr and it has
			 * sufficient tailroom space for the pullup
			 */
			start_length = data_len;
			copyptr = data_ptr + start_length;
			paddr = iovec->paddr;

			/* Unset temporarily so that pop headers works */
			xdp2_pvbuf_iovec_unset_ent(pvbuf, index);

			goto doit;
		}
	}

	/* Allocate a new pbuf to hold the pulled up data, we'll prepend
	 * this in the pvbuf
	 */
	paddr = __xdp2_pbuf_alloc(pvmgr, *length, false, &data_ptr);
	if (!data_ptr) {
		*length = 0;
		return NULL;
	}

	copyptr = data_ptr;
	start_length = 0;

doit:
	/* Pop the headers up to the pullup length and copy the dropped data
	 * into the new pbuf. Pop headers cannot fail on object allocation so
	 * we don't need to check the return value which is the number of bytes
	 * dropped
	 */
	copy_length = *length - start_length;
	*length = __xdp2_pvbuf_pop_hdrs(pvmgr, pvbuf_paddr, copy_length,
					compress, &copyptr) + start_length;

	XDP2_ASSERT(copy_length == (uintptr_t)copyptr - (uintptr_t)data_ptr -
							start_length,
		     "Length headers popped doesn't equal length copied: "
		     "%lu != %lu\n", copy_length,
		     (uintptr_t)copyptr - (uintptr_t)data_ptr);

	/* Now prepend the pbuf back into the pvbuf to complete the pullup */
	if (!__xdp2_pvbuf_prepend_pbuf_pullup(pvmgr, pvbuf_paddr, paddr,
					       *length, compress)) {
		*length = 0;
		__xdp2_pbuf_free(pvmgr, paddr);
		return NULL;
	}

	return data_ptr;
}

/* Pull up a number of bytes into a contiguous buffer in a pvbuf managed
 * by the global packet buffer manager
 */
static inline void *xdp2_pvbuf_pullup(xdp2_paddr_t paddr,
				       size_t length, bool compress)
{
	size_t orig_length = length;
	void *ret;

	ret = __xdp2_pvbuf_pullup(&xdp2_pvbuf_global_mgr, paddr, &length,
				   compress);

	if (ret && length < orig_length)
		ret = NULL;

	return ret;
}

/* Helper function to pull the tail of a pbuf when doing a append */
bool __xdp2_pvbuf_append_pbuf_pulltail(struct xdp2_pvbuf_mgr *pvmgr,
					xdp2_paddr_t paddr, __u64 pbuf_paddr,
					size_t length);

/* Find the last non-NULL iovec in a pvbuf */
static inline struct xdp2_iovec *xdp2_pvbuf_find_last_iovec(
		struct xdp2_pvbuf_mgr *pvmgr, xdp2_paddr_t paddr)
{
	struct xdp2_pvbuf *pvbuf = __xdp2_pvbuf_paddr_to_addr(pvmgr, paddr);
	int i = XDP2_PVBUF_NUM_IOVECS_FROM_PADDR(paddr) - 1;

	xdp2_pvbuf_iovec_map_rev_foreach(pvbuf, i) {
		XDP2_ASSERT(pvbuf->iovec[i].paddr != XDP2_PADDR_NULL,
			    "NULL iovec but iovec bit set");

		if (!XDP2_IOVEC_IS_PVBUF(&pvbuf->iovec[i]))
			return &pvbuf->iovec[i];

		/* Must be pvbuf, recurse */
		return xdp2_pvbuf_find_last_iovec(pvmgr, pvbuf->iovec[i].paddr);
	}

	return NULL;
}

/* Pull a number of bytes from the tail of a pvbuf into a contiguous buffer.
 * length is a pointer: on input this is the number of bytes pulled, and on
 * output it equals the number of bytes actually pulled
 *
 * Return a pointer the to the pbuf with the pulled data or NULL in the
 * case of pbuf or pvbuf allocation failure. Note that the operation is
 * potentially destructive to the so if NULL is returned then the caller
 * should free the pvbuf with no further processing on it
 */
/* Pull the tail of a packet up a number of bytes into a contiguous buffer.
 * length contains the number of bytes to pull up
 */
static inline void *__xdp2_pvbuf_pulltail(struct xdp2_pvbuf_mgr *pvmgr,
					   xdp2_paddr_t pvbuf_paddr,
					   size_t *length, bool compress)
{
	size_t start_length = 0, copy_length;
	void *copyptr, *orig_copyptr;
	struct xdp2_iovec *iovec;
	xdp2_paddr_t paddr;
	void *pbuf = NULL;

	/* XXXTH Don't compress when doing a pulltail. When compress is
	 * enabled, there are test failures on corrupted data. Need to
	 * investigate further at some point. There is a abd interaction
	 * between copying the data to a new pbuf and compressing. The
	 * corrupted data but no length error suggest that data is being
	 * placed out of order. The problem takes a while to reproduce and
	 * since compression isn't essential here we'll just skip compressing
	 */
	compress = false;

	iovec = xdp2_pvbuf_find_last_iovec(pvmgr, pvbuf_paddr);

	if (iovec) {
		size_t data_len;
		void *data_ptr;

		/* Must be a non-pvbuf */
		__xdp2_pvbuf_get_ptr_len_from_iovec(pvmgr, iovec, &data_ptr,
						    &data_len, true);

		if (data_len >= *length) {
			void *ret;

			ret = data_ptr + data_len - *length;
			*length = data_len;

			return ret;
		}
	}
	/* Allocate a new pbuf and pull the tail into it */
	paddr = __xdp2_pbuf_alloc(pvmgr, *length, false, &pbuf);
	if (!pbuf) {
		*length = 0;
		return NULL;
	}

	copyptr = pbuf + *length;

	orig_copyptr = copyptr;

	/* Pop the trailers up to the pullup length and copy the dropped data
	 * into the new pbuf. Pop trailers cannot fail on object allocation so
	 * we don't need to check the return value which is the number of bytes
	 * dropped
	 */
	copy_length = *length - start_length;
	*length = __xdp2_pvbuf_pop_trailers(pvmgr, pvbuf_paddr,
					     copy_length, compress,
					     &copyptr) + start_length;
	XDP2_ASSERT(copy_length == (uintptr_t)orig_copyptr - (uintptr_t)copyptr,
		     "Length tail popped doesn't equal length copied "
		     "%lu != %lu\n", copy_length,
		     (uintptr_t)orig_copyptr - (uintptr_t)copyptr);

	if (!__xdp2_pvbuf_append_pbuf_pulltail(pvmgr, pvbuf_paddr, paddr,
					       *length)) {
		*length = 0;
		__xdp2_pbuf_free(pvmgr, paddr);
		return NULL;
	}

	return copyptr;
}

/* Pull the tail of a pvbuf managed by the global packet buffer manager */
static inline void *xdp2_pvbuf_pulltail(xdp2_paddr_t paddr, size_t length,
					 bool compress)
{
	size_t orig_length = length;
	void *ret;

	ret = __xdp2_pvbuf_pulltail(&xdp2_pvbuf_global_mgr, paddr, &length,
				     compress);

	if (ret && length < orig_length)
		ret = NULL;

	return ret;
}

/* Iterator functions */

/* Iterator call back. The iterator is called for each pbuf (data is a
 * pointer the first byte of the pbuf and len is the length of the pbuf
 */
typedef bool (*xdp2_pvbuf_iterate_t)(void *priv, __u8 *data, size_t len);

/* Perform an iterator over a pvbuf. Walk over the pvbuf and for each pbuf
 * call the callback function
 */
static inline bool __xdp2_pvbuf_iterate(struct xdp2_pvbuf_mgr *pvmgr,
					 xdp2_paddr_t paddr,
					 xdp2_pvbuf_iterate_t func,
					 void *priv)
{
	struct xdp2_pvbuf *pvbuf = __xdp2_pvbuf_paddr_to_addr(pvmgr, paddr);
	unsigned int i = 0;
	bool cont = true;
	size_t data_len;
	void *data_ptr;

	xdp2_pvbuf_iovec_map_foreach(pvbuf, i) {
		struct xdp2_iovec *iovec = &pvbuf->iovec[i];

		if (XDP2_IOVEC_IS_PVBUF(iovec)) {
			cont = __xdp2_pvbuf_iterate(pvmgr, iovec->paddr,
						     func, priv);
		} else {
			/* Not a pvbuf */
			__xdp2_pvbuf_get_ptr_len_from_iovec(pvmgr, iovec,
						&data_ptr, &data_len, false);
			cont = func(priv, data_ptr, data_len);

			if (xdp2_iovec_paddr_addr_tag(iovec) ==
					XDP2_PADDR_TAG_LONG_ADDR) {
				/* Long address paddrs occupy two slots so
				 * we need to skip
				 */
				i++;

				XDP2_ASSERT(xdp2_pvbuf_iovec_map_isset(pvbuf,
								       i),
					"Expected map bit %u of second word "
					"of long addr set in pvbuf iterate", i);
			}
		}

		if (!cont)
			break;
	}
	return cont;
}

/* Front end function to iterate over a pvbuf */
static inline bool xdp2_pvbuf_iterate(xdp2_paddr_t paddr,
				       xdp2_pvbuf_iterate_t func,
				       void *priv)
{
	return __xdp2_pvbuf_iterate(&xdp2_pvbuf_global_mgr,
				    paddr, func, priv);
}

/* Iterator call back with paddr argument. The iterator is called for each
 * pbuf where paddr encodes the address and length from an iovec
 */
typedef bool (*xdp2_pvbuf_iterate_paddr_t)(void *priv, xdp2_paddr_t paddr,
					    xdp2_paddr_t *new_paddr);

/* Iterator call back with iovec pointer argument. The iterator is called for
 * each non-pvbuf iovec. The iovec is writable back to the iovec in the
 * pvbuf
 */
typedef bool (*xdp2_pvbuf_iterate_iovec_t)(void *priv,
					   struct xdp2_iovec *iovec);

/* Perform an iterator over a pvbuf and provide an iovec in the callback. Walk
 * over the pvbuf and for each iovec call the callback function
 */
static inline bool __xdp2_pvbuf_iterate_iovec(struct xdp2_pvbuf_mgr *pvmgr,
		xdp2_paddr_t pvbuf_paddr, xdp2_pvbuf_iterate_iovec_t func,
		void *priv)
{
	struct xdp2_pvbuf *pvbuf =
				__xdp2_pvbuf_paddr_to_addr(pvmgr, pvbuf_paddr);
	unsigned int i = 0;
	bool cont = true;

	xdp2_pvbuf_iovec_map_foreach(pvbuf, i) {
		struct xdp2_iovec *iovec = &pvbuf->iovec[i];

		XDP2_ASSERT(iovec->paddr, "Address is NULL in iterate iovec");

		switch (xdp2_iovec_paddr_addr_tag(iovec)) {
		case XDP2_PADDR_TAG_PVBUF:
			cont = __xdp2_pvbuf_iterate_iovec(pvmgr, iovec->paddr,
							  func, priv);
			break;
		case XDP2_PADDR_TAG_LONG_ADDR:
			cont = func(priv, iovec);

			/* Long address paddrs occupy two slots so we need to
			 * skip
			 */
			i++;

			XDP2_ASSERT(xdp2_pvbuf_iovec_map_isset(pvbuf,
							       i),
				"Expected map bit %u of second word "
				"of long addr set in pvbuf iterate", i);
			break;
		case XDP2_PADDR_TAG_PBUF:
		case XDP2_PADDR_TAG_PBUF_1REF:
		case XDP2_PADDR_TAG_ALL_SHORT_ADDR_CASES:
			cont = func(priv, iovec);
			break;
		default:
			XDP2_IOVEC_PADDR_TYPE_DEFAULT_ERR(iovec,
						"Pvbuf iterate iovec");
		}

		if (!cont)
			break;
	}

	return cont;
}

/* Front end function to iterate over a pvbuf and provide a paddr */
static inline void xdp2_pvbuf_iterate_iovec(xdp2_paddr_t pvbuf_paddr,
					    xdp2_pvbuf_iterate_iovec_t func,
					    void *priv)
{
	__xdp2_pvbuf_iterate_iovec(&xdp2_pvbuf_global_mgr, pvbuf_paddr,
				   func, priv);
}

/* Initialization functions */

/* Initialize a packet buffer manager */
int __xdp2_pvbuf_init(struct xdp2_pvbuf_mgr *pvmgr,
		      struct xdp2_pbuf_init_allocator *pbuf_allocs,
		      struct xdp2_pvbuf_init_allocator *pvbuf_allocs,
		      const char *name,
		      uintptr_t external_base_offset,
		      bool random_pvbuf_size, bool alloc_one_ref,
		      struct xdp2_short_addr_config *short_addr_config,
		      struct xdp2_long_addr_config *long_addr_config);

/* Initialize the global packet buffer manager */
static inline int xdp2_pvbuf_init(struct xdp2_pbuf_init_allocator
								*pbuf_allocs,
				  struct xdp2_pvbuf_init_allocator
								*pvbuf_allocs,
				  bool random_pvbuf_size,
				  bool alloc_one_ref,
				  struct xdp2_short_addr_config
							*short_addr_config,
				  struct xdp2_long_addr_config
							*long_addr_config)
{
	return __xdp2_pvbuf_init(&xdp2_pvbuf_global_mgr, pbuf_allocs,
				 pvbuf_allocs, "global", 0,
				 random_pvbuf_size, alloc_one_ref,
				 short_addr_config, long_addr_config);
}

/* Print functions */

/* Print a pvbuf managed by the global packet buffer manager */
static inline void xdp2_pvbuf_print(xdp2_paddr_t paddr);

/* Print information for one pvbuf managed by the packet buffer manager in the
 * pvmgr argument
 */
void __xdp2_pvbuf_print(struct xdp2_pvbuf_mgr *pvmgr,
			unsigned int level, xdp2_paddr_t paddr);

/* Print information for one pvbuf managed by the global packet buffer manager
 * in the pvmgr argument
 */
static inline void xdp2_pvbuf_print(xdp2_paddr_t paddr)
{
	__xdp2_pvbuf_print(&xdp2_pvbuf_global_mgr, 0, paddr);
}

/* Print information for a packet buffer manager */
void ___xdp2_pvbuf_show_buffer_manager(struct xdp2_pvbuf_mgr *pvmgr,
	void *cli, void (*cb)(struct xdp2_obj_allocator *allocator,
			      void *cli, const char *arg), const char *arg);

static inline void __xdp2_pvbuf_show_buffer_manager(
					struct xdp2_pvbuf_mgr *pvmgr,
					void *cli)
{
	___xdp2_pvbuf_show_buffer_manager(pvmgr, cli, NULL, NULL);
}

/* Print information for the global packet buffer manager */
static inline void xdp2_pvbuf_show_buffer_manager(void *cli)
{
	__xdp2_pvbuf_show_buffer_manager(&xdp2_pvbuf_global_mgr, cli);
}

/* Print detailed information for a packet buffer manager */
void __xdp2_pvbuf_show_buffer_manager_details(struct xdp2_pvbuf_mgr *pvmgr,
					       void *cli);

/* Print detailed information for the global packet buffer manager */
static inline void xdp2_pvbuf_show_buffer_manager_details(void *cli)
{
	__xdp2_pvbuf_show_buffer_manager_details(&xdp2_pvbuf_global_mgr, cli);
}

void xdp2_pvbuf_show_buffer_manager_from_cli(void *cli,
					struct xdp2_cli_thread_info *info,
					const char *arg);

void xdp2_pvbuf_show_buffer_manager_details_from_cli(void *cli,
					struct xdp2_cli_thread_info *info,
					const char *arg);

#define XDP2_PVBUF_SHOW_BUFFER_MANAGER_CLI()				\
	XDP2_CLI_ADD_SHOW_CONFIG("buffer-manager",			\
		xdp2_pvbuf_show_buffer_manager_from_cli, 0xffff);	\
	XDP2_CLI_ADD_SHOW_CONFIG("buffer-manager-details",		\
		xdp2_pvbuf_show_buffer_manager_details_from_cli,	\
								0xffff)

/* Pvbuf clone via iterate */

/* Iterator struct for pvbuf clone operation */
struct __xdp2_pvbuf_iter_clone {
	struct xdp2_pvbuf_mgr *pvmgr;
	size_t length;
	size_t offset;
	unsigned int num_iovecs;
	struct xdp2_pvbuf *pvbuf;
	unsigned int index;
	int error;
};

int __xdp2_pvbuf_iterate_iovec_clone_common(
		struct __xdp2_pvbuf_iter_clone *ist,
		struct xdp2_iovec *iovec, struct xdp2_paddr *pbaddr_1,
		struct xdp2_paddr *pbaddr_2, size_t *data_len,
		unsigned int *num_slots);

bool __xdp2_check_clone_add_iovec(struct __xdp2_pvbuf_iter_clone *ist,
				  bool set_length, unsigned int pvbuf_size);

xdp2_paddr_t __xdp2_pvbuf_clone(struct xdp2_pvbuf_mgr *pvmgr,
				  xdp2_paddr_t paddr, size_t offset,
				  size_t length, size_t *retlen);

static inline xdp2_paddr_t xdp2_pvbuf_clone(xdp2_paddr_t paddr,
					      size_t offset, size_t length,
					      size_t *retlen)
{
	return __xdp2_pvbuf_clone(&xdp2_pvbuf_global_mgr, paddr, offset,
				   length, retlen);
}

/* Copy data functions */

size_t __xdp2_pvbuf_copy_data_to_pvbuf(struct xdp2_pvbuf_mgr *pvmgr,
					xdp2_paddr_t paddr,
					void *data, size_t len, size_t offset);

static inline size_t xdp2_pvbuf_copy_data_to_pvbuf(xdp2_paddr_t pvbuf_paddr,
						   void *data, size_t len,
						   size_t offset)
{
	return __xdp2_pvbuf_copy_data_to_pvbuf(&xdp2_pvbuf_global_mgr,
					pvbuf_paddr, data, len, offset);
}

size_t __xdp2_pvbuf_copy_pvbuf_to_data(struct xdp2_pvbuf_mgr *pvmgr,
					xdp2_paddr_t pvbuf_paddr,
					void *data, size_t len, size_t offset);

static inline size_t xdp2_pvbuf_copy_pvbuf_to_data(xdp2_paddr_t pvbuf_paddr,
						   void *data, size_t len,
						   size_t offset)
{
	return __xdp2_pvbuf_copy_pvbuf_to_data(&xdp2_pvbuf_global_mgr,
					       pvbuf_paddr, data, len, offset);
}

/* Compute the checksum over a pvbuf or pbuf */

__u16 __xdp2_pvbuf_checksum(struct xdp2_pvbuf_mgr *pvmgr,
			    xdp2_paddr_t pvbuf_paddr, size_t len,
			    size_t offset);

static inline __u16 xdp2_pvbuf_checksum(xdp2_paddr_t pvbuf_paddr,
					size_t len, size_t offset)
{
	return __xdp2_pvbuf_checksum(&xdp2_pvbuf_global_mgr, pvbuf_paddr,
				     len, offset);
}

/* Create an iovec array corresponding to the pbufs in a pvbuf. Returns the
 * number of iovec entries used, or -1 if there are enough IO vectors in the
 * array
 */

int __xdp2_pvbuf_make_iovecs(struct xdp2_pvbuf_mgr *pvmgr,
			     xdp2_paddr_t pvbuf_paddr, struct iovec *iovecs,
			     unsigned int max_vecs, size_t len, size_t offset);

static inline int xdp2_pvbuf_make_iovecs(xdp2_paddr_t pvbuf_paddr,
					 struct iovec *iovecs,
					 unsigned int max_vecs,
					 size_t len, size_t offset)
{
	return __xdp2_pvbuf_make_iovecs(&xdp2_pvbuf_global_mgr, pvbuf_paddr,
					iovecs, max_vecs, len, offset);
}

/* Check functions */

void __xdp2_pvbuf_check(struct xdp2_pvbuf_mgr *pvmgr,
			xdp2_paddr_t pvbuf_paddr, const char *where);

static inline void xdp2_pvbuf_check(xdp2_paddr_t pvbuf_paddr, const char *where)
{
	__xdp2_pvbuf_check(&xdp2_pvbuf_global_mgr, pvbuf_paddr, where);
}

void __xdp2_pvbuf_check_empty(struct xdp2_pvbuf_mgr *pvmgr,
			       xdp2_paddr_t pvbuf_paddr, const char *where);

static inline void xdp2_pvbuf_check_empty(xdp2_paddr_t pvbuf_paddr,
					   const char *where)
{
	__xdp2_pvbuf_check_empty(&xdp2_pvbuf_global_mgr, pvbuf_paddr, where);
}

#endif /* __XDP2_PVBUF_H__ */
