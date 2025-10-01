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

#ifndef __XDP2_BITMAP_WORD_H__
#define __XDP2_BITMAP_WORD_H__

#include <string.h>

#include "xdp2/pmacro.h"
#include "xdp2/utility.h"

/******** Bitmap word operations */

/* Set operations */

#define __XDP2_BITMAP_WORD_SET(N)					\
static inline __u##N __xdp2_bitmap_word##N##_set(__u##N v,		\
						  unsigned int index)	\
{									\
	return v | XDP2_BIT(index);					\
}

XDP2_PMACRO_APPLY_ALL(__XDP2_BITMAP_WORD_SET, 8, 16, 32, 64)

#define xdp2_bitmap_word8_set(V, INDEX)				\
	(void)((V) = __xdp2_bitmap_word8_set(V, INDEX))
#define xdp2_bitmap_word16_set(V, INDEX)				\
	(void)((V) = __xdp2_bitmap_word16_set(V, INDEX))
#define xdp2_bitmap_word32_set(V, INDEX)				\
	(void)((V) = __xdp2_bitmap_word32_set(V, INDEX))
#define xdp2_bitmap_word64_set(V, INDEX)				\
	(void)((V) = __xdp2_bitmap_word64_set(V, INDEX))

/* Unset operations */

#define __XDP2_BITMAP_WORD_UNSET(N)					\
static inline __u##N __xdp2_bitmap_word##N##_unset(__u##N v,		\
						    unsigned int index)	\
{									\
	return v & ~XDP2_BIT(index);					\
}

XDP2_PMACRO_APPLY_ALL(__XDP2_BITMAP_WORD_UNSET, 8, 16, 32, 64)

#define xdp2_bitmap_word8_unset(V, INDEX)				\
	(void)((V) = __xdp2_bitmap_word8_unset(V, INDEX))
#define xdp2_bitmap_word16_unset(V, INDEX)				\
	(void)((V) = __xdp2_bitmap_word16_unset(V, INDEX))
#define xdp2_bitmap_word32_unset(V, INDEX)				\
	(void)((V) = __xdp2_bitmap_word32_unset(V, INDEX))
#define xdp2_bitmap_word64_unset(V, INDEX)				\
	(void)((V) = __xdp2_bitmap_word64_unset(V, INDEX))

/* Isset operations */

#define __XDP2_BITMAP_WORD_ISSET(N)					\
static inline bool xdp2_bitmap_word##N##_isset(__u##N v,		\
						unsigned int index)	\
{									\
	return !!(v & XDP2_BIT(index));				\
}

XDP2_PMACRO_APPLY_ALL(__XDP2_BITMAP_WORD_ISSET, 8, 16, 32, 64)

/******** Compute hamming weight on words */

#define __XDP2_BITMAP_WORD_WEIGHT_FUNCT(N, CAST, FUNC)			\
static inline unsigned int						\
		__xdp2_bitmap_word##N##_weight_popcount(__u##N v)	\
{									\
	return FUNC((CAST)v);						\
}

__XDP2_BITMAP_WORD_WEIGHT_FUNCT(8, __u32, __builtin_popcount)
__XDP2_BITMAP_WORD_WEIGHT_FUNCT(16, __u32, __builtin_popcount)
__XDP2_BITMAP_WORD_WEIGHT_FUNCT(32, __u32, __builtin_popcount)
__XDP2_BITMAP_WORD_WEIGHT_FUNCT(64, __u64, __builtin_popcountl)

#define __XDP2_BITMAP_WEIGHT_DEFAULT_FUNCT(N)				\
static inline unsigned int						\
			__xdp2_bitmap_word##N##_default(__u##N v)	\
{									\
	unsigned int w = 0;						\
	int i;								\
									\
	for (i = 0; i < (N); i++)					\
		w += !!(v & XDP2_BIT(i));				\
	return w;							\
}

XDP2_PMACRO_APPLY_ALL(__XDP2_BITMAP_WEIGHT_DEFAULT_FUNCT, 8, 16, 32, 64)

