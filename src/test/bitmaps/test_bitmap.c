// SPDX-License-Identifier: BSD-2-Clause-FreeBSD
/*
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

#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "xdp2/bitmap.h"
#include "xdp2/cli.h"
#include "xdp2/utility.h"

#include "test_bitmap.h"

/* Test for XDP2 bitmaps
 *
 * Run: ./test_bitmap [ -c <test-count> ] [ -v <verbose> ]
 *                    [ -I <report-interval> ][ -C <cli_port_num> ]
 *                    [-R] [ -F <function> ] [ -l ]
 */

#define NUM_WORDS (((MAX_NBITS - 1) / 64) + 1)

struct test_block {
	union {
		__u64 data64[NUM_WORDS];
		__u32 data32[NUM_WORDS * (sizeof(__u64) / sizeof(__u32))];
		__u8 data8[NUM_WORDS * (sizeof(__u64) / sizeof(__u8))];
	};
};

int verbose;

/* CLI commands */

XDP2_CLI_MAKE_NUMBER_SET(verbose, verbose);

static void print_map(const char *label,
		      struct test_block *block, unsigned int nbits)
{
	int i;

	printf("%s\n", label);
	for (i = 0; i < nbits / 64; i++)
		printf("\t%016llx\n", block->data64[i]);

	if (nbits % 64)
		printf("\t%016llx\n",
		      block->data64[i] & (-1ULL >> (64 - (nbits % 64))));
}

/* Hand compute shift and rotate functions for verification. If just_shift
 * is true then a shift is being doing, else a rotate is. If shift is
 * greater then zero then a shift or rotate left is done for that many bits,
 * and if shift is less than zero then a shift or rotate right is being done
 */
static void shift_rotate(__u8 *dest, const __u8 *src, int shift,
			 bool just_shift, unsigned int nbits,
			 struct bitmap_func *func)
{
	unsigned int pos;
	int i, dindex;
	size_t len;

	len = (nbits + 7) / 8;

	if (func->flags & FLAG_REV)
		memset(dest + ((MAX_NBITS / 8) - len), 0, len);
	else
		memset(dest, 0, len);

	for (i = 0, dindex = shift; i < nbits; i++, dindex++) {
		if (dindex < 0) {
			if (just_shift)
				continue;
			pos = nbits + dindex;
		} else if (dindex >= nbits) {
			if (just_shift)
				continue;
			pos = dindex - nbits;
		} else {
			pos = dindex;
		}
		test_bit_set_val(dest, pos, test_bit_isset(src, i, func),
				 func);
	}
}

#define MAKE_READ_TEST_VALUE(NUM, DEST, SRC, SRC_POS, FUNC) ({		\
	struct bitmap_func _targ_func = *(FUNC);			\
	XDP2_JOIN2(__u, NUM) _res = 0;					\
	bool _v;							\
	int _i;								\
									\
	_targ_func.num_word_bits = NUM;					\
	_targ_func.flags &= ~FLAG_REV;					\
									\
	for (_i = 0; _i < NUM; _i++) {					\
		_v = test_bit_isset(SRC, (SRC_POS) + _i, FUNC);		\
		test_bit_set_val(DEST, _i, _v, &_targ_func);		\
	}								\
	_res;								\
})

static void read_val(__u8 *dest, const __u8 *src,
		     unsigned int src_pos, struct bitmap_func *func)
{
	switch (func->test_op) {
	case TEST_READ8:
		MAKE_READ_TEST_VALUE(8, dest, src, src_pos, func);
		break;
	case TEST_READ16:
		MAKE_READ_TEST_VALUE(16, dest, src, src_pos, func);
		break;
	case TEST_READ32:
		MAKE_READ_TEST_VALUE(32, dest, src, src_pos, func);
		break;
	case TEST_READ64:
		MAKE_READ_TEST_VALUE(64, dest, src, src_pos, func);
		break;
	default:
		XDP2_ERR(1, "Bad test op in read");
	}
}

#define MAKE_WRITE_TEST_VALUE(NUM, DEST, SRC, DEST_POS, FUNC) ({	\
	struct bitmap_func _targ_func = *(FUNC);			\
	XDP2_JOIN2(__u, NUM) _res = 0;					\
	bool _v;							\
	int _i;								\
									\
	_targ_func.num_word_bits = NUM;					\
	_targ_func.flags &= ~FLAG_REV;					\
									\
	for (_i = 0; _i < NUM; _i++) {					\
		_v = test_bit_isset(SRC, _i, &_targ_func);		\
		test_bit_set_val(DEST, (DEST_POS) + _i, _v, FUNC);	\
	}								\
	_res;								\
})

static void write_val(__u8 *dest, const __u8 *src,
		      unsigned int dest_pos, struct bitmap_func *func)
{
	switch (func->test_op) {
	case TEST_WRITE8:
		MAKE_WRITE_TEST_VALUE(8, dest, src, dest_pos, func);
		break;
	case TEST_WRITE16:
		MAKE_WRITE_TEST_VALUE(16, dest, src, dest_pos, func);
		break;
	case TEST_WRITE32:
		MAKE_WRITE_TEST_VALUE(32, dest, src, dest_pos, func);
		break;
	case TEST_WRITE64:
		MAKE_WRITE_TEST_VALUE(64, dest, src, dest_pos, func);
		break;
	default:
		XDP2_ERR(1, "Bad test op in write");
	}
}

