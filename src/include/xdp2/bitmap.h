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

#ifndef __XDP2_BITMAP_H__
#define __XDP2_BITMAP_H__

/* Bitmaps
 *
 * Functions and structures for processing bitmaps. A bitmap is defined by a
 * word size __u{8,16,32,64} * and a number of bits (nbits). A bitmap may span
 * multiple words and the bits are numbered start with the low order bit
 * of the first word. If N is the number of bits in each word, the numbering
 * is like:
 *
 *	bit_0 = addr[0] & (1 << 0)
 *	bit_1 = addr[0] & (1 << 1)
 *	bit_2 = addr[0] & (1 << 2)
 *	...
 *	bit_(N-2) = addr[0] & (1 << (N-2))
 *	bit_(N-1) = addr[0] & (1 << (N-1))
 *	...
 *	bit_(nbits - 1) = addr[(nbits - 1) / N] & (1 << (nbits - 1) % N)
 *
 * Reverse bitmaps structure the words in increasing order. If NUM_WORDS
 * is the maximum number of words in a bitmap then the bits are numbered
 * as:
 *
 *	bit_0 = addr[NUM_WORDS - 1] & (1 << 0)
 *	bit_1 = addr[NUM_WORDS - 1] & (1 << 1)
 *	bit_2 = addr[NUM_WORDS - 1] & (1 << 2)
 *	...
 *
 * The words of a __u16,  __u32, __u16 may be in little endian or big
 * endian byte order (also called "Network Byte Order"). Operations on
 * bitmaps that don't match the endinanness of the host can be done
 * using the "swap" variants of bitmap operations
 *
 * Bitmap operation functions have the general format:
 *
 *	xdp2_bitmap[8, 16, 32, 64][swp]_< operation>](bitmap, ARGS, ...)
 *	xdp2_rbitmap[8, 16, 32, 64][swp]_< operation>](bitmap, ARGS, ...,
 *					       NUM_WORDS)
 *
 * If the 'r' is present before the bitmap then the bitmap is treated as
 * a reverse bitmap. A number (8, 16, 32, or 64) following bitmap indicates
 * the word size. If the number is absent the default size is used. "swp"
 * indicates that the bytes of a word are swapped for endinanness.
 * <operation> gives the bitmap operation, 'bitmap' referes to the input
 * bitmap as a pointer to the first words,  and the ARGS list gives the
 * arguments for the operation
 *
 * If 'swp' is present then the 16, 32, or 64 bit words of a bitmap are swapped
 * relative to host byte order
 *
 * If 'wsrc' is presenn then the first source operand is single word and not a
 * bit map. This acts as a pattern that can be applied to operations on the
 * second source
 *
 * If destsrc is present then the desintation bitmap is also the (second) source
 * operand
 *
 * If a number of bits is not specified in an operation then the type is
 * unsigned long. This matches the bitmaps used in the Linux kernel. For these
 * default types swap and reverse bitmaps are not defined
 *
 * The various operations are described below
 */

#include <asm/bitsperlong.h>
#include <linux/types.h>
#include <stdbool.h>

#include "xdp2/bitmap_word.h"
#include "xdp2/bswap.h"
#include "xdp2/utility.h"

#define XDP2_BITMAP_BITS_PER_WORD      __BITS_PER_LONG

#define XDP2_BITMAP_NUM_BITS_TO_WORDS(NUM_BITS)				\
	((NUM_BITS + XDP2_BITMAP_BITS_PER_WORD - 1) /			\
				XDP2_BITMAP_BITS_PER_WORD)

/* Argument to bitmap test functions */
enum {
	XDP2_BITMAP_TEST_NONE = 0,
	XDP2_BITMAP_TEST_WEIGHT,	/* Compute weight */
	XDP2_BITMAP_TEST_ANY_SET,	/* Any bit set */
	XDP2_BITMAP_TEST_ANY_ZERO,	/* Any bit is zero */
	XDP2_BITMAP_TEST_MAX = XDP2_BITMAP_TEST_ANY_ZERO
};

#include "xdp2/bitmap_helpers.h"

/******** Get and set functions
 *
 * xdp2_[r]bitmap[N][swp]_set
 * xdp2_[r]bitmap[N][swp]_setval
 * xdp2_[r]bitmap[N][swp]_unset
 * xdp2_[r]bitmap[N][swp]_isset
 *
 * Basic operations to set of get the value of a bit in a bitmap
 *
 * void xdp2_bitmap[N][swp]_set(__uN *src, unsigned int pos)
 * bool xdp2_bitmap[N][swp]_set_val(__uN *src, unsigned int pos,
 *				    bool val)
 * void xdp2_bitmap[N][swp]_unset(const __uN *src, unsigned int pos)
 * bool xdp2_bitmap[N][swp]_isset(const __uN *src, unsigned int pos)
 *
 * void xdp2_rbitmap[N][swp]_set(__uN *src, unsigned int pos,
 *				 unsigned int num_words)
 * bool xdp2_rbitmap[N][swp]_set_val(__uN *src, unsigned int pos,
 *				     bool val, unsigned int num_words)
 * void xdp2_rbitmap[N][swp]_unset(const __uN *src, unsigned int pos,
 *				   unsigned int num_words)
 * bool xdp2_rbitmap[N][swp]_isset(const __uN *src, unsigned int pos,
 *				   unsigned int num_words)
 */

/* Set bit in bitmap by index */
#define __XDP2_MAKE_BITMAP_SET(NUM_WORD_BITS, TYPE, SUFFIX, CONV_FUNC)	\
static inline void XDP2_JOIN3(xdp2_bitmap, SUFFIX, _set)(		\
	TYPE *map, unsigned int index)					\
{									\
	map[index / NUM_WORD_BITS] |=					\
			CONV_FUNC((XDP2_JOIN2(__u, NUM_WORD_BITS))	\
				(1ULL << (index % NUM_WORD_BITS)));	\
}									\
									\
static inline void XDP2_JOIN3(xdp2_rbitmap, SUFFIX, _set)(		\
	TYPE *map, unsigned int index, unsigned int num_words)		\
{									\
	map[num_words - 1 - index / NUM_WORD_BITS] |=			\
			CONV_FUNC((XDP2_JOIN2(__u, NUM_WORD_BITS))	\
				(1ULL << (index % NUM_WORD_BITS)));	\
}


/* Unset bit in bitmap by index */
#define __XDP2_MAKE_BITMAP_UNSET(NUM_WORD_BITS, TYPE, SUFFIX,		\
				 CONV_FUNC)				\
static inline void XDP2_JOIN3(xdp2_bitmap, SUFFIX, _unset)(		\
	TYPE *map, unsigned int index)					\
{									\
	map[index / NUM_WORD_BITS] &=					\
			CONV_FUNC((XDP2_JOIN2(__u, NUM_WORD_BITS))	\
				(~(1ULL << (index % NUM_WORD_BITS))));	\
}									\
									\
static inline void XDP2_JOIN3(xdp2_rbitmap, SUFFIX, _unset)(		\
	TYPE *map, unsigned int index, unsigned int num_words)		\
{									\
	map[num_words - 1 - index / NUM_WORD_BITS] &=			\
			CONV_FUNC((XDP2_JOIN2(__u, NUM_WORD_BITS))	\
				(~(1ULL << (index % NUM_WORD_BITS))));	\
}

/* Set value (0 or 1) in bitmap by index */
#define __XDP2_MAKE_BITMAP_SETVAL(NUM_WORD_BITS, TYPE, SUFFIX,		\
				  CONV_FUNC)				\
static inline void XDP2_JOIN3(xdp2_bitmap, SUFFIX, _set_val)(		\
	TYPE *map, unsigned int index, bool val)			\
{									\
	unsigned int offset = index / NUM_WORD_BITS;			\
	unsigned int shift = index % NUM_WORD_BITS;			\
									\
	map[offset] = (map[offset] & CONV_FUNC(				\
		(XDP2_JOIN2(__u, NUM_WORD_BITS))(~(1ULL << shift)))) |	\
			CONV_FUNC(((XDP2_JOIN2(				\
				__u, NUM_WORD_BITS))val << shift));	\
}									\
									\
static inline void XDP2_JOIN3(xdp2_rbitmap, SUFFIX, _set_val)(		\
	TYPE *map, unsigned int index,					\
	bool val, unsigned int num_words)				\
{									\
	unsigned int offset = num_words - 1 - index / NUM_WORD_BITS;	\
	unsigned int shift = index % NUM_WORD_BITS;			\
									\
	map[offset] = (map[offset] & CONV_FUNC(				\
		(XDP2_JOIN2(__u, NUM_WORD_BITS))(~(1ULL << shift)))) |	\
			CONV_FUNC(((XDP2_JOIN2(				\
				__u, NUM_WORD_BITS))val << shift));	\
}

/* Return value (0 or 1) in bitmap by index */
#define __XDP2_MAKE_BITMAP_ISSET(NUM_WORD_BITS, TYPE, SUFFIX,		\
				 CONV_FUNC)				\
static inline bool XDP2_JOIN3(xdp2_bitmap, SUFFIX, _isset)(		\
	const TYPE *map, unsigned int index)				\
{									\
	return !!(map[index / NUM_WORD_BITS] &				\
		  CONV_FUNC((1ULL << (index % NUM_WORD_BITS))));	\
}									\
									\
static inline bool XDP2_JOIN3(xdp2_rbitmap, SUFFIX, _isset)(		\
	const TYPE *map, unsigned int index,				\
	unsigned int num_words)						\
{									\
	return !!(map[num_words - 1 - index / NUM_WORD_BITS] &		\
		  CONV_FUNC((1ULL << (index % NUM_WORD_BITS))));	\
}

#define __XDP2_BITMAP_FUNCS(NUM_WORD_BITS, TYPE, SUFFIX, CONV_FUNC)	\
	__XDP2_MAKE_BITMAP_SET(NUM_WORD_BITS, TYPE, SUFFIX,		\
			       CONV_FUNC)				\
	__XDP2_MAKE_BITMAP_UNSET(NUM_WORD_BITS, TYPE, SUFFIX,		\
				 CONV_FUNC)				\
	__XDP2_MAKE_BITMAP_SETVAL(NUM_WORD_BITS, TYPE, SUFFIX,		\
				  CONV_FUNC)				\
	__XDP2_MAKE_BITMAP_ISSET(NUM_WORD_BITS, TYPE, SUFFIX,		\
				 CONV_FUNC)