/* Use popcount variant by default */
#define xdp2_bitmap_word8_weight __xdp2_bitmap_word8_weight_popcount
#define xdp2_bitmap_word16_weight __xdp2_bitmap_word16_weight_popcount
#define xdp2_bitmap_word32_weight __xdp2_bitmap_word32_weight_popcount
#define xdp2_bitmap_word64_weight __xdp2_bitmap_word64_weight_popcount

/******** Fundamental find operations on words */

/* Find first set bit */

#define __XDP2_BITMAP_WORD_FIND_FUNCT(N, CAST, FUNC)			\
static inline unsigned int						\
			__xdp2_bitmap_word##N##_find_ffs(__u##N v)	\
{									\
	unsigned int ret;						\
									\
	ret = FUNC((CAST)v);						\
	return ret ? ret - 1 : N;					\
}

__XDP2_BITMAP_WORD_FIND_FUNCT(8, __u32, ffs)
__XDP2_BITMAP_WORD_FIND_FUNCT(16, __u32, ffs)
__XDP2_BITMAP_WORD_FIND_FUNCT(32, __u32, ffs)
__XDP2_BITMAP_WORD_FIND_FUNCT(64, __u64, ffsl)

#define __XDP2_BITMAP_FIND_DEFAULT_FUNCT(N)				\
static inline unsigned int						\
			__xdp2_bitmap_word##N##_find_default(__u##N v)	\
{									\
	unsigned int i;							\
									\
	for (i = 0; i < N; i++)						\
		if (v & XDP2_BIT(i))					\
			break;						\
	return i;							\
}

XDP2_PMACRO_APPLY_ALL(__XDP2_BITMAP_FIND_DEFAULT_FUNCT, 8, 16, 32, 64)

/* Use ffs variant by default */
#define xdp2_bitmap_word8_find __xdp2_bitmap_word8_find_ffs
#define xdp2_bitmap_word16_find __xdp2_bitmap_word16_find_ffs
#define xdp2_bitmap_word32_find __xdp2_bitmap_word32_find_ffs
#define xdp2_bitmap_word64_find __xdp2_bitmap_word64_find_ffs

/* Find first set bit from a starting position */

#define __XDP2_BITMAP_WORD_FIND_START(N)				\
static inline unsigned int xdp2_bitmap_word##N##_find_start(		\
					__u##N v, unsigned int pos)	\
{									\
	__u##N mask = ~(XDP2_BIT(pos) - 1);				\
									\
	return xdp2_bitmap_word##N##_find(v & mask);			\
}

XDP2_PMACRO_APPLY_ALL(__XDP2_BITMAP_WORD_FIND_START, 8, 16, 32, 64)

/* Find first zero bit */

#define __XDP2_BITMAP_FIND_ZERO_DEFAULT_FUNCT(N)			\
static inline unsigned int __xdp2_bitmap_word##N##_find_zero_default(	\
					__u##N v, unsigned int pos)	\
{									\
	return xdp2_bitmap_word##N##_find(~v);				\
}

XDP2_PMACRO_APPLY_ALL(__XDP2_BITMAP_FIND_ZERO_DEFAULT_FUNCT, 8, 16, 32, 64)

#define xdp2_bitmap_word8_find_zero __xdp2_bitmap_word8_find_zero_default
#define xdp2_bitmap_word16_find_zero __xdp2_bitmap_word16_find_zero_default
#define xdp2_bitmap_word32_find_zero __xdp2_bitmap_word32_find_zero_default
#define xdp2_bitmap_word64_find_zero __xdp2_bitmap_word64_find_zero_default

/* Find first zero bit from a starting position */

#define __XDP2_BITMAP_WORD_FIND_ZERO_START(N)				\
static inline unsigned int xdp2_bitmap_word##N##_find_zero_start(	\
					__u##N v, unsigned int pos)	\
{									\
	return xdp2_bitmap_word##N##_find_start(~v, pos);		\
}

