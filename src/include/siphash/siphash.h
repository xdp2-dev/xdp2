/* SPDX-License-Identifier: BSD-2-Clause OR GPL-2.0-or-later */

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

#ifndef __SIPHASH_H__
#define __SIPHASH_H__

/* Adapted from kernel include/linux/siphash.h */

#include <asm/byteorder.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <stdbool.h>
#include <stddef.h>

static inline __u64 siphash_rol64(__u64 word, unsigned int shift)
{
	return (word << (shift & 63)) | (word >> ((-shift) & 63));
}

static inline __u32 siphash_rol32(__u32 word, unsigned int shift)
{
	return (word << (shift & 31)) | (word >> ((-shift) & 31));
}


#ifndef IS_ALIGNED
#define IS_ALIGNED(x, a)	(((x) & ((typeof(x))(a) - 1)) == 0)
#endif

#define SIPHASH_ALIGNMENT __alignof__(__u64)
typedef struct {
	__u64 key[2];
} siphash_key_t;

static inline bool siphash_key_is_zero(const siphash_key_t *key)
{
	return !(key->key[0] | key->key[1]);
}

__u64 __siphash_aligned(const void *data, size_t len, const siphash_key_t *key);
#ifndef CONFIG_HAVE_EFFICIENT_UNALIGNED_ACCESS
__u64 __siphash_unaligned(const void *data, size_t len,
			  const siphash_key_t *key);
#endif

__u64 siphash_1u64(const __u64 a, const siphash_key_t *key);
__u64 siphash_2u64(const __u64 a, const __u64 b, const siphash_key_t *key);
__u64 siphash_3u64(const __u64 a, const __u64 b, const __u64 c,
		 const siphash_key_t *key);
__u64 siphash_4u64(const __u64 a, const __u64 b, const __u64 c,
		   const __u64 d, const siphash_key_t *key);
__u64 siphash_16u64(void *data, const siphash_key_t *key);
__u64 siphash_1u32(const __u32 a, const siphash_key_t *key);
__u64 siphash_3u32(const __u32 a, const __u32 b, const __u32 c,
		 const siphash_key_t *key);

static inline __u64 siphash_2u32(const __u32 a, const __u32 b,
			       const siphash_key_t *key)
{
	return siphash_1u64((__u64)b << 32 | a, key);
}
static inline __u64 siphash_4u32(const __u32 a, const __u32 b, const __u32 c,
			       const __u32 d, const siphash_key_t *key)
{
	return siphash_2u64((__u64)b << 32 | a, (__u64)d << 32 | c, key);
}


static inline __u64 ___siphash_aligned(const __le64 *data, size_t len,
				     const siphash_key_t *key)
{
	if (__builtin_constant_p(len) && len == 4)
		return siphash_1u32(__le32_to_cpup((const __le32 *)data), key);
	if (__builtin_constant_p(len) && len == 8)
		return siphash_1u64(__le64_to_cpu(data[0]), key);
	if (__builtin_constant_p(len) && len == 16)
		return siphash_2u64(__le64_to_cpu(data[0]),
				    __le64_to_cpu(data[1]),
				    key);
	if (__builtin_constant_p(len) && len == 24)
		return siphash_3u64(__le64_to_cpu(data[0]),
				    __le64_to_cpu(data[1]),
				    __le64_to_cpu(data[2]), key);
	if (__builtin_constant_p(len) && len == 32)
		return siphash_4u64(__le64_to_cpu(data[0]),
				    __le64_to_cpu(data[1]),
				    __le64_to_cpu(data[2]),
				    __le64_to_cpu(data[3]), key);
	return __siphash_aligned(data, len, key);
}

/**
 * siphash - compute 64-bit siphash PRF value
 * @data: buffer to hash
 * @size: size of @data
 * @key: the siphash key
 */
static inline __u64 siphash(const void *data, size_t len,
			  const siphash_key_t *key)
{
#ifndef CONFIG_HAVE_EFFICIENT_UNALIGNED_ACCESS
	if (!IS_ALIGNED((unsigned long)data, SIPHASH_ALIGNMENT))
		return __siphash_unaligned(data, len, key);
#endif
	return ___siphash_aligned(data, len, key);
}

#define HSIPHASH_ALIGNMENT __alignof__(unsigned long)
typedef struct {
	unsigned long key[2];
} hsiphash_key_t;

__u32 __hsiphash_aligned(const void *data, size_t len,
		       const hsiphash_key_t *key);
#ifndef CONFIG_HAVE_EFFICIENT_UNALIGNED_ACCESS
__u32 __hsiphash_unaligned(const void *data, size_t len,
			 const hsiphash_key_t *key);
#endif

