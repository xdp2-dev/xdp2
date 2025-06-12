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

#ifndef __TEST_BITMAP_H__
#define __TEST_BITMAP_H__

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_NBITS	256

/* Functions and definitions for generic bitmap test infrastructure */

/* Test flags */
#define FLAG_TEST	(1 << 0)
#define FLAG_GEN	(1 << 1)
#define FLAG_DESTSRC	(1 << 2)
#define FLAG_1SRC	(1 << 3)
#define FLAG_CMP	(1 << 4)
#define FLAG_FIND_NEXT	(1 << 5)
#define FLAG_SET	(1 << 6)
#define FLAG_P2		(1 << 7)
#define FLAG_READ_WRITE	(1 << 8)
#define FLAG_SUBMDM	(1 << 9)
#define FLAG_SHROT	(1 << 10)
#define FLAG_BSWAP	(1 << 11)
#define FLAG_REV	(1 << 12)
#define FLAG_ROLL	(1 << 13)
#define FLAG_WORD_SRC	(1 << 14)

struct bitmap_func {
	void *func;
	char *name;
	unsigned int test_op;
	unsigned int argmode;
	unsigned int flags;
	unsigned int num_word_bits;
};

static inline struct bitmap_func *lookup_bitmap_func(char *name,
					      struct bitmap_func *funcs,
					      unsigned int num)
{
	int i;

	for (i = 0; i < num; i++)
		if (!strcmp(name, funcs[i].name))
			return &funcs[i];
	return NULL;
}

static inline void list_funcs(struct bitmap_func *funcs,
			      unsigned int num)
{
	int i;

	for (i = 0; i < num; i++)
		printf("%s\n", funcs[i].name);
}

static inline void init_test_data(__u32 *data, unsigned int numwords)
{
	int i;

	for (i = 0; i < numwords; i++)
		data[i] = random();
}

enum test_variant {
	TEST_AND = 0,
	TEST_OR,
	TEST_XOR,
	TEST_NAND,
	TEST_NOR,
	TEST_NXOR,
	TEST_AND_NOT,
	TEST_OR_NOT,
	TEST_XOR_NOT,
	TEST_NAND_NOT,
	TEST_NOR_NOT,
	TEST_NXOR_NOT,
	TEST_COPY,
	TEST_NOT,
	TEST_CMP,
	TEST_FIRST_AND,
	TEST_FIRST_OR,
	TEST_FIRST_AND_ZERO,
	TEST_FIRST_OR_ZERO,
	TEST_SHIFT_LEFT,
	TEST_SHIFT_RIGHT,
	TEST_ROTATE_LEFT,
	TEST_ROTATE_RIGHT,
	TEST_READ8,
	TEST_READ16,
	TEST_READ32,
	TEST_READ64,
	TEST_WRITE8,
	TEST_WRITE16,
	TEST_WRITE32,
	TEST_WRITE64,
};

/* Perform one test operation on two bits */
static inline bool test_one(enum test_variant which, bool src1, bool src2,
			    bool *found)
{
	switch (which) {
	case TEST_AND:
		return (src1 & src2);
	case TEST_OR:
		return (src1 | src2);
	case TEST_XOR:
		return (src1 ^ src2);
	case TEST_NAND:
		return !(src1 & src2);
	case TEST_NOR:
		return !(src1 | src2);
	case TEST_NXOR:
		return !(src1 ^ src2);
	case TEST_AND_NOT:
		return (src1 & !src2);
	case TEST_OR_NOT:
		return (src1 | !src2);
	case TEST_XOR_NOT:
		return (src1 ^ !src2);
	case TEST_NAND_NOT:
		return !(src1 & !src2);
	case TEST_NOR_NOT:
		return !(src1 | !src2);
	case TEST_NXOR_NOT:
		return !(src1 ^ !src2);
	case TEST_COPY:
		return src1;
	case TEST_NOT:
		return !src1;
	case TEST_CMP:
		if (src1 != src2) {
			*found = true;
			return 1;
		}
		return 0;
	case TEST_FIRST_AND:
		if (src1 & src2) {
			*found = true;
			return 1;
		}
		return 0;
	case TEST_FIRST_OR:
		if (src1 | src2) {
			*found = true;
			return 1;
		}
		return 0;
	case TEST_FIRST_AND_ZERO:
		if (!(src1 | src2)) {
			*found = true;
			return 1;
		}
		return 0;
	case TEST_FIRST_OR_ZERO:
		if (!(src1 & src2)) {
			*found = true;
			return 1;
		}
		return 0;
	default:
		fprintf(stderr, "Invalid test variant %u\n", which);
		exit(-1);
	}
}