/* Run one test to get baseline */
static unsigned int test_op(struct bitmap_func *func,
			    __u8 *dest, unsigned int dest_pos,
			    const __u8 *src1, unsigned int src1_pos,
			    const __u8 *src2, unsigned int src2_pos,
			    unsigned int nbits, unsigned int test)
{
	unsigned int num_word_bits = func->num_word_bits ? :
						XDP2_BITMAP_BITS_PER_WORD;
	struct bitmap_func src1_func = *func;
	bool found = false, no_set = false;
	bool bit1, bit2, res_bit;
	unsigned int w = 0, spos;
	int i;

	switch (func->test_op) {
	case TEST_CMP:
	case TEST_FIRST_AND:
	case TEST_FIRST_OR:
	case TEST_FIRST_AND_ZERO:
	case TEST_FIRST_OR_ZERO:
		no_set = true;
		break;
	case TEST_SHIFT_LEFT:
		shift_rotate(dest, src1, dest_pos, true, nbits, func);
		return 0;
	case TEST_SHIFT_RIGHT:
		shift_rotate(dest, src1, -dest_pos, true, nbits, func);
		return 0;
	case TEST_ROTATE_LEFT:
		shift_rotate(dest, src1, dest_pos, false, nbits, func);
		return 0;
	case TEST_ROTATE_RIGHT:
		shift_rotate(dest, src1, -dest_pos, false, nbits, func);
		return 0;
	case TEST_READ8:
	case TEST_READ16:
	case TEST_READ32:
	case TEST_READ64:
		read_val(dest, src1, src1_pos, func);
		return 0;
	case TEST_WRITE8:
	case TEST_WRITE16:
	case TEST_WRITE32:
	case TEST_WRITE64:
		write_val(dest, src1, dest_pos, func);
		return 0;
	default:
		break;
	}

	if (func->flags & FLAG_WORD_SRC)
		src1_func.flags = src1_func.flags & ~FLAG_REV;

	for (i = 0; i < nbits - dest_pos; i++) {
		if (func->flags & FLAG_WORD_SRC)
			spos = (src1_pos + i) % num_word_bits;
		else
			spos = src1_pos + i;
		bit1 = test_bit_isset(src1, spos, &src1_func);
		bit2 = test_bit_isset(src2, src2_pos + i, func);
		res_bit = test_one(func->test_op, bit1, bit2, &found);
		if (res_bit)
			w++;

		if (found)
			break;

		if (no_set)
			continue;

		test_bit_set_val(dest, dest_pos + i, res_bit, func);
	}

	switch (func->test_op) {
	case TEST_CMP:
	case TEST_FIRST_AND:
	case TEST_FIRST_OR:
	case TEST_FIRST_AND_ZERO:
	case TEST_FIRST_OR_ZERO:
		if (!(func->flags & FLAG_ROLL) || i + dest_pos < nbits)
			return i + dest_pos;

		/* Roll to find bit */
		for (i = 0; i < nbits; i++) {
			bit1 = test_bit_isset(src1, i, func);
			bit2 = test_bit_isset(src2, i, func);
			test_one(func->test_op, bit1, bit2, &found);
			if (found)
				break;
		}
		return i;
	default:
		break;
	}

	switch (test) {
	case XDP2_BITMAP_TEST_NONE:
	case XDP2_BITMAP_TEST_WEIGHT:
	default:
		break;
	case XDP2_BITMAP_TEST_ANY_SET:
		w = !!w;
		break;
	case XDP2_BITMAP_TEST_ANY_ZERO:
		w = (w != nbits - dest_pos);
		break;
	}

	return w;
}

/* The foreach_xor test execises the foreach iterator functionality. If
 * basically performs a bit by bit copy using the foreach_bit and
 * foreach_zero_bit iterators
 */
#define MAKE_FOREACH_XOR_TEST(TYPE, SUFFIX)				\
static void XDP2_JOIN3(xdp2_bitmap, SUFFIX, _foreach_xor_test)(		\
		       TYPE *dest, unsigned int dest_pos,		\
		       const TYPE *src1, unsigned int src1_pos,		\
		       const TYPE *src2, unsigned int src2_pos,		\
		       unsigned int nbits)				\
{									\
	unsigned int s1nbits = src1_pos + (nbits - dest_pos);		\
	unsigned int pos = src1_pos;					\
	bool bit1, bit2, res_bit;					\
									\
	XDP2_JOIN3(xdp2_bitmap, SUFFIX, _foreach_bit)(src1, pos,	\
		   s1nbits) {						\
		bit1 = XDP2_JOIN3(xdp2_bitmap, SUFFIX,_isset)(src1,	\
							      pos);	\
		bit2 = XDP2_JOIN3(xdp2_bitmap, SUFFIX, _isset)(src2,	\
					src2_pos + (pos - src1_pos));	\
		res_bit = bit1 ^ bit2;					\
									\
		if (res_bit)						\
			XDP2_JOIN3(xdp2_bitmap, SUFFIX, _set)(dest,	\
				dest_pos + (pos - src1_pos));		\
		else							\
			XDP2_JOIN3(xdp2_bitmap, SUFFIX, _set_val)(	\
				dest, dest_pos + (pos - src1_pos),	\
							res_bit);	\
	}								\
									\
	pos = src1_pos;							\
	XDP2_JOIN3(xdp2_bitmap, SUFFIX, _foreach_zero_bit)(src1, pos,	\
		   s1nbits) {						\
		bit1 = XDP2_JOIN3(xdp2_bitmap, SUFFIX, _isset)(src1,	\
							       pos);	\
		bit2 = XDP2_JOIN3(xdp2_bitmap, SUFFIX, _isset)(		\
				src2,src2_pos + (pos - src1_pos));	\
		res_bit = bit1 ^ bit2;					\
									\
		if (!res_bit)						\
			XDP2_JOIN3(xdp2_bitmap, SUFFIX, _unset)		\
				(dest, dest_pos + (pos - src1_pos));	\
		else							\
			XDP2_JOIN3(xdp2_bitmap, SUFFIX, _set_val)(	\
				dest, dest_pos + (pos - src1_pos),	\
							res_bit);	\
	}								\
}

