// SPDX-License-Identifier: BSD-2-Clause OR GPL-2.0-or-later

/* Copyright (C) 2016 Jason A. Donenfeld <Jason@zx2c4.com>. All Rights Reserved.
 *
 * This file is provided under a dual BSD/GPLv2 license.
 *
 * SipHash: a fast short-input PRF
 * https://131002.net/siphash/
 *
 * This implementation is specifically for SipHash2-4 for a secure PRF
 * and HalfSipHash1-3/SipHash1-3 for an insecure PRF only suitable for
 * hashtables.
 */

/* Adapted from kernel lib/siphash.c */

#include <linux/types.h>

#include "siphash/siphash.h"

/* Rotate functions */

#ifdef XXX_UNUSED
static inline __u64 rol64(__u64 word, unsigned int shift)
{
	return (word << (shift & 63)) | (word >> ((-shift) & 63));
}
#endif

static inline __u32 rol32(__u32 word, unsigned int shift)
{
	return (word << (shift & 31)) | (word >> ((-shift) & 31));
}

/* Unaligned access */

static inline __u16 get_unaligned_le16(const void *data)
{
	const __u8 *p = (const __u8 *)data;

	return p[0] | p[1] << 8;
}

static inline __u32 get_unaligned_le32(const void *data)
{
	const __u8 *p = (const __u8 *)data;

	return p[0] | p[1] << 8 | p[2] << 16 | p[3] << 24;
}

static inline __u64 get_unaligned_le64(const void *data)
{
	const __u8 *p = (const __u8 *)data;

	return (__u64)get_unaligned_le32(p + 4) << 32 |
		get_unaligned_le32(p);
}

#define PREAMBLE(len) \
	__u64 v0 = 0x736f6d6570736575ULL; \
	__u64 v1 = 0x646f72616e646f6dULL; \
	__u64 v2 = 0x6c7967656e657261ULL; \
	__u64 v3 = 0x7465646279746573ULL; \
	__u64 b = ((__u64)(len)) << 56; \
	v3 ^= key->key[1]; \
	v2 ^= key->key[0]; \
	v1 ^= key->key[1]; \
	v0 ^= key->key[0];

__u64 __siphash_aligned(const void *data, size_t len, const siphash_key_t *key)
{
	const __u8 *end = data + len - (len % sizeof(__u64));
	const __u8 left = len & (sizeof(__u64) - 1);
	__u64 m;
	PREAMBLE(len)

	for (; data != end; data += sizeof(__u64)) {
		m = __le64_to_cpup(data);
		v3 ^= m;
		SIPHASH_SIPROUND(v0, v1, v2, v3);
#ifdef SIPHASH_KERNEL_METHOD
		SIPHASH_SIPROUND(v0, v1, v2, v3);
#endif
		v0 ^= m;
	}

#if defined(CONFIG_DCACHE_WORD_ACCESS) && BITS_PER_LONG == 64
	if (left)
		b |= le64_to_cpu((__force __le64)(load_unaligned_zeropad(data) &
						  bytemask_from_count(left)));
#else
	switch (left) {
	case 7:
		b |= ((__u64)end[6]) << 48;
		/* fall through */
	case 6:
		b |= ((__u64)end[5]) << 40;
		/* fall through */
	case 5:
		b |= ((__u64)end[4]) << 32;
		/* fall through */
	case 4:
		b |= __le32_to_cpup(data);
		break;
	case 3:
		b |= ((__u64)end[2]) << 16;
		/* fall through */
	case 2:
		b |= __le16_to_cpup(data);
		break;
	case 1:
		b |= end[0];
		break;
	}
#endif
	SIPHASH_POSTAMBLE(v0, v1, v2, v3, b);
}