static unsigned int get_index(unsigned int pos, unsigned int num_word_bits,
			      unsigned int num_words)
{
	unsigned int index = pos / num_word_bits;

	if (num_words)
		index = (num_words - 1) - index;

	return index;
}

/* Set a bit */
static inline void test_bit_set(void *addr, unsigned int pos,
				struct bitmap_func *func)
{
	unsigned int num_word_bits = func->num_word_bits;
	bool bswap = !!(func->flags &  FLAG_BSWAP);
	unsigned int index, num_words;

	if (!num_word_bits)
		num_word_bits = XDP2_BITMAP_BITS_PER_WORD;

	num_words = func->flags & FLAG_REV ?
					MAX_NBITS / num_word_bits: 0;

	index = get_index(pos, num_word_bits, num_words);

	switch (num_word_bits) {
	case 64: {
		__u64 data = 1ULL << (pos % 64);

		if (bswap)
			data = bswap_64(data);

		((__u64 *)addr)[index] |= data;
		break;
	}
	case 32: {
		__u32 data = 1ULL << (pos % 32);

		if (bswap)
			data = bswap_32(data);

		((__u32 *)addr)[index] |= data;
		break;
	}
	case 16: {
		__u16 data = 1ULL << (pos % 16);

		if (bswap)
			data = bswap_16(data);

		((__u16 *)addr)[index] |= data;
		break;
	}
	case 8: {
		__u8 data = 1ULL << (pos % 8);

		((__u8 *)addr)[index] |= data;
		break;
	}
	default:
		XDP2_ERR(1, "Bad num_word_bits %u", num_word_bits);
	}
}

/* Unset a bit */
static inline void test_bit_unset(void *addr, unsigned int pos,
				  struct bitmap_func *func)
{
	unsigned int num_word_bits = func->num_word_bits;
	bool bswap = !!(func->flags &  FLAG_BSWAP);
	unsigned int index, num_words;

	if (!num_word_bits)
		num_word_bits = XDP2_BITMAP_BITS_PER_WORD;

	num_words = func->flags & FLAG_REV ?
					MAX_NBITS / num_word_bits: 0;

	index = get_index(pos, num_word_bits, num_words);

	switch (num_word_bits) {
	case 64: {
		__u64 data = ~(1ULL << (pos % 64));

		if (bswap)
			data = bswap_64(data);

		((__u64 *)addr)[index] &= data;
		break;
	}
	case 32: {
		__u32 data = ~(1ULL << (pos % 32));

		if (bswap)
			data = bswap_32(data);

		((__u32 *)addr)[index] &= data;
		break;
	}
	case 16: {
		__u16 data = ~(1ULL << (pos % 16));

		if (bswap)
			data = bswap_16(data);

		((__u16 *)addr)[index] &= data;
		break;
	}
	case 8: {
		__u8 data = ~(1ULL << (pos % 8));

		((__u8 *)addr)[index] &= data;
		break;
	}
	default:
		XDP2_ERR(1, "Bad num_word_bits %u", num_word_bits);
	}

}

/* Set a bit value (1 or 0) */
static inline void test_bit_set_val(void *addr, unsigned int pos, bool val,
				    struct bitmap_func *func)
{
	if (val)
		test_bit_set(addr, pos, func);
	else
		test_bit_unset(addr, pos, func);
}

/* Check if a bit is set */
static inline bool test_bit_isset(const void *addr, unsigned int pos,
				  struct bitmap_func *func)
{
	unsigned int num_word_bits = func->num_word_bits;
	bool bswap = !!(func->flags &  FLAG_BSWAP);
	unsigned int index, num_words;

	if (!num_word_bits)
		num_word_bits = XDP2_BITMAP_BITS_PER_WORD;

	num_words = func->flags & FLAG_REV ?
					MAX_NBITS / num_word_bits: 0;

	index = get_index(pos, num_word_bits, num_words);

	switch (num_word_bits) {
	case 64: {
		__u64 data = 1ULL << (pos % 64);

		if (bswap)
			data = bswap_64(data);

		return !!(((__u64 *)addr)[index] & data);
	}
	case 32: {
		__u32 data = 1ULL << (pos % 32);

		if (bswap)
			data = bswap_32(data);

		return !!(((__u32 *)addr)[index] & data);
	}
	case 16: {
		__u16 data = 1ULL << (pos % 16);

		if (bswap)
			data = bswap_16(data);

		return !!(((__u16 *)addr)[index] & data);
	}
	case 8: {
		__u8 data = 1ULL << (pos % 8);

		return !!(((__u8 *)addr)[index] & data);
	}
	default:
		XDP2_ERR(1, "Bad num_word_bits %u", num_word_bits);
	}
}

static char *tests[] = {
	"",
	" test-weight",
	" test-set",
	" test-zero",
	" test-first",
};

#endif /* __TEST_BITMAP_H__ */