XDP2_PMACRO_APPLY_ALL(__XDP2_BITMAP_WORD_FIND_ZERO_START, 8, 16, 32, 64)

/* Find next set bit after a position */

#define __XDP2_BITMAP_WORD_FIND_NEXT(N)				\
static inline unsigned int xdp2_bitmap_word##N##_find_next(		\
					__u##N v, unsigned int pos)	\
{									\
	__u##N _mask = ~(XDP2_BIT(pos + 1) - 1);			\
									\
	if (pos >= N - 1)						\
		return N;						\
	_mask = ~(XDP2_BIT(pos + 1) - 1);				\
									\
	return xdp2_bitmap_word##N##_find(v & _mask);			\
}

XDP2_PMACRO_APPLY_ALL(__XDP2_BITMAP_WORD_FIND_NEXT, 8, 16, 32, 64)

/* Find next zero bit after a position */

#define __XDP2_BITMAP_WORD_FIND_NEXT_ZERO(N)				\
static inline unsigned int xdp2_bitmap_word##N##_find_next_zero(	\
					__u##N v, unsigned int pos)	\
{									\
	return xdp2_bitmap_word##N##_find_next(~v, pos);		\
}

XDP2_PMACRO_APPLY_ALL(__XDP2_BITMAP_WORD_FIND_NEXT_ZERO, 8, 16, 32, 64)

/* Foreach set bit macros */

#define xdp2_bitmap_word8_foreach_bit(WORD, POS)			\
	for (POS = xdp2_bitmap_word8_find_start(WORD, POS);		\
	     POS < 8;							\
	     POS = xdp2_bitmap_word8_find_next(WORD, POS))

#define xdp2_bitmap_word16_foreach_bit(WORD, POS)			\
	for (POS = xdp2_bitmap_word16_find_start(WORD, POS);		\
	     POS < 16;							\
	     POS = xdp2_bitmap_word16_find_next(WORD, POS))

#define xdp2_bitmap_word32_foreach_bit(WORD, POS)			\
	for (POS = xdp2_bitmap_word32_find_start(WORD, POS);		\
	     POS < 32;							\
	     POS = xdp2_bitmap_word32_find_next(WORD, POS))

#define xdp2_bitmap_word64_foreach_bit(WORD, POS)			\
	for (POS = xdp2_bitmap_word64_find_start(WORD, POS);		\
	     POS < 64;							\
	     POS = xdp2_bitmap_word64_find_next(WORD, POS))

/* Foreach zero bit macros */

#define xdp2_bitmap_word8_foreach_zero_bit(WORD, POS)			\
	for (POS = xdp2_bitmap_word8_find_start(WORD, POS);		\
	     POS < 8;							\
	     POS = xdp2_bitmap_word8_find_next(WORD, POS))

#define xdp2_bitmap_word16_foreach_zero_bit(WORD, POS)			\
	for (POS = xdp2_bitmap_word16_find_start(WORD, POS);		\
	     POS < 16;							\
	     POS = xdp2_bitmap_word16_find_next(WORD, POS))

#define xdp2_bitmap_word32_foreach_zero_bit(WORD, POS)			\
	for (POS = xdp2_bitmap_word32_find_start(WORD, POS);		\
	     POS < 32;							\
	     POS = xdp2_bitmap_word32_find_next(WORD, POS))

#define xdp2_bitmap_word64_foreach_zero_bit(WORD, POS)			\
	for (POS = xdp2_bitmap_word64_find_start(WORD, POS);		\
	     POS < 64;							\
	     POS = xdp2_bitmap_word64_find_next(WORD, POS))

/******** Fundamental reverse find operations on words */

/* Find last set bit */

#define __XDP2_BITMAP_REV_FIND_DEFAULT_FUNCT(N)			\
static inline unsigned int						\
	__xdp2_bitmap_word##N##_rev_find_default(__u##N v)		\
{									\
	int i;								\
									\
	for (i = (N - 1); i >= 0; i--)					\
		if (v & XDP2_BIT(i))					\
			break;						\
	return i < 0 ? (N) : (unsigned int)i;				\
}