__XDP2_BITMAP_FUNCS(XDP2_BITMAP_BITS_PER_WORD, unsigned long,,)
__XDP2_BITMAP_FUNCS(8, __u8, 8,)
__XDP2_BITMAP_FUNCS(16, __u16, 16,)
__XDP2_BITMAP_FUNCS(32, __u32, 32,)
__XDP2_BITMAP_FUNCS(64, __u64, 64,)
__XDP2_BITMAP_FUNCS(16, __u16, 16swp, bswap_16)
__XDP2_BITMAP_FUNCS(32, __u32, 32swp, bswap_32)
__XDP2_BITMAP_FUNCS(64, __u64, 64swp, bswap_64)

/******** Iterators to do foreach over the set or zero bits in a bitmap
 *
 * Iterate over the set or unset bits in a bitmap. pos argument gives
 * the initial position and is set the index of each match iternation
 *
 * xdp2_[r]bitmap[N][swp]_foreach_bit
 * xdp2_[r]bitmap[N][swp]_foreach_zero_bit
 *
 * xdp2_bitmap[N][swp]_foreach_bit(map, pos, nbits) {
 *	 <code>
 * }
 * xdp2_bitmap[N][swp]_foreach_zero_bit(map, pos, nbits) {
 *	 <code>
 * }
 *
 * xdp2_rbitmap[N][swp]_foreach_bit(map, pos, nbits, num_words) {
 *	 <code>
 * }
 * xdp2_bitmap[N][swp]_foreach_zero_bit(map, pos, nbits, num_words) {
 *	 <code>
 * }
 */

#define __XDP2_MAKE_FOREACH(MAP, POS, NBITS, SUFFIX, ZERO)		\
	for (POS = XDP2_JOIN4(xdp2_bitmap, SUFFIX, _find, ZERO)(	\
	     MAP, POS, NBITS); POS < NBITS;				\
	     POS = XDP2_JOIN4(xdp2_bitmap, SUFFIX,			\
			      _find_next, ZERO)(MAP, POS, NBITS))

#define __XDP2_MAKE_REV_FOREACH(MAP, POS, NBITS, SUFFIX, ZERO,		\
				NUM_WORDS)				\
	for (POS = XDP2_JOIN4(xdp2_rbitmap, SUFFIX, _find, ZERO)(	\
		     MAP, POS, NBITS, NUM_WORDS); POS < NBITS;		\
	     POS = XDP2_JOIN4(xdp2_rbitmap, SUFFIX,			\
			      _find_next, ZERO)(MAP, POS, NBITS,	\
			      NUM_WORDS))

/* Work on each set bit */
#define xdp2_bitmap_foreach_bit(MAP, POS, NBITS)			\
	__XDP2_MAKE_FOREACH(MAP, POS, NBITS,,)
#define xdp2_bitmap8_foreach_bit(MAP, POS, NBITS)			\
	__XDP2_MAKE_FOREACH(MAP, POS, NBITS, 8,)
#define xdp2_bitmap16_foreach_bit(MAP, POS, NBITS)			\
	__XDP2_MAKE_FOREACH(MAP, POS, NBITS, 16,)
#define xdp2_bitmap32_foreach_bit(MAP, POS, NBITS)			\
	__XDP2_MAKE_FOREACH(MAP, POS, NBITS, 32,)
#define xdp2_bitmap64_foreach_bit(MAP, POS, NBITS)			\
	__XDP2_MAKE_FOREACH(MAP, POS, NBITS, 64,)
#define xdp2_bitmap16swp_foreach_bit(MAP, POS, NBITS)			\
	__XDP2_MAKE_FOREACH(MAP, POS, NBITS, 16swp,)
#define xdp2_bitmap32swp_foreach_bit(MAP, POS, NBITS)			\
	__XDP2_MAKE_FOREACH(MAP, POS, NBITS, 32swp,)
#define xdp2_bitmap64swp_foreach_bit(MAP, POS, NBITS)			\
	__XDP2_MAKE_FOREACH(MAP, POS, NBITS, 64swp,)

#define xdp2_rbitmap_foreach_bit(MAP, POS, NBITS, NUM_WORDS)		\
	__XDP2_MAKE_REV_FOREACH(MAP, POS, NBITS,,, NUM_WORDS)
#define xdp2_rbitmap8_foreach_bit(MAP, POS, NBITS, NUM_WORDS)		\
	__XDP2_MAKE_REV_FOREACH(MAP, POS, NBITS, 8,, NUM_WORDS)
#define xdp2_rbitmap16_foreach_bit(MAP, POS, NBITS, NUM_WORDS)		\
	__XDP2_MAKE_REV_FOREACH(MAP, POS, NBITS, 16,, NUM_WORDS)
#define xdp2_rbitmap32_foreach_bit(MAP, POS, NBITS, NUM_WORDS)		\
	__XDP2_MAKE_REV_FOREACH(MAP, POS, NBITS, 32,, NUM_WORDS)
#define xdp2_rbitmap64_foreach_bit(MAP, POS, NBITS, NUM_WORDS)		\
	__XDP2_MAKE_REV_FOREACH(MAP, POS, NBITS, 64,, NUM_WORDS)
#define xdp2_rbitmap16swp_foreach_bit(MAP, POS, NBITS, NUM_WORDS)	\
	__XDP2_MAKE_REV_FOREACH(MAP, POS, NBITS, 16swp,, NUM_WORDS)
#define xdp2_rbitmap32swp_foreach_bit(MAP, POS, NBITS, NUM_WORDS)	\
	__XDP2_MAKE_REV_FOREACH(MAP, POS, NBITS, 32swp,, NUM_WORDS)
#define xdp2_rbitmap64swp_foreach_bit(MAP, POS, NBITS, NUM_WORDS)	\
	__XDP2_MAKE_REV_FOREACH(MAP, POS, NBITS, 64swp,, NUM_WORDS)

/* Work on each zero bit */
#define xdp2_bitmap_foreach_zero_bit(MAP, POS, NBITS)			\
	__XDP2_MAKE_FOREACH(MAP, POS, NBITS,, _zero)
#define xdp2_bitmap8_foreach_zero_bit(MAP, POS, NBITS)			\
	__XDP2_MAKE_FOREACH(MAP, POS, NBITS, 8, _zero)
#define xdp2_bitmap16_foreach_zero_bit(MAP, POS, NBITS)			\
	__XDP2_MAKE_FOREACH(MAP, POS, NBITS, 16, _zero)
#define xdp2_bitmap32_foreach_zero_bit(MAP, POS, NBITS)			\
	__XDP2_MAKE_FOREACH(MAP, POS, NBITS, 32, _zero)
#define xdp2_bitmap64_foreach_zero_bit(MAP, POS, NBITS)			\
	__XDP2_MAKE_FOREACH(MAP, POS, NBITS, 64, _zero)
#define xdp2_bitmap16swp_foreach_zero_bit(MAP, POS, NBITS)		\
	__XDP2_MAKE_FOREACH(MAP, POS, NBITS, 16swp, _zero)
#define xdp2_bitmap32swp_foreach_zero_bit(MAP, POS, NBITS)		\
	__XDP2_MAKE_FOREACH(MAP, POS, NBITS, 32swp, _zero)
#define xdp2_bitmap64swp_foreach_zero_bit(MAP, POS, NBITS)		\
	__XDP2_MAKE_FOREACH(MAP, POS, NBITS, 64swp, _zero)

#define xdp2_rbitmap_foreach_zero_bit(MAP, POS, NBITS, NUM_WORDS)	\
	__XDP2_MAKE_REV_FOREACH(MAP, POS, NBITS,, _zero, NUM_WORDS)
#define xdp2_rbitmap8_foreach_zero_bit(MAP, POS, NBITS, NUM_WORDS)	\
	__XDP2_MAKE_REV_FOREACH(MAP, POS, NBITS, 8, _zero, NUM_WORDS)
#define xdp2_rbitmap16_foreach_zero_bit(MAP, POS, NBITS, NUM_WORDS)	\
	__XDP2_MAKE_REV_FOREACH(MAP, POS, NBITS, 16, _zero, NUM_WORDS)
#define xdp2_rbitmap32_foreach_zero_bit(MAP, POS, NBITS, NUM_WORDS)	\
	__XDP2_MAKE_REV_FOREACH(MAP, POS, NBITS, 32, _zero, NUM_WORDS)
#define xdp2_rbitmap64_foreach_zero_bit(MAP, POS, NBITS, NUM_WORDS)	\
	__XDP2_MAKE_REV_FOREACH(MAP, POS, NBITS, 64, _zero, NUM_WORDS)
#define xdp2_rbitmap16swp_foreach_zero_bit(MAP, POS, NBITS, NUM_WORDS)	\
	__XDP2_MAKE_REV_FOREACH(MAP, POS, NBITS, 16swp, _zero, NUM_WORDS)
#define xdp2_rbitmap32swp_foreach_zero_bit(MAP, POS, NBITS, NUM_WORDS)	\
	__XDP2_MAKE_REV_FOREACH(MAP, POS, NBITS, 32swp, _zero, NUM_WORDS)
#define xdp2_rbitmap64swp_foreach_zero_bit(MAP, POS, NBITS, NUM_WORDS)	\
	__XDP2_MAKE_REV_FOREACH(MAP, POS, NBITS, 64swp, _zero, NUM_WORDS)

/******** Test functions
 *
 * xdp2_[r]bitmap[N][swp]_test
 *
 * Test bitmap (for all zeroes, all ones, weight)
 *
 * bool xdp2_bitmap[N][swp]_test(const __uN *src, unsigned int pos,
 *			unsigned int nbits, unsigned int test)
 *
 * bool xdp2_rbitmap[N][swp]_test(const __uN *src, unsigned int pos,
 *			unsigned int nbits, unsigned int test,
 *			unsigned int num_words)
 *
 * Test is on one of the enum
 *
 *	XDP2_BITMAP_TEST_NONE = 0
 *      XDP2_BITMAP_TEST_WEIGHT
 *      XDP2_BITMAP_TEST_ANY_SET
 *      XDP2_BITMAP_TEST_ANY_ZERO
 *
 * Return 1 or 0 for simple check or value of weight
 */

#define __XDP2_MAKE_1ARG_TEST_FUNCTS(NUM_WORD_BITS, TYPE, SUFFIX,	\
				     CONV_FUNC)				\
	__XDP2_BITMAP_1ARG_TEST_FUNCT(test,, NUM_WORD_BITS, TYPE,	\
				      SUFFIX, CONV_FUNC)

