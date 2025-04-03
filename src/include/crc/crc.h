//-----------------------------------------------------------------------------
// MurmurHash3 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.

#ifndef CRC_H
#define CRC_H

//-----------------------------------------------------------------------------
// Platform-specific functions and macros
#include <linux/types.h>
#include <stdint.h>
#include <stddef.h>

#define CRC64     3
#define CRC16     1
#define CRC32     2

//-----------------------------------------------------------------------------

//_u64 crc64(_u64 crc, const unsigned char *s, _u64 l);
//-----------------------------------------------------------------------------

/* SDPU DSA Changes */
__u64 crc_dsa(unsigned char bitw, __u64 crc, const void *data, size_t len);

// TODO: Support pvbuf changes for crc

#endif // CRC_H
