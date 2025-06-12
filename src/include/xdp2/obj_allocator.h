/* SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2020,2021 Tom Herbert
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

#ifndef __XDP2_OBJ_ALLOCATOR_H__
#define __XDP2_OBJ_ALLOCATOR_H__

#include <stdint.h>

#include "xdp2/utility.h"

#include <stdio.h>

#define XDP2_OBJ_ALLOC_MAGIC_NUM 0x43b3c9ef64bba98c

#define __XDP2_OBJ_ALLOC_CHECK_MAGIC(ALLOCATOR, FILE, LINE)	\
	XDP2_ASSERT((ALLOCATOR)->magic_num ==			\
				XDP2_OBJ_ALLOC_MAGIC_NUM,	\
		"Object allocator does not match magic "	\
		"number: %llu at %s:%u\n",			\
		(ALLOCATOR)->magic_num, FILE, LINE)

#define XDP2_OBJ_ALLOC_CHECK_MAGIC(ALLOCATOR)			\
	__XDP2_OBJ_ALLOC_CHECK_MAGIC(ALLOCATOR, __FILE__, __LINE__)

/* Definitions and functions for the XDP2 object allocator */

#define XDP2_OBJ_ALLOC_NAME_LEN 31

struct xdp2_obj_allocator_free_list {
	void *free_list;

	unsigned int num_free;
	unsigned long allocs;
	unsigned long alloc_fails;

	pthread_mutex_t mutex;
};

struct xdp2_obj_allocator {
	__u64 magic_num;

	/* Add one to make sure name is NULL terminated */
	char name[XDP2_OBJ_ALLOC_NAME_LEN + 1];

	unsigned int max_objs;
	size_t obj_size;

	unsigned int base_index;

	void *base;

	struct xdp2_obj_allocator_free_list alloc_free_list;
};

/* Size of struct xdp2_obj_allocator for FIFO allocators. This is the size
 * of the base object allocator structure plus the size of the queue entries
 * (the number of objects times 8 (each queue entry is eight bytes)
 */
#define XDP2_OBJ_ALLOC_SIZE(NUM_OBJS)					\
	(sizeof(struct xdp2_obj_allocator) + (NUM_OBJS) * sizeof(__u64))

#define XDP2_OBJ_ALLOC_SIZE_NOZERO(NUM_OBJS)				\
	((NUM_OBJS) ? XDP2_OBJ_ALLOC_SIZE(NUM_OBJS) : 0)

#define XDP2_OBJ_ALLOC_BASE_INDEX 1

#define XDP2_OBJ_ALLOC_CHECK(ALLOCATOR, OBJ, LABEL) do {		\
	struct xdp2_obj_allocator *_allocator = (ALLOCATOR);		\
	void *_obj = (OBJ);						\
	unsigned int _index = xdp2_obj_alloc_obj_to_index(_allocator,	\
							   _obj);	\
									\
	XDP2_ASSERT(_index != 0, LABEL "Zero index for allocator %s "	\
		     "with max_objs %u\n", _allocator->name,		\
		     _allocator->max_objs);				\
	XDP2_ASSERT(_index <= _allocator->max_objs, LABEL "Bad index "	\
		     "%u for allocator %s with max_objs %u\n",		\
		     _index, _allocator->name, _allocator->max_objs);	\
} while (0)

void __xdp2_obj_alloc_free_list_init(
			struct xdp2_obj_allocator *allocator,
			unsigned int num_objs, size_t obj_size, void *base,
			const char *name, unsigned int base_index);

bool xdp2_obj_alloc_free_list_init(struct xdp2_obj_allocator *allocator,
			unsigned int num_objs, size_t obj_size,
			const char *name);

void __xdp2_obj_alloc_validate(struct xdp2_obj_allocator *allocator);

void __xdp2_obj_alloc_check_freed(struct xdp2_obj_allocator *allocator,
				   void *cobj);

void __xdp2_obj_alloc_show_allocator(struct xdp2_obj_allocator *allocator,
		void *cli, void (*cb)(struct xdp2_obj_allocator *allocator,
				      void *cli, const char *arg),
				      const char *arg);

static inline void xdp2_obj_alloc_show_allocator(
		struct xdp2_obj_allocator *allocator, void *cli)
{
	__xdp2_obj_alloc_show_allocator(allocator, cli, NULL, NULL);
}

static inline size_t xdp2_obj_alloc_index_to_offset(
		struct xdp2_obj_allocator *allocator, unsigned int index)
{
	return (index - allocator->base_index) * allocator->obj_size;
}

static inline void *xdp2_obj_alloc_index_to_obj(
				struct xdp2_obj_allocator *allocator,
				unsigned int index)
{
	XDP2_OBJ_ALLOC_CHECK_MAGIC(allocator);

	XDP2_ASSERT(index >= allocator->base_index &&
		     index < allocator->max_objs + allocator->base_index,
		     "Allocator %s: TIllegal allocator index: %u\n",
		     allocator->name, index);

	/* Return the absolute address of the object based on the base
	 * memory address
	 */
	return allocator->base +
			xdp2_obj_alloc_index_to_offset(allocator, index);
}

static unsigned int xdp2_obj_alloc_obj_to_index(
			struct xdp2_obj_allocator *allocator, void *obj)
{
	XDP2_OBJ_ALLOC_CHECK_MAGIC(allocator);

	/* Return the index by computing the address offset from the absolute
	 * address of the base memory address
	 */
	return (((uintptr_t)obj -
		(uintptr_t)allocator->base) / allocator->obj_size) +
						allocator->base_index;
}