XDP2_PMACRO_APPLY_ALL(__XDP2_BITMAP_REV_FIND_DEFAULT_FUNCT, 8, 16, 32, 64)

#define __XDP2_BITMAP_REV_FIND_LOG_FUNCT(N)				\
static inline unsigned int						\
	__xdp2_bitmap_word##N##_rev_find_log(__u##N v)			\
{									\
	return v ? XDP2_LOG_##N##BITS(v) : (N);			\
}

XDP2_PMACRO_APPLY_ALL(__XDP2_BITMAP_REV_FIND_LOG_FUNCT, 8, 16, 32, 64)

#define xdp2_bitmap_word8_rev_find __xdp2_bitmap_word8_rev_find_log
#define xdp2_bitmap_word16_rev_find __xdp2_bitmap_word16_rev_find_log
#define xdp2_bitmap_word32_rev_find __xdp2_bitmap_word32_rev_find_log
#define xdp2_bitmap_word64_rev_find __xdp2_bitmap_word64_rev_find_log

/* Find last zero bit */

#define __XDP2_BITMAP_REV_FIND_ZERO_DEFAULT_FUNCT(N)			\
static inline unsigned int						\
	__xdp2_bitmap_word##N##_rev_find_zero_default(__u##N v)	\
{									\
	return __xdp2_bitmap_word##N##_rev_find_default(~v);		\
}

#define xdp2_bitmap_word8_rev_find_zero				\
		__xdp2_bitmap_word8_rev_find_zero_default
#define xdp2_bitmap_word16_rev_find_zero				\
		__xdp2_bitmap_word16_rev_find_zero_default
#define xdp2_bitmap_word32_rev_find_zero				\
		__xdp2_bitmap_word32_rev_find_zero_default
#define xdp2_bitmap_word64_rev_find_zero				\
		__xdp2_bitmap_word64_rev_find_zero_default

/* Find last set bit from a starting position */

#define __XDP2_BITMAP_WORD_REV_FIND_START(N)				\
static inline unsigned int xdp2_bitmap_word##N##_rev_find_start(	\
					__u##N v, unsigned int pos)	\
{									\
	__u##N mask = pos == 63 ? (__u##N)-1ULL :			\
						XDP2_BIT(pos + 1) - 1;	\
									\
	return xdp2_bitmap_word##N##_rev_find(v & mask);		\
}

XDP2_PMACRO_APPLY_ALL(__XDP2_BITMAP_WORD_REV_FIND_START, 8, 16, 32, 64)

/* Find last zero bit from a starting position */

#define __XDP2_BITMAP_WORD_REV_FIND_ZERO_START(N)			\
static inline unsigned int xdp2_bitmap_word##N##_rev_find_zero_start(	\
					__u##N v, unsigned int pos)	\
{									\
	return xdp2_bitmap_word##N##_rev_find_start(~v, pos);		\
}

XDP2_PMACRO_APPLY_ALL(__XDP2_BITMAP_WORD_REV_FIND_ZERO_START, 8, 16, 32, 64)

/* Find last set bit after a position */

#define __XDP2_BITMAP_WORD_REV_FIND_NEXT(N)				\
static inline unsigned int						\
		xdp2_bitmap_word##N##_rev_find_next(			\
					__u##N v, unsigned int pos)	\
{									\
	__u##N _mask;							\
									\
	if (pos == 0)							\
		return (N);						\
									\
	_mask = XDP2_BIT(pos) - 1;					\
									\
	return xdp2_bitmap_word##N##_rev_find(v & _mask);		\
}

XDP2_PMACRO_APPLY_ALL(__XDP2_BITMAP_WORD_REV_FIND_NEXT, 8, 16, 32, 64)

/* Find last zero bit after a position */