MAKE_FOREACH_XOR_TEST(unsigned long,)
MAKE_FOREACH_XOR_TEST(__u8, 8)
MAKE_FOREACH_XOR_TEST(__u16, 16)
MAKE_FOREACH_XOR_TEST(__u32, 32)
MAKE_FOREACH_XOR_TEST(__u64, 64)
MAKE_FOREACH_XOR_TEST(__u16, 16swp)
MAKE_FOREACH_XOR_TEST(__u32, 32swp)
MAKE_FOREACH_XOR_TEST(__u64, 64swp)

#define MAKE_REV_FOREACH_XOR_TEST(TYPE, SUFFIX)				\
static void XDP2_JOIN3(xdp2_rbitmap, SUFFIX, _foreach_xor_test)(	\
		       TYPE *dest, unsigned int dest_pos,		\
		       const TYPE *src1, unsigned int src1_pos,		\
		       const TYPE *src2, unsigned int src2_pos,		\
		       unsigned int nbits, unsigned int num_words)	\
{									\
	unsigned int s1nbits = src1_pos + (nbits - dest_pos);		\
	unsigned int pos = src1_pos;					\
	bool bit1, bit2, res_bit;					\
									\
	XDP2_JOIN3(xdp2_rbitmap, SUFFIX, _foreach_bit)(src1, pos,	\
		   s1nbits, num_words) {				\
		bit1 = XDP2_JOIN3(xdp2_rbitmap, SUFFIX,_isset)(src1,	\
						      pos, num_words);	\
		XDP2_ASSERT(bit1, "Bad one isset pos: %u", pos);	\
		bit2 = XDP2_JOIN3(xdp2_rbitmap, SUFFIX, _isset)(src2,	\
			src2_pos + (pos - src1_pos), num_words);	\
		res_bit = bit1 ^ bit2;					\
									\
		if (res_bit)						\
			XDP2_JOIN3(xdp2_rbitmap, SUFFIX, _set)(dest,	\
			dest_pos + (pos - src1_pos), num_words);	\
		else							\
			XDP2_JOIN3(xdp2_rbitmap, SUFFIX, _set_val)(	\
				dest, dest_pos + (pos - src1_pos),	\
						res_bit, num_words);	\
	}								\
									\
	pos = src1_pos;							\
	XDP2_JOIN3(xdp2_rbitmap, SUFFIX, _foreach_zero_bit)(src1, pos,	\
		   s1nbits, num_words) {				\
		bit1 = XDP2_JOIN3(xdp2_rbitmap, SUFFIX, _isset)(src1,	\
						       pos, num_words);	\
		XDP2_ASSERT(!bit1, "Bad zero isset pos: %u", pos);	\
		bit2 = XDP2_JOIN3(xdp2_rbitmap, SUFFIX, _isset)(	\
			src2,src2_pos + (pos - src1_pos), num_words);	\
		res_bit = bit1 ^ bit2;					\
									\
		if (!res_bit)						\
		    XDP2_JOIN3(xdp2_rbitmap, SUFFIX, _unset)		\
			(dest, dest_pos + (pos - src1_pos), num_words);	\
		else							\
			XDP2_JOIN3(xdp2_rbitmap, SUFFIX, _set_val)(	\
				dest, dest_pos + (pos - src1_pos),	\
						res_bit, num_words);	\
	}								\
}

MAKE_REV_FOREACH_XOR_TEST(unsigned long,)
MAKE_REV_FOREACH_XOR_TEST(__u8, 8)
MAKE_REV_FOREACH_XOR_TEST(__u16, 16)
MAKE_REV_FOREACH_XOR_TEST(__u32, 32)
MAKE_REV_FOREACH_XOR_TEST(__u64, 64)
MAKE_REV_FOREACH_XOR_TEST(__u16, 16swp)
MAKE_REV_FOREACH_XOR_TEST(__u32, 32swp)
MAKE_REV_FOREACH_XOR_TEST(__u64, 64swp)

enum {
	ARGMODE_B,
	ARGMODE_BB,
	ARGMODE_BBW,
	ARGMODE_BBB,
	ARGMODE_BBBW,
};

/* Define argument signatures. Note that we cast function pointers so make
 * sure the argument signature is correct!
 */
#define ARGS_REV unsigned int num_words

#define ARGS_B void *addr, unsigned int pos, unsigned int nbits