#ifndef CONFIG_HAVE_EFFICIENT_UNALIGNED_ACCESS
__u64 __siphash_unaligned(const void *data, size_t len,
			  const siphash_key_t *key)
{
	const __u8 *end = data + len - (len % sizeof(__u64));
	const __u8 left = len & (sizeof(__u64) - 1);
	__u64 m;
	PREAMBLE(len)

	for (; data != end; data += sizeof(__u64)) {
		m = get_unaligned_le64(data);
		v3 ^= m;
		SIPHASH_SIPROUND(v0, v1, v2, v3);
#ifdef SIPHASH_KERNEL_METHOD
		SIPHASH_SIPROUND(v0, v1, v2, v3);
#endif
		v0 ^= m;
	}
#if defined(CONFIG_DCACHE_WORD_ACCESS) && BITS_PER_LONG == 64
	if (left)
		b |= le64_to_cpu((__force __le64)(load_unaligned_zeropad(data) &
						  bytemask_from_count(left)));
#else
	switch (left) {
	case 7:
		b |= ((__u64)end[6]) << 48;
		/* fall through */
	case 6:
		b |= ((__u64)end[5]) << 40;
		/* fall through */
	case 5:
		b |= ((__u64)end[4]) << 32;
		/* fall through */
	case 4:
		b |= get_unaligned_le32(end);
		break;
	case 3:
		b |= ((__u64)end[2]) << 16;
		/* fall through */
	case 2:
		b |= get_unaligned_le16(end);
		break;
	case 1:
		b |= end[0];
		break;
	}
#endif
	SIPHASH_POSTAMBLE(v0, v1, v2, v3, b);
}
#endif

/**
 * siphash_1u64 - compute 64-bit siphash PRF value of a u64
 * @first: first u64
 * @key: the siphash key
 */
__u64 siphash_1u64(const __u64 first, const siphash_key_t *key)
{
	PREAMBLE(8)

	v3 ^= first;
	SIPHASH_SIPROUND(v0, v1, v2, v3);
#ifdef SIPHASH_KERNEL_METHOD
	SIPHASH_SIPROUND(v0, v1, v2, v3);
#endif
	v0 ^= first;
	SIPHASH_POSTAMBLE(v0, v1, v2, v3, b);
}

/**
 * siphash_2u64 - compute 64-bit siphash PRF value of 2 u64
 * @first: first u64
 * @second: second u64
 * @key: the siphash key
 */
__u64 siphash_2u64(const __u64 first, const __u64 second,
		   const siphash_key_t *key)
{
	PREAMBLE(16)

	v3 ^= first;
	SIPHASH_SIPROUND(v0, v1, v2, v3);
#ifdef SIPHASH_KERNEL_METHOD
	SIPHASH_SIPROUND(v0, v1, v2, v3);
#endif
	v0 ^= first;
	v3 ^= second;
	SIPHASH_SIPROUND(v0, v1, v2, v3);
#ifdef SIPHASH_KERNEL_METHOD
	SIPHASH_SIPROUND(v0, v1, v2, v3);
#endif
	v0 ^= second;
	SIPHASH_POSTAMBLE(v0, v1, v2, v3, b);
}

/**
 * siphash_3u64 - compute 64-bit siphash PRF value of 3 u64
 * @first: first u64
 * @second: second u64
 * @third: third u64
 * @key: the siphash key
 */
__u64 siphash_3u64(const __u64 first, const __u64 second, const __u64 third,
		   const siphash_key_t *key)
{
	PREAMBLE(24)

	v3 ^= first;
	SIPHASH_SIPROUND(v0, v1, v2, v3);
#ifdef SIPHASH_KERNEL_METHOD
	SIPHASH_SIPROUND(v0, v1, v2, v3);
#endif
	v0 ^= first;
	v3 ^= second;
	SIPHASH_SIPROUND(v0, v1, v2, v3);
#ifdef SIPHASH_KERNEL_METHOD
	SIPHASH_SIPROUND(v0, v1, v2, v3);
#endif
	v0 ^= second;
	v3 ^= third;
	SIPHASH_SIPROUND(v0, v1, v2, v3);
#ifdef SIPHASH_KERNEL_METHOD
	SIPHASH_SIPROUND(v0, v1, v2, v3);
#endif
	v0 ^= third;
	SIPHASH_POSTAMBLE(v0, v1, v2, v3, b);
}

/**
 * siphash_4u64 - compute 64-bit siphash PRF value of 4 u64
 * @first: first u64
 * @second: second u64
 * @third: third u64
 * @forth: forth u64
 * @key: the siphash key
 */
