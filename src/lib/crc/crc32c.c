/* crc32c.c -- compute CRC-32C using the Intel crc32 instruction
 * Copyright (C) 2013, 2015 Mark Adler
 * Version 1.3  31 Dec 2015  Mark Adler
 */

/*
  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the author be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  Mark Adler
  madler@alumni.caltech.edu
 */

/* Use hardware CRC instruction on Intel SSE 4.2 processors.  This computes a
   CRC-32C, *not* the CRC-32 used by Ethernet and zip, gzip, etc.  A software
   version is provided as a fall-back, as well as for speed comparisons. */

/* Version history:
   1.0  10 Feb 2013  First version
   1.1   1 Aug 2013  Correct comments on why three crc instructions in parallel
   1.2   1 Nov 2015  Add const qualifier to avoid compiler warning
		     Load entire input into memory (test code)
		     Argument gives number of times to repeat (test code)
		     Argument < 0 forces software implementation (test code)
   1.3  31 Dec 2015  Check for Intel architecture using compiler macro
		     Support big-endian processors in software calculation
		     Add header for external use
 */

#include <pthread.h>
#include "crc/crc32c.h"

crc_func crc32c;

/* CRC-32C (iSCSI) polynomial in reversed bit order. */
#define POLY 0x82f63b78

static __u32 crc32c_sw_little(__u32 crc, void const *buf, size_t len);
static __u32 crc32c_sw_big(__u32 crc, void const *buf, size_t len);

void crc32c_init(void)
{
    crc32c = crc32c_sw;
}

/* Construct table for software CRC-32C little-endian calculation. */
static pthread_once_t crc32c_once_little = PTHREAD_ONCE_INIT;
static __u32 crc32c_table_little[8][256];
static void crc32c_init_sw_little(void)
{
	unsigned int n;

	for (n = 0; n < 256; n++) {
		__u32 crc = n;

		crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
		crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
		crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
		crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
		crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
		crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
		crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
		crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
		crc32c_table_little[0][n] = crc;
	}
	for (n = 0; n < 256; n++) {
		__u32 crc = crc32c_table_little[0][n];
		unsigned int k;

		for (k = 1; k < 8; k++) {
			crc = crc32c_table_little[0][crc & 0xff] ^ (crc >> 8);
			crc32c_table_little[k][n] = crc;
		}
	}
}

/* Compute a CRC-32C in software assuming a little-endian architecture,
 * constructing the required table if that hasn't already been done.
 */
__u32 crc32c_sw_little(__u32 crc, void const *buf, size_t len)
{
	unsigned char const *next = buf;

	pthread_once(&crc32c_once_little, crc32c_init_sw_little);
	crc = ~crc;
	while (len && ((uintptr_t)next & 7) != 0) {
		crc = crc32c_table_little[0][(crc ^ *next++) & 0xff] ^
								(crc >> 8);
		len--;
	}
	if (len >= 8) {
		__u64 crcw = crc;

		do {
			crcw ^= *(__u64 const *)next;
			crcw = crc32c_table_little[7][crcw & 0xff] ^
			       crc32c_table_little[6][(crcw >> 8) & 0xff] ^
			       crc32c_table_little[5][(crcw >> 16) & 0xff] ^
			       crc32c_table_little[4][(crcw >> 24) & 0xff] ^
			       crc32c_table_little[3][(crcw >> 32) & 0xff] ^
			       crc32c_table_little[2][(crcw >> 40) & 0xff] ^
			       crc32c_table_little[1][(crcw >> 48) & 0xff] ^
			       crc32c_table_little[0][crcw >> 56];
			next += 8;
			len -= 8;
		} while (len >= 8);

		crc = crcw;
	}
	while (len) {
		crc = crc32c_table_little[0][(crc ^ *next++) & 0xff] ^
								(crc >> 8);
		len--;
	}
	return ~crc;
}

/* Swap the bytes in a __u64.  (Only for big-endian.) */
#if defined(__has_builtin) || (defined(__GNUC__) && \
    (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)))
#  define swap __builtin_bswap64
#else
static inline __u64 swap(__u64 x)
{
	x = ((x << 8) & 0xff00ff00ff00ff00) | ((x >> 8) & 0xff00ff00ff00ff);
	x = ((x << 16) & 0xffff0000ffff0000) | ((x >> 16) & 0xffff0000ffff);
	return (x << 32) | (x >> 32);
}
#endif

/* Construct tables for software CRC-32C big-endian calculation. */
static pthread_once_t crc32c_once_big = PTHREAD_ONCE_INIT;
static __u32 crc32c_table_big_byte[256];
static __u64 crc32c_table_big[8][256];
static void crc32c_init_sw_big(void)
{
	unsigned int n;

	for (n = 0; n < 256; n++) {
		__u32 crc = n;

		crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
		crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
		crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
		crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
		crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
		crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
		crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
		crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
		crc32c_table_big_byte[n] = crc;
	}

	for (n = 0; n < 256; n++) {
		__u32 crc = crc32c_table_big_byte[n];
		unsigned int k;

		crc32c_table_big[0][n] = swap(crc);
		for (k = 1; k < 8; k++) {
			crc = crc32c_table_big_byte[crc & 0xff] ^ (crc >> 8);
			crc32c_table_big[k][n] = swap(crc);
		}
	}
}

/* Compute a CRC-32C in software assuming a big-endian architecture,
 * constructing the required tables if that hasn't already been done.
 */
__u32 crc32c_sw_big(__u32 crc, void const *buf, size_t len)
{
	unsigned char const *next = buf;

	pthread_once(&crc32c_once_big, crc32c_init_sw_big);
	crc = ~crc;
	while (len && ((uintptr_t)next & 7) != 0) {
		crc = crc32c_table_big_byte[(crc ^ *next++) & 0xff] ^
								(crc >> 8);
		len--;
	}
	if (len >= 8) {
		__u64 crcw = swap(crc);

		do {
			crcw ^= *(__u64 const *)next;
			crcw = crc32c_table_big[0][crcw & 0xff] ^
			       crc32c_table_big[1][(crcw >> 8) & 0xff] ^
			       crc32c_table_big[2][(crcw >> 16) & 0xff] ^
			       crc32c_table_big[3][(crcw >> 24) & 0xff] ^
			       crc32c_table_big[4][(crcw >> 32) & 0xff] ^
			       crc32c_table_big[5][(crcw >> 40) & 0xff] ^
			       crc32c_table_big[6][(crcw >> 48) & 0xff] ^
			       crc32c_table_big[7][(crcw >> 56)];
			next += 8;
			len -= 8;
		} while (len >= 8);
		crc = swap(crcw);
	}
	while (len) {
		crc = crc32c_table_big_byte[(crc ^ *next++) & 0xff] ^
								(crc >> 8);
		len--;
	}
	return ~crc;
}

/* Table-driven software CRC-32C.  This is about 15 times slower than using the
 * hardware instructions.  Determine the endianness of the processor and proceed
 * accordingly.  Ideally the endianness will be determined at compile time, in
 * which case the unused functions and tables for the other endianness will be
 * removed by the optimizer.  If not, then the proper routines and tables will
 * be used, even if the endianness is changed mid-stream.  (Yes, there are
 * processors that permit that -- go figure.)
 */
__u32 crc32c_sw(__u32 crc, void const *buf, size_t len)
{
	static int const little = 1;

	if (*(char const *)&little)
		return crc32c_sw_little(crc, buf, len);

	return crc32c_sw_big(crc, buf, len);
}