#define __XDP2_BITMAP_WORD_REV_FIND_NEXT_ZERO(N)			\
static inline unsigned int xdp2_bitmap_word##N##_rev_find_next_zero(	\
					__u##N v, unsigned int pos)	\
{									\
	return xdp2_bitmap_word##N##_rev_find_next(~v, pos);	\
}

XDP2_PMACRO_APPLY_ALL(__XDP2_BITMAP_WORD_REV_FIND_NEXT_ZERO, 8, 16, 32, 64)

/* Foreach last set bit macros */

#define xdp2_bitmap_word8_rev_foreach_bit(WORD, POS)			\
	for (POS = xdp2_bitmap_word8_rev_find_start(WORD, POS);	\
	     POS < 8;							\
	     POS = xdp2_bitmap_word8_rev_find_next(WORD, POS))

#define xdp2_bitmap_word16_rev_foreach_bit(WORD, POS)			\
	for (POS = xdp2_bitmap_word16_rev_find_start(WORD, POS);	\
	     POS < 16;							\
	     POS = xdp2_bitmap_word16_rev_find_next(WORD, POS))

#define xdp2_bitmap_word32_rev_foreach_bit(WORD, POS)			\
	for (POS = xdp2_bitmap_word32_rev_find_start(WORD, POS);	\
	     POS < 32;							\
	     POS = xdp2_bitmap_word32_rev_find_next(WORD, POS))

#define xdp2_bitmap_word64_rev_foreach_bit(WORD, POS)			\
	for (POS = xdp2_bitmap_word64_rev_find_start(WORD, POS);	\
	     POS < 64;							\
	     POS = xdp2_bitmap_word64_rev_find_next(WORD, POS))

/* Foreach last zero bit macros */

#define xdp2_bitmap_word8_rev_foreach_zero_bit(WORD, POS)		\
	for (POS = xdp2_bitmap_word8_rev_find_start(WORD, POS);	\
	     POS < 8;							\
	     POS = xdp2_bitmap_word8_rev_find_next(WORD, POS))

#define xdp2_bitmap_word16_rev_foreach_zero_bit(WORD, POS)		\
	for (POS = xdp2_bitmap_word16_rev_find_start(WORD, POS);	\
	     POS < 16;							\
	     POS = xdp2_bitmap_word16_rev_find_next(WORD, POS))

#define xdp2_bitmap_word32_rev_foreach_zero_bit(WORD, POS)		\
	for (POS = xdp2_bitmap_word32_rev_find_start(WORD, POS);	\
	     POS < 32;							\
	     POS = xdp2_bitmap_word32_rev_find_next(WORD, POS))

#define xdp2_bitmap_word64_rev_foreach_zero_bit(WORD, POS)		\
	for (POS = xdp2_bitmap_word64_rev_find_start(WORD, POS);	\
	     POS < 64;							\
	     POS = xdp2_bitmap_word64_rev_find_next(WORD, POS))

/* Return different bits */

#define __XDP2_BITMAP_WORD_CMP(N)					\
static inline unsigned int xdp2_bitmap_word##N##_cmp(__u##N v1,	\
						      __u##N v2)	\
{									\
	return xdp2_bitmap_word##N##_find(v1 ^ v2);			\
}

XDP2_PMACRO_APPLY_ALL(__XDP2_BITMAP_WORD_CMP, 8, 16, 32, 64)

/* Return different bits from a starting postion */

#define __XDP2_BITMAP_WORD_CMP_START(N)				\
static inline unsigned int xdp2_bitmap_word##N##_cmp_start(		\
				__u##N v1, __u##N v2, unsigned int pos)	\
{									\
	__u##N mask = ~(XDP2_BIT(pos) - 1);				\
									\
	return xdp2_bitmap_word##N##_cmp(v1 & mask, v2 & mask);	\
}

XDP2_PMACRO_APPLY_ALL(__XDP2_BITMAP_WORD_CMP_START, 8, 16, 32, 64)

/* Return different bits from a reverse starting postion */