__XDP2_MAKE_1ARG_TEST_FUNCTS(XDP2_BITMAP_BITS_PER_WORD, unsigned long,,)
__XDP2_MAKE_1ARG_TEST_FUNCTS(8, __u8, 8,)
__XDP2_MAKE_1ARG_TEST_FUNCTS(16, __u16, 16,)
__XDP2_MAKE_1ARG_TEST_FUNCTS(32, __u32, 32,)
__XDP2_MAKE_1ARG_TEST_FUNCTS(64, __u64, 64,)
__XDP2_MAKE_1ARG_TEST_FUNCTS(16, __u16, 16swp, bswap_16)
__XDP2_MAKE_1ARG_TEST_FUNCTS(32, __u32, 32swp, bswap_32)
__XDP2_MAKE_1ARG_TEST_FUNCTS(64, __u64, 64swp, bswap_64)

/******** Find functions
 *
 * xdp2_bitmap_find
 * xdp2_bitmap_find_zero
 * xdp2_bitmap_find_next
 * xdp2_bitmap_find_zero_next
 *
 * Find first set or zero bit in a bitmap
 *
 * unsigned int xdp2_bitmap[N][swp]_find(const _uN *addr, unsigned int pos,
 *					 unsigned int nbits)
 * unsigned int xdp2_bitmap[N][swp]_find_zero(const _uN *src,
 *					      unsigned int pos,
 *					      unsigned int nbits)
 * unsigned int xdp2_bitmap[N][swp]_find_next(const _uN *src,
 *					      unsigned int pos,
 *					      unsigned int nbits)
 * unsigned int xdp2_bitmap[N][swp]_find_zero_next(const _uN *src,
 *						   unsigned int pos,
 *						   unsigned int nbits)
 *
 * unsigned int xdp2_rbitmap[N][swp]_find(const _uN *addr, unsigned int pos,
 *					  unsigned int nbits,
 *					  unsigned int num_words)
 * unsigned int xdp2_rbitmap[N][swp]_find_zero(const _uN *src, unsigned int pos,
 *					       unsigned int nbits,
 *					       unsigned int num_words)
 * unsigned int xdp2_rbitmap[N][swp]_find_next(const _uN *src, unsigned int pos,
 *					       unsigned int nbits,
 *					       unsigned int num_words)
 * unsigned int xdp2_rbitmap[N][swp]_find_zero_next(const _uN *src, unsigned int pos,
 *						    unsigned int nbits,
 *						    unsigned int num_words)
 *
 * Returns
 *	<  nbits position if bit is found
 *	>= nbits if no set bit is found
 */

#define __XDP2_MAKE_1ARG_FIND_FUNCTS(NUM_WORD_BITS, TYPE, SUFFIX,	\
				     CONV_FUNC)				\
	__XDP2_BITMAP_1ARG_FIND_FUNCT(find,, 0, NUM_WORD_BITS, TYPE,	\
				      SUFFIX, false, CONV_FUNC)		\
	__XDP2_BITMAP_1ARG_FIND_FUNCT(find_zero, ~, 0, NUM_WORD_BITS,	\
				      TYPE, SUFFIX, false, CONV_FUNC)	\
	__XDP2_BITMAP_1ARG_FIND_FUNCT(find_next,,  1, NUM_WORD_BITS,	\
				      TYPE, SUFFIX, false, CONV_FUNC)	\
	__XDP2_BITMAP_1ARG_FIND_FUNCT(find_next_zero, ~, 1,		\
				      NUM_WORD_BITS, TYPE, SUFFIX,	\
				      false, CONV_FUNC)			\
	__XDP2_BITMAP_1ARG_FIND_FUNCT(find_roll,, 0, NUM_WORD_BITS,	\
				      TYPE, SUFFIX, true, CONV_FUNC)	\
	__XDP2_BITMAP_1ARG_FIND_FUNCT(find_roll_zero, ~, 0,		\
				      NUM_WORD_BITS, TYPE, SUFFIX,	\
				      true, CONV_FUNC)			\
	__XDP2_BITMAP_1ARG_FIND_FUNCT(find_roll_next,,  1,		\
				      NUM_WORD_BITS, TYPE, SUFFIX,	\
				      true, CONV_FUNC)			\
	__XDP2_BITMAP_1ARG_FIND_FUNCT(find_roll_next_zero, ~, 1,	\
				      NUM_WORD_BITS, TYPE, SUFFIX,	\
				      true, CONV_FUNC)

__XDP2_MAKE_1ARG_FIND_FUNCTS(XDP2_BITMAP_BITS_PER_WORD, unsigned long,,)
__XDP2_MAKE_1ARG_FIND_FUNCTS(8, __u8, 8,)
__XDP2_MAKE_1ARG_FIND_FUNCTS(16, __u16, 16,)
__XDP2_MAKE_1ARG_FIND_FUNCTS(32, __u32, 32,)
__XDP2_MAKE_1ARG_FIND_FUNCTS(64, __u64, 64,)
__XDP2_MAKE_1ARG_FIND_FUNCTS(16, __u16, 16swp, bswap_16)
__XDP2_MAKE_1ARG_FIND_FUNCTS(32, __u32, 32swp, bswap_32)
__XDP2_MAKE_1ARG_FIND_FUNCTS(64, __u64, 64swp, bswap_64)

/******** Set functions
 *
 * xdp2_[r]bitmap[N][swp]_set_zeroes
 * xdp2_[r]bitmap[N][swp]_set_ones
 *
 * Set a bitmap to all zeroes or ones
 *
 * void xdp2_bitmap[N][swp]_set_zeroes(__uN *addr, unsigned int pos,
 *				       unsigned int nbits)
 * void xdp2_bitmap[N][swp]_set_ones(__uN *addr, unsigned int pos,
 *				     unsigned int nbits)
 *
 * void xdp2_rbitmap[N][swp]_set_zeroes(__u8 *addr, unsigned int pos,
 *					unsigned int nbits,
 *					unsigned int num_words)
 * void xdp2_bitmap[N][swp]_set_ones(__u8 *addr, unsigned int pos,
 *				     unsigned int nbits,
 *				     unsigned int num_words)
 */
#define __XDP2_MAKE_1ARG_SET_FUNCTS(NUM_WORD_BITS, TYPE, SUFFIX,	\
				    CONV_FUNC)				\
	__XDP2_BITMAP_1ARG_SET_FUNCT(set_zeroes, 0, 1, NUM_WORD_BITS,	\
				     TYPE, SUFFIX, CONV_FUNC)		\
	__XDP2_BITMAP_1ARG_SET_FUNCT(set_ones, 1, 0, NUM_WORD_BITS,	\
				     TYPE, SUFFIX, CONV_FUNC)

__XDP2_MAKE_1ARG_SET_FUNCTS(XDP2_BITMAP_BITS_PER_WORD, unsigned long,,)
__XDP2_MAKE_1ARG_SET_FUNCTS(8, __u8, 8,)
__XDP2_MAKE_1ARG_SET_FUNCTS(16, __u16, 16,)
__XDP2_MAKE_1ARG_SET_FUNCTS(32, __u32, 32,)
__XDP2_MAKE_1ARG_SET_FUNCTS(64, __u64, 64,)
__XDP2_MAKE_1ARG_SET_FUNCTS(16, __u16, 16swp, bswap_16)
__XDP2_MAKE_1ARG_SET_FUNCTS(32, __u32, 32swp, bswap_32)
__XDP2_MAKE_1ARG_SET_FUNCTS(64, __u64, 64swp, bswap_64)

/********* Shift and rotate functions
 *
 * xdp2_[r]bitmap[N][swp]_shift_left
 * xdp2_[r]bitmap[N][swp]_dstsrc_rotate_left
 * xdp2_[r]bitmap[N][swp]_shift_right
 * xdp2_[r]bitmap[N][swp]_dstsrc_shift_right
 * xdp2_[r]bitmap[N][swp]_rotate_left
 * xdp2_[r]bitmap[N][swp]_dstsrc_rotate_left
 * xdp2_[r]bitmap[N][swp]_rotate_right
 * xdp2_[r]bitmap[N][swp]_dstsrc_rotate_right
 *
 * Shift a bitmap left or right by so many bits. The source and desintation
 * may be the same bitmap (explicit in dstsrc functions). Note that nbits
 * must be a multiple of 8
 *
 * void xdp2_bitmap[N][swp]_shift_left(__uN *dest, __uN *src,
 *				       unsigned int num_shift,
 *				       unsigned int nbits)
 *
 * void xdp2_bitmap[N][swp]_dstsrc_shift_left(__uN *dest,
 *				       unsigned int num_shift,
 *				       unsigned int nbits)
 *
 * void xdp2_bitmap[N][swp]_shift_right(__uN *dest, __uN *src,
 *				 unsigned int num_shift,
 *				 unsigned int nbits)
 *
 * void xdp2_bitmap[N][swp]_dstsrc_shift_right(__uN *dest,
 *					unsigned int num_shift,
 *					unsigned int nbits)
 *
 * void xdp2_rbitmap[N][swp]_shift_left(__uN *dest, __uN *src,
 *					unsigned int num_shift,
 *					unsigned int nbits,
 *					unsigned int num_words)
 *
 * void xdp2_rbitmap[N][swp]_dstsrc_shift_left(__uN *dest,
 *				       unsigned int num_shift,
 *				       unsigned int nbits,
 *				       unsigned int num_words)
 *
 * void xdp2_rbitmap[N][swp]_shift_right(__uN *dest, __uN *src,
 *				 unsigned int num_shift,
 *				 unsigned int nbits,
 *				 unsigned int num_words)
 *
 * void xdp2_rbitmap[N][swp]_dstsrc_shift_right(__uN *dest,
 *					unsigned int num_shift,
 *					unsigned int nbits,
 *					unsigned int num_words)
 *
 * Rotate a bitmap left or right by so many bits. The source and desintation
 * may be the same bitmap (explicit in dstsrc functions). Note that nbits
 * must be a multiple of 8
 *
 * void xdp2_bitmap[N][swp]_rotate_left(__uN *dest, __uN *src,
 *				 unsigned int num_rotate,
 *				 unsigned nbits)
 *
 * void xdp2_bitmap[N][swp]_dstsrc_rotate_left(__uN *dest,
 *					unsigned int num_rotate,
 *					unsigned nbits)
 *
 * void xdp2_bitmap[N][swp]_rotate_right(__uN *dest, __uN *src,
 *				  unsigned int num_rotate,
 *				  unsigned nbits)
 *
 * void xdp2_bitmap[N][swp]_dstsrc_rotate_right(__uN *dest,
 *				 unsigned int num_rotate,
 *				 unsigned nbits)
 *
 * void xdp2_rbitmap[N][swp]_rotate_left(__uN *dest, __uN *src,
 *				 unsigned int num_rotate,
 *				 unsigned nbits,
 *				 unsigned int num_words)
 *
 * void xdp2_rbitmap[N][swp]_dstsrc_rotate_left(__uN *dest,
 *					unsigned int num_rotate,
 *					unsigned nbits,
 *					unsigned int num_words)
 *
 * void xdp2_rbitmap[N][swp]_rotate_right(__uN *dest, __uN *src,
 *				  unsigned int num_rotate,
 *				  unsigned nbits,
 *				  unsigned int num_words)
 *
 * void xdp2_rbitmap[N][swp]_dstsrc_rotate_right(__uN *dest,
 *				 unsigned int num_rotate,
 *				 unsigned nbits,
 *				 unsigned int num_words)
 */