__u64 siphash_4u64(const __u64 first, const __u64 second, const __u64 third,
		   const __u64 forth, const siphash_key_t *key)
{
	PREAMBLE(32)

	v3 ^= first;
	SIPHASH_SIPROUND(v0, v1, v2, v3);
#ifdef SIPHASH_KERNEL_METHOD
	SIPHASH_SIPROUND(v0, v1, v2, v3);
#endif
	v0 ^= first;
	v3 ^= second;
	SIPHASH_SIPROUND(v0, v1, v2, v3);
#ifdef SIPHASH_KERNEL_METHOD
	SIPHASH_SIPROUND(v0, v1, v2, v3);
#endif
	v0 ^= second;
	v3 ^= third;
	SIPHASH_SIPROUND(v0, v1, v2, v3);
#ifdef SIPHASH_KERNEL_METHOD
	SIPHASH_SIPROUND(v0, v1, v2, v3);
#endif
	v0 ^= third;
	v3 ^= forth;
	SIPHASH_SIPROUND(v0, v1, v2, v3);
#ifdef SIPHASH_KERNEL_METHOD
	SIPHASH_SIPROUND(v0, v1, v2, v3);
#endif
	v0 ^= forth;
	SIPHASH_POSTAMBLE(v0, v1, v2, v3, b);
}

#ifdef SIPHASH_KERNEL_METHOD
#define MAKE_ROUND(DATA) do {					\
	v3 ^= DATA;						\
	SIPHASH_SIPROUND(v0, v1, v2, v3);			\
	SIPHASH_SIPROUND(v0, v1, v2, v3);			\
	v0 ^= DATA;						\
} while (0)
#else
#define MAKE_ROUND(DATA) do {					\
	v3 ^= DATA;						\
	SIPHASH_SIPROUND(v0, v1, v2, v3);			\
	v0 ^= DATA;						\
} while (0)
#endif

__u64 siphash_16u64(void *data, const siphash_key_t *key)
{
	__u64 *data64 = (__u64 *)data;

	PREAMBLE(128)

	MAKE_ROUND(data64[0]);
	MAKE_ROUND(data64[1]);
	MAKE_ROUND(data64[2]);
	MAKE_ROUND(data64[3]);
	MAKE_ROUND(data64[4]);
	MAKE_ROUND(data64[5]);
	MAKE_ROUND(data64[6]);
	MAKE_ROUND(data64[7]);
	MAKE_ROUND(data64[8]);
	MAKE_ROUND(data64[9]);
	MAKE_ROUND(data64[10]);
	MAKE_ROUND(data64[11]);
	MAKE_ROUND(data64[12]);
	MAKE_ROUND(data64[13]);
	MAKE_ROUND(data64[14]);
	MAKE_ROUND(data64[15]);

	SIPHASH_POSTAMBLE(v0, v1, v2, v3, b);
}

__u64 siphash_1u32(const __u32 first, const siphash_key_t *key)
{
	PREAMBLE(4)

	b |= first;
	SIPHASH_POSTAMBLE(v0, v1, v2, v3, b);
}

__u64 siphash_3u32(const __u32 first, const __u32 second, const __u32 third,
		   const siphash_key_t *key)
{
	__u64 combined = (__u64)second << 32 | first;
	PREAMBLE(12)

	v3 ^= combined;
	SIPHASH_SIPROUND(v0, v1, v2, v3);
#ifdef SIPHASH_KERNEL_METHOD
	SIPHASH_SIPROUND(v0, v1, v2, v3);
#endif
	v0 ^= combined;
	b |= third;
	SIPHASH_POSTAMBLE(v0, v1, v2, v3, b);
}

#if BITS_PER_LONG == 64
/* Note that on 64-bit, we make HalfSipHash1-3 actually be SipHash1-3, for
 * performance reasons. On 32-bit, below, we actually implement HalfSipHash1-3.
 */

#define HSIPROUND SIPHASH_SIPROUND(v0, v1, v2, v3)
#define HPREAMBLE(len) PREAMBLE(len)
#define HPOSTAMBLE \
	v3 ^= b; \
	HSIPROUND; \
	v0 ^= b; \
	v2 ^= 0xff; \
	HSIPROUND; \
	HSIPROUND; \
	HSIPROUND; \
	return (v0 ^ v1) ^ (v2 ^ v3);