typedef unsigned int (*f_cmp_b_t)(ARGS_B);
typedef unsigned int (*f_rcmp_b_t)(ARGS_B, ARGS_REV);
typedef void (*f_set_b_t)(ARGS_B);
typedef void (*f_rset_b_t)(ARGS_B, ARGS_REV);
typedef unsigned int (*f_b_test_t)(ARGS_B, unsigned int test);
typedef unsigned int (*f_rb_test_t)(ARGS_B, ARGS_REV, unsigned int test);

#define ARGS_BB void *dest, const void *src1, unsigned int pos,		\
		unsigned int nbits

typedef void (*f_bb_t)(ARGS_BB);
typedef void (*f_rbb_t)(ARGS_BB, ARGS_REV);
typedef unsigned int (*f_cmp_bb_t)(ARGS_BB);
typedef unsigned int (*f_rcmp_bb_t)(ARGS_BB, ARGS_REV);
typedef unsigned int (*f_bb_test_t)(ARGS_BB, unsigned int test);
typedef unsigned int (*f_rbb_test_t)(ARGS_BB, ARGS_REV, unsigned int test);

#define ARGS_BBW void *dest, __u64 src1, unsigned int pos,		\
		unsigned int nbits

typedef void (*f_bbw_t)(ARGS_BBW);
typedef void (*f_rbbw_t)(ARGS_BBW, ARGS_REV);
typedef unsigned int (*f_cmp_bbw_t)(ARGS_BBW);
typedef unsigned int (*f_rcmp_bbw_t)(ARGS_BBW, ARGS_REV);
typedef unsigned int (*f_bbw_test_t)(ARGS_BBW, unsigned int test);
typedef unsigned int (*f_rbbw_test_t)(ARGS_BBW, ARGS_REV, unsigned int test);

#define ARGS_BB_GEN							\
	void *dest, unsigned int dest_pos,				\
	const void *src, unsigned int src_pos,				\
	unsigned int nbits

typedef void (*f_bb_gen_t)(ARGS_BB_GEN);
typedef void (*f_rbb_gen_t)(ARGS_BB_GEN, ARGS_REV);
typedef unsigned int (*f_bb_cmp_gen_t)(ARGS_BB_GEN);
typedef unsigned int (*f_rbb_cmp_gen_t)(ARGS_BB_GEN, ARGS_REV);
typedef unsigned int (*f_bb_test_gen_t)(ARGS_BB_GEN, unsigned int test);
typedef unsigned int (*f_rbb_test_gen_t)(ARGS_BB_GEN, ARGS_REV,
					 unsigned int test);

#define ARGS_BBW_GEN							\
	void *dest, unsigned int dest_pos,				\
	__u64 src, unsigned int src_pos,				\
	unsigned int nbits

typedef void (*f_bbw_gen_t)(ARGS_BBW_GEN);
typedef void (*f_rbbw_gen_t)(ARGS_BBW_GEN, ARGS_REV);
typedef unsigned int (*f_bbw_cmp_gen_t)(ARGS_BBW_GEN);
typedef unsigned int (*f_rbbw_cmp_gen_t)(ARGS_BBW_GEN, ARGS_REV);
typedef unsigned int (*f_bbw_test_gen_t)(ARGS_BBW_GEN, unsigned int test);
typedef unsigned int (*f_rbbw_test_gen_t)(ARGS_BBW_GEN, ARGS_REV,
					 unsigned int test);

#define ARGS_BBB							\
	void *dest, const void *src1, const void *src2,			\
	unsigned int pos, unsigned int nbits

typedef void (*f_bbb_t)(ARGS_BBB);
typedef void (*f_rbbb_t)(ARGS_BBB, ARGS_REV);
typedef unsigned int (*f_bbb_test_t)(ARGS_BBB, unsigned int test);
typedef unsigned int (*f_rbbb_test_t)(ARGS_BBB, ARGS_REV, unsigned int test);

#define ARGS_BBBW							\
	void *dest, __u64 src1, const void *src2,			\
	unsigned int pos, unsigned int nbits

typedef void (*f_bbbw_t)(ARGS_BBBW);
typedef void (*f_rbbbw_t)(ARGS_BBBW, ARGS_REV);
typedef unsigned int (*f_bbbw_test_t)(ARGS_BBBW, unsigned int test);
typedef unsigned int (*f_rbbbw_test_t)(ARGS_BBBW, ARGS_REV, unsigned int test);

#define ARGS_BBB_GEN							\
	void *dest, unsigned int dest_pos,				\
	const void *src1, unsigned int src1_pos,			\
	const void *src2, unsigned int src2_pos,			\
	unsigned int nbits

typedef void (*f_bbb_gen_t)(ARGS_BBB_GEN);
typedef void (*f_rbbb_gen_t)(ARGS_BBB_GEN, ARGS_REV);
typedef unsigned int (*f_bbb_test_gen_t)(ARGS_BBB_GEN, unsigned int test);
typedef unsigned int (*f_rbbb_test_gen_t)(ARGS_BBB_GEN, ARGS_REV,
					  unsigned int test);

#define ARGS_BBBW_GEN							\
	void *dest, unsigned int dest_pos,				\
	__u64 src1, unsigned int src1_pos,				\
	const void *src2, unsigned int src2_pos,			\
	unsigned int nbits

