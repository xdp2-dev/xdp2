//-----------------------------------------------------------------------------
// MurmurHash3 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.

#ifndef MURMURHASH3_H
#define MURMURHASH3_H

//-----------------------------------------------------------------------------
// Platform-specific functions and macros
#include <stdint.h>
#include <stddef.h>

//-----------------------------------------------------------------------------

uint32_t MurmurHash3_x86_32(const void *key, size_t length);

//-----------------------------------------------------------------------------

/* SDPU DSA Changes */
uint64_t murmur3hash_dsa(const void *data, size_t len);
# if 0

// TODO: Support pvbuf changes for murmur3hash

/* murmur3hash iterator */

struct murmur3hash_iter {
	__u64 v0;
	__u64 v1;
	__u64 v2;
	__u64 v3;
	__u64 b;

	__u8 residual[8];
	__u8 residual_length;
};

void murmur3hash_iter_start(struct murmur3hash_iter *iter, size_t len,
		const murmur3hash_key_t *key);

__u64 murmur3hash_iter_end(struct murmur3hash_iter *iter);

void murmur3hash_iter(struct murmur3hash_iter *iter, void *data, size_t len);
#endif

#endif // MURMURHASH3_H