#define __XDP2_BITMAP_WORD_CMP_REV_START(N)				\
static inline unsigned int xdp2_bitmap_word##N##_rev_start(		\
				__u##N v1, __u##N v2, unsigned int pos)	\
{									\
	__u##N mask = (XDP2_BIT(pos + 1) - 1);				\
									\
	return xdp2_bitmap_word##N##_cmp(v1 & mask, v2 & mask);	\
}

XDP2_PMACRO_APPLY_ALL(__XDP2_BITMAP_WORD_CMP_REV_START, 8, 16, 32, 64)

/* Return bits (copy) */

#define __XDP2_BITMAP_1SRC_WORD_COPY(N)				\
static inline __u##N xdp2_bitmap_word##N##_copy(__u##N v)		\
{									\
	return v;							\
}

XDP2_PMACRO_APPLY_ALL(__XDP2_BITMAP_1SRC_WORD_COPY, 8, 16, 32, 64)

/* Return bits (copy) from a starting postion */

#define __XDP2_BITMAP_1SRC_WORD_COPY_START(N)				\
static inline __u##N xdp2_bitmap_word##N##_copy_start(__u##N v,	\
						  unsigned int pos)	\
{									\
	return xdp2_bitmap_word##N##_copy(				\
					v & (~(XDP2_BIT(pos) - 1)));	\
}

XDP2_PMACRO_APPLY_ALL(__XDP2_BITMAP_1SRC_WORD_COPY_START, 8, 16, 32, 64)

/* Return bits (copy) from a reverse starting postion */

#define __XDP2_BITMAP_1SRC_WORD_COPY_REV_START(N)			\
static inline __u##N xdp2_bitmap_word##N##_copy_rev_start(__u##N v,	\
						  unsigned int pos)	\
{									\
	return xdp2_bitmap_word##N##_copy(				\
					v & (~(XDP2_BIT(pos) - 1)));	\
}

XDP2_PMACRO_APPLY_ALL(__XDP2_BITMAP_1SRC_WORD_COPY_REV_START,
		       8, 16, 32, 64)

/* Return not'ed bits */

#define __XDP2_BITMAP_1SRC_WORD_NOT(N)					\
static inline __u##N xdp2_bitmap_word##N##_not(__u##N v)		\
{									\
	return ~v;							\
}

XDP2_PMACRO_APPLY_ALL(__XDP2_BITMAP_1SRC_WORD_NOT, 8, 16, 32, 64)

/* Return not'ed bits from a starting postion */

#define __XDP2_BITMAP_1SRC_WORD_NOT_START(N)				\
static inline __u##N xdp2_bitmap_word##N##_not_start(__u##N v,	\
						  unsigned int pos)	\
{									\
	return xdp2_bitmap_word##N##_not(v & (~(XDP2_BIT(pos) - 1)));	\
}

XDP2_PMACRO_APPLY_ALL(__XDP2_BITMAP_1SRC_WORD_NOT_START, 8, 16, 32, 64)

/* Return not'ed bits from a reverse starting postion */

#define __XDP2_BITMAP_1SRC_WORD_NOT_REV_START(N)			\
static inline __u##N xdp2_bitmap_word##N##_not_rev_start(__u##N v,	\
						  unsigned int pos)	\
{									\
	return xdp2_bitmap_word##N##_not(v & ((XDP2_BIT(pos) - 1)));	\
}

XDP2_PMACRO_APPLY_ALL(__XDP2_BITMAP_1SRC_WORD_NOT_REV_START, 8, 16, 32, 64)

/* Binary operations macro */

#define __XDP2_BITMAP_WORD(N, NAME, OP, NOT, S1_OP, S2_OP)		\
static inline __u##N xdp2_bitmap_word##N##_##NAME(__u##N v1,		\
						   __u##N v2)		\
{									\
	return NOT ((S1_OP v1) OP (S2_OP v2));				\
}