typedef void (*f_bbbw_gen_t)(ARGS_BBBW_GEN);
typedef void (*f_rbbbw_gen_t)(ARGS_BBBW_GEN, ARGS_REV);
typedef unsigned int (*f_bbbw_test_gen_t)(ARGS_BBBW_GEN, unsigned int test);
typedef unsigned int (*f_rbbbw_test_gen_t)(ARGS_BBBW_GEN, ARGS_REV,
					  unsigned int test);

#define MAKE_READ_WRITE_TEST(NUM_WORD_BITS, TYPE, SUFFIX, NUM)		\
static void XDP2_JOIN5(xdp2_bitmap, SUFFIX, _read, NUM, _test)(		\
		       TYPE *dest, const TYPE *src,			\
		       unsigned int src_pos, unsigned int nbits)	\
{									\
	XDP2_JOIN2(__u, NUM) targ_val = XDP2_JOIN4(xdp2_bitmap, SUFFIX,	\
						   _read, NUM)(		\
					src, src_pos);			\
									\
	*((XDP2_JOIN2(__u, NUM) *)dest) = targ_val;			\
}									\
									\
static void XDP2_JOIN5(xdp2_rbitmap, SUFFIX, _read, NUM, _test)(	\
		       TYPE *dest, const TYPE *src,			\
		       unsigned int src_pos, unsigned int nbits,	\
		       unsigned int num_words)				\
{									\
	XDP2_JOIN2(__u, NUM) targ_val = XDP2_JOIN4(xdp2_rbitmap, SUFFIX,\
						   _read, NUM)(		\
					src, src_pos, num_words);	\
									\
	*((XDP2_JOIN2(__u, NUM) *)dest) = targ_val;			\
}									\
									\
static void XDP2_JOIN5(xdp2_bitmap, SUFFIX, _write, NUM, _test)(	\
		       TYPE *dest, const TYPE *src,			\
		       unsigned int dest_pos, unsigned int nbits)	\
{									\
	XDP2_JOIN2(__u, NUM) targ_val = *((XDP2_JOIN2(__u, NUM) *)src);	\
									\
									\
	XDP2_JOIN4(xdp2_bitmap, SUFFIX,	_write, NUM)(			\
					targ_val, dest, dest_pos);	\
}									\
									\
static void XDP2_JOIN5(xdp2_rbitmap, SUFFIX, _write, NUM, _test)(	\
		       TYPE *dest, const TYPE *src,			\
		       unsigned int dest_pos, unsigned int nbits,	\
		       unsigned int num_words)				\
{									\
	XDP2_JOIN2(__u, NUM) targ_val = *((XDP2_JOIN2(__u, NUM) *)src);	\
									\
									\
	XDP2_JOIN4(xdp2_rbitmap, SUFFIX, _write, NUM)(			\
				targ_val, dest, dest_pos, num_words);	\
}

#define MAKE_READ_WRITE_VARIANTS(NUM_WORD_BITS, TYPE, SUFFIX)		\
	MAKE_READ_WRITE_TEST(NUM_WORD_BITS, TYPE, SUFFIX, 8)		\
	MAKE_READ_WRITE_TEST(NUM_WORD_BITS, TYPE, SUFFIX, 16)		\
	MAKE_READ_WRITE_TEST(NUM_WORD_BITS, TYPE, SUFFIX, 32)		\
	MAKE_READ_WRITE_TEST(NUM_WORD_BITS, TYPE, SUFFIX, 64)

MAKE_READ_WRITE_VARIANTS(XDP2_BITMAP_BITS_PER_WORD, unsigned long,)
MAKE_READ_WRITE_VARIANTS(8, __u8, 8)
MAKE_READ_WRITE_VARIANTS(16, __u16, 16)
MAKE_READ_WRITE_VARIANTS(32, __u32, 32)
MAKE_READ_WRITE_VARIANTS(64, __u64, 64)
MAKE_READ_WRITE_VARIANTS(16, __u16, 16swp)
MAKE_READ_WRITE_VARIANTS(32, __u32, 32swp)
MAKE_READ_WRITE_VARIANTS(64, __u64, 64swp)

/* Define the tests structure. This contains all the tests including a function
 * pointer to the xdp2 bitmap function
 */
struct bitmap_func bitmap_funcs[] = {
#include "bitmap_funcs.h"
};