__u32 hsiphash_1u32(const __u32 a, const hsiphash_key_t *key);
__u32 hsiphash_2u32(const __u32 a, const __u32 b, const hsiphash_key_t *key);
__u32 hsiphash_3u32(const __u32 a, const __u32 b, const __u32 c,
		  const hsiphash_key_t *key);
__u32 hsiphash_4u32(const __u32 a, const __u32 b, const __u32 c,
		    const __u32 d, const hsiphash_key_t *key);

static inline __u32 ___hsiphash_aligned(const __le32 *data, size_t len,
					const hsiphash_key_t *key)
{
	if (__builtin_constant_p(len) && len == 4)
		return hsiphash_1u32(__le32_to_cpu(data[0]), key);
	if (__builtin_constant_p(len) && len == 8)
		return hsiphash_2u32(__le32_to_cpu(data[0]),
				     __le32_to_cpu(data[1]), key);
	if (__builtin_constant_p(len) && len == 12)
		return hsiphash_3u32(__le32_to_cpu(data[0]),
				     __le32_to_cpu(data[1]),
				     __le32_to_cpu(data[2]), key);
	if (__builtin_constant_p(len) && len == 16)
		return hsiphash_4u32(__le32_to_cpu(data[0]),
				     __le32_to_cpu(data[1]),
				     __le32_to_cpu(data[2]),
				     __le32_to_cpu(data[3]), key);
	return __hsiphash_aligned(data, len, key);
}

/**
 * hsiphash - compute 32-bit hsiphash PRF value
 * @data: buffer to hash
 * @size: size of @data
 * @key: the hsiphash key
 */
static inline __u32 hsiphash(const void *data, size_t len,
			   const hsiphash_key_t *key)
{
#ifndef CONFIG_HAVE_EFFICIENT_UNALIGNED_ACCESS
	if (!IS_ALIGNED((unsigned long)data, HSIPHASH_ALIGNMENT))
		return __hsiphash_unaligned(data, len, key);
#endif
	return ___hsiphash_aligned(data, len, key);
}

/* SIPHASH iterator */

struct siphash_iter {
	__u64 v0;
	__u64 v1;
	__u64 v2;
	__u64 v3;
	__u64 b;

	__u8 residual[8];
	__u8 residual_length;
};

void siphash_iter_start(struct siphash_iter *iter, size_t len,
			const siphash_key_t *key);

__u64 siphash_iter_end(struct siphash_iter *iter);

void siphash_iter(struct siphash_iter *iter, void *data, size_t len);

#define SIPHASH_SIPROUND(V0, V1, V2, V3) do {				\
	(V0) += (V1);							\
	(V1) = siphash_rol64((V1), 13);					\
	(V1) ^= (V0);							\
	(V0) = siphash_rol64((V0), 32);					\
	(V2) += (V3);							\
	(V3) = siphash_rol64((V3), 16);					\
	(V3) ^= (V2);							\
	(V0) += (V3);							\
	(V3) = siphash_rol64((V3), 21);					\
	(V3) ^= (V0);							\
	(V2) += (V1);							\
	(V1) = siphash_rol64((V1), 17);					\
	(V1) ^= (V2);							\
	(V2) = siphash_rol64((V2), 32);					\
} while (0)

#ifdef SIPHASH_KERNEL_METHOD
#define SIPHASH_POSTAMBLE(V0, V1, V2, V3, B) do {			\
	(V3) ^= (B);							\
	SIPHASH_SIPROUND(V0, V1, V2, V3);				\
	SIPHASH_SIPROUND(V0, V1, V2, V3);				\
	(v0) ^= (B);							\
	(v2) ^= 0xff;							\
	SIPHASH_SIPROUND(V0, V1, V2, V3);				\
	SIPHASH_SIPROUND(V0, V1, V2, V3);				\
	SIPHASH_SIPROUND(V0, V1, V2, V3);				\
	SIPHASH_SIPROUND(V0, V1, V2, V3);				\
	return ((V0) ^ (V1)) ^ ((V2) ^ (V3));				\
} while (0)
#else
#define SIPHASH_POSTAMBLE(V0, V1, V2, V3, B) do {			\
	(V3) ^= (B);							\
	SIPHASH_SIPROUND(V0, V1, V2, V3);				\
	(v0) ^= (B);							\
	(v2) ^= 0xff;							\
	SIPHASH_SIPROUND(V0, V1, V2, V3);				\
	SIPHASH_SIPROUND(V0, V1, V2, V3);				\
	return ((V0) ^ (V1)) ^ ((V2) ^ (V3));				\
} while (0)
#endif