static inline void *__xdp2_obj_alloc_free_list_alloc(
		struct xdp2_obj_allocator *allocator, unsigned int *num,
		char *_file, int _line)
{
	struct xdp2_obj_allocator_free_list *afl = &allocator->alloc_free_list;
	void *obj;

	__XDP2_OBJ_ALLOC_CHECK_MAGIC(allocator, _file, _line);

	pthread_mutex_lock(&afl->mutex);

	if (!afl->num_free) {
		/* No memory, return NULL and let the caller deal with it.
		 * Maybe in the future we can devise some handling when under
		 * memory pressure
		 */
		afl->alloc_fails++;
		pthread_mutex_unlock(&afl->mutex);
		return NULL;
	}

	XDP2_ASSERT(afl->num_free, "Allocator %s: num free is zero",
		     allocator->name);

	/* Translate relative address to absolute address of the object */
	obj = afl->free_list;
	if (!obj) {
		XDP2_ASSERT(!afl->num_free, "Allocator %s: no free objects "
			     "for allocator, but num free is non-zero",
			     allocator->name);
		pthread_mutex_unlock(&afl->mutex);
	}
	afl->free_list = *(void **)obj;
	afl->num_free--;
	afl->allocs++;

	pthread_mutex_unlock(&afl->mutex);

#ifdef XDP2_OBJ_ALLOC_DEBUG // For debugging
	__xdp2_obj_alloc_validate(allocator);
#endif
	*num = xdp2_obj_alloc_obj_to_index(allocator, obj);

	return obj;
}

#define xdp2_obj_alloc_free_list_alloc(ALLOCATOR, NUM)			\
		__xdp2_obj_alloc_free_list_alloc(ALLOCATOR, NUM,	\
						  __FILE__, __LINE__)

static inline void __xdp2_obj_alloc_free_list_free(
		struct xdp2_obj_allocator *allocator, void *obj,
		char *_file, int _line)
{
	struct xdp2_obj_allocator_free_list *afl = &allocator->alloc_free_list;

	__XDP2_OBJ_ALLOC_CHECK_MAGIC(allocator, _file, _line);

#ifdef XDP2_OBJ_ALLOC_DEBUG // For debugging
	__xdp2_obj_alloc_check_freed(allocator, obj);
#endif
	pthread_mutex_lock(&afl->mutex);

	*(void **)obj = afl->free_list;
	/* Save object by its relative address */
	afl->free_list = obj;
	afl->num_free++;

	pthread_mutex_unlock(&afl->mutex);

#ifdef XDP2_OBJ_ALLOC_DEBUG // For debugging
	__xdp2_obj_alloc_validate(allocator);
#endif
}

#define xdp2_obj_alloc_free_list_free(ALLOCATOR, OBJ)			\
	__xdp2_obj_alloc_free_list_free(ALLOCATOR, OBJ, __FILE__, __LINE__)

#define xdp2_obj_alloc_free_list_free_by_index(ALLOCATOR, INDEX)	\
		xdp2_obj_alloc_free_list_free(allocator,		\
			xdp2_obj_alloc_index_to_obj(allocator, index))

static inline void __xdp2_obj_alloc_init(
		struct xdp2_obj_allocator *allocator,
		unsigned int num_objs, size_t obj_size,
		void *base, const char *name)
{
	__xdp2_obj_alloc_free_list_init(allocator, num_objs, obj_size,
					 base, name, XDP2_OBJ_ALLOC_BASE_INDEX);
}

static inline void __xdp2_obj_alloc_init_base_index(
		struct xdp2_obj_allocator *allocator,
		unsigned int num_objs, size_t obj_size,
		void *base, const char *name, unsigned int base_index)
{
	__xdp2_obj_alloc_free_list_init(allocator, num_objs, obj_size,
					 base, name, base_index);
}

static inline bool xdp2_obj_alloc_init(struct xdp2_obj_allocator *allocator,
					unsigned int num_objs, size_t obj_size,
					const char *name)
{
	return xdp2_obj_alloc_free_list_init(allocator, num_objs, obj_size,
					      name);
}

static inline void *__xdp2_obj_alloc_alloc(struct xdp2_obj_allocator
								*allocator,
					    unsigned int *num,
					    char *_file, int _line)
{
	void *obj;

	obj = __xdp2_obj_alloc_free_list_alloc(allocator, num, _file, _line);
	if (!obj)
		return NULL;

	XDP2_OBJ_ALLOC_CHECK(allocator, obj, "Allocate object: ");

	XDP2_ASSERT(xdp2_obj_alloc_obj_to_index(allocator, obj) ==
		     *num, "Alloc object: index discrepancy %u != %u",
		     xdp2_obj_alloc_obj_to_index(allocator, obj), *num);

	return obj;
}

#define xdp2_obj_alloc_alloc(ALLOCATOR, NUM)				\
		__xdp2_obj_alloc_alloc(ALLOCATOR, NUM, __FILE__, __LINE__)

static inline void xdp2_obj_alloc_free(struct xdp2_obj_allocator *allocator,
					void *obj)
{
	XDP2_OBJ_ALLOC_CHECK(allocator, obj, "Free object: ");

	xdp2_obj_alloc_free_list_free(allocator, obj);
}

static inline void xdp2_obj_alloc_free_by_index(
		struct xdp2_obj_allocator *allocator, unsigned int index)
{
	xdp2_obj_alloc_free_list_free_by_index(allocator, index);
}

/* Return the base of the accelerator objects, this is a relative address */
static inline void *xdp2_obj_alloc_get_base(
				struct xdp2_obj_allocator *allocator)
{
	return allocator->base;
}

#endif /* __XDP2_OBJ_ALLOCATOR_H__ */