#define __XDP2_MAKE_SHIFT_FUNCTS(NUM_WORD_BITS, TYPE, SUFFIX,		\
				 CONV_FUNC)				\
	__XDP2_BITMAP_SHIFT_LEFT_FUNC(NUM_WORD_BITS, TYPE, SUFFIX,	\
				      CONV_FUNC)			\
	__XDP2_BITMAP_SHIFT_RIGHT_FUNC(NUM_WORD_BITS, TYPE, SUFFIX,	\
				       CONV_FUNC)

__XDP2_MAKE_SHIFT_FUNCTS(XDP2_BITMAP_BITS_PER_WORD, unsigned long,,)
__XDP2_MAKE_SHIFT_FUNCTS(8, __u8, 8,)
__XDP2_MAKE_SHIFT_FUNCTS(16, __u16, 16,)
__XDP2_MAKE_SHIFT_FUNCTS(32, __u32, 32,)
__XDP2_MAKE_SHIFT_FUNCTS(64, __u64, 64,)
__XDP2_MAKE_SHIFT_FUNCTS(16, __u16, 16swp, bswap_16)
__XDP2_MAKE_SHIFT_FUNCTS(32, __u32, 32swp, bswap_32)
__XDP2_MAKE_SHIFT_FUNCTS(64, __u64, 64swp, bswap_64)

__XDP2_BITMAP_ROTATE_FUNC(XDP2_BITMAP_BITS_PER_WORD, unsigned long,,)
__XDP2_BITMAP_ROTATE_FUNC(8, __u8, 8,)
__XDP2_BITMAP_ROTATE_FUNC(16, __u16, 16,)
__XDP2_BITMAP_ROTATE_FUNC(32, __u32, 32,)
__XDP2_BITMAP_ROTATE_FUNC(64, __u64, 64,)
__XDP2_BITMAP_ROTATE_FUNC(16, __u16, 16swp, bswap_16)
__XDP2_BITMAP_ROTATE_FUNC(32, __u32, 32swp, bswap_32)
__XDP2_BITMAP_ROTATE_FUNC(64, __u64, 64swp, bswap_64)

/******** Operations that take a source and a destination bitmap
 *
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]copy
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]not
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]copy_test
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]not_test
 *
 * void xdp2_bitmap[N][swp]_{*}(__uN *dest, const __uN *src, unsigned int pos,
 *		       unsigned int nbits)
 *
 * unsigned int xdp2_bitmap[N][swp]_{*}_test(__uN *dest, const __uN *src
 *				      unsigned int pos, unsigned int nbits,
 *				      unsigned int test)
 *
 * void xdp2_rbitmap[N][swp]_{*}(__uN *dest, const __uN *src, unsigned int pos,
 *		       unsigned int nbits, unsigned int num_words)
 *
 * unsigned int xdp2_rbitmap[N][swp]_{*}_test(__uN *dest, const __uN *src
 *				      unsigned int pos, unsigned int nbits,
 *				      unsigned int test,
 *				      unsigned int num_words)
 *
 * void xdp2_bitmap[N][swp]_wsrc_{*}(__uN *dest, __uN src, unsigned int pos,
 *		       unsigned int nbits)
 *
 * unsigned int xdp2_bitmap[N][swp]_wsrc_{*}_test(__uN *dest, __uN src
 *				      unsigned int pos, unsigned int nbits,
 *				      unsigned int test)
 *
 * void xdp2_rbitmap[N][swp]_wsrc_{*}(__uN *dest, __uN src, unsigned int pos,
 *		       unsigned int nbits, unsigned int num_words)
 *
 * unsigned int xdp2_rbitmap[N][swp]_wsrc_{*}_test(__uN *dest, __uN src
 *				      unsigned int pos, unsigned int nbits,
 *				      unsigned int test,
 *				      unsigned int num_words)
 */
#define __XDP2_MAKE_2ARG_PERMUTE_FUNCTS(NUM_WORD_BITS, TYPE, SUFFIX,	\
					CONV_FUNC)			\
	__XDP2_BITMAP_2ARG_PERMUTE_FUNCT(copy,,				\
					 NUM_WORD_BITS, TYPE, SUFFIX,	\
					 CONV_FUNC)			\
	__XDP2_BITMAP_2ARG_PERMUTE_FUNCT(not, ~, NUM_WORD_BITS, TYPE,	\
					 SUFFIX, CONV_FUNC)

__XDP2_MAKE_2ARG_PERMUTE_FUNCTS(XDP2_BITMAP_BITS_PER_WORD, unsigned long,,)
__XDP2_MAKE_2ARG_PERMUTE_FUNCTS(8, __u8, 8,)
__XDP2_MAKE_2ARG_PERMUTE_FUNCTS(16, __u16, 16,)
__XDP2_MAKE_2ARG_PERMUTE_FUNCTS(32, __u32, 32,)
__XDP2_MAKE_2ARG_PERMUTE_FUNCTS(64, __u64, 64,)
__XDP2_MAKE_2ARG_PERMUTE_FUNCTS(16, __u16, 16swp, bswap_16)
__XDP2_MAKE_2ARG_PERMUTE_FUNCTS(32, __u32, 32swp, bswap_32)
__XDP2_MAKE_2ARG_PERMUTE_FUNCTS(64, __u64, 64swp, bswap_64)

/******** Compare operations that compare two bitmaps
 *
 *	xdp2_[r]bitmap[N][swp]_cmp
 *	xdp2_[r]bitmap[N][swp]_first_and
 *	xdp2_[r]bitmap[N][swp]_first_or
 *	xdp2_[r]bitmap[N][swp]_first_and_zero
 *	xdp2_[r]bitmap[N][swp]_first_or_zero
 *
 * unsigned int xdp2_bitmap[N][swp]_{*}(const __uN *src1,
 *				 const __uN *src2,
 *				 unsigned int pos,
 *				 unsigned int nbits)
 *
 * unsigned int xdp2_bitmap[N][swp]_{*}(const __uN *src1,
 *				 const __uN *src2,
 *				 unsigned int pos,
 *				 unsigned int nbits)
 *
 *
 * unsigned int xdp2_rbitmap[N][swp]_{*}(const __uN *src1,
 *				 const __uN *src2,
 *				 unsigned int pos,
 *				 unsigned int nbits,
 *				 unsigned int num_words)
 *
 * unsigned int xdp2_rbitmap[N][swp]_{*}(const __uN *src1,
 *				 const __uN *src2,
 *				 unsigned int pos,
 *				 unsigned int nbits,
 *				 unsigned int num_words)
 */
#define __XDP2_MAKE_2ARG_CMP_ONE_FUNCT(NAME, OP, NOT, NUM_WORD_BITS,	\
				       TYPE, SUFFIX, CONV_FUNC)		\
	__XDP2_BITMAP_2ARG_CMP_FUNCT(NAME, OP, NOT,,, NUM_WORD_BITS,	\
				     TYPE, SUFFIX, CONV_FUNC,		\
				     const,, *,, false)			\
	__XDP2_BITMAP_2ARG_CMP_FUNCT(NAME, OP, NOT,,, NUM_WORD_BITS,	\
				     TYPE, SUFFIX, CONV_FUNC,,		\
				     wsrc_,, &,	true)			\

#define __XDP2_MAKE_2ARG_CMP_FUNCTS(NUM_WORD_BITS, TYPE, SUFFIX,	\
				    CONV_FUNC)				\
	__XDP2_MAKE_2ARG_CMP_ONE_FUNCT(cmp, ^,, NUM_WORD_BITS,		\
				       TYPE, SUFFIX, CONV_FUNC)		\
	__XDP2_MAKE_2ARG_CMP_ONE_FUNCT(first_and, &,, NUM_WORD_BITS,	\
				       TYPE, SUFFIX, CONV_FUNC)		\
	__XDP2_MAKE_2ARG_CMP_ONE_FUNCT(first_or, |,, NUM_WORD_BITS,	\
				       TYPE, SUFFIX, CONV_FUNC)		\
	__XDP2_MAKE_2ARG_CMP_ONE_FUNCT(first_and_zero, |, ~,		\
				       NUM_WORD_BITS, TYPE, SUFFIX,	\
				       CONV_FUNC)			\
	__XDP2_MAKE_2ARG_CMP_ONE_FUNCT(first_or_zero, &, ~,		\
				       NUM_WORD_BITS, TYPE, SUFFIX,	\
				       CONV_FUNC)

__XDP2_MAKE_2ARG_CMP_FUNCTS(XDP2_BITMAP_BITS_PER_WORD, unsigned long,,)
__XDP2_MAKE_2ARG_CMP_FUNCTS(8, __u8, 8,)
__XDP2_MAKE_2ARG_CMP_FUNCTS(16, __u16, 16,)
__XDP2_MAKE_2ARG_CMP_FUNCTS(32, __u32, 32,)
__XDP2_MAKE_2ARG_CMP_FUNCTS(64, __u64, 64,)
__XDP2_MAKE_2ARG_CMP_FUNCTS(16, __u16, 16swp, bswap_16)
__XDP2_MAKE_2ARG_CMP_FUNCTS(32, __u32, 32swp, bswap_32)
__XDP2_MAKE_2ARG_CMP_FUNCTS(64, __u64, 64swp, bswap_64)