/* Call the xdp2 function for a test case */
static unsigned int run_one_bitmap(struct bitmap_func *func, ARGS_BBB_GEN,
				   unsigned int test)
{
	unsigned int num_words = func->flags & FLAG_REV ? MAX_NBITS : 0;
	unsigned int num_word_bits = func->num_word_bits ? :
						XDP2_BITMAP_BITS_PER_WORD;
	unsigned int pos = dest_pos;
	unsigned int ret = 0;

	num_words /= num_word_bits;

	if (func->flags & FLAG_GEN) {
		if (func->flags & FLAG_TEST) {
			switch (func->argmode) {
			case ARGMODE_BB:
				if (num_words)
					ret = ((f_rbb_test_gen_t)func->func)
						(dest, dest_pos, src1, src1_pos,
						   nbits, test, num_words);
				else
					ret = ((f_bb_test_gen_t)func->func)
						(dest, dest_pos, src1, src1_pos,
						   nbits, test);
				break;
			case ARGMODE_BBW:
				if (num_words)
					ret = ((f_rbbw_test_gen_t)func->func)
						(dest, dest_pos,
						 *(__u64 *)src1, src1_pos,
						 nbits, test, num_words);
				else
					ret = ((f_bbw_test_gen_t)func->func)
						(dest, dest_pos,
						 *(__u64 *)src1, src1_pos,
						 nbits, test);
				break;
			case ARGMODE_BBB:
				if (num_words)
					ret = ((f_rbbb_test_gen_t)func->func)
						(dest, dest_pos, src1, src1_pos,
						 src2, src2_pos, nbits,
						 test, num_words);
				else
					ret = ((f_bbb_test_gen_t)func->func)
						(dest, dest_pos, src1, src1_pos,
						 src2, src2_pos, nbits, test);
				break;
			case ARGMODE_BBBW:
				if (num_words)
					ret = ((f_rbbbw_test_gen_t)func->func)
						(dest, dest_pos,
						 *(__u64 *)src1, src1_pos,
						 src2, src2_pos, nbits,
						 test, num_words);
				else
					ret = ((f_bbbw_test_gen_t)func->func)
						(dest, dest_pos,
						 *(__u64 *)src1, src1_pos,
						 src2, src2_pos, nbits, test);
				break;
			}
		} else if (func->flags & FLAG_CMP) {
			switch (func->argmode) {
			case ARGMODE_BB:
				if (num_words)
					ret = ((f_rbb_cmp_gen_t)func->func)
						(dest, dest_pos, src1, src1_pos,
						 nbits, num_words);
				else
					ret = ((f_bb_cmp_gen_t)func->func)
						(dest, dest_pos, src1, src1_pos,
						 nbits);
				break;
			case ARGMODE_BBW:
				if (num_words)
					ret = ((f_rbbw_cmp_gen_t)func->func)
						(dest, dest_pos,
						 *(__u64 *)src1, src1_pos,
						 nbits, num_words);
				else
					ret = ((f_bbw_cmp_gen_t)func->func)
						(dest, dest_pos,
						 *(__u64 *)src1, src1_pos,
						 nbits);
				break;
			}
		} else {
			switch (func->argmode) {
			case ARGMODE_BB:
				if (num_words)
					((f_rbb_gen_t)func->func)
						(dest, dest_pos, src1, src1_pos,
						   nbits, num_words);
				else
					((f_bb_gen_t)func->func)
						(dest, dest_pos, src1, src1_pos,
						   nbits);
				break;
			case ARGMODE_BBW:
				if (num_words)
					((f_rbbw_gen_t)func->func)
						(dest, dest_pos,
						 *(__u64 *)src1, src1_pos,
						 nbits, num_words);
				else
					((f_bbw_gen_t)func->func)
						(dest, dest_pos,
						 *(__u64 *)src1, src1_pos,
						 nbits);
				break;
			case ARGMODE_BBB:
				if (num_words)
					((f_rbbb_gen_t)func->func)
						(dest, dest_pos, src1, src1_pos,
						 src2, src2_pos, nbits,
						 num_words);
				else
					((f_bbb_gen_t)func->func)
						(dest, dest_pos, src1, src1_pos,
						   src2, src2_pos, nbits);
				break;
			case ARGMODE_BBBW:
				if (num_words)
					((f_rbbbw_gen_t)func->func)
						(dest, dest_pos,
						 *(__u64 *)src1, src1_pos,
						 src2, src2_pos, nbits,
						 num_words);
				else
					((f_bbbw_gen_t)func->func)
						(dest, dest_pos,
						 *(__u64 *)src1, src1_pos,
						 src2, src2_pos, nbits);
				break;
			}
		}
	} else {
		if (func->flags & FLAG_TEST) {
			switch (func->argmode) {
			case ARGMODE_B:
				if (num_words)
					ret = ((f_rb_test_t)func->func)
						(dest, pos, nbits, test,
						 num_words);
				else
					ret = ((f_b_test_t)func->func)
						(dest, pos, nbits, test);
				break;
			case ARGMODE_BB:
				if (num_words)
					ret = ((f_rbb_test_t)func->func)
						(dest, src1, pos, nbits,
						 test, num_words);
				else
					ret = ((f_bb_test_t)func->func)
						(dest, src1, pos, nbits, test);
				break;
			case ARGMODE_BBW:
				if (num_words)
					ret = ((f_rbbw_test_t)func->func)
						(dest, *(__u64 *)src1,
						 pos, nbits,
						 test, num_words);
				else
					ret = ((f_bbw_test_t)func->func)
						(dest, *(__u64 *)src1,
						 pos, nbits, test);
				break;
			case ARGMODE_BBB:
				if (num_words)
					ret = ((f_rbbb_test_t)func->func)
						(dest, src1, src2, pos,
						 nbits, test, num_words);
				else
					ret = ((f_bbb_test_t)func->func)
						(dest, src1, src2, pos,
						 nbits, test);
				break;
			case ARGMODE_BBBW:
				if (num_words)
					ret = ((f_rbbbw_test_t)func->func)
						(dest, *(__u64 *)src1, src2,
						 pos, nbits, test, num_words);
				else
					ret = ((f_bbbw_test_t)func->func)
						(dest, *(__u64 *)src1, src2,
						 pos, nbits, test);
				break;
			}
		} else if (func->flags & FLAG_CMP) {
			switch (func->argmode) {
			case ARGMODE_B:
				if (num_words)
					ret = ((f_rcmp_b_t)func->func)
						(dest, pos, nbits, num_words);
				else
					ret = ((f_cmp_b_t)func->func)
						(dest, pos, nbits);
				break;
			case ARGMODE_BB:
				if (num_words)
					ret = ((f_rcmp_bb_t)func->func)
						(dest, src1, pos, nbits,
						 num_words);
				else
					ret = ((f_cmp_bb_t)func->func)
						(dest, src1, pos, nbits);
				break;
			case ARGMODE_BBW:
				if (num_words)
					ret = ((f_rcmp_bbw_t)func->func)
						(dest, *(__u64 *)src1, pos,
						 nbits, num_words);
				else
					ret = ((f_cmp_bbw_t)func->func)
						(dest, *(__u64 *)src1, pos,
						 nbits);
				break;
			}
		} else if (func->flags & FLAG_SET) {
			switch (func->argmode) {
			case ARGMODE_B:
				if (num_words)
					((f_rset_b_t)func->func)
						(dest, pos, nbits, num_words);
				else
					((f_set_b_t)func->func)
						(dest, pos, nbits);
				break;
			}
		} else {
			switch (func->argmode) {
			case ARGMODE_B:
				if (num_words)
					((f_rset_b_t)func->func)
						(dest, pos, nbits, num_words);
				else
					((f_set_b_t)func->func)
						(dest, pos, nbits);
				break;
			case ARGMODE_BB:
				if (num_words)
					((f_rbb_t)func->func)
						(dest, src1, pos, nbits,
						 num_words);
				else
					((f_bb_t)func->func)
						(dest, src1, pos, nbits);
				break;
			case ARGMODE_BBW:
				if (num_words)
					((f_rbbw_t)func->func)
						(dest, *(__u64 *)src1, pos,
						 nbits, num_words);
				else
					((f_bbw_t)func->func)
						(dest, *(__u64 *)src1, pos,
						 nbits);
				break;
			case ARGMODE_BBB:
				if (num_words)
					((f_rbbb_t)func->func)
						(dest, src1, src2, pos,
						 nbits, num_words);
				else
					((f_bbb_t)func->func)
						(dest, src1, src2, pos, nbits);

				break;
			case ARGMODE_BBBW:
				if (num_words)
					((f_rbbbw_t)func->func)
						(dest, *(__u64 *)src1, src2,
						 pos, nbits, num_words);
				else
					((f_bbbw_t)func->func)
						(dest, *(__u64 *)src1, src2,
						 pos, nbits);

				break;
			}
		}
	}
	return ret;
}

