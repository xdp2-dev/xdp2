/* SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2026 XDPnet Inc.
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

#ifndef __XDP2_VSTRUCT_HELPERS_H__
#define __XDP2_VSTRUCT_HELPERS_H__

#include "xdp2/compiler_helpers.h"
#include "xdp2/fifo.h"
#include "xdp2/utility.h"
#include "xdp2/vstruct.h"

#define XDP2_VSTRUCT_MAKE_FIFO(PREFIX, LNAME, UNAME, NUM, CONFIG)	\
	(XDP2_JOIN2(LNAME, _fifo), __u8, struct xdp2_fifo,		\
	 NUM * XDP2_FIFO_SIZE(XDP2_CONFIG_DEF_GET(CONFIG, PREFIX,	\
		XDP2_JOIN2(UNAME, _FIFO_LIMIT))), 0),			\
	(XDP2_JOIN2(LNAME, _fifo_stats), struct xdp2_fifo_stats,	\
	struct xdp2_fifo_stats, NUM, 0)

#define XDP2_VSTRUCT_MAKE_POLL_GROUP(LNAME, NUM)			\
	(XDP2_JOIN2(LNAME, _poll_group), struct xdp2_fifo_poll_group,	\
	 struct xdp2_fifo_poll_group, NUM, 0),				\
	(XDP2_JOIN2(LNAME, _poll_group_stats),				\
	 struct xdp2_fifo_poll_group_stats,				\
	 struct xdp2_fifo_poll_group_stats, NUM, 0)


#ifdef XDP2_CONFIG_USE_DEFAULT_CONFIG
#define XDP2_VSTRUCT__MAKE_VS_STRUCT(STRUCT) struct STRUCT##_vsconst vsconst

#define XDP2_VSTRUCT_VP(STRUCT, FIELD) XDP2_VSTRUCT_GETPTR_VSCONST(	\
							STRUCT, FIELD)
#define XDP2_VSTRUCT(STRUCT, CONFIG_TYPE, ...)				\
		XDP2_VSTRUCT_VSCONST(STRUCT, CONFIG_TYPE, __VA_ARGS__)

#define XDP2_VSTRUCT_PRINT(CLI, STRUCT, VSMAP, DEF_VSMAP,		\
			   PRINT_INDENT, NO_ZERO_SIZE_PRINT,		\
			   SHORT_OUTPUT, CSV, LABEL)			\
	XDP2_VSTRUCT_PRINT_VSCONST(CLI, STRUCT, VSMAP, DEF_VSMAP,	\
				   PRINT_INDENT, NO_ZERO_SIZE_PRINT,	\
				   SHORT_OUTPUT, CSV, LABEL)

#define XDP2_VSTRUCT_BANNER(CLI, STRUCT, TEXT, VSMAP, DEF_VSMAP, ADD)	\
do {									\
	unsigned int _count, _zeros;					\
									\
	count = XDP2_JOIN2(STRUCT, _count_vsconst)(&_zeros);		\
	XDP2_CLI_PRINT(CLI, "%s (VSCONST %u non-zero entries, "		\
			    "%u total)%s\n",				\
			TEXT, _count - _zeros, _count, ADD);		\
} while (0)

#else
#define XDP2_VSTRUCT_MAKE_VS_STRUCT(STRUCT) struct STRUCT##_vsmap vsmap

#define XDP2_VSTRUCT_VP(STRUCT, FIELD)					\
				XDP2_VSTRUCT_GETPTR_VSMAP(STRUCT, FIELD)

#define XDP2_VSTRUCT(STRUCT, CONFIG_TYPE, ...)				\
		XDP2_VSTRUCT_VSMAP(STRUCT, CONFIG_TYPE, __VA_ARGS__)

#define XDP2_VSTRUCT_PRINT(CLI, STRUCT, VSMAP, DEF_VSMAP,		\
			   PRINT_INDENT, NO_ZERO_SIZE_PRINT,		\
			   SHORT_OUTPUT, CSV, LABEL)			\
	XDP2_VSTRUCT_PRINT_VSMAP(CLI, STRUCT, VSMAP, DEF_VSMAP,	\
				 PRINT_INDENT, NO_ZERO_SIZE_PRINT,	\
				 SHORT_OUTPUT, CSV, LABEL)

#define XDP2_VSTRUCT_BANNER(CLI, STRUCT, TEXT, VSMAP, DEF_VSMAP, ADD)	\
do {									\
	unsigned int _count, _zeros;					\
									\
	_count = xdp2_vstruct_count_vsmap(&(VSMAP)->gen,		\
					  &(DEF_VSMAP)->gen, &_zeros);	\
	XDP2_CLI_PRINT(CLI, "%s (VSMAP %u non-zero entries, "		\
			    "%u total)%s\n",				\
		       TEXT, _count - _zeros, _count, ADD);		\
} while (0)

#endif

static inline void *__xdp2_vstruct_get_local_mem(size_t size, void *raw_mem,
						 size_t req_size,
						 bool *alloced_mem, char *name)
{
	*alloced_mem = false;

	/* Check the requested size against the size of the variable data
	 * structure (zero indicates no limit). If the local memory is
	 * provided by the caller then req_size must be nonzero (checked below)
	 */
	if (req_size && size > req_size) {
		fprintf(stderr, "Size of %s is greater than requested size\n",
			name);
		goto fail;
	}

	if (raw_mem) {
		if (!req_size) {
			fprintf(stderr, "Memory was provided for %s, but "
					"requested size is zero\n", name);
			goto fail;
		}
		size = req_size;

		/* If raw memory was provided don't zero it */
	} else {
		/* Need to alloc shared memory regions */
		raw_mem = malloc(size);
		if (!raw_mem) {
			fprintf(stderr, "Memory alloc failed for %s\n", name);
			goto fail;
		}
		*alloced_mem = true;

		memset(raw_mem, 0, size);
	}

	return raw_mem;

