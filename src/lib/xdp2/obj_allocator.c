// SPDX-License-Identifier: BSD-2-Clause-FreeBSD
/*
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

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "xdp2/obj_allocator.h"

static void *alloc_objects(size_t num_objs, size_t obj_size)
{
	if (obj_size < sizeof(void *) || num_objs == 0)
		return NULL;

	return calloc(num_objs, obj_size);
}

static void __xdp2_obj_alloc_common_init(
		struct xdp2_obj_allocator *allocator, unsigned int num_objs,
		size_t obj_size, void *base, const char *name,
		unsigned int base_index)
{
	allocator->magic_num = XDP2_OBJ_ALLOC_MAGIC_NUM;
	/* Save the base address as a relative address */
	allocator->base = base;
	allocator->obj_size = obj_size;
	allocator->max_objs = num_objs;
	allocator->base_index = base_index;

	strncpy(allocator->name, name, XDP2_OBJ_ALLOC_NAME_LEN);
	/* Ensure NULL terminated */
	allocator->name[XDP2_OBJ_ALLOC_NAME_LEN] = '\0';
}

void __xdp2_obj_alloc_free_list_init(
		struct xdp2_obj_allocator *allocator, unsigned int num_objs,
		size_t obj_size, void *base, const char *name,
		unsigned int base_index)
{
	struct xdp2_obj_allocator_free_list *afl = &allocator->alloc_free_list;
	void *obj;
	int i;

	/* Create free list
	 *
	 * All the objects are in the same address space, so perform address
	 * translation on any pointers, absolute address to relative address,
	 * to the objects that are save in the allocator structure
	 */
	for (obj = base, i = 0; i < num_objs - 1; i++, obj += obj_size)
		*(void **)obj = obj + obj_size;

	*(void **)obj = NULL;

	__xdp2_obj_alloc_common_init(allocator, num_objs, obj_size,
				      base, name, base_index);
	afl->num_free = num_objs;
	/* Save the base object by its relative address */
	afl->free_list = base;

	pthread_mutex_init(&afl->mutex, NULL);
}

bool xdp2_obj_alloc_free_list_init(struct xdp2_obj_allocator *allocator,
				    unsigned int num_objs, size_t obj_size,
				    const char *name)
{
	void *base;

	base = alloc_objects(num_objs, obj_size);
	if (!base)
		return false;

	__xdp2_obj_alloc_free_list_init(allocator, num_objs, obj_size, base,
					 name, XDP2_OBJ_ALLOC_BASE_INDEX);

	return true;
}

void __xdp2_obj_alloc_validate(struct xdp2_obj_allocator *allocator)
{
	struct xdp2_obj_allocator_free_list *afl =
				&allocator->alloc_free_list;
	unsigned int count = 0;
	void *obj;

	XDP2_OBJ_ALLOC_CHECK_MAGIC(allocator);

	pthread_mutex_lock(&afl->mutex);

	/* Don't need to do address translation here since we're only
	 * counting NULL objects (the translated address of NULL
	 * address is also NULL)
	 */
	obj = afl->free_list;
	while (obj) {
		count++;
		obj = *(void **)obj;
	}

	XDP2_ASSERT(count <= allocator->max_objs,
	     "Allocator %s: Too many items on allocator object free "
	     "list %u > %u\n", allocator->name, count,
	     allocator->max_objs);
	XDP2_ASSERT(count == afl->num_free,
		    "Allocator %s: object count "
		    "bad. Counted %u, num_free datapath is %u\n",
		    allocator->name, count, afl->num_free);

	pthread_mutex_unlock(&afl->mutex);
}

void __xdp2_obj_alloc_check_freed(struct xdp2_obj_allocator *allocator,
				   void *cobj)
{
	struct xdp2_obj_allocator_free_list *afl =
				&allocator->alloc_free_list;
	void *obj;

	XDP2_OBJ_ALLOC_CHECK_MAGIC(allocator);

		/* Translate each object on the list to its absolute address
		 * for the compare
		 */
	for (obj = afl->free_list; obj; obj = *(void **)obj)
		XDP2_ASSERT(obj != cobj, "Allocator %s: Object %p is "
					  "already free when trying "
					  "to free it",
					  allocator->name, cobj);
}

void __xdp2_obj_alloc_show_allocator(struct xdp2_obj_allocator *allocator,
		void *cli, void (*cb)(struct xdp2_obj_allocator *allocator,
				      void *cli, const char *arg),
		const char *arg)
{
	struct xdp2_obj_allocator_free_list *afl =
				&allocator->alloc_free_list;

	XDP2_OBJ_ALLOC_CHECK_MAGIC(allocator);

	XDP2_CLI_PRINT(cli, "%s:: obj_size: %lu, max_objs: %u, "
			    "num_free: %u, num_allocs: %lu, "
			    "alloc_fails: %lu\n",
			allocator->name, allocator->obj_size,
			allocator->max_objs, afl->num_free,
			afl->allocs, afl->alloc_fails);

	if (cb)
		cb(allocator, cli, arg);
}