#define __XDP2_BITMAP_WORD_PERMUTE(NAME, OP, NOT, S1_OP, S2_OP)	\
	__XDP2_BITMAP_WORD(8, NAME, OP, NOT, S1_OP, S2_OP)		\
	__XDP2_BITMAP_WORD(16, NAME, OP, NOT, S1_OP, S2_OP)		\
	__XDP2_BITMAP_WORD(32, NAME, OP, NOT, S1_OP, S2_OP)		\
	__XDP2_BITMAP_WORD(64, NAME, OP, NOT, S1_OP, S2_OP)

#define ___XDP2_BITMAP_WORD_PERMUTE(ARGS)				\
	__XDP2_BITMAP_WORD_PERMUTE ARGS

XDP2_PMACRO_APPLY_ALL(___XDP2_BITMAP_WORD_PERMUTE,
	(and, &,,,), (or, |,,,), (xor, ^,,,), (nand, &, !,,),
	(nor, |, !,,), (nxor, ^, !,,), (and_not, &,,, !), (or_not, |,,, !),
	(xor_not, ^,,, !), (nand_not, &, !,, !))

/* Binary operations from a starting postion */

#define __XDP2_BITMAP_WORD_START(N, NAME, OP, NOT, S1_OP, S2_OP)	\
static inline __u##N xdp2_bitmap_word##N##_##NAME##_start(__u##N v1,	\
				__u##N v2, unsigned int pos)		\
{									\
	__u##N mask = ~(XDP2_BIT(pos) - 1);				\
									\
	return (NOT((S1_OP v1) OP (S2_OP v2))) & mask;			\
}

#define __XDP2_BITMAP_WORD_START_PERMUTE(NAME, OP, NOT, S1_OP, S2_OP)	\
	__XDP2_BITMAP_WORD_START(8, NAME, OP, NOT, S1_OP, S2_OP)	\
	__XDP2_BITMAP_WORD_START(16, NAME, OP, NOT, S1_OP, S2_OP)	\
	__XDP2_BITMAP_WORD_START(32, NAME, OP, NOT, S1_OP, S2_OP)	\
	__XDP2_BITMAP_WORD_START(64, NAME, OP, NOT, S1_OP, S2_OP)

#define ___XDP2_BITMAP_WORD_START_PERMUTE(ARGS)			\
	__XDP2_BITMAP_WORD_START_PERMUTE ARGS

XDP2_PMACRO_APPLY_ALL(___XDP2_BITMAP_WORD_START_PERMUTE,
	(and, &,,,), (or, |,,,), (xor, ^,,,), (nand, &, !,,),
	(nor, |, !,,), (nxor, ^, !,,), (and_not, &,,, !), (or_not, |,,, !),
	(xor_not, ^,,, !), (nand_not, &, !,, !))

/* Binary operations from a reverse starting postion */

#define __XDP2_BITMAP_WORD_REV_START(N, NAME, OP, NOT, S1_OP, S2_OP)	\
static inline __u##N							\
		xdp2_bitmap_word##N##_##NAME##_rev_start(__u##N v1,	\
				__u##N v2, unsigned int pos)		\
{									\
	__u##N mask = XDP2_BIT(pos + 1) - 1;				\
									\
	return (NOT((S1_OP v1) OP (S2_OP v2))) & mask;			\
}

#define __XDP2_BITMAP_WORD_REV_START_PERMUTE(NAME, OP, NOT,		\
					      S1_OP, S2_OP)		\
	__XDP2_BITMAP_WORD_REV_START(8, NAME, OP, NOT, S1_OP, S2_OP)	\
	__XDP2_BITMAP_WORD_REV_START(16, NAME, OP, NOT, S1_OP, S2_OP)	\
	__XDP2_BITMAP_WORD_REV_START(32, NAME, OP, NOT, S1_OP, S2_OP)	\
	__XDP2_BITMAP_WORD_REV_START(64, NAME, OP, NOT, S1_OP, S2_OP)

#define ___XDP2_BITMAP_WORD_REV_START_PERMUTE(ARGS)			\
	__XDP2_BITMAP_WORD_REV_START_PERMUTE ARGS