/******** Operations that take generic a source and a destination bitmap
 *
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]copy_gen
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]not_gen
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]copy_test_gen
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]not_test_gen
 *
 * void xdp2_bitmap[N][swp]_{*}_gen(__uN *dest, unsigned int dest_pos,
 *			     const __uN *src, unsigned int src_pos,
 *			     unsigned int nbits)
 *
 * unsigned int xdp2_bitmap[N][swp]_{*}_test_gen(__uN *dest,
 *					  unsigned int dest_pos,
 *					  const __uN *src,
 *					  unsigned int src_pos,
 *					  unsigned int nbits,
 *					  unsigned int test)
 *
 * void xdp2_rbitmap[N][swp]_{*}_gen(__uN *dest, unsigned int dest_pos,
 *			     const __uN *src, unsigned int src_pos,
 *			     unsigned int nbits, unsigned int num_words)
 *
 * unsigned int xdp2_rbitmap[N][swp]_{*}_test_gen(__uN *dest,
 *					  unsigned int dest_pos,
 *					  const __uN *src,
 *					  unsigned int src_pos,
 *					  unsigned int nbits,
 *					  unsigned int test,
 *					  unsigned int num_words)
 *
 * void xdp2_bitmap[N][swp]_wsrc_{*}_gen(__uN *dest, unsigned int dest_pos,
 *			     __uN src, unsigned int src_pos,
 *			     unsigned int nbits)
 *
 * unsigned int xdp2_bitmap[N][swp]_wsrc_{*}_test_gen(__uN *dest,
 *					  unsigned int dest_pos,
 *					  __uN src,
 *					  unsigned int src_pos,
 *					  unsigned int nbits,
 *					  unsigned int test)
 *
 * void xdp2_rbitmap[N][swp]_wsrc_{*}_gen(__uN *dest, unsigned int dest_pos,
 *			     __uN src, unsigned int src_pos,
 *			     unsigned int nbits, unsigned int num_words)
 *
 * unsigned int xdp2_rbitmap[N][swp]_wsrc_{*}_test_gen(__uN *dest,
 *					  unsigned int dest_pos,
 *					  __uN src,
 *					  unsigned int src_pos,
 *					  unsigned int nbits,
 *					  unsigned int test,
 *					  unsigned int num_words)
 */
#define __XDP2_MAKE_GEN_PERMUTE_FUNCTS(NUM_WORD_BITS, SUFFIX,		\
				       TYPE, CONV_FUNC)			\
	__XDP2_BITMAP_2ARG_GEN_PERMUTE_FUNCT(copy,, NUM_WORD_BITS,	\
					     TYPE, SUFFIX, CONV_FUNC)	\
	__XDP2_BITMAP_2ARG_GEN_PERMUTE_FUNCT(not, ~, NUM_WORD_BITS,	\
					     TYPE, SUFFIX, CONV_FUNC)

__XDP2_MAKE_GEN_PERMUTE_FUNCTS(XDP2_BITMAP_BITS_PER_WORD, unsigned long,,)
__XDP2_MAKE_GEN_PERMUTE_FUNCTS(8, __u8, 8,)
__XDP2_MAKE_GEN_PERMUTE_FUNCTS(16, __u16, 16,)
__XDP2_MAKE_GEN_PERMUTE_FUNCTS(32, __u32, 32,)
__XDP2_MAKE_GEN_PERMUTE_FUNCTS(64, __u64, 64,)
__XDP2_MAKE_GEN_PERMUTE_FUNCTS(16, __u16, 16swp, bswap_16)
__XDP2_MAKE_GEN_PERMUTE_FUNCTS(32, __u32, 32swp, bswap_32)
__XDP2_MAKE_GEN_PERMUTE_FUNCTS(64, __u64, 64swp, bswap_64)

/******** Compare operations with generic arguments
 *
 *	xdp2_[r]bitmap[N][swp]_cmp_gen
 *	xdp2_[r]bitmap[N][swp]_first_and_gen
 *	xdp2_[r]bitmap[N][swp]_first_or_gen
 *	xdp2_[r]bitmap[N][swp]_first_and_zero_gen
 *	xdp2_[r]bitmap[N][swp]_first_or_zero_gen
 *
 * unsigned int xdp2_bitmap[N][swp]_{*}_gen(const __uN *src1,
 *				     unsigned int src1_pos,
 *				     const __uN *src2,
 *				     unsigned int src2_pos,
 *				     unsigned int nbits)
 *
 * unsigned int xdp2_rbitmap[N][swp]_{*}_gen(const __uN *src1,
 *				     unsigned int src1_pos,
 *				     const __uN *src2,
 *				     unsigned int src2_pos,
 *				     unsigned int nbits,
 *				     unsigned int num_words)
 */
#define __XDP2_MAKE_2ARG_GEN_CMP_FUNCTS(NUM_WORD_BITS, TYPE, SUFFIX,	\
					CONV_FUNC)			\
	__XDP2_BITMAP_2ARG_CMP_GEN_FUNCT(cmp, ^,,,, NUM_WORD_BITS,	\
					 TYPE, SUFFIX, CONV_FUNC)	\
	__XDP2_BITMAP_2ARG_CMP_GEN_FUNCT(first_and, &,,,,		\
					 NUM_WORD_BITS,			\
					 TYPE, SUFFIX, CONV_FUNC)	\
	__XDP2_BITMAP_2ARG_CMP_GEN_FUNCT(first_or, |,,,, NUM_WORD_BITS,	\
					 TYPE, SUFFIX, CONV_FUNC)	\
	__XDP2_BITMAP_2ARG_CMP_GEN_FUNCT(first_and_zero, |, ~,,,	\
					 NUM_WORD_BITS, TYPE, SUFFIX,	\
					 CONV_FUNC)			\
	__XDP2_BITMAP_2ARG_CMP_GEN_FUNCT(first_or_zero, &, ~,,,		\
					 NUM_WORD_BITS, TYPE, SUFFIX,	\
					 CONV_FUNC)

__XDP2_MAKE_2ARG_GEN_CMP_FUNCTS(XDP2_BITMAP_BITS_PER_WORD, unsigned long,,)
__XDP2_MAKE_2ARG_GEN_CMP_FUNCTS(8, __u8, 8,)
__XDP2_MAKE_2ARG_GEN_CMP_FUNCTS(16, __u16, 16,)
__XDP2_MAKE_2ARG_GEN_CMP_FUNCTS(32, __u32, 32,)
__XDP2_MAKE_2ARG_GEN_CMP_FUNCTS(64, __u64, 64,)
__XDP2_MAKE_2ARG_GEN_CMP_FUNCTS(16, __u16, 16swp, bswap_16)
__XDP2_MAKE_2ARG_GEN_CMP_FUNCTS(32, __u32, 32swp, bswap_32)
__XDP2_MAKE_2ARG_GEN_CMP_FUNCTS(64, __u64, 64swp, bswap_64)

/******** Operations that take a source source-dest bitmap
 *
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]destsrc_and
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]destsrc_or
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]destsrc_xor
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]destsrc_nand
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]destsrc_nor
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]destsrc_nxor
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]destsrc_and_not
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]destsrc_or_not
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]destsrc_xor_not
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]destsrc_nand_not
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]destsrc_nor_not
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]destsrc_nxor_not
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]destsrc_and_test
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]destsrc_or_test
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]destsrc_xor_test
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]destsrc_nand_test
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]destsrc_nor_test
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]destsrc_nxor_test
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]destsrc_and_not_test
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]destsrc_or_not_test
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]destsrc_xor_not_test
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]destsrc_nand_not_test
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]destsrc_nor_not_test
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]destsrc_nxor_not_test
 *
 * void xdp2_bitmap[N][swp]_destsrc_{*}(__uN *dest, const __uN *src,
 *				 unsigned int pos, unsigned int nbits)
 *
 * unsigned int xdp2_bitmap[N][swp]_destsrc_{*}_test(__uN *dest,
 *					      const __uN *src,
 *					      unsigned int pos,
 *					      unsigned int nbits,
 *					      unsigned int test)
 *
 * void xdp2_rbitmap[N][swp]_destsrc_{*}(__uN *dest, const __uN *src,
 *				 unsigned int pos, unsigned int nbits)
 *				 unsigned int num_words)
 *
 * unsigned int xdp2_rbitmap[N][swp]_destsrc_{*}_test(__uN *dest,
 *					      const __uN *src,
 *					      unsigned int pos,
 *					      unsigned int nbits,
 *					      unsigned int test,
 *					      unsigned int num_words)
 *
 * void xdp2_bitmap[N][swp]_wsrc_destsrc_{*}(__uN *dest, __uN src,
 *				 unsigned int pos, unsigned int nbits)
 *
 * unsigned int xdp2_bitmap[N][swp]_wsrc_destsrc_{*}_test(__uN *dest,
 *					      __uN src,
 *					      unsigned int pos,
 *					      unsigned int nbits,
 *					      unsigned int test)
 *
 * void xdp2_rbitmap[N][swp]_wsrc_destsrc_{*}(__uN *dest, __uN src,
 *				 unsigned int pos, unsigned int nbits)
 *				 unsigned int num_words)
 *
 * unsigned int xdp2_rbitmap[N][swp]_wsrc_destsrc_{*}_test(__uN *dest,
 *					      __uN src,
 *					      unsigned int pos,
 *					      unsigned int nbits,
 *					      unsigned int test,
 *					      unsigned int num_words)
 */
#define __XDP2_MAKE_DESTSRC_PERMUTE_FUNCTS(NUM_WORD_BITS, TYPE, SUFFIX,	\
					   CONV_FUNC)			\
	__XDP2_BITMAP_2ARG_DESTSRC_PERMUTE_FUNCT(destsrc_and, &,,,,	\
						 NUM_WORD_BITS, TYPE,	\
						 SUFFIX, CONV_FUNC)	\
	__XDP2_BITMAP_2ARG_DESTSRC_PERMUTE_FUNCT(destsrc_or, |,,,,	\
						 NUM_WORD_BITS, TYPE,	\
						 SUFFIX, CONV_FUNC)	\
	__XDP2_BITMAP_2ARG_DESTSRC_PERMUTE_FUNCT(destsrc_xor, ^,,,,	\
						 NUM_WORD_BITS, TYPE,	\
						 SUFFIX, CONV_FUNC)	\
	__XDP2_BITMAP_2ARG_DESTSRC_PERMUTE_FUNCT(destsrc_nand, &, ~,,,	\
						 NUM_WORD_BITS, TYPE,	\
						 SUFFIX, CONV_FUNC)	\
	__XDP2_BITMAP_2ARG_DESTSRC_PERMUTE_FUNCT(destsrc_nor, |, ~,,,	\
						 NUM_WORD_BITS, TYPE,	\
						 SUFFIX, CONV_FUNC)	\
	__XDP2_BITMAP_2ARG_DESTSRC_PERMUTE_FUNCT(destsrc_nxor, ^, ~,,,	\
						 NUM_WORD_BITS, TYPE,	\
						 SUFFIX, CONV_FUNC)	\
	__XDP2_BITMAP_2ARG_DESTSRC_PERMUTE_FUNCT(destsrc_and_not,	\
						 &,,, ~,		\
						 NUM_WORD_BITS, TYPE,	\
						 SUFFIX, CONV_FUNC)	\
	__XDP2_BITMAP_2ARG_DESTSRC_PERMUTE_FUNCT(destsrc_or_not,	\
						 |,,, ~,		\
						 NUM_WORD_BITS, TYPE,	\
						 SUFFIX, CONV_FUNC)	\
	__XDP2_BITMAP_2ARG_DESTSRC_PERMUTE_FUNCT(destsrc_xor_not,	\
						 ^,,, ~,		\
						 NUM_WORD_BITS, TYPE,	\
						 SUFFIX, CONV_FUNC)	\
	__XDP2_BITMAP_2ARG_DESTSRC_PERMUTE_FUNCT(destsrc_nand_not,	\
						 &, ~,, ~,		\
						 NUM_WORD_BITS, TYPE,	\
						 SUFFIX, CONV_FUNC)	\
	__XDP2_BITMAP_2ARG_DESTSRC_PERMUTE_FUNCT(destsrc_nor_not,	\
						 |, ~,, ~,		\
						 NUM_WORD_BITS, TYPE,	\
						 SUFFIX, CONV_FUNC)	\
	__XDP2_BITMAP_2ARG_DESTSRC_PERMUTE_FUNCT(destsrc_nxor_not,	\
						 ^, ~,, ~,		\
						 NUM_WORD_BITS, TYPE,	\
						 SUFFIX, CONV_FUNC)