__u32 __hsiphash_aligned(const void *data, size_t len,
			 const hsiphash_key_t *key)
{
	const __u8 *end = data + len - (len % sizeof(__u64));
	const __u8 left = len & (sizeof(__u64) - 1);
	__u64 m;
	HPREAMBLE(len)

	for (; data != end; data += sizeof(__u64)) {
		m = le64_to_cpup(data);
		v3 ^= m;
		HSIPROUND;
		v0 ^= m;
	}
#if defined(CONFIG_DCACHE_WORD_ACCESS) && BITS_PER_LONG == 64
	if (left)
		b |= le64_to_cpu((__force __le64)(load_unaligned_zeropad(data) &
						  bytemask_from_count(left)));
#else
	switch (left) {
	case 7:
		b |= ((__u64)end[6]) << 48;
		/* fall through */
	case 6:
		b |= ((__u64)end[5]) << 40;
		/* fall through */
	case 5:
		b |= ((__u64)end[4]) << 32;
		/* fall through */
	case 4:
		b |= le32_to_cpup(data);
		break;
	case 3:
		b |= ((__u64)end[2]) << 16;
		/* fall through */
	case 2:
		b |= le16_to_cpup(data);
		break;
	case 1:
		b |= end[0];
	}
#endif
	HPOSTAMBLE
}

#ifndef CONFIG_HAVE_EFFICIENT_UNALIGNED_ACCESS
__u32 __hsiphash_unaligned(const void *data, size_t len,
			   const hsiphash_key_t *key)
{
	const __u8 *end = data + len - (len % sizeof(__u64));
	const __u8 left = len & (sizeof(__u64) - 1);
	__u64 m;
	HPREAMBLE(len)

	for (; data != end; data += sizeof(__u64)) {
		m = get_unaligned_le64(data);
		v3 ^= m;
		HSIPROUND;
		v0 ^= m;
	}
#if defined(CONFIG_DCACHE_WORD_ACCESS) && BITS_PER_LONG == 64
	if (left)
		b |= le64_to_cpu((__force __le64)(load_unaligned_zeropad(data) &
						  bytemask_from_count(left)));
#else
	switch (left) {
	case 7:
		b |= ((__u64)end[6]) << 48;
		/* fall through */
	case 6:
		b |= ((__u64)end[5]) << 40;
		/* fall through */
	case 5:
		b |= ((__u64)end[4]) << 32;
		/* fall through */
	case 4:
		b |= get_unaligned_le32(end);
		break;
	case 3:
		b |= ((__u64)end[2]) << 16;
		/* fall through */
	case 2:
		b |= get_unaligned_le16(end);
		break;
	case 1:
		b |= end[0];
	}
#endif
	HPOSTAMBLE
}
#endif

/**
 * hsiphash_1u32 - compute 64-bit hsiphash PRF value of a u32
 * @first: first u32
 * @key: the hsiphash key
 */
__u32 hsiphash_1u32(const __u32 first, const hsiphash_key_t *key)
{
	HPREAMBLE(4)

	b |= first;
	HPOSTAMBLE
}

/**
 * hsiphash_2u32 - compute 32-bit hsiphash PRF value of 2 u32
 * @first: first u32
 * @second: second u32
 * @key: the hsiphash key
 */
__u32 hsiphash_2u32(const __u32 first, const __u32 second,
		    const hsiphash_key_t *key)
{
	__u64 combined = (__u64)second << 32 | first;
	HPREAMBLE(8)

	v3 ^= combined;
	HSIPROUND;
	v0 ^= combined;
	HPOSTAMBLE
}

/**
 * hsiphash_3u32 - compute 32-bit hsiphash PRF value of 3 u32
 * @first: first u32
 * @second: second u32
 * @third: third u32
 * @key: the hsiphash key
 */
__u32 hsiphash_3u32(const __u32 first, const __u32 second, const __u32 third,
		    const hsiphash_key_t *key)
{
	__u64 combined = (__u64)second << 32 | first;
	HPREAMBLE(12)

	v3 ^= combined;
	HSIPROUND;
	v0 ^= combined;
	b |= third;
	HPOSTAMBLE
}

/**
 * hsiphash_4u32 - compute 32-bit hsiphash PRF value of 4 u32
 * @first: first u32
 * @second: second u32
 * @third: third u32
 * @forth: forth u32
 * @key: the hsiphash key
 */