XDP2_PMACRO_APPLY_ALL(___XDP2_BITMAP_WORD_REV_START_PERMUTE,
	(and, &,,,), (or, |,,,), (xor, ^,,,), (nand, &, !,,),
	(nor, |, !,,), (nxor, ^, !,,), (and_not, &,,, !), (or_not, |,,, !),
	(xor_not, ^,,, !), (nand_not, &, !,, !))

static inline __u64 xdp2_bitmap_word_expand(__u64 v, __u64 p)
{
	unsigned int pos, shift;
	__u64 ret = 0;

	pos = xdp2_bitmap_word64_find(p);
	for (shift = 0; pos < 64; shift++) {
		ret |= xdp2_bitmap_word64_isset(v, shift) << pos;
		pos = xdp2_bitmap_word64_find_next(p, pos);
	}

	return ret;
}

static inline __u64 xdp2_bitmap_word_extract(__u64 v, __u64 p)
{
	unsigned long pos, shift;
	__u64 ret = 0;

	pos = xdp2_bitmap_word64_find(p);

	for (shift = 0; pos < sizeof(p) * 8; shift++) {
		ret |= xdp2_bitmap_word64_isset(v, pos) << shift;
		pos = xdp2_bitmap_word64_find_next(p, pos);
	}

	return ret;
}

/* Generate function to rotate left for different word sizes */
#define XDP2_BITMAP_MAKE_ROL(SIZE)					\
static inline __u##SIZE xdp2_bitmap_word_rol##SIZE(			\
		__u##SIZE val, unsigned int num_rotate)			\
{									\
	return (val << num_rotate) | (val >> (SIZE - num_rotate));	\
}

XDP2_BITMAP_MAKE_ROL(8)
XDP2_BITMAP_MAKE_ROL(16)
XDP2_BITMAP_MAKE_ROL(32)
XDP2_BITMAP_MAKE_ROL(64)

/* Generic rotate left */
static inline __u64 xdp2_bitmap_word_rol(__u64 val, unsigned int num_rotate,
					  unsigned int nbits)
{
	num_rotate %= nbits;

	switch (nbits) {
	case 8:
		return xdp2_bitmap_word_rol8((__u8)val, num_rotate);
	case 16:
		return xdp2_bitmap_word_rol16((__u16)val, num_rotate);
	case 32:
		return xdp2_bitmap_word_rol32((__u32)val, num_rotate);
	case 64:
		return xdp2_bitmap_word_rol64((__u64)val, num_rotate);
	default:
		XDP2_ERR(1, "bitmap word rol need nbit multiple of "
			     "eight bytes: %u %% 8 != 0", nbits);
	}
}

/* Generate function to rotate right for different word sizes */
#define XDP2_BITMAP_MAKE_ROR(SIZE)					\
static inline __u##SIZE xdp2_bitmap_word_ror##SIZE(			\
		__u##SIZE val, unsigned int num_rotate)			\
{									\
	return (val >> num_rotate) | (val << (SIZE - num_rotate));	\
}

XDP2_BITMAP_MAKE_ROR(8)
XDP2_BITMAP_MAKE_ROR(16)
XDP2_BITMAP_MAKE_ROR(32)
XDP2_BITMAP_MAKE_ROR(64)

/* Generic rotate right */
static inline __u64 xdp2_bitmap_word_ror(__u64 val, unsigned int num_rotate,
					  unsigned int nbits)
{
	num_rotate %= nbits;

	switch (nbits) {
	case 8:
		return xdp2_bitmap_word_ror8((__u8)val, num_rotate);
	case 16:
		return xdp2_bitmap_word_ror16((__u16)val, num_rotate);
	case 32:
		return xdp2_bitmap_word_ror32((__u32)val, num_rotate);
	case 64:
		return xdp2_bitmap_word_ror64((__u64)val, num_rotate);
	default:
		XDP2_ERR(1, "bitmap word ror need nbits multiple of "
			     "eight bytes: %u %% 8 != 0", nbits);
	}
}

#endif /* __XDP2_BITMAP_WORD_H__ */