static void init_test_block(struct test_block *block)
{
	init_test_data(block->data32, ARRAY_SIZE(block->data32));
}

static void print_info(struct bitmap_func *func,
	unsigned int dest_pos, unsigned int src1_pos,
	unsigned int src2_pos, unsigned int nbits, unsigned int test)
{
	printf("function=%s%s:%u, src1_pos=%u, src2_pos=%u, "
	       "dest_pos=%u, nbits=%u, real_nbits=%u\n",
	       func->name, tests[test], func->num_word_bits, src1_pos,
	       src2_pos, dest_pos, nbits, nbits - dest_pos);
}

static void do_test(struct bitmap_func *ifunc, unsigned long count,
		    unsigned int intv)
{
	struct test_block src1_map, src2_map, dest_map, verify_map;
	unsigned int dest_pos, src1_pos, src2_pos;
	unsigned int nbits, ret, retv, test;
	struct bitmap_func *func;
	unsigned long i;

	init_test_block(&src1_map);
	init_test_block(&src2_map);
	init_test_block(&dest_map);

	for (i = 0; i < count; i++) {
		if (!(i % intv) && i)
			printf("I: %lu\n", i);

		func = ifunc ? ifunc :
			&bitmap_funcs[random() % ARRAY_SIZE(bitmap_funcs)];

		if (func->flags & FLAG_READ_WRITE) {
			/* Assumed MAX_NBITS is at least 64 */
			nbits = 64;
		} else if (func->flags & FLAG_FIND_NEXT) {
			nbits = random() % ((MAX_NBITS) - 1) + 1;
		} else {
			nbits = random() % (MAX_NBITS);
		}

		dest_pos = random() % ((MAX_NBITS) - nbits);
		src1_pos = random() % ((MAX_NBITS) - nbits);
		src2_pos = random() % ((MAX_NBITS) - nbits);
		nbits += dest_pos;

		switch (func->test_op) {
		case TEST_SHIFT_LEFT:
		case TEST_SHIFT_RIGHT:
		case TEST_ROTATE_LEFT:
		case TEST_ROTATE_RIGHT:
			/* Shift and rotate require number of bytes
			 * divisible by number of word bits
			 */
			nbits = xdp2_round_up(nbits, func->num_word_bits ?
						: XDP2_BITMAP_BITS_PER_WORD);
			break;
		default:
			break;
		}

		if (func->flags & FLAG_TEST)
			test = (random() % XDP2_BITMAP_TEST_MAX) + 1;
		else
			test = 0;

		if (verbose) {
			printf("Number %lu: ", i);
			print_info(func, dest_pos, src1_pos, src2_pos,
				   nbits, test);
		}

		memcpy(&verify_map, &dest_map, sizeof(verify_map));

		ret = run_one_bitmap(func, &dest_map, dest_pos,
				  &src1_map, src1_pos,
				  &src2_map, src2_pos,
				  nbits, test);

		if (func->flags & FLAG_FIND_NEXT)
			dest_pos++;

		if (func->argmode == ARGMODE_B) {
			memcpy(&src1_map, &verify_map, sizeof(src1_map));

			retv = test_op(func,
				       (__u8 *)&verify_map, dest_pos,
				       (__u8 *)&src1_map, dest_pos,
				       (__u8 *)&verify_map, dest_pos,
					nbits, test);
		} else if (func->flags & FLAG_GEN) {
			if (func->flags & FLAG_DESTSRC)
				retv = test_op(func,
					       (__u8 *)&verify_map, dest_pos,
					       (__u8 *)&src1_map, src1_pos,
					       (__u8 *)&verify_map, dest_pos,
					       nbits, test);
			else
				retv = test_op(func,
					       (__u8 *)&verify_map, dest_pos,
					       (__u8 *)&src1_map, src1_pos,
					       (__u8 *)&src2_map, src2_pos,
					       nbits, test);
		} else {
			unsigned int pos = dest_pos;

			if (func->flags & FLAG_DESTSRC)
				retv = test_op(func,
					       (__u8 *)&verify_map, pos,
					       (__u8 *)&src1_map, pos,
					       (__u8 *)&verify_map, pos,
					       nbits, test);
			else
				retv = test_op(func,
					       (__u8 *)&verify_map, pos,
					       (__u8 *)&src1_map, pos,
					       (__u8 *)&src2_map, pos,
					       nbits, test);
		}

		if (memcmp(&verify_map, &dest_map, sizeof(verify_map))) {
			printf("Dest map is different ");
			printf("Number %lu: ", i);
			print_info(func, dest_pos, src1_pos, src2_pos,
				   nbits, test);
			print_map("Source", &src1_map, 8 * sizeof(verify_map));
			print_map("Source2", &src2_map, 8 * sizeof(verify_map));
			print_map("Dest", &dest_map, 8 * sizeof(verify_map));
			print_map("Verify", &verify_map,
				  8 * sizeof(verify_map));
			exit(-1);
		}

		if ((func->flags & (FLAG_TEST|FLAG_CMP)) && ret != retv) {
			printf("Return mismatch %u and %u\n", ret, retv);

			print_info(func, dest_pos, src1_pos, src2_pos,
				   nbits, test);
			exit(-1);
		}

		if (verbose)
			printf("\tReturn values %u and %u\n", ret, retv);

		if (verbose && func->flags & FLAG_TEST) {
			switch (test) {
			case XDP2_BITMAP_TEST_NONE:
			default:
				printf("\tValuess %u and %u\n", ret, retv);
				break;
			case XDP2_BITMAP_TEST_WEIGHT:
				printf("\tWeights %u and %u\n", ret, retv);
				break;
			case XDP2_BITMAP_TEST_ANY_SET:
				printf("\tAny set %u and %u\n", ret, retv);
				break;
			case XDP2_BITMAP_TEST_ANY_ZERO:
				printf("\tAny zero %u and %u\n", ret, retv);
				break;
			}
		}
	}
}

