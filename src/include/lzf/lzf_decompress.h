

//-----------------------------------------------------------------------------
// lzf_decompress was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.

#ifndef LZF_DECOMPRESS_H
#define LZF_DECOMPRESS_H

//-----------------------------------------------------------------------------
// Platform-specific functions and macros
#include <linux/types.h>
#include <stdint.h>
#include <stddef.h>

//-----------------------------------------------------------------------------

size_t lzf_decompress(const void *data, size_t len, void *data_out,
		      size_t len_out);

//-----------------------------------------------------------------------------

/* SDPU DSA Changes */
__u64 lzf_decompress_dsa(const void *data, size_t len, void *data_out,
			 size_t len_out);

// TODO: Support pvbuf changes for lzf_decompress

#endif // LZF_DECOMPRESS_H
