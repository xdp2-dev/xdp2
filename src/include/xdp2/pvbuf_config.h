/* SPDX-License-Identifier: BSD-2-Clause-FreeBSD *
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

#ifndef __XDP2_PVBUF_CONFIG_H__
#define __XDP2_PVBUF_CONFIG_H__

#include "xdp2/pvbuf.h"
#include "xdp2/utility.h"

#define __XDP2_PVBUF_MAKE_PBUF_CONFIG(SIZE)				\
	(PVBUF_SIZE_##SIZE, DEFAULT_PVBUF_SIZE_##SIZE, __u32, 0,	\
	 1 << (36 - XDP2_LOG_32BITS(SIZE)), #SIZE " byte pbufs",),

#define __XDP2_PVBUF_MAKE_PVBUF_CONFIG(SIZE)				\
	(NUM_PVBUFS_##SIZE, DEFAULT_PVBUFS_##SIZE, __u32,		\
	 0, 1000000, #SIZE " number of pvbufs",),

#define XDP2_PVBUF_MAKE_CONFIGS()					\
	XDP2_PMACRO_APPLY_ALL(__XDP2_PVBUF_MAKE_PBUF_CONFIG,		\
			      64, 128, 256, 512, 1024, 2048, 4096,	\
			      8192, 16384, 32768, 65536, 131072,	\
			      262144, 524288, 1048576)			\
									\
	XDP2_PMACRO_APPLY_ALL(__XDP2_PVBUF_MAKE_PVBUF_CONFIG, 1, 2, 3,	\
			      4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15)	\
									\
	/* Expand the last one since we can't include a comma after	\
	 * it								\
	 */								\
	(NUM_PVBUFS_16, DEFAULT_PVBUFS_16, __u32,			\
	 0, 1000000, " 16 number of pvbufs",)

#define __XDP2_PVBUF_MAKE_PBUF_CONFIG_ZERO_DEFAULT(SIZE)		\
	(PVBUF_SIZE_##SIZE, 0, __u32, 0,				\
	 1 << (36 - XDP2_LOG_32BITS(SIZE)), #SIZE " byte pbufs",),

#define __XDP2_PVBUF_MAKE_PVBUF_CONFIG_ZERO_DEFAULT(SIZE)		\
	(NUM_PVBUFS_##SIZE, 0, __u32,		\
	 0, 1000000, #SIZE " number of pvbufs",),

#define XDP2_PVBUF_MAKE_CONFIGS_ZERO_DEFAULT()				\
	XDP2_PMACRO_APPLY_ALL(						\
		__XDP2_PVBUF_MAKE_PBUF_CONFIG_ZERO_DEFAULT, 64, 128,	\
		256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536,	\
		131072, 262144, 524288,	1048576)			\
									\
	XDP2_PMACRO_APPLY_ALL(						\
		__XDP2_PVBUF_MAKE_PVBUF_CONFIG_ZERO_DEFAULT, 1, 2, 3,	\
		4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15)		\
									\
	/* Expand the last one since we can't include a comma after	\
	 * it								\
	 */								\
	(NUM_PVBUFS_16, 0, __u32, 0, 1000000, " 16 number of pvbufs",)

#define __XDP2_MAKE_PBUF_INIT_ENTRY1(PREFIX, INIT_STRUCT, CONFIG,	\
				     NUM, SIZE) do {			\
	struct xdp2_pbuf_init_allocator *_pbuf_init_alloc =		\
						&INIT_STRUCT;		\
									\
	_pbuf_init_alloc->obj[NUM].num_objs =				\
		XDP2_JOIN3(PREFIX, PVBUF_SIZE_, SIZE)(CONFIG);		\
} while (0);

#define __XDP2_MAKE_PBUF_INIT_ENTRY(NARG, ARG, SIZE)			\
	__XDP2_MAKE_PBUF_INIT_ENTRY1(XDP2_GET_POS_ARG3_1 ARG,		\
				     XDP2_GET_POS_ARG3_2 ARG,		\
				     XDP2_GET_POS_ARG3_3 ARG,		\
				     NARG, SIZE)

#define __XDP2_MAKE_PVBUF_INIT_ENTRY1(PREFIX, INIT_STRUCT, CONFIG,	\
				      NUM, SIZE) do {			\
	struct xdp2_pvbuf_init_allocator *_pvbuf_init_alloc =		\
						&INIT_STRUCT;		\
									\
	_pvbuf_init_alloc->obj[NUM].num_pvbufs =			\
		XDP2_JOIN3(PREFIX, NUM_PVBUFS_, SIZE)(CONFIG);		\
} while (0);

#define __XDP2_MAKE_PVBUF_INIT_ENTRY(NARG, ARG, SIZE)			\
	__XDP2_MAKE_PVBUF_INIT_ENTRY1(XDP2_GET_POS_ARG3_1 ARG,		\
				     XDP2_GET_POS_ARG3_2 ARG,		\
				     XDP2_GET_POS_ARG3_3 ARG,		\
				     NARG, SIZE)

#define XDP2_PVBUF_INIT_FROM_CONFIG(CONFIG_PREFIX, CONFIG) do {		\
	struct xdp2_pbuf_init_allocator pbuf_allocs;			\
	struct xdp2_pvbuf_init_allocator pvbuf_allocs;			\
									\
	memset(&pbuf_allocs, 0, sizeof(pbuf_allocs));			\
	memset(&pvbuf_allocs, 0, sizeof(pvbuf_allocs));			\
									\
	XDP2_PMACRO_APPLY_ALL_NARG_CARG(__XDP2_MAKE_PBUF_INIT_ENTRY,	\
		(CONFIG_PREFIX, pbuf_allocs, CONFIG), 64, 128, 256,	\
		512, 1024, 2048, 4096, 8192, 16384, 32768,		\
		65536, 131072, 262144, 524288, 1048576)			\
									\
	XDP2_PMACRO_APPLY_ALL_NARG_CARG(__XDP2_MAKE_PVBUF_INIT_ENTRY,	\
		(CONFIG_PREFIX, pvbuf_allocs, CONFIG), 1, 2, 3, 4, 5,	\
		6, 7, 8, 9, 10, 11, 12, 13, 14, 15)			\
} while (0)

#endif /* __XDP2_PVBUF_CONFIG_H__ */