__XDP2_MAKE_DESTSRC_PERMUTE_FUNCTS(XDP2_BITMAP_BITS_PER_WORD, unsigned long,,)
__XDP2_MAKE_DESTSRC_PERMUTE_FUNCTS(8, __u8, 8,)
__XDP2_MAKE_DESTSRC_PERMUTE_FUNCTS(16, __u16, 16,)
__XDP2_MAKE_DESTSRC_PERMUTE_FUNCTS(32, __u32, 32,)
__XDP2_MAKE_DESTSRC_PERMUTE_FUNCTS(64, __u64, 64,)
__XDP2_MAKE_DESTSRC_PERMUTE_FUNCTS(16, __u16, 16swp, bswap_16)
__XDP2_MAKE_DESTSRC_PERMUTE_FUNCTS(32, __u32, 32swp, bswap_32)
__XDP2_MAKE_DESTSRC_PERMUTE_FUNCTS(64, __u64, 64swp, bswap_64)

/******** Operations that take generic a source source-dest bitmap
 *
 *	xdp2_[r]bitmap[N][swp]_[_wsrc_]destsrc_and_gen
 *	xdp2_[r]bitmap[N][swp]_[_wsrc_]destsrc_or_gen
 *	xdp2_[r]bitmap[N][swp]_[_wsrc_]destsrc_xor_gen
 *	xdp2_[r]bitmap[N][swp]_[_wsrc_]destsrc_nand_gen
 *	xdp2_[r]bitmap[N][swp]_[_wsrc_]destsrc_nor_gen
 *	xdp2_[r]bitmap[N][swp]_[_wsrc_]destsrc_nxor_gen
 *	xdp2_[r]bitmap[N][swp]_[_wsrc_]destsrc_and_not_gen
 *	xdp2_[r]bitmap[N][swp]_[_wsrc_]destsrc_or_not_gen
 *	xdp2_[r]bitmap[N][swp]_[_wsrc_]destsrc_xor_not_gen
 *	xdp2_[r]bitmap[N][swp]_[_wsrc_]destsrc_nand_not_gen
 *	xdp2_[r]bitmap[N][swp]_[_wsrc_]destsrc_nor_not_gen
 *	xdp2_[r]bitmap[N][swp]_[_wsrc_]destsrc_nxor_not_gen
 *	xdp2_[r]bitmap[N][swp]_[_wsrc_]destsrc_and_test_gen
 *	xdp2_[r]bitmap[N][swp]_[_wsrc_]destsrc_or_test_gen
 *	xdp2_[r]bitmap[N][swp]_[_wsrc_]destsrc_xor_test_gen
 *	xdp2_[r]bitmap[N][swp]_[_wsrc_]destsrc_nand_test_gen
 *	xdp2_[r]bitmap[N][swp]_[_wsrc_]destsrc_nor_test_gen
 *	xdp2_[r]bitmap[N][swp]_[_wsrc_]destsrc_nxor_test_gen
 *	xdp2_[r]bitmap[N][swp]_[_wsrc_]destsrc_and_not_test_gen
 *	xdp2_[r]bitmap[N][swp]_[_wsrc_]destsrc_or_not_test_gen
 *	xdp2_[r]bitmap[N][swp]_[_wsrc_]destsrc_xor_not_test_gen
 *	xdp2_[r]bitmap[N][swp]_[_wsrc_]destsrc_nand_not_test_gen
 *	xdp2_[r]bitmap[N][swp]_[_wsrc_]destsrc_nor_not_test_gen
 *	xdp2_[r]bitmap[N][swp]_[_wsrc_]destsrc_nxor_not_test_gen
 *
 * void xdp2_bitmap[N][swp]_destsrc_{*}_gen(__uN *dest,
 *				     unsigned int dest_pos,
 *				     const __uN *src,
 *				     unsigned int src_pos,
 *				     unsigned int nbits)
 *
 * unsigned ini xdp2_bitmap[N][swp]_destsrc_{*}_test_gen(__uN *dest,
 *						  unsigned int dest_pos,
 *						  const __uN *src,
 *						  unsigned int src_pos,
 *						  unsigned int nbits,
 *						  unsigned int test)
 * void xdp2_rbitmap[N][swp]_destsrc_{*}_gen(__uN *dest,
 *				     unsigned int dest_pos,
 *				     const __uN *src,
 *				     unsigned int src_pos,
 *				     unsigned int nbits,
 *				     unsigned int num_words)
 *
 * unsigned ini xdp2_rbitmap[N][swp]_destsrc_{*}_test_gen(__uN *dest,
 *						  unsigned int dest_pos,
 *						  const __uN *src,
 *						  unsigned int src_pos,
 *						  unsigned int nbits,
 *						  unsigned int test,
 *						  unsigned int num_words)
 *
 * void xdp2_bitmap[N][swp]_wsrc_destsrc_{*}_gen(__uN *dest,
 *				     unsigned int dest_pos,
 *				     __uN src,
 *				     unsigned int src_pos,
 *				     unsigned int nbits)
 *
 * unsigned ini xdp2_bitmap[N][swp]_wsrc_destsrc_{*}_test_gen(__uN *dest,
 *						  unsigned int dest_pos,
 *						  __uN src,
 *						  unsigned int src_pos,
 *						  unsigned int nbits,
 *						  unsigned int test)
 * void xdp2_rbitmap[N][swp]_wsrc_destsrc_{*}_gen(__uN *dest,
 *				     unsigned int dest_pos,
 *				     __uN src,
 *				     unsigned int src_pos,
 *				     unsigned int nbits,
 *				     unsigned int num_words)
 *
 * unsigned ini xdp2_rbitmap[N][swp]_wsrc_destsrc_{*}_test_gen(__uN *dest,
 *						  unsigned int dest_pos,
 *						  __uN src,
 *						  unsigned int src_pos,
 *						  unsigned int nbits,
 *						  unsigned int test,
 *						  unsigned int num_words)
 */
#define __XDP2_MAKE_DESTSRC_GEN_PERMUTE_FUNCTS(NUM_WORD_BITS, TYPE,	\
					       SUFFIX, CONV_FUNC)	\
	__XDP2_BITMAP_2ARG_DESTSRC_GEN_PERMUTE_FUNCT(destsrc_and,	\
						     &,,,,NUM_WORD_BITS,\
						     TYPE, SUFFIX,	\
						     CONV_FUNC)		\
	__XDP2_BITMAP_2ARG_DESTSRC_GEN_PERMUTE_FUNCT(destsrc_or,	\
						     |,,,,NUM_WORD_BITS,\
						     TYPE, SUFFIX,	\
						     CONV_FUNC)		\
	__XDP2_BITMAP_2ARG_DESTSRC_GEN_PERMUTE_FUNCT(destsrc_xor,	\
						     ^,,,,NUM_WORD_BITS,\
						     TYPE, SUFFIX,	\
						     CONV_FUNC)		\
	__XDP2_BITMAP_2ARG_DESTSRC_GEN_PERMUTE_FUNCT(destsrc_nand,	\
						     &, ~,,,		\
						     NUM_WORD_BITS,	\
						     TYPE, SUFFIX,	\
						     CONV_FUNC)		\
	__XDP2_BITMAP_2ARG_DESTSRC_GEN_PERMUTE_FUNCT(destsrc_nor,	\
						     |, ~,,,		\
						     NUM_WORD_BITS,	\
						     TYPE, SUFFIX,	\
						     CONV_FUNC)		\
	__XDP2_BITMAP_2ARG_DESTSRC_GEN_PERMUTE_FUNCT(destsrc_nxor,	\
						     ^, ~,,,		\
						     NUM_WORD_BITS,	\
						     TYPE, SUFFIX,	\
						     CONV_FUNC)		\
	__XDP2_BITMAP_2ARG_DESTSRC_GEN_PERMUTE_FUNCT(destsrc_and_not,	\
						     &,,, ~,		\
						     NUM_WORD_BITS,	\
						     TYPE, SUFFIX,	\
						     CONV_FUNC)		\
	__XDP2_BITMAP_2ARG_DESTSRC_GEN_PERMUTE_FUNCT(destsrc_or_not,	\
						     |,,, ~,		\
						     NUM_WORD_BITS,	\
						     TYPE, SUFFIX,	\
						     CONV_FUNC)		\
	__XDP2_BITMAP_2ARG_DESTSRC_GEN_PERMUTE_FUNCT(destsrc_xor_not,	\
						     ^,,, ~,		\
						     NUM_WORD_BITS,	\
						     TYPE, SUFFIX,	\
						     CONV_FUNC)		\
	__XDP2_BITMAP_2ARG_DESTSRC_GEN_PERMUTE_FUNCT(destsrc_nand_not,	\
						     &, ~,, ~,		\
						     NUM_WORD_BITS,	\
						     TYPE, SUFFIX,	\
						     CONV_FUNC)		\
	__XDP2_BITMAP_2ARG_DESTSRC_GEN_PERMUTE_FUNCT(destsrc_nor_not,	\
						     |, ~,, ~,		\
						     NUM_WORD_BITS,	\
						     TYPE, SUFFIX,	\
						     CONV_FUNC)		\
	__XDP2_BITMAP_2ARG_DESTSRC_GEN_PERMUTE_FUNCT(destsrc_nxor_not,	\
						     ^, ~,, ~,		\
						     NUM_WORD_BITS,	\
						     TYPE, SUFFIX,	\
						     CONV_FUNC)