fail:
	if (*alloced_mem)
		free(raw_mem);

	return NULL;
}

static inline void *xdp2_vstruct_get_local_mem(size_t size, void *raw_mem,
					       size_t req_size, char *name)
{
	bool alloced_mem;

	return __xdp2_vstruct_get_local_mem(size, raw_mem, req_size,
					    &alloced_mem, name);
}

#define __XDP2_VSTRUCT_MAKE_VSMAP_FUNC(NAME, CONFIG_TYPE, TEXT,		\
				       DEBUG_FLAG)			\
static inline size_t XDP2_JOIN2(NAME, _make_vsmap)(			\
		struct XDP2_JOIN2(NAME,	_vsmap) *vsmap,			\
		struct CONFIG_TYPE *config,				\
		unsigned int debug_mask)				\
{									\
	struct XDP2_JOIN2(NAME, _def_vsmap) def_vsmap;			\
	size_t size;							\
									\
	size = XDP2_JOIN2(NAME,	_instantiate_vsmap_from_config)(	\
			 config, vsmap, &def_vsmap,			\
			 sizeof(struct XDP2_JOIN2(NAME,)));		\
	if (debug_mask & DEBUG_FLAG) {					\
		XDP2_VSTRUCT_BANNER(NULL, XDP2_JOIN2(NAME,),		\
				    TEXT, vsmap, &def_vsmap, "");	\
									\
		XDP2_VSTRUCT_PRINT(NULL, XDP2_JOIN2(NAME,),		\
				   vsmap, &def_vsmap, "\t", true,	\
				   false, false, false);		\
	}								\
									\
	return size;							\
}

/* Return the computed size for the variable structure of a cluster shared
 * memory instance
 */
