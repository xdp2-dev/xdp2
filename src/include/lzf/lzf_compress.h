
//-----------------------------------------------------------------------------
// lzf_compress was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.

#ifndef LZF_COMPRESS_H
#define LZF_COMPRESS_H

//-----------------------------------------------------------------------------
// Platform-specific functions and macros
#include <linux/types.h>
#include <stdint.h>
#include <stddef.h>

//-----------------------------------------------------------------------------

size_t lzf_compress(const void *data, size_t len, void *data_out,
		    size_t len_out);

//-----------------------------------------------------------------------------

/* SDPU DSA Changes */
__u64 lzf_compress_dsa(const void *data, size_t len, void *data_out,
		       size_t len_out);
__u64 lzfc_crc_pipeline_dsa(const void *data, size_t len, void *data_out,
			    size_t len_out, size_t bitw, __u64 *crc);


// TODO: Support pvbuf changes for lzf_compress

#endif // LZF_COMPRESS_H