__XDP2_MAKE_DESTSRC_GEN_PERMUTE_FUNCTS(XDP2_BITMAP_BITS_PER_WORD,
				       unsigned long,,)
__XDP2_MAKE_DESTSRC_GEN_PERMUTE_FUNCTS(8, __u8, 8,)
__XDP2_MAKE_DESTSRC_GEN_PERMUTE_FUNCTS(16, __u16, 16,)
__XDP2_MAKE_DESTSRC_GEN_PERMUTE_FUNCTS(32, __u32, 32,)
__XDP2_MAKE_DESTSRC_GEN_PERMUTE_FUNCTS(64, __u64, 64,)
__XDP2_MAKE_DESTSRC_GEN_PERMUTE_FUNCTS(16, __u16, 16swp, bswap_16)
__XDP2_MAKE_DESTSRC_GEN_PERMUTE_FUNCTS(32, __u32, 32swp, bswap_32)
__XDP2_MAKE_DESTSRC_GEN_PERMUTE_FUNCTS(64, __u64, 64swp, bswap_64)

/******** Operations that take two sources and a destination bitmap */

/* XDP2 bitmap operator functions.
 *
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]and
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]or
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]xor
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]nand
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]nor
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]xor
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]and_not
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]or_not
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]xor_not
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]nand_not
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]nor_not
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]nxor_not
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]and_test
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]or_test
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]xor_test
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]nand_test
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]nor_test
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]xor_test
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]and_not_test
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]or_not_test
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]xor_not_test
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]nand_not_test
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]nor_not_test
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]nxor_not_test
 *
 * void xdp2_bitmap[N][swp]_{*}(__uN *dest, const __uN *src1, const __uN *src2,
 *			 unsigned int pos, unsigned int nbits)
 *
 * unsigned int xdp2_bitmap[N][swp]_{*}_test(__uN *dest, const __uN *src1,
 *				      const __uN src2, unsigned int pos,
 *				      unsigned int nbits, unsigned int test)
 *
 * void xdp2_rbitmap[N][swp]_{*}(__uN *dest, const __uN *src1,
 *			 const __uN *src2,
 *			 unsigned int pos, unsigned int nbits,
 *			 unsigned int num_words)
 *
 * unsigned int xdp2_rbitmap[N][swp]_{*}_test(__uN *dest, const __uN *src1,
 *				      const __uN src2, unsigned int pos,
 *				      unsigned int nbits, unsigned int test,
 *				      unsigned int num_words)
 *
 * void xdp2_bitmap[N][swp]_wsrc{*}(__uN *dest, __uN src1, const __uN *src2,
 *			 unsigned int pos, unsigned int nbits)
 *
 * unsigned int xdp2_bitmap[N][swp]_wsrc_{*}_test(__uN *dest, __uN src1,
 *				      const __uN src2, unsigned int pos,
 *				      unsigned int nbits, unsigned int test)
 *
 * void xdp2_rbitmap[N][swp]__wsrc{*}(__uN *dest, __uN src1,
 *			 const __uN *src2,
 *			 unsigned int pos, unsigned int nbits,
 *			 unsigned int num_words)
 *
 * unsigned int xdp2_rbitmap[N][swp]_wsrc_{*}_test(__uN *dest, __uN src1,
 *				      const __uN src2, unsigned int pos,
 *				      unsigned int nbits, unsigned int test,
 *				      unsigned int num_words)
 */
#define __XDP2_MAKE_3ARG_PERMUTE_FUNCTS(NUM_WORD_BITS, TYPE, SUFFIX,	\
					CONV_FUNC)			\
	__XDP2_BITMAP_3ARG_PERMUTE_FUNCT(and, &,,,, NUM_WORD_BITS,	\
					 TYPE, SUFFIX, CONV_FUNC)	\
	__XDP2_BITMAP_3ARG_PERMUTE_FUNCT(or, |,,,, NUM_WORD_BITS,	\
					 TYPE, SUFFIX, CONV_FUNC)	\
	__XDP2_BITMAP_3ARG_PERMUTE_FUNCT(xor, ^,,,, NUM_WORD_BITS,	\
					 TYPE, SUFFIX, CONV_FUNC)	\
	__XDP2_BITMAP_3ARG_PERMUTE_FUNCT(nand, &, ~,,, NUM_WORD_BITS,	\
					 TYPE, SUFFIX, CONV_FUNC)	\
	__XDP2_BITMAP_3ARG_PERMUTE_FUNCT(nor, |, ~,,, NUM_WORD_BITS,	\
					 TYPE, SUFFIX, CONV_FUNC)	\
	__XDP2_BITMAP_3ARG_PERMUTE_FUNCT(nxor, ^, ~,,, NUM_WORD_BITS,	\
					 TYPE, SUFFIX, CONV_FUNC)	\
	__XDP2_BITMAP_3ARG_PERMUTE_FUNCT(and_not, &,,, ~,		\
					 NUM_WORD_BITS, TYPE, SUFFIX,	\
					 CONV_FUNC)			\
	__XDP2_BITMAP_3ARG_PERMUTE_FUNCT(or_not, |,,, ~, NUM_WORD_BITS,	\
					 TYPE, SUFFIX, CONV_FUNC)	\
	__XDP2_BITMAP_3ARG_PERMUTE_FUNCT(xor_not, ^,,, ~, NUM_WORD_BITS,\
					 TYPE, SUFFIX, CONV_FUNC)	\
	__XDP2_BITMAP_3ARG_PERMUTE_FUNCT(nand_not, &, ~,, ~,		\
					 NUM_WORD_BITS, TYPE, SUFFIX,	\
					 CONV_FUNC)			\
	__XDP2_BITMAP_3ARG_PERMUTE_FUNCT(nor_not, |, ~,, ~,		\
					 NUM_WORD_BITS, TYPE, SUFFIX,	\
					 CONV_FUNC)			\
	__XDP2_BITMAP_3ARG_PERMUTE_FUNCT(nxor_not, ^, ~,, ~,		\
					 NUM_WORD_BITS, TYPE, SUFFIX,	\
					 CONV_FUNC)

__XDP2_MAKE_3ARG_PERMUTE_FUNCTS(XDP2_BITMAP_BITS_PER_WORD, unsigned long,,)
__XDP2_MAKE_3ARG_PERMUTE_FUNCTS(8, __u8, 8,)
__XDP2_MAKE_3ARG_PERMUTE_FUNCTS(16, __u16, 16,)
__XDP2_MAKE_3ARG_PERMUTE_FUNCTS(32, __u32, 32,)
__XDP2_MAKE_3ARG_PERMUTE_FUNCTS(64, __u64, 64,)
__XDP2_MAKE_3ARG_PERMUTE_FUNCTS(16, __u16, 16swp, bswap_16)
__XDP2_MAKE_3ARG_PERMUTE_FUNCTS(32, __u32, 32swp, bswap_32)
__XDP2_MAKE_3ARG_PERMUTE_FUNCTS(64, __u64, 64swp, bswap_64)

/******** Operations that take two generic sources and a destination bitmap
 *
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]and_gen
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]or_gen
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]xor_gen
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]nand_gen
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]nor_gen
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]nxor_gen
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]and_not_gen
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]or_not_gen
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]xor_not_gen
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]nand_not_gen
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]nor_not_gen
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]nxor_not_gen
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]and_test_gen
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]or_test_gen
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]xor_test_gen
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]nand_test_gen
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]nor_test_gen
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]nxor_test_gen
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]and_not_test_gen
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]or_not_test_gen
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]xor_not_test_gen
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]nand_not_test_gen
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]nor_not_test_gen
 *	xdp2_[r]bitmap[N][swp]_[wsrc_]nxor_not_test_gen
 *
 * void xdp2_bitmap[N][swp]_{*}_gen(__uN *dest,
 *			     unsigned int dest_pos,
 *			     const __uN *src1,
 *			     unsigned int src1_pos,
 *			     const __uN *src2,
 *			     unsigned int src2_pos,
 *			     unsigned int nbits)
 *
 * unsigned int xdp2_bitmap[N][swp]_{*}_test_gen(__uN *dest,
 *					  unsigned int dest_pos,
 *					  const __uN *src1,
 *					  unsigned int src1_pos,
 *					  const __uN *src2,
 *					  unsigned int src2_pos,
 *					  unsigned int nbits,
 *					  unsigned int test)
 *
 * void xdp2_rbitmap[N][swp]_{*}_gen(__uN *dest,
 *			     unsigned int dest_pos,
 *			     const __uN *src1,
 *			     unsigned int src1_pos,
 *			     const __uN *src2,
 *			     unsigned int src2_pos,
 *			     unsigned int nbits,
 *			     unsigned int num_words)
 *
 * unsigned int xdp2_rbitmap[N][swp]_{*}_test_gen(__uN *dest,
 *					  unsigned int dest_pos,
 *					  const __uN *src1,
 *					  unsigned int src1_pos,
 *					  const __uN *src2,
 *					  unsigned int src2_pos,
 *					  unsigned int nbits,
 *					  unsigned int test,
 *					  unsigned int num_words)
 *
 * void xdp2_bitmap[N][swp]_wsrc_{*}_gen(__uN *dest,
 *			     unsigned int dest_pos,
 *			     __uN src1,
 *			     unsigned int src1_pos,
 *			     const __uN *src2,
 *			     unsigned int src2_pos,
 *			     unsigned int nbits)
 *
 * unsigned int xdp2_bitmap[N][swp]_wsrc_{*}_test_gen(__uN *dest,
 *					  unsigned int dest_pos,
 *					  __uN src1,
 *					  unsigned int src1_pos,
 *					  const __uN *src2,
 *					  unsigned int src2_pos,
 *					  unsigned int nbits,
 *					  unsigned int test)
 *
 * void xdp2_rbitmap[N][swp]_wsrc_{*}_gen(__uN *dest,
 *			     unsigned int dest_pos,
 *			     __uN src1,
 *			     unsigned int src1_pos,
 *			     const __uN *src2,
 *			     unsigned int src2_pos,
 *			     unsigned int nbits,
 *			     unsigned int num_words)
 *
 * unsigned int xdp2_rbitmap[N][swp]_wsrc_{*}_test_gen(__uN *dest,
 *					  unsigned int dest_pos,
 *					  __uN src1,
 *					  unsigned int src1_pos,
 *					  const __uN *src2,
 *					  unsigned int src2_pos,
 *					  unsigned int nbits,
 *					  unsigned int test,
 *					  unsigned int num_words)
 */