#define ARGS "c:v:I:C:RF:l"

static void usage(char *prog)
{
	fprintf(stderr, "Usage: %s [ -c <test-count> ] [ -v <verbose> ]\n",
		prog);
	fprintf(stderr, "\t[ -I <report-interval> ][ -C <cli_port_num> ]\n");
	fprintf(stderr, "\t[-R] [ -F <function> ] [-l]\n");

	exit(-1);
}

int main(int argc, char *argv[])
{
	static struct xdp2_cli_thread_info cli_thread_info;
	struct bitmap_func *func = NULL;
	unsigned int cli_port_num = 0;
	unsigned long count = 1000;
	bool random_seed = false;
	unsigned int intv = -1U;
	char *func_name = NULL;
	bool do_list = false;
	int c;

	while ((c = getopt(argc, argv, ARGS)) != -1) {
		switch (c) {
		case 'c':
			count = strtoull(optarg, NULL, 10);
			break;
		case 'v':
			verbose = strtoul(optarg, NULL, 10);
			break;
		case 'I':
			intv = strtoul(optarg, NULL, 10);
			break;
		case 'C':
			cli_port_num = strtoul(optarg, NULL, 10);
			break;
		case 'R':
			random_seed = true;
			break;
		case 'F':
			func_name = optarg;
			break;
		case 'l':
			do_list = true;
			break;
		default:
			usage(argv[0]);
		}
	}

	if (random_seed)
		srand(time(NULL));

	if (do_list) {
		list_funcs(bitmap_funcs, ARRAY_SIZE(bitmap_funcs));
		exit(0);
	}

	if (func_name) {
		func = lookup_bitmap_func(func_name, bitmap_funcs,
					  ARRAY_SIZE(bitmap_funcs));
		if (!func) {
			fprintf(stderr, "Function %s not found\n", func_name);
			exit(-1);
		}
	}

	if (cli_port_num) {
		cli_thread_info.port_num = cli_port_num;
		cli_thread_info.classes = -1U;
		cli_thread_info.label = "bitmap_test";
		cli_thread_info.major_num = 0xff;
		cli_thread_info.minor_num = 0xff;

		xdp2_cli_start(&cli_thread_info);
	}

	do_test(func, count, intv);
}