__u32 hsiphash_4u32(const __u32 first, const __u32 second, const __u32 third,
		    const __u32 forth, const hsiphash_key_t *key)
{
	__u64 combined = (__u64)second << 32 | first;
	HPREAMBLE(16)

	v3 ^= combined;
	HSIPROUND;
	v0 ^= combined;
	combined = (__u64)forth << 32 | third;
	v3 ^= combined;
	HSIPROUND;
	v0 ^= combined;
	HPOSTAMBLE
}
#else
#define HSIPROUND \
	do { \
		v0 += v1; v1 = rol32(v1, 5); v1 ^= v0; v0 = rol32(v0, 16); \
		v2 += v3; v3 = rol32(v3, 8); v3 ^= v2; \
		v0 += v3; v3 = rol32(v3, 7); v3 ^= v0; \
		v2 += v1; v1 = rol32(v1, 13); v1 ^= v2; v2 = rol32(v2, 16); \
	} while (0)

#define HPREAMBLE(len) \
	__u32 v0 = 0; \
	__u32 v1 = 0; \
	__u32 v2 = 0x6c796765U; \
	__u32 v3 = 0x74656462U; \
	__u32 b = ((__u32)(len)) << 24; \
	v3 ^= key->key[1]; \
	v2 ^= key->key[0]; \
	v1 ^= key->key[1]; \
	v0 ^= key->key[0];

#define HPOSTAMBLE \
	v3 ^= b; \
	HSIPROUND; \
	v0 ^= b; \
	v2 ^= 0xff; \
	HSIPROUND; \
	HSIPROUND; \
	HSIPROUND; \
	return v1 ^ v3;

__u32 __hsiphash_aligned(const void *data, size_t len,
			 const hsiphash_key_t *key)
{
	const __u8 *end = data + len - (len % sizeof(__u32));
	const __u8 left = len & (sizeof(__u32) - 1);
	__u32 m;
	HPREAMBLE(len)

	for (; data != end; data += sizeof(__u32)) {
		m = __le32_to_cpup(data);
		v3 ^= m;
		HSIPROUND;
		v0 ^= m;
	}
	switch (left) {
	case 3:
		b |= ((__u32)end[2]) << 16;
		/* fall through */
	case 2:
		b |= __le16_to_cpup(data);
		break;
	case 1:
		b |= end[0];
		break;
	}
	HPOSTAMBLE
}

#ifndef CONFIG_HAVE_EFFICIENT_UNALIGNED_ACCESS
__u32 __hsiphash_unaligned(const void *data, size_t len,
			 const hsiphash_key_t *key)
{
	const __u8 *end = data + len - (len % sizeof(__u32));
	const __u8 left = len & (sizeof(__u32) - 1);
	__u32 m;
	HPREAMBLE(len)

	for (; data != end; data += sizeof(__u32)) {
		m = get_unaligned_le32(data);
		v3 ^= m;
		HSIPROUND;
		v0 ^= m;
	}
	switch (left) {
	case 3:
		b |= ((__u32)end[2]) << 16;
		/* fall through */
	case 2:
		b |= get_unaligned_le16(end);
		break;
	case 1:
		b |= end[0];
		break;
	}
	HPOSTAMBLE
}
#endif

/**
 * hsiphash_1u32 - compute 32-bit hsiphash PRF value of a u32
 * @first: first u32
 * @key: the hsiphash key
 */
__u32 hsiphash_1u32(const __u32 first, const hsiphash_key_t *key)
{
	HPREAMBLE(4)

	v3 ^= first;
	HSIPROUND;
	v0 ^= first;
	HPOSTAMBLE
}

/**
 * hsiphash_2u32 - compute 32-bit hsiphash PRF value of 2 u32
 * @first: first u32
 * @second: second u32
 * @key: the hsiphash key
 */
__u32 hsiphash_2u32(const __u32 first, const __u32 second,
		    const hsiphash_key_t *key)
{
	HPREAMBLE(8)

	v3 ^= first;
	HSIPROUND;
	v0 ^= first;
	v3 ^= second;
	HSIPROUND;
	v0 ^= second;
	HPOSTAMBLE
}

/**
 * hsiphash_3u32 - compute 32-bit hsiphash PRF value of 3 u32
 * @first: first u32
 * @second: second u32
 * @third: third u32
 * @key: the hsiphash key
 */
__u32 hsiphash_3u32(const __u32 first, const __u32 second, const __u32 third,
		    const hsiphash_key_t *key)
{
	HPREAMBLE(12)

	v3 ^= first;
	HSIPROUND;
	v0 ^= first;
	v3 ^= second;
	HSIPROUND;
	v0 ^= second;
	v3 ^= third;
	HSIPROUND;
	v0 ^= third;
	HPOSTAMBLE
}