#define __XDP2_VSTRUCT_MAKE_VSMAP_SIZE_FUNC(NAME, CONFIG_TYPE)		\
static inline size_t XDP2_JOIN2(NAME, _size)(				\
		const struct CONFIG_TYPE *config)			\
{									\
	struct XDP2_JOIN2(NAME, _def_vsmap) def_vsmap;			\
	struct XDP2_JOIN2(NAME, _vsmap) vsmap;				\
									\
	return XDP2_JOIN2(NAME,	_instantiate_vsmap_from_config)(	\
			config, &vsmap, &def_vsmap,			\
			sizeof(struct XDP2_JOIN2(NAME,)));		\
}

#define __XDP2_VSTRUCT_MAKE_SHM_FUNC(NAME, CONFIG_TYPE, SET_SHM,	\
				     TEXT_MAKE_VSMAP, TEXT_MAKE_SHM)	\
static inline bool XDP2_JOIN2(NAME, _make_multi_shm)(			\
		struct CONFIG_TYPE *config,				\
		void *raw_mem[],					\
		struct XDP2_JOIN2(NAME,) *shms[],			\
		unsigned int count, unsigned int debug_mask)		\
{									\
	struct XDP2_JOIN2(NAME, _def_vsmap) def_vsmap;			\
	struct XDP2_JOIN2(NAME, _vsmap) vsmap;				\
	struct XDP2_JOIN2(NAME,) *shm;					\
	size_t size;							\
	int i;								\
									\
	size = XDP2_JOIN2(NAME, _instantiate_vsmap_from_config)(	\
			config, &vsmap, &def_vsmap, sizeof(*shm));	\
									\
	for (i = 0; i < count; i++) {					\
		shms[i] = xdp2_vstruct_get_local_mem(size, raw_mem[i],	\
			      size, TEXT_MAKE_SHM);			\
		if (!shms[i])						\
			return false;					\
									\
		if (SET_SHM)						\
			shms[i]->vsmap = vsmap;				\
	}								\
									\
	if (debug_mask & KANDOU_DEBUG_F_SHOW_MEM) {			\
		char buf[sizeof(" -- XXX instances")];			\
									\
		sprintf(buf, " -- %u instances", (__u8)count);		\
		XDP2_VSTRUCT_BANNER(NULL,				\
			XDP2_JOIN2(NAME, count),			\
			TEXT_MAKE_VSMAP, &vsmap, &def_vsmap, buf);	\
									\
		XDP2_VSTRUCT_PRINT(NULL,				\
			XDP2_JOIN2(NAME,), &vsmap, &def_vsmap, "\t",	\
			true, false, false, "");			\
	}								\
									\
	return true;							\
}									\
									\
static inline struct XDP2_JOIN2(NAME,)					\
			*XDP2_JOIN2(NAME, _make_shm)(			\
		struct CONFIG_TYPE *config, void *raw_mem,		\
		unsigned int debug_mask)				\
{									\
	struct XDP2_JOIN2(NAME,) *shm;					\
	bool v;								\
									\
	v = XDP2_JOIN2(NAME, _make_multi_shm)(				\
		      config, &raw_mem, &shm, 1, debug_mask);		\
									\
	return v ? shm : NULL;						\
}

/* Create an instance of iochiplet shared memory */
#define XDP2_VSTRUCT_MAKE_VSMAP_FUNCS(NAME, CONFIG_TYPE, USE_CONST,	\
				TEXT_MAKE_VSMAP, TEXT_MAKE_SHM,		\
				DEBUG_FLAG)				\
	__XDP2_VSTRUCT_MAKE_SHM_FUNC(NAME, CONFIG_TYPE, !USE_CONST,	\
				     TEXT_MAKE_VSMAP, TEXT_MAKE_SHM)	\
	__XDP2_VSTRUCT_MAKE_VSMAP_FUNC(NAME, CONFIG_TYPE,		\
				       TEXT_MAKE_VSMAP,	DEBUG_FLAG)	\
	__XDP2_VSTRUCT_MAKE_VSMAP_SIZE_FUNC(NAME, CONFIG_TYPE)		\

#endif /* __XDP2_VSTRUCT_HELPERS_H__ */