#define __XDP2_MAKE_3ARG_GEN_PERMUTE_FUNCTS(NUM_WORD_BITS, TYPE,	\
					    SUFFIX, CONV_FUNC)		\
	__XDP2_BITMAP_3ARG_GEN_PERMUTE_FUNCT(and, &,,,, NUM_WORD_BITS,	\
					     TYPE, SUFFIX, CONV_FUNC)	\
	__XDP2_BITMAP_3ARG_GEN_PERMUTE_FUNCT(or, |,,,, NUM_WORD_BITS,	\
					     TYPE, SUFFIX, CONV_FUNC)	\
	__XDP2_BITMAP_3ARG_GEN_PERMUTE_FUNCT(xor, ^,,,, NUM_WORD_BITS,	\
					     TYPE, SUFFIX, CONV_FUNC)	\
	__XDP2_BITMAP_3ARG_GEN_PERMUTE_FUNCT(nand, &, ~,,,		\
					     NUM_WORD_BITS, TYPE,	\
					     SUFFIX, CONV_FUNC)		\
	__XDP2_BITMAP_3ARG_GEN_PERMUTE_FUNCT(nor, |, ~,,, NUM_WORD_BITS,\
					     TYPE, SUFFIX, CONV_FUNC)	\
	__XDP2_BITMAP_3ARG_GEN_PERMUTE_FUNCT(nxor, ^, ~,,,		\
					     NUM_WORD_BITS, TYPE,	\
					     SUFFIX, CONV_FUNC)		\
	__XDP2_BITMAP_3ARG_GEN_PERMUTE_FUNCT(and_not, &,,, ~,		\
					     NUM_WORD_BITS, TYPE,	\
					     SUFFIX, CONV_FUNC)		\
	__XDP2_BITMAP_3ARG_GEN_PERMUTE_FUNCT(or_not, |,,, ~,		\
					     NUM_WORD_BITS, TYPE,	\
					     SUFFIX, CONV_FUNC)		\
	__XDP2_BITMAP_3ARG_GEN_PERMUTE_FUNCT(xor_not, ^,,, ~,		\
					     NUM_WORD_BITS, TYPE,	\
					     SUFFIX, CONV_FUNC)		\
	__XDP2_BITMAP_3ARG_GEN_PERMUTE_FUNCT(nand_not, &, ~,, ~,	\
					     NUM_WORD_BITS, TYPE,	\
					     SUFFIX, CONV_FUNC)		\
	__XDP2_BITMAP_3ARG_GEN_PERMUTE_FUNCT(nor_not, |, ~,, ~,		\
					     NUM_WORD_BITS, TYPE,	\
					     SUFFIX, CONV_FUNC)		\
	__XDP2_BITMAP_3ARG_GEN_PERMUTE_FUNCT(nxor_not, ^, ~,, ~,	\
					     NUM_WORD_BITS, TYPE,	\
					     SUFFIX, CONV_FUNC)

__XDP2_MAKE_3ARG_GEN_PERMUTE_FUNCTS(XDP2_BITMAP_BITS_PER_WORD,
				    unsigned long,,)
__XDP2_MAKE_3ARG_GEN_PERMUTE_FUNCTS(8, __u8, 8,)
__XDP2_MAKE_3ARG_GEN_PERMUTE_FUNCTS(16, __u16, 16,)
__XDP2_MAKE_3ARG_GEN_PERMUTE_FUNCTS(32, __u32, 32,)
__XDP2_MAKE_3ARG_GEN_PERMUTE_FUNCTS(64, __u64, 64,)
__XDP2_MAKE_3ARG_GEN_PERMUTE_FUNCTS(16, __u16, 16swp, bswap_16)
__XDP2_MAKE_3ARG_GEN_PERMUTE_FUNCTS(32, __u32, 32swp, bswap_32)
__XDP2_MAKE_3ARG_GEN_PERMUTE_FUNCTS(64, __u64, 64swp, bswap_64)

/* Read and write words into a bitmap */

#define __XDP2_BITMAP_READ_WRITE_PERMUTE(NUM_WORD_BITS, TYPE, SUFFIX)	\
	__XDP2_BITMAP_READ_WRITE_FUNC(NUM_WORD_BITS, TYPE,		\
				      SUFFIX, false, 8)			\
	__XDP2_BITMAP_READ_WRITE_FUNC(NUM_WORD_BITS, TYPE,		\
				      SUFFIX, false, 16)		\
	__XDP2_BITMAP_READ_WRITE_FUNC(NUM_WORD_BITS, TYPE,		\
				      SUFFIX, false, 32)		\
	__XDP2_BITMAP_READ_WRITE_FUNC(NUM_WORD_BITS, TYPE,		\
				      SUFFIX, false, 64)

#define __XDP2_BITMAP_READ_WRITE_SWP(NUM_WORD_BITS, TYPE,		\
				     SUFFIX, TARG_BITS)			\
	__XDP2_BITMAP_READ_WRITE_FUNC(NUM_WORD_BITS, TYPE,		\
				      SUFFIX, true, TARG_BITS)

#define __XDP2_BITMAP_READ_WRITE_PERMUTE_SWP(NUM_WORD_BITS, TYPE,	\
					     SUFFIX)			\
	__XDP2_BITMAP_READ_WRITE_SWP(NUM_WORD_BITS, TYPE,		\
				     SUFFIX, 8)				\
	__XDP2_BITMAP_READ_WRITE_SWP(NUM_WORD_BITS, TYPE,		\
				     SUFFIX, 16)			\
	__XDP2_BITMAP_READ_WRITE_SWP(NUM_WORD_BITS, TYPE,		\
				     SUFFIX, 32)			\
	__XDP2_BITMAP_READ_WRITE_SWP(NUM_WORD_BITS, TYPE,		\
				     SUFFIX, 64)

__XDP2_BITMAP_READ_WRITE_PERMUTE(XDP2_BITMAP_BITS_PER_WORD, unsigned long,)
__XDP2_BITMAP_READ_WRITE_PERMUTE(8, __u8, 8)
__XDP2_BITMAP_READ_WRITE_PERMUTE(16, __u16, 16)
__XDP2_BITMAP_READ_WRITE_PERMUTE(32, __u32, 32)
__XDP2_BITMAP_READ_WRITE_PERMUTE(64, __u64, 64)

__XDP2_BITMAP_READ_WRITE_PERMUTE_SWP(16, __u16, 16swp)
__XDP2_BITMAP_READ_WRITE_PERMUTE_SWP(32, __u32, 32swp)
__XDP2_BITMAP_READ_WRITE_PERMUTE_SWP(64, __u64, 64swp)

/* The bitmap functions in linux/bitmap.h can be mapped to XDP2 bitmap
 * operations as shown below.
 *
 * bitmap_zero(dst, nbits)			*dst = 0UL
 *	xdp2_bitmap_set_zeroes(dst, 0, nbits)
 *	xdp2_bitmap_copy_wsrc(dst, 0, 0, nbits)
 * bitmap_fill(dst, nbits)			*dst = ~0UL
 *	xdp2_bitmap_set_ones(dst, 0, nbits)
 *	xdp2_bitmap_copy_wsrc(dst, ~0UL, 0, nbits)
 * bitmap_copy(dst, src, nbits)			*dst = *src
 *	xdp2_bitmap_copy(dst, src, 0, nbits)
 * bitmap_and(dst, src1, src2, nbits)		*dst = *src1 & *src2
 *	xdp2_bitmap_and(dst, src, 0, nbits)
 * bitmap_or(dst, src1, src2, nbits)		*dst = *src1 | *src2
 *	xdp2_bitmap_or(dst, src, 0, nbits)
 * bitmap_xor(dst, src1, src2, nbits)		*dst = *src1 ^ *src2
 *	xdp2_bitmap_xor(dst, src, 0, nbits)
 * bitmap_andnot(dst, src1, src2, nbits)	*dst = *src1 & ~(*src2)
 *	xdp2_bitmap_and_not(dst, src1, src2, 0, nbits)
 * bitmap_complement(dst, src, nbits)		*dst = ~(*src)
 *	xdp2_bitmap_not(dst, src, 0, nbits)
 * bitmap_equal(src1, src2, nbits)		Are *src1 and *src2 equal?
 *	xdp2_bitmap_cmp(src1, src1, 0, nbits)
 * bitmap_intersects(src1, src2, nbits)		Do *src1 and *src2 overlap?
 *	xdp2_bitmap_and_test(src1, src2, 0, nbits, XDP2_BITMAP_TEST_ANY_SET)
 * bitmap_subset(src1, src2, nbits)		Is *src1 a subset of *src2?
 *	xdp2_bitmap_cmp(src1, src1, 0, nbits)
 * bitmap_empty(src, nbits)			Are all bits zero in *src?
 *	!xdp2_bitmap_test(src, 0, nbits, XDP2_BITMAP_TEST_ANY_SET)
 * bitmap_full(src, nbits)			Are all bits set in *src?
 *	!xdp2_bitmap_test(src, 0, nbits, XDP2_BITMAP_TEST_ANY_ZERO)
 * bitmap_weight(src, nbits)			Hamming Weight: number set bits
 *	xdp2_bitmap_test(src, 0, nbits, XDP2_BITMAP_TEST_WEIGHT)
 * bitmap_shift_right(dst, src, n, nbits)	*dst = *src >> n
 *	xdp2_shift_right(dst, src, n, nbits)
 * bitmap_shift_left(dst, src, n, nbits)	*dst = *src << n
 *	xdp2_shift_left(dst, src, n, nbits)
 *
 * The following are more advanced operations. These can be implemented in
 * a backend C library using the basic routines. Also, these won't require
 * different word sizes, swap, or endinanness
 *
 * bitmap_remap(dst, src, old, new, nbits)	*dst = map(old, new)(src)
 * bitmap_bitremap(oldbit, old, new, nbits)	newbit = map(old, new)(oldbit)
 * bitmap_onto(dst, orig, relmap, nbits)	*dst = orig relative to relmap
 * bitmap_fold(dst, orig, sz, nbits)		dst bits = orig bits mod sz
 * bitmap_scnprintf(buf, len, src, nbits)	Print bitmap src to buf
 * bitmap_parse(buf, buflen, dst, nbits)       Parse bitmap dst from kernel buf
 * bitmap_parse_user(ubuf, ulen, dst, nbits)	Parse bitmap dst from user buf
 * bitmap_scnlistprintf(buf, len, src, nbits)	Print bitmap src as list to buf
 * bitmap_parselist(buf, dst, nbits)		Parse bitmap dst from list
 * bitmap_find_free_region(bitmap, bits, order)	Find and allocate bit region
 * bitmap_release_region(bitmap, pos, order)	Free specified bit region
 * bitmap_allocate_region(bitmap, pos, order)	Allocate specified bit region
 */

#endif /* __XDP2_BITMAP_H__ */