/**
 * hsiphash_4u32 - compute 32-bit hsiphash PRF value of 4 u32
 * @first: first u32
 * @second: second u32
 * @third: third u32
 * @forth: forth u32
 * @key: the hsiphash key
 */
__u32 hsiphash_4u32(const __u32 first, const __u32 second, const __u32 third,
		    const __u32 forth, const hsiphash_key_t *key)
{
	HPREAMBLE(16)

	v3 ^= first;
	HSIPROUND;
	v0 ^= first;
	v3 ^= second;
	HSIPROUND;
	v0 ^= second;
	v3 ^= third;
	HSIPROUND;
	v0 ^= third;
	v3 ^= forth;
	HSIPROUND;
	v0 ^= forth;
	HPOSTAMBLE
}
#endif

void siphash_iter_start(struct siphash_iter *iter, size_t len,
			const siphash_key_t *key)
{
	PREAMBLE(len);

	iter->v0 = v0;
	iter->v1 = v1;
	iter->v2 = v2;
	iter->v3 = v3;
	iter->b = b;

	iter->residual_length = 0;
}

__u64 siphash_iter_end(struct siphash_iter *iter)
{
	__u8 left = iter->residual_length;
	const __u8 *data = iter->residual;
	__u64 v0 = iter->v0;
	__u64 v1 = iter->v1;
	__u64 v2 = iter->v2;
	__u64 v3 = iter->v3;
	__u64 b = iter->b;


#if defined(CONFIG_DCACHE_WORD_ACCESS) && BITS_PER_LONG == 64
	if (left)
		b |= le64_to_cpu((__force __le64)(load_unaligned_zeropad(left) &
						  bytemask_from_count(left)));
#else
	switch (left) {
	case 7:
		b |= ((__u64)data[6]) << 48;
		/* fall through */
	case 6:
		b |= ((__u64)data[5]) << 40;
		/* fall through */
	case 5:
		b |= ((__u64)data[4]) << 32;
		/* fall through */
	case 4:
		b |= __le32_to_cpup((__u32 *)data);
		break;
	case 3:
		b |= ((__u64)data[2]) << 16;
		/* fall through */
	case 2:
		b |= __le16_to_cpup((__u16 *)data);
		break;
	case 1:
		b |= data[0];
	}
#endif
	SIPHASH_POSTAMBLE(v0, v1, v2, v3, b);
}

#define MIN(A, B) ((A) < (B) ? (A) : (B))

void siphash_iter(struct siphash_iter *iter, void *data, size_t len)
{
	__u64 v0 = iter->v0;
	__u64 v1 = iter->v1;
	__u64 v2 = iter->v2;
	__u64 v3 = iter->v3;
	const __u8 *end;
	size_t clen;
	__u8 left;
	__u64 m;
	int i;

	if (iter->residual_length) {
		clen = MIN(len, 8 - iter->residual_length);
		for (i = 0; i < clen; i++)
			iter->residual[iter->residual_length + i] =
							((__u8 *)data)[i];
		if (clen < 8 - iter->residual_length) {
			iter->residual_length += clen;
			return;
		}

		m = __le64_to_cpup((__u64 *)iter->residual);
		v3 ^= m;
		SIPHASH_SIPROUND(v0, v1, v2, v3);
#ifdef SIPHASH_KERNEL_METHOD
		SIPHASH_SIPROUND(v0, v1, v2, v3);
#endif
		v0 ^= m;

		data += clen;
		len -= clen;
		iter->residual_length = 0;
	}

	end = data + len - (len % sizeof(__u64));
	left = len & (sizeof(__u64) - 1);

	for (; data != end; data += sizeof(__u64)) {
		m = __le64_to_cpup(data);
		v3 ^= m;
		SIPHASH_SIPROUND(v0, v1, v2, v3);
#ifdef SIPHASH_KERNEL_METHOD
		SIPHASH_SIPROUND(v0, v1, v2, v3);
#endif
		v0 ^= m;
	}

	if (left) {
		for (i = 0; i < left; i++)
			iter->residual[i] = ((__u8 *)end)[i];
		iter->residual_length = left;
	}

	iter->v0 = v0;
	iter->v1 = v1;
	iter->v2 = v2;
	iter->v3 = v3;
}