/* Start siphash backend
 *
 * The function takes up to thirty-two bytes of input from integer registers.
 *
 * If the specified length is <=32 then all of the input is present so the
 * siphash operation is completed in one shot and the hash is returned
 *
 * If the specified length is >32 then the function starts a siphash chain.
 * The siphash state is returned in args[2]-args[6] and subsequently
 * __siphash_round is called
 *
 * Input:
 *
 * args[0] is data0
 * args[1] is data1
 * args[2] is data2
 * args[3] is data3
 * args[4] is length
 * args[5] is key[0]
 * args[6] is key[1]
 *
 * Output:
 *
 * args[0] is hash value (if length <= 32)
 * args[1] is unused
 * args[2] is b (siphash state)
 * args[3] is v0 (siphash state)
 * args[4] is v1 (siphash state)
 * args[5] is v2 (siphash state)
 * args[6] is v3 (siphash state)
 */
static inline unsigned long __siphash_start(unsigned long *args)
{
	__u64 len = args[4];
	__u64 v0 = 0x736f6d6570736575ULL;
	__u64 v1 = 0x646f72616e646f6dULL;
	__u64 v2 = 0x6c7967656e657261ULL;
	__u64 v3 = 0x7465646279746573ULL;
	__u64 b = ((__u64)(len)) << 56;
	__u64 m;
	int i;

	v3 ^= args[6];
	v2 ^= args[5];
	v1 ^= args[6];
	v0 ^= args[5];

	for (i = 0; i < 4 && i < len / sizeof(__u64); i++) {
		m = __le64_to_cpu(args[i]);
		v3 ^= m;
		SIPHASH_SIPROUND(v0, v1, v2, v3);
#ifdef SIPHASH_KERNEL_METHOD
		SIPHASH_SIPROUND(v0, v1, v2, v3);
#endif
		v0 ^= m;
	}

	if (len <= 32) {
		if (len & 0x7)
			b |= __le64_to_cpu((__u64)args[i]);

		SIPHASH_POSTAMBLE(v0, v1, v2, v3, b); /* Returns */
	}

	args[2] = b;
	args[3] = v0;
	args[4] = v1;
	args[5] = v2;
	args[6] = v3;

	return 0; /* Unused */
}

/* Do siphash round
 *
 * The is called to hash over data after siphash_start was called to open a
 * chain
 *
 * If the end flag is not set then this is an intermediate round. The function
 * takes two values to hash from args[0] and args[1] and the state of the
 * siphash chain in args[2]-args[6]. The updated siphash state is returned in
 * args[2]-args[6] and subsequently __siphash_round is called
 *
 * If the end flag is set then the function closes a siphash chain. If one_arg
 * is set then the input data is eight bytes from args[0], if one_arg is not
 * set then the sixteen bytes of input data is taken from args[0] and args[1].
 * The final hash result is returned
 *
 * Input
 *
 * args[0] is data0
 * args[1] is data1
 * args[2] is b
 * args[3] is v0
 * args[4] is v1
 * args[5] is v2
 * args[6] is v3
 *
 * Output
 *
 * args[0] is hash value (end is true)
 * args[1] is unused
 * args[2] is b (siphash state)
 * args[3] is v0 (siphash state)
 * args[4] is v1 (siphash state)
 * args[5] is v2 (siphash state)
 * args[6] is v3 (siphash state)
 */
static inline unsigned long __siphash_round(unsigned long *args, bool one_arg,
					    bool end, bool partial)
{
	__u64 b = args[2];
	__u64 v0 = args[3];
	__u64 v1 = args[4];
	__u64 v2 = args[5];
	__u64 v3 = args[6];
	__u64 m;

	if (one_arg && partial && end) {
		b |= __le64_to_cpu((__u64)args[0]);
		SIPHASH_POSTAMBLE(v0, v1, v2, v3, b); /* Returns */
	}


	m = __le64_to_cpu((__u64)args[0]);
	v3 ^= m;
	SIPHASH_SIPROUND(v0, v1, v2, v3);
#ifdef SIPHASH_KERNEL_METHOD
	SIPHASH_SIPROUND(v0, v1, v2, v3);
#endif
	v0 ^= m;

	if (!one_arg) {
		if (partial && end) {
			b |= __le64_to_cpu((__u64)args[1]);
			SIPHASH_POSTAMBLE(v0, v1, v2, v3, b); /* Returns */
		}

		m = __le64_to_cpu((__u64)args[1]);
		v3 ^= m;
		SIPHASH_SIPROUND(v0, v1, v2, v3);
#ifdef SIPHASH_KERNEL_METHOD
		SIPHASH_SIPROUND(v0, v1, v2, v3);
#endif
		v0 ^= m;
	}

	if (end)
		SIPHASH_POSTAMBLE(v0, v1, v2, v3, b); /* Returns */

	args[2] = b;
	args[3] = v0;
	args[4] = v1;
	args[5] = v2;
	args[6] = v3;

	return 0; /* Unused */
}

/* Place holder for domain specific accelerator */
__u64 siphash_dsa(const void *data, size_t len,
		  const siphash_key_t *key);

#endif /* __SIPHASH_H __*/
