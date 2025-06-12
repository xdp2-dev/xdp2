// SPDX-License-Identifier: BSD-2-Clause-FreeBSD
/*
 * Copyright (c) 2020 Tom Herbert
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

/* Test for pvbufs */

#include <arpa/inet.h>
#include <errno.h>
#include <getopt.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "xdp2/bitmap.h"
#include "xdp2/cli.h"
#include "xdp2/config.h"
#include "xdp2/obj_allocator.h"
#include "xdp2/pmacro.h"
#include "xdp2/pvbuf.h"
#include "xdp2/pvbuf_config.h"
#include "xdp2/pvpkt.h"
#include "xdp2/utility.h"
#include "xdp2/vstruct.h"

#include "siphash/siphash.h"

/* Test for PVbufs
 *
 * Run: ./test_pvbufs [ -c <test-count> ] [ -v <verbose> ]
 *		      [ -I <report-interval> ][ -C <cli_port_num> ]
 *		      [-R] [ -X <config-string> ] [-P] [ -i <check-interval> ]
 *		      [ -M <maxlen>] [ -N <num_test_buffers> [-r] [-O]
 */

#define MAX_MAX_LEN 100000000

unsigned int verbose;
unsigned int check_interval = 1;
unsigned int check_at;

__u8 data_check[MAX_MAX_LEN];

xdp2_paddr_t data_check_paddr;
struct xdp2_pvbuf *data_check_pvbuf;
bool do_show_buffer;

/* Configuration */

XDP2_CONFIG_MAKE_ALL_CONFIGS(
	(PVBUF_TEST, pvbuf_config, NULL, false),
	(
		XDP2_PVBUF_MAKE_CONFIGS_ZERO_DEFAULT()
	)
)

#define PVBUF_PVBUF_VS1(NUM)						\
	      (pvbuf_pvbuf_##NUM##_objs, __u8,				\
	       struct xdp2_pvbuf,					\
	       PVBUF_TEST_NUM_PVBUFS_##NUM(config) * (NUM * 64), 64),	\
	      (pvbuf_pvbuf_##NUM##_objs_allocator,			\
	       __u8, struct xdp2_obj_allocator,				\
	       XDP2_OBJ_ALLOC_SIZE(					\
			PVBUF_TEST_NUM_PVBUFS_##NUM(config)), 0)

#define PVBUF_PVBUF_VS1_COMMA(NUM) PVBUF_PVBUF_VS1(NUM),

#define PVBUF_VS1(SIZE)							\
	      (pvbuf_##SIZE##_objs, __u8, __u8,				\
	       SIZE * PVBUF_TEST_PVBUF_SIZE_##SIZE(config),		\
	       !!PVBUF_TEST_PVBUF_SIZE_##SIZE(config) && SIZE),		\
	      (pvbuf_##SIZE##_objs_allocator, __u8,			\
	       struct xdp2_obj_allocator,				\
	       XDP2_OBJ_ALLOC_SIZE_NOZERO(				\
		PVBUF_TEST_PVBUF_SIZE_##SIZE(config)), 0),		\
	      (pvbuf_##SIZE##_pbuf_allocator, __u8,			\
	       struct xdp2_pbuf_allocator,				\
	       !!PVBUF_TEST_PVBUF_SIZE_##SIZE(config) *			\
			sizeof(struct xdp2_pbuf_allocator) +		\
			sizeof(atomic_uint) *				\
			PVBUF_TEST_PVBUF_SIZE_##SIZE(config), 0)

#define PVBUF_VS1_COMMA(NUM) PVBUF_VS1(NUM),

struct xdp2_buffer_manager_config_arg {
	struct pvbuf_config *config;
	struct xdp2_pbuf_init_allocator init_allocator;
	struct xdp2_pvbuf_init_allocator init_pvbuf_allocator;
	struct pvbuf_test_mem *test_mem;
};

struct pvbuf_config config;

#define DO_CONFIG(CONFIG, SIZE) do {					\
	struct xdp2_buffer_manager_config_arg *_carg =	(CONFIG);	\
	struct xdp2_pbuf_init_allocator *_init_alloc =			\
		&_carg->init_allocator;					\
	struct pvbuf_config *_config = _carg->config;			\
	unsigned int _size_shift, _tag;					\
									\
	_size_shift = xdp2_get_log_round_up(SIZE);			\
	_tag = xdp2_pbuf_size_shift_to_buffer_tag(_size_shift);	\
	_init_alloc->obj[_tag].num_objs =				\
			PVBUF_TEST_PVBUF_SIZE_##SIZE(config);		\
	_init_alloc->obj[_tag].base =					\
			VP(pvbuf_test_mem, pvbuf_##SIZE##_objs);	\
	_init_alloc->obj[_tag].allocator =				\
		VP(pvbuf_test_mem, pvbuf_##SIZE##_objs_allocator);	\
	_init_alloc->obj[_tag].pallocator =				\
		VP(pvbuf_test_mem, pvbuf_##SIZE##_pbuf_allocator);	\
	_init_alloc->obj[_tag].fifo_stats = NULL;			\
} while (0); /* Need the semicolon here */

#define DO_PVBUF_CONFIG(CONFIG, SIZE) do {				\
	struct xdp2_buffer_manager_config_arg *_carg =	(CONFIG);	\
	struct xdp2_pvbuf_init_allocator *_init_alloc =			\
		&_carg->init_pvbuf_allocator;				\
	struct pvbuf_config *_config = _carg->config;			\
									\
	_init_alloc->obj[SIZE - 1].num_pvbufs =				\
			XDP2_NUM_PVBUFS_##SIZE(_config);		\
	_init_alloc->obj[SIZE - 1].base =				\
			VP(pvbuf_test_mem, pvbuf_pvbuf_##SIZE##_objs);	\
	_init_alloc->obj[SIZE - 1].allocator =				\
		VP(pvbuf_test_mem,					\
				pvbuf_pvbuf_##SIZE##_objs_allocator);	\
	_init_alloc->obj[SIZE - 1].fifo_stats =				\
		XDP2_ENABLE_FIFO_STATS(config) ?			\
		VP(pvbuf_test_mem,					\
		   pvbuf_pvbuf_##SIZE##_objs_allocator_stats) : NULL;	\
} while (0); /* Need the semicolon here */


XDP2_VSTRUCT_VSMAP(pvbuf_test_mem, struct pvbuf_config *,
	XDP2_PMACRO_APPLY_ALL(PVBUF_VS1_COMMA, 64, 128, 256, 512, 1024,
			       2048, 4096, 8192, 16384, 32768, 65536,
			       131072, 262144, 524288, 1048576)

	XDP2_PMACRO_APPLY_ALL(PVBUF_PVBUF_VS1_COMMA, 1, 2, 3, 4, 5, 6, 7, 8,
			       9, 10, 11, 12, 13, 14, 15)
	PVBUF_PVBUF_VS1(16)
)

struct pvbuf_test_mem {
	struct pvbuf_test_mem_vsmap vsmap;
};

/* Private structure for iterating over pvbufs */
struct iterate_struct {
	unsigned int next_val;
	unsigned int index;
	unsigned int max_index;
	size_t offset;
	xdp2_paddr_t pvbuf_paddr;
};

/* Private structure for iterating siphash over pvbufs */
struct iterate_siphash_struct {
	struct siphash_iter iter;
	size_t len;
	size_t offset;
};

/* CLI commands */

XDP2_CLI_MAKE_NUMBER_SET(verbose, verbose);
XDP2_CLI_MAKE_NUMBER_SET(check_interval, check-interval);

XDP2_PVBUF_SHOW_BUFFER_MANAGER_CLI();

static void show_pvbuf_config_plain(void *cli,
		struct xdp2_cli_thread_info *info, const char *arg)
{
	xdp2_config_print_config(&pvbuf_config_table, cli, "", &config);
}

XDP2_CLI_ADD_SHOW_CONFIG("config", show_pvbuf_config_plain, 0xffff);

static void show_buffer(void *cli,
		struct xdp2_cli_thread_info *info, const char *arg)
{
	do_show_buffer = true;
}

XDP2_CLI_ADD_SHOW_CONFIG("buffer", show_buffer, 0xffff);

static void set_pvbuf_data(xdp2_paddr_t pvbuf_paddr, size_t offset)
{
	xdp2_pvbuf_copy_data_to_pvbuf(pvbuf_paddr, data_check + offset, 0, 0);
}

static void set_buf_data(void *buf, size_t len, size_t offset)
{
	memcpy(buf, data_check + offset, len);
}

/* Iterator callback function for computing siphash */
static bool siphash_iterate(void *priv, __u8 *data, size_t len)
{
	struct iterate_siphash_struct *sip_ist = priv;

	if (sip_ist->offset) {
		if (len <= sip_ist->offset) {
			sip_ist->offset -= len;
			return true;
		}
		data += sip_ist->offset;
		len -= sip_ist->offset;

		sip_ist->offset = 0;
	}

	if (sip_ist->len) {
		len = xdp2_min(sip_ist->len, len);
		siphash_iter(&sip_ist->iter, data, len);
		sip_ist->len -= len;
		if (!sip_ist->len)
			return false;
	} else {
		siphash_iter(&sip_ist->iter, data, len);
	}

	return true;
}

/* Iterator callback function for check a buf in a pvbuf */
static bool __iterate_check(void *priv, __u8 *data, size_t len,
			    const char *where)
{
	struct iterate_struct *ist = priv;
	int i;

	if (ist->offset) {
		if (ist->offset >= len) {
			/* Skipping over offset */
			ist->offset -= len;
			return true;
		}
		/* Non-zero lies within this buf */
		len -= ist->offset;
		data += ist->offset;
		ist->offset = 0;
	}

	/* Do a byte by byte compare with check data */
	for (i = 0; i < len; i++, ist->index++) {
		if (data[i] != data_check[ist->next_val++]) {
			fprintf(stderr, "Data check mismatch in %s at %u, "
					"got %u expected %u, address %p\n",
				where, ist->index, data[i],
				data_check[ist->next_val - 1], data);
			xdp2_pvbuf_print(ist->pvbuf_paddr);
			exit(1);
		}
	}

	return true;
}

static bool iterate_check(void *priv, __u8 *data, size_t len)
{
	return __iterate_check(priv, data, len, "plain check");
}

static bool iterate_check_segment(void *priv, __u8 *data, size_t len)
{
	return __iterate_check(priv, data, len, "segment check");
}

/* Function to get and check length of a pvbuf */
static size_t get_pvbuf_length(xdp2_paddr_t pvbuf_paddr, size_t len, char *op)
{
	size_t l1, l2;

	l1 = xdp2_pvbuf_calc_length(pvbuf_paddr);
	l2 = xdp2_pvbuf_calc_length_deep(pvbuf_paddr);

	if (l1 != l2) {
		fprintf(stderr, "pvbuf length calc mismatch %lu != %lu for "
				"%s\n", l1, l2, op);
		xdp2_pvbuf_print(pvbuf_paddr);
		exit(1);
	}

	if (l1 != len) {
		fprintf(stderr, "pvbuf length calc mismatch with set length "
				"%lu != %lu\n", l1, len);
		xdp2_pvbuf_print(pvbuf_paddr);
		exit(1);
	}

	return l1;
}

/* Function to check one buf */
static void check_one_buf(void *buf, size_t len, size_t check_offset,
			   size_t prefix_offset)
{
	siphash_key_t key = {
		.key[0] = 0xabcdef019d8d,
		.key[1] = 0xab7de49fe8a6bc0,
	};
	struct iterate_siphash_struct iter = {};
	struct iterate_struct ist = {};
	__u64 v0, v1;

	if (!check_interval || (random() % check_interval))
		return;

	ist.next_val = check_offset;
	ist.index = 0;
	ist.offset = prefix_offset;

	iterate_check_segment(&ist, buf, len);

	v0 = siphash(&data_check[check_offset], len, &key);
	siphash_iter_start(&iter.iter, len, &key);
	iter.offset = prefix_offset;
	siphash_iterate(&iter, buf, len);
	v1 = siphash_iter_end(&iter.iter);

	if (v0 != v1) {
		fprintf(stderr, "buf siphash mismatch: got %llx expected "
				"%llx\n", v0, v1);
		exit(-1);
	}
}

/* Check the data in a pvbuf */
static void check_data(xdp2_paddr_t pvbuf_paddr, size_t len,
		       size_t check_offset, size_t prefix_offset)
{
	siphash_key_t key = {
		.key[0] = 0xabcdef019d8d,
		.key[1] = 0xab7de49fe8a6bc0,
	};
	struct iterate_siphash_struct iter = {};
	struct iterate_struct ist = {};
	__u64 v0, v1;

	/* Check meta information (iovec_map) */
	xdp2_pvbuf_check(pvbuf_paddr, "pvbuf test");

	if (!check_interval || (random() % check_interval))
		return;

	/* Do deep checks */

	/* Do a byte by byte check */
	ist.next_val = check_offset;
	ist.pvbuf_paddr = pvbuf_paddr;
	ist.offset = prefix_offset;
	xdp2_pvbuf_iterate(pvbuf_paddr, iterate_check, &ist);

	/* Check siphash over the data */
	v0 = siphash(&data_check[check_offset], len - prefix_offset, &key);
	siphash_iter_start(&iter.iter, len - prefix_offset, &key);
	iter.offset = prefix_offset;
	xdp2_pvbuf_iterate(pvbuf_paddr, siphash_iterate, &iter);
	v1 = siphash_iter_end(&iter.iter);

	if (v0 != v1) {
		fprintf(stderr, "Siphash mismatch: got %llx expected %llx\n",
			v0, v1);
		xdp2_pvbuf_print(pvbuf_paddr);
		exit(-1);
	}

	if (len) {
		size_t clen, coff;

		/* Check siphash over some random sub-region of the data */
		clen = random() % (len - prefix_offset);
		if (clen) {
			coff = random() % (len - prefix_offset - clen);
			v0 = siphash(&data_check[check_offset + coff], clen,
				     &key);

			ist.pvbuf_paddr = pvbuf_paddr;
			iter.len = clen;
			iter.offset = coff + prefix_offset;

			siphash_iter_start(&iter.iter, clen, &key);
			xdp2_pvbuf_iterate(pvbuf_paddr,
					    siphash_iterate, &iter);
			v1 = siphash_iter_end(&iter.iter);

			if (v0 != v1) {
				fprintf(stderr, "Subset siphash mismatch: "
						"got %llx expected %llx\n",
					v0, v1);
				xdp2_pvbuf_print(pvbuf_paddr);
				exit(-1);
			}
		}
	}
}

struct addr_refcnt {
	size_t offset;
	size_t max; /* offset + size */
	void *addr;
	atomic_uint refcnt;
};

struct addr_mem_region {
	unsigned long *refcnts_bitmap;
	unsigned int num_refcnts;
	void *addr_base;
	struct addr_refcnt refcnts[];
};

struct addr_mem_region *short_addr_mem_regions[3];

static void short_addr_bump_refcnt(struct xdp2_pvbuf_mgr *pvmgr,
				   xdp2_paddr_t paddr)
{
	size_t offset = xdp2_paddr_short_addr_get_offset_from_paddr(paddr);
	struct addr_mem_region *mem_region;
	unsigned int index = 0, mem_reg_num;
	struct addr_refcnt *refcnt;

	mem_reg_num = xdp2_paddr_short_addr_get_mem_region_from_paddr(paddr);
	mem_reg_num--;

	mem_region = short_addr_mem_regions[mem_reg_num];

	xdp2_bitmap_foreach_bit(mem_region->refcnts_bitmap, index,
				mem_region->num_refcnts) {
		refcnt = &mem_region->refcnts[index];

		if (offset < refcnt->offset || offset >= refcnt->max)
			continue;

		atomic_fetch_add(&refcnt->refcnt, 1);

		return;
	}

	XDP2_ERR(1, "Test PVbuf bump refcnt short addr: reference count for "
		    "paddr %llx with offset %lu not found", paddr, offset);
}

static void short_addr_free(struct xdp2_pvbuf_mgr *pvmgr,
			    xdp2_paddr_t paddr)
{
	size_t offset = xdp2_paddr_short_addr_get_offset_from_paddr(paddr);
	unsigned int index = 0, mem_reg_num;
	struct addr_mem_region *mem_region;
	struct addr_refcnt *refcnt;

	mem_reg_num = xdp2_paddr_short_addr_get_mem_region_from_paddr(paddr);
	mem_reg_num--;

	mem_region = short_addr_mem_regions[mem_reg_num];

	xdp2_bitmap_foreach_bit(mem_region->refcnts_bitmap, index,
				mem_region->num_refcnts) {
		refcnt = &mem_region->refcnts[index];

		if (offset < refcnt->offset || offset >= refcnt->max)
			continue;

		if (atomic_fetch_sub(&refcnt->refcnt, 1) != 1)
			return;

		xdp2_bitmap_unset(mem_region->refcnts_bitmap, index);

		xdp2_paddr_short_addr_paddr_to_addr(paddr);

		free(refcnt->addr);

		return;
	}

	XDP2_ERR(1, "Test PVbuf free short addr: reference count for "
		    "paddr %llx with offset %lu not found", paddr, offset);
}

struct xdp2_pvbuf_ops short_addr_ops = {
	.free = short_addr_free,
	.bump_refcnt = short_addr_bump_refcnt,
};

void *global_addr_base;

static xdp2_paddr_t alloc_short_addr(size_t size, void **raddr)
{
	struct addr_mem_region *mem_region;
	unsigned int mem_reg_num = -1U;
	struct addr_refcnt *refcnt;
	unsigned int index;
	xdp2_paddr_t paddr;
	size_t offset;
	void *addr;
	int i;

	addr = malloc(size);
	XDP2_ASSERT(addr, "Test PVbuf bump refcnt short addr: malloc failed");

	offset = (uintptr_t)(addr - global_addr_base);

	/* Check is the offset and size fit into the pooled memory regions */
	for (i = 0; i < 3; i++) {
		size_t start_offset = i * XDP2_PADDR_SHORT_ADDR_MAX_OFFSET;
		size_t limit = start_offset + XDP2_PADDR_SHORT_ADDR_MAX_OFFSET;

		if (offset >= limit)
			continue;

		if (offset + size >= limit)
			break;

		offset -= start_offset;
		mem_reg_num = i;
		break;
	}

	if (mem_reg_num == -1U) {
		/* The offset doesn't fit in to 48TB, or the offset + size
		 * straddle a 16TB boundary
		 */
		free(addr);

		return xdp2_pbuf_alloc(size, raddr);
	}

	mem_region = short_addr_mem_regions[mem_reg_num];

	index = xdp2_bitmap_find_zero(mem_region->refcnts_bitmap, 0,
				      mem_region->num_refcnts);
	refcnt = &mem_region->refcnts[index];

	XDP2_ASSERT(index < mem_region->num_refcnts, "Test PVbuf bump refcnt "
						     "short addr: no free "
						     "refcnts for alloc");

	refcnt->offset = offset;
	refcnt->max = offset + size;
	refcnt->addr = addr;
	atomic_store(&refcnt->refcnt, 1);

	xdp2_bitmap_set(mem_region->refcnts_bitmap, index);

	paddr = xdp2_paddr_short_addr_make_paddr(mem_reg_num + 1, offset);

	*raddr = addr;

	return paddr;
}

struct addr_mem_region *long_addr_mem_regions[64];

static void long_addr_bump_refcnt(struct xdp2_pvbuf_mgr *pvmgr,
				  xdp2_paddr_t paddr_1, xdp2_paddr_t paddr_2)
{
	size_t offset = xdp2_paddr_long_addr_get_offset_from_paddr(paddr_1,
								   paddr_2);
	struct addr_mem_region *mem_region;
	unsigned int index = 0, mem_reg_num;
	struct addr_refcnt *refcnt;

	mem_reg_num = xdp2_paddr_long_addr_get_mem_region_from_paddr(
			paddr_1, paddr_2);
	mem_region = long_addr_mem_regions[mem_reg_num];

	xdp2_bitmap_foreach_bit(mem_region->refcnts_bitmap, index,
				mem_region->num_refcnts) {
		refcnt = &mem_region->refcnts[index];

		if (offset < refcnt->offset || offset >= refcnt->max)
			continue;

		atomic_fetch_add(&refcnt->refcnt, 1);

		return;
	}

	XDP2_ERR(1, "Test PVbuf bump refcnt for long addr: reference count for "
		    "paddr %llx %llx with offset %lu not found",
		    paddr_1, paddr_2, offset);
}

static void long_addr_free(struct xdp2_pvbuf_mgr *pvmgr,
			   xdp2_paddr_t paddr_1, xdp2_paddr_t paddr_2)
{
	size_t offset = xdp2_paddr_long_addr_get_offset_from_paddr(
							paddr_1, paddr_2);
	struct addr_mem_region *mem_region;
	unsigned int index = 0, mem_reg_num;
	struct addr_refcnt *refcnt;

	mem_reg_num = xdp2_paddr_long_addr_get_mem_region_from_paddr(
			paddr_1, paddr_2);
	mem_region = long_addr_mem_regions[mem_reg_num];

	xdp2_bitmap_foreach_bit(mem_region->refcnts_bitmap, index,
				mem_region->num_refcnts) {
		refcnt = &mem_region->refcnts[index];

		if (offset < refcnt->offset || offset >= refcnt->max)
			continue;

		if (atomic_fetch_sub(&refcnt->refcnt, 1) != 1)
			return;

		xdp2_bitmap_unset(mem_region->refcnts_bitmap, index);

		xdp2_paddr_long_paddr_to_addr(paddr_1, paddr_2);

		free(refcnt->addr);

		return;
	}

	XDP2_ERR(1, "Test PVbuf free long addr: reference count for "
		    "paddr %llx %llx with offset %lu not found",
		    paddr_1, paddr_2, offset);
}

struct xdp2_pvbuf_long_ops long_addr_ops = {
	.free = long_addr_free,
	.bump_refcnt = long_addr_bump_refcnt,
};

static void *alloc_long_addr(size_t size, xdp2_paddr_t *paddr_1,
			     xdp2_paddr_t *paddr_2)
{
	struct addr_mem_region *mem_region;
	struct addr_refcnt *refcnt;
	unsigned int mem_reg_num;
	unsigned int index;
	size_t offset;
	void *addr;

	addr = malloc(size);
	XDP2_ASSERT(addr, "Test PVbuf alloc long addr: malloc failed");

	offset = (uintptr_t)(addr - global_addr_base);

	mem_reg_num = 22;

	mem_region = long_addr_mem_regions[mem_reg_num];

	index = xdp2_bitmap_find_zero(mem_region->refcnts_bitmap, 0,
				      mem_region->num_refcnts);
	refcnt = &mem_region->refcnts[index];

	XDP2_ASSERT(index < mem_region->num_refcnts, "Test PVbuf alloc "
						     "long addr: no free "
						     "refcnts for alloc");

	refcnt->offset = offset;
	refcnt->max = offset + size;
	refcnt->addr = addr;
	atomic_store(&refcnt->refcnt, 1);

	xdp2_bitmap_set(mem_region->refcnts_bitmap, index);

	xdp2_paddr_long_addr_make_paddr(mem_reg_num, paddr_1,
					paddr_2, offset);

	return addr;
}

#define NUM_ADDR_REFCNTS 1000

static void init_short_addr_regions(void)
{
	struct addr_mem_region *mem_region;
	size_t bitmap_size = sizeof(unsigned long) *
			XDP2_BITMAP_NUM_BITS_TO_WORDS(NUM_ADDR_REFCNTS);
	size_t size = sizeof(*mem_region) + NUM_ADDR_REFCNTS *
				sizeof(struct addr_refcnt) + bitmap_size;
	int i;

	for (i = 0; i < 3; i++) {
		mem_region = malloc(size);
		XDP2_ASSERT(mem_region, "Malloc short addr memory region %u "
			    "failed", i);

		memset(mem_region, 0, size);

		mem_region->refcnts_bitmap = (unsigned long *)(
				&mem_region->refcnts[NUM_ADDR_REFCNTS]);
		mem_region->num_refcnts = NUM_ADDR_REFCNTS;
		mem_region->addr_base = global_addr_base +
			i * (1ULL << XDP2_PADDR_SHORT_ADDR_OFFSET_BITS);

		short_addr_mem_regions[i] = mem_region;
	}
}

static void init_long_addr_regions(void)
{
	struct addr_mem_region *mem_region;
	size_t bitmap_size = sizeof(unsigned long) *
			XDP2_BITMAP_NUM_BITS_TO_WORDS(NUM_ADDR_REFCNTS);
	size_t size = sizeof(*mem_region) + NUM_ADDR_REFCNTS *
				sizeof(struct addr_refcnt) + bitmap_size;
	int i;

	for (i = 0; i < 64; i++) {
		mem_region = malloc(size);
		XDP2_ASSERT(mem_region, "Malloc long addr memory region %u "
			    "failed", i);

		memset(mem_region, 0, size);

		mem_region->refcnts_bitmap = (unsigned long *)(
				&mem_region->refcnts[NUM_ADDR_REFCNTS]);
		mem_region->num_refcnts = NUM_ADDR_REFCNTS;
		mem_region->addr_base = global_addr_base;

		long_addr_mem_regions[i] = mem_region;
	}
}

static void test_short_addr_regions(void)
{
	xdp2_paddr_t paddr;
	void *buf;

	paddr = alloc_short_addr(1000, &buf);

	short_addr_bump_refcnt(&xdp2_pvbuf_global_mgr, paddr);

	short_addr_free(&xdp2_pvbuf_global_mgr, paddr);
	short_addr_free(&xdp2_pvbuf_global_mgr, paddr);

	/* Uncomment out to test double free */
#if 0
	short_addr_free(&xdp2_pvbuf_global_mgr, paddr);
#endif
}

static void test_long_addr_regions(void)
{
	xdp2_paddr_t paddr_1, paddr_2;

	alloc_long_addr(1000, &paddr_1, &paddr_2);

	long_addr_bump_refcnt(&xdp2_pvbuf_global_mgr, paddr_1, paddr_2);

	long_addr_free(&xdp2_pvbuf_global_mgr, paddr_1, paddr_2);
	long_addr_free(&xdp2_pvbuf_global_mgr, paddr_1, paddr_2);
	/* Uncomment out to test double free */
#if 0
	long_addr_free(&xdp2_pvbuf_global_mgr, paddr_1, paddr_2);
#endif
}

static void *alloc_buffer(size_t count, xdp2_paddr_t *paddr_1,
			  xdp2_paddr_t *paddr_2)
{
	unsigned int sel;
	void *addr;

	/* Randomly alloc one of the three buffer types */
	sel = rand() % 3;
	switch (sel) {
	case 0:
		return alloc_long_addr(count, paddr_1, paddr_2);
	case 1:
		if (count > XDP2_PADDR_SHORT_ADDR_MAX_DATA_LENGTH || !count) {
			*paddr_1 = xdp2_pbuf_alloc(count, &addr);
			return addr;
		}

		*paddr_1 = alloc_short_addr(count, &addr);
		break;
	case 2:
		*paddr_1 = xdp2_pbuf_alloc(count, &addr);

		XDP2_ASSERT(*paddr_1, "Alloc %llx failed for size %lu\n",
			    *paddr_1, count);
		break;
	}
	return addr;
}

/* Pop headers and verify the result */
static bool pop_headers(xdp2_paddr_t pvbuf_paddr, size_t *check_offset,
			size_t count, size_t *len, bool compress)
{
	size_t len1 = *len - count;

	xdp2_pvbuf_pop_hdrs(pvbuf_paddr, count, compress);
	*len = get_pvbuf_length(pvbuf_paddr, len1, "pop headers");

	*check_offset += count;

	check_data(pvbuf_paddr, *len, *check_offset, 0);

	return true;
}

/* Pop trailers and verify the result */
static bool pop_trailers(xdp2_paddr_t pvbuf_paddr, size_t *check_offset,
			 size_t count, size_t *len,
			 bool compress)
{
	size_t len1 = *len - count;

	xdp2_pvbuf_pop_trailers(pvbuf_paddr, count, compress);
	*len = get_pvbuf_length(pvbuf_paddr, len1, "pop trailers");

	check_data(pvbuf_paddr, *len, *check_offset, 0);

	return true;
}

/* Push headers and verify the result */
static bool push_headers(xdp2_paddr_t pvbuf_paddr, size_t *check_offset,
			 size_t count, size_t *len, bool compress,
			 bool clone)
{
	struct xdp2_pvbuf *pvbuf2;
	xdp2_paddr_t pvbuf2_paddr;
	size_t len1, retlen;
	bool ret = true;

	if (clone) {
		pvbuf2_paddr = xdp2_pvbuf_clone(data_check_paddr,
						 *check_offset - count, count,
						 &retlen);
	} else {
		pvbuf2_paddr = xdp2_pvbuf_alloc(count, &pvbuf2);
		if (!pvbuf2_paddr)
			return false;

		set_pvbuf_data(pvbuf2_paddr, *check_offset - count);
	}

	if (!xdp2_pvbuf_prepend_pvbuf(pvbuf_paddr, pvbuf2_paddr, count,
				       compress)) {
		if (verbose >= 17)
			printf("Prepend failed\n");
		len1 = *len;
		xdp2_pvbuf_free(pvbuf2_paddr);
		ret = false;
	} else {
		*check_offset -= count;
		len1 = *len + count;
	}

	*len = get_pvbuf_length(pvbuf_paddr, len1, "push headers");

	check_data(pvbuf_paddr, *len, *check_offset, 0);

	return ret;
}

/* Push headers from a buf and verify the result */
static bool push_headers_from_buf(xdp2_paddr_t pvbuf_paddr,
				  size_t *check_offset, size_t count,
				  size_t *len, bool compress)
{
	xdp2_paddr_t paddr_1, paddr_2;
	bool ret = true;
	size_t len1;
	void *buf;

	buf = alloc_buffer(count, &paddr_1, &paddr_2);
	if (!buf)
		return false;

	set_buf_data(buf, count, *check_offset - count);

	if (!xdp2_pvbuf_prepend_paddr(pvbuf_paddr, paddr_1, paddr_2, count,
				      compress)) {
		if (verbose >= 17)
			printf("Prepend buf failed\n");
		len1 = *len;
		xdp2_paddr_free(paddr_1, paddr_2);
		ret = false;
	} else {
		len1 = *len + count;
		*check_offset -= count;
	}

	*len = get_pvbuf_length(pvbuf_paddr, len1, "push headers from paddr");

	check_data(pvbuf_paddr, *len, *check_offset, 0);

	return ret;
}

/* Append trailers and verify the result */
static bool append_trailers(xdp2_paddr_t pvbuf_paddr, size_t *check_offset,
			    size_t count, size_t *len, bool compress,
			    bool clone)
{
	size_t len1 = *len + count, retlen;
	struct xdp2_pvbuf *pvbuf2;
	xdp2_paddr_t pvbuf2_paddr;
	bool ret = true;

	if (clone) {
		pvbuf2_paddr = xdp2_pvbuf_clone(data_check_paddr,
						 *check_offset + *len, count,
						 &retlen);
	} else {
		pvbuf2_paddr = xdp2_pvbuf_alloc(count, &pvbuf2);
		if (!pvbuf2_paddr)
			return false;

		set_pvbuf_data(pvbuf2_paddr, *check_offset + *len);
	}

	if (!xdp2_pvbuf_append_pvbuf(pvbuf_paddr, pvbuf2_paddr, count,
				      compress)) {
		if (verbose >= 17)
			printf("Append pvbuf failed\n");
		len1 = *len;
		ret = false;
		xdp2_pvbuf_free(pvbuf2_paddr);
	} else {
		len1 = *len + count;
	}

	*len = get_pvbuf_length(pvbuf_paddr, len1, "push trailers");

	check_data(pvbuf_paddr, *len, *check_offset, 0);

	return ret;
}

/* Pullup header bytes into a contiguous buffer and verify the result */
static bool pullup(xdp2_paddr_t pvbuf_paddr, size_t *check_offset,
		   size_t count, size_t *len, bool compress)
{
	bool ret = true;
	void *buf;
	int i;

	buf = xdp2_pvbuf_pullup(pvbuf_paddr, count, compress);
	if (!buf) {
		size_t nlen = xdp2_pvbuf_calc_length(pvbuf_paddr);

		if (verbose >= 17)
			printf("Pullup pvbuf failed\n");
		if (nlen < *len) {
			if (verbose == 20)
				printf("Pullup pvbuf failed with less length "
				       "%lu bytes\n", *len - nlen);
			*check_offset += *len - nlen;
			*len = nlen;
		} else if (nlen > *len) {
			fprintf(stderr, "Bad fail length\n");
			exit(1);
		}
		ret = false;
	} else {
		struct xdp2_pvbuf *pvbuf =
				xdp2_pvbuf_paddr_to_addr(pvbuf_paddr);

		check_data(pvbuf_paddr, *len, *check_offset, 0);

		i = xdp2_pvbuf_iovec_map_find(pvbuf);
		if (i < XDP2_PVBUF_MAX_IOVECS) {
			struct xdp2_iovec *iovec = &pvbuf->iovec[i];
			size_t plen;
			void *data;

			__xdp2_pvbuf_get_ptr_len_from_iovec(
					&xdp2_pvbuf_global_mgr, iovec,
					&data, &plen,
					(i == XDP2_PVBUF_MAX_IOVECS - 1));

			if (plen < count && *len != plen) {
				xdp2_pvbuf_print(pvbuf_paddr);
				fprintf(stderr, "Bad first buf length after "
						"pullup: buf length %lu < "
						"count %lu and whole length "
						"%lu != buf length %lu\n",
					plen, count, *len, plen);
				exit(1);
			}
		}
	}

	*len = get_pvbuf_length(pvbuf_paddr, *len, "pullup");
	check_data(pvbuf_paddr, *len, *check_offset, 0);

	return ret;
}

/* Pullup tail bytes into a contiguous buffer and verify the result */
static bool pulltail(xdp2_paddr_t pvbuf_paddr, size_t check_offset,
		     size_t count, size_t *len,
		     bool compress)
{
	bool ret = true;
	void *buf;

	*len = get_pvbuf_length(pvbuf_paddr, *len, "pullup");
	buf = xdp2_pvbuf_pulltail(pvbuf_paddr, count, compress);
	if (!buf) {
		size_t nlen = xdp2_pvbuf_calc_length(pvbuf_paddr);

		if (verbose >= 17)
			printf("Pulltail pvbuf failed\n");
		if (nlen < *len) {
			if (verbose >= 20)
				printf("Pulltail pvbuf failed with less "
				       "length %lu bytes\n", *len - nlen);
			*len = nlen;
		} else if (nlen > *len) {
			fprintf(stderr, "Bad fail length\n");
			exit(1);
		}
		ret = false;
	}

	*len = get_pvbuf_length(pvbuf_paddr, *len, "pullup");

	check_data(pvbuf_paddr, *len, check_offset, 0);

	return ret;
}

/* Iterator callback to check a packet (pvpkt) */
static bool check_one_pkt(void *priv, struct xdp2_iovec *iovec)
{
	struct iterate_struct *ist = (struct iterate_struct *)priv;
	size_t len;
	void *addr;

	if (xdp2_iovec_paddr_addr_tag(iovec) == XDP2_PADDR_TAG_PVBUF) {
		len = xdp2_pvbuf_get_data_len_from_iovec(iovec);

		check_data(iovec->paddr, len, ist->next_val, ist->offset);

		ist->next_val += len - ist->offset;
	} else {
		xdp2_pvbuf_get_ptr_len_from_iovec(iovec, &addr,
						  &len, false);
		check_one_buf(addr, len, ist->next_val, ist->offset);
	}

	return true;
}

/* Check segmented data */
static void check_segment_data(xdp2_paddr_t pvbuf_paddr, size_t len,
			       size_t check_offset, size_t prefix_offset)
{
	struct iterate_struct ist = {};

	ist.next_val = check_offset;
	ist.offset = prefix_offset;

	xdp2_pvpkt_iterate(pvbuf_paddr, check_one_pkt, &ist);

	xdp2_pvbuf_check(pvbuf_paddr, "pvbuf test");

	if (!check_interval || (random() % check_interval) || prefix_offset)
		return;

	check_data(pvbuf_paddr, len, check_offset, 0);
}

/* Segment a pvbuf and verify the result */
static bool segment_pvbuf(xdp2_paddr_t pvbuf_paddr, size_t check_offset,
			  size_t count, size_t segment_size, size_t *len)
{
	xdp2_paddr_t paddr;
	bool ret = true;
	size_t retlen;

	*len = get_pvbuf_length(pvbuf_paddr, *len, "pullup");
	paddr = xdp2_pvpkt_segment(pvbuf_paddr, 0, count, segment_size,
				    &retlen);

	check_data(paddr, *len, check_offset, 0);

	xdp2_pvbuf_free(paddr);

	return ret;
}

/* Segment callback (append a header buffer) */
static ssize_t segment_cb(xdp2_paddr_t paddr, void *priv)
{
	xdp2_paddr_t paddr_1, paddr_2;
	void *buf;

	buf = alloc_buffer(128, &paddr_1, &paddr_2);

	if (!buf)
		return -ENOMEM;

	if (!xdp2_pvbuf_prepend_paddr(paddr, paddr_1, paddr_2, 128, false)) {
		if (verbose >= 17)
			printf("Prepend buf failed in segment cb\n");
		return -ENOMEM;
	}

	return 128;
}

/* Segment a pvbuf and prepend a buf and verify the result */
static bool segment_pvbuf_with_prefix(xdp2_paddr_t pvbuf_paddr,
				      size_t check_offset, size_t count,
				      size_t segment_size, size_t *len)
{
	xdp2_paddr_t paddr;
	bool ret = true;
	size_t retlen;

	*len = get_pvbuf_length(pvbuf_paddr, *len, "segment with prefix");
	paddr = xdp2_pvpkt_segment_cb(pvbuf_paddr, 0, count, segment_size,
				       segment_cb, NULL, &retlen);

	XDP2_ASSERT(paddr, "xdp2_pvpkt_segment_cb returned null");
	check_segment_data(paddr, *len, check_offset, 128);

	xdp2_pvbuf_free(paddr);

	return ret;
}

/* Append trailers from buf and verify the result */
static bool append_trailers_from_buf(xdp2_paddr_t pvbuf_paddr,
				     size_t *check_offset,
				     size_t count, size_t *len,
				     bool compress)
{
	xdp2_paddr_t paddr_1, paddr_2;
	bool ret = true;
	size_t len1;
	void *buf;

	buf = alloc_buffer(count, &paddr_1, &paddr_2);
	if (!buf)
		return false;

	set_buf_data(buf, count, *check_offset + *len);

	if (!xdp2_pvbuf_append_paddr(pvbuf_paddr, paddr_1, paddr_2, count,
				     compress)) {
		if (verbose >= 17)
			printf("Append buf failed\n");
		len1 = *len;
		xdp2_paddr_free(paddr_1, paddr_2);
		ret = false;
	} else {
		len1 = *len + count;
	}
	*len = get_pvbuf_length(pvbuf_paddr, len1, "append headers from buf");

	check_data(pvbuf_paddr, *len, *check_offset, 0);

	return ret;
}

#define NUM_OPS 10

struct buf_rec {
	struct xdp2_pvbuf *pvbuf;
	xdp2_paddr_t pvbuf_paddr;
	size_t check_offset;
	size_t len;
};

struct {
	unsigned long count;
	unsigned long fails;
} op_counts[NUM_OPS];

const char *opnames[NUM_OPS] = { "pop headers", "prepend headers",
				 "append trailers", "pop trailers",
				 "push buf headers",
				 "push buf trailers",
				 "pullup headers", "pullup trailers",
				 "segment pvbuf", "segment pvbuf prefix" };

/* Print op counts */
static void print_op_counts(void *cli)
{
	int i;

	XDP2_CLI_PRINT(cli, "Operation counts:\n");

	for (i = 0; i < NUM_OPS; i++)
		XDP2_CLI_PRINT(cli, "\t%s: %lu (success: %lu, fails: %lu)\n",
				opnames[i], op_counts[i].count,
				op_counts[i].count - op_counts[i].fails,
				op_counts[i].fails);
}

static void print_op_counts_cli(void *cli, struct xdp2_cli_thread_info *info,
				const char *arg)
{
	print_op_counts(cli);
}

XDP2_CLI_ADD_SHOW_CONFIG("op-counts", print_op_counts_cli, 0xffff);

/* Main test function */
static void test_pvbuf(unsigned int maxlen, unsigned long count,
		       unsigned int num_buffers, unsigned int report_itvl,
		       size_t max_buf_size, bool compress)
{
	struct buf_rec *buf_recs;
	unsigned long i;

	buf_recs = malloc(num_buffers * sizeof(*buf_recs));
	if (!buf_recs) {
		fprintf(stderr, "Malloc pvbufs pointers failed\n");
		exit(1);
	}

	if (verbose >= 5) {
		printf("Buffer manager at start\n");
		xdp2_pvbuf_show_buffer_manager(NULL);
		if (verbose >= 7) {
			printf("Buffer manager details\n");
			xdp2_pvbuf_show_buffer_manager_details(NULL);
		}
	}

	/* Create the initial pvbufs */
	for (i = 0; i < num_buffers; i++) {
		buf_recs[i].pvbuf_paddr = xdp2_pvbuf_alloc_params(
				maxlen / 2, 32768, 2,
				&buf_recs[i].pvbuf);
		if (!buf_recs[i].pvbuf_paddr) {
			fprintf(stderr, "Alloc initial pvbufs failed\n");
			exit(1);
		}

		buf_recs[i].len = maxlen / 2;
		buf_recs[i].check_offset = maxlen / 2;

		/* Set the contents */
		set_pvbuf_data(buf_recs[i].pvbuf_paddr,
			       buf_recs[i].check_offset),

		/* Validate the initial pvbuf */
		check_data(buf_recs[i].pvbuf_paddr, buf_recs[i].len,
			   buf_recs[i].check_offset, 0);
		if (verbose >= 20) {
			printf("Initial pvbuf %lu\n", i);
			xdp2_pvbuf_print(buf_recs[i].pvbuf_paddr);
		}

	}

	for (i = 0; i < count; i++) {
		unsigned int pind = rand() % num_buffers;
		unsigned int op = rand() % NUM_OPS;
		xdp2_paddr_t pvbuf_paddr;
		unsigned int tcnt;
		bool ret;

		if (check_at && i >= check_at)
			check_interval = 1;

		if (do_show_buffer) {
			int i;

			for (i = 0; i < num_buffers; i++)
				xdp2_pvbuf_print(buf_recs[i].pvbuf_paddr);

			do_show_buffer = false;
		}

		pvbuf_paddr = buf_recs[pind].pvbuf_paddr;

		if (report_itvl && !(i % report_itvl)) {
			printf("Iter: %lu\n", i);
			if (verbose >= 7)
				xdp2_pvbuf_show_buffer_manager(NULL);
			if (verbose >= 10)
				xdp2_pvbuf_print(pvbuf_paddr);
		}

		switch (op) {
		case 0: /* Pop headers up to length */
			tcnt = rand() % (buf_recs[pind].len + 1);

			if (verbose >= 5)
				printf("Operation: %s\n", opnames[op]);

			ret = pop_headers(pvbuf_paddr,
					  &buf_recs[pind].check_offset,
					  tcnt, &buf_recs[pind].len, compress);

			break;
		case 1: /* Push headers up to check_offset */
			if (!buf_recs[pind].check_offset)
				continue;
			tcnt = rand() % buf_recs[pind].check_offset + 1;

			if (verbose >= 5)
				printf("Operation: %s\n", opnames[op]);

			ret = push_headers(pvbuf_paddr,
					   &buf_recs[pind].check_offset,
					   tcnt, &buf_recs[pind].len,
					   compress, true);
			break;
		case 2: /* Append headers up to max - check_offset - len */
			if (buf_recs[pind].check_offset +
						buf_recs[pind].len == maxlen)
				continue;
			if (!((maxlen - buf_recs[pind].check_offset -
							buf_recs[pind].len)))
				continue;

			if (verbose >= 5)
				printf("Operation: %s\n", opnames[op]);

			tcnt = rand() % (maxlen - buf_recs[pind].check_offset -
							buf_recs[pind].len) + 1;

			ret = append_trailers(pvbuf_paddr,
					      &buf_recs[pind].check_offset,
					      tcnt, &buf_recs[pind].len,
					      compress, true);
			break;
		case 3: /* Pop trailers up to length */
			tcnt = rand() % (buf_recs[pind].len + 1);

			if (verbose >= 5)
				printf("Operation: %s\n", opnames[op]);

			ret = pop_trailers(pvbuf_paddr,
					   &buf_recs[pind].check_offset,
					   tcnt, &buf_recs[pind].len, compress);
			break;
		case 4: { /* Push headers up to check_offset from buf */
			size_t range =
				xdp2_min(buf_recs[pind].check_offset + 1,
						 max_buf_size + 1);

			/* A buf with length 1 << 20 is a special case where
			 * iovec length is zero and pvbuf address tag is 15.
			 * Make sure to test it
			 */
			if (range == (1 << 20) + 1 && !(rand() % 100))
				tcnt = 1 << 20;
			else
				tcnt = rand() % xdp2_min(
					buf_recs[pind].check_offset + 1,
							  max_buf_size + 1);

			if (!tcnt)
				continue;

			if (verbose >= 5)
				printf("Operation: %s\n", opnames[op]);

			ret = push_headers_from_buf(pvbuf_paddr,
					&buf_recs[pind].check_offset,
					tcnt, &buf_recs[pind].len, compress);
			break;
		}
		case 5: { /* Append headers up to check_offset from buf */
			size_t range = xdp2_min(maxlen -
					buf_recs[pind].check_offset -
							buf_recs[pind].len + 1,
					max_buf_size + 1);

			/* A buf with length 1 << 20 is a special case where
			 * iovec length is zero and pvbuf address tag is 15.
			 * Make sure to test it
			 */
			if (range == (1 << 20) + 1 && !(rand() % 100))
				tcnt = 1 << 20;
			else
				tcnt = rand() % range;

			if (!tcnt)
				continue;

			if (verbose >= 5)
				printf("Operation: %s\n", opnames[op]);

			ret = append_trailers_from_buf(pvbuf_paddr,
					&buf_recs[pind].check_offset,
					tcnt, &buf_recs[pind].len, compress);
			break;
		}
		case 6: { /* Pullup headers */
			size_t range = xdp2_min(buf_recs[pind].len + 1,
						 max_buf_size + 1);

			if (!buf_recs[pind].len)
				continue;

			tcnt = rand() % range;

			if (verbose >= 5)
				printf("Operation: %s\n", opnames[op]);

			ret = pullup(pvbuf_paddr, &buf_recs[pind].check_offset,
				     tcnt, &buf_recs[pind].len, compress);

			break;
		}
		case 7: { /* Pulltail trailer */
			size_t range = xdp2_min(buf_recs[pind].len + 1,
						 max_buf_size + 1);

			if (!buf_recs[pind].len)
				continue;

			tcnt = rand() % range;

			if (verbose >= 5)
				printf("Operation: %s\n", opnames[op]);

			ret = pulltail(pvbuf_paddr, buf_recs[pind].check_offset,
				       tcnt, &buf_recs[pind].len, compress);

			break;
		}
		case 8: { /* Segment pvbuf */
			unsigned int num_segs = (random() % 100) + 1;
			size_t segment_size = buf_recs[pind].len / num_segs;

			if (!segment_size)
				continue;

			if (verbose >= 5)
				printf("Operation: %s\n", opnames[op]);

			ret = segment_pvbuf(pvbuf_paddr,
					    buf_recs[pind].check_offset,
					    buf_recs[pind].len,
					    segment_size, &buf_recs[pind].len);
			break;
		}
		case 9: { /* Segment pvbuf */
			unsigned int num_segs = (random() % 100) + 1;
			size_t segment_size = buf_recs[pind].len / num_segs;

			if (!segment_size)
				continue;

			if (verbose >= 5)
				printf("Operation: %s\n", opnames[op]);

			ret = segment_pvbuf_with_prefix(pvbuf_paddr,
					    buf_recs[pind].check_offset,
					    buf_recs[pind].len,
					    segment_size, &buf_recs[pind].len);
			break;
		}
		default:
			XDP2_ERR(1, "Not supposed to get here");
		}
		if (verbose >= 20)
			xdp2_pvbuf_print(buf_recs[pind].pvbuf_paddr);

		if (!ret)
			op_counts[op].fails++;

		op_counts[op].count++;
	}

	if (verbose >= 10)
		for (i = 0; i < num_buffers; i++) {
			printf("PVbuf %lu\n", i);
			xdp2_pvbuf_print(buf_recs[i].pvbuf_paddr);
		}

	for (i = 0; i < num_buffers; i++)
		xdp2_pvbuf_free(buf_recs[i].pvbuf_paddr);

	if (verbose >= 5) {
		XDP2_CLI_PRINT(NULL, "Buffer manager at end\n");
		xdp2_pvbuf_show_buffer_manager(NULL);
		print_op_counts(NULL);
	}
}

static struct xdp2_pbuf_init_allocator config_num_pbufs;
static struct xdp2_pvbuf_init_allocator config_num_pvbufs;
static struct xdp2_pbuf_init_allocator config_num_pbufs_default;

#define SET_PBUFS_ALLOCATOR(SIZE)					\
	config_num_pbufs_default.obj[					\
				XDP2_LOG_32BITS(SIZE) - 6].num_objs =	\
		PVBUF_TEST_PVBUF_SIZE_##SIZE(&config);

static struct xdp2_pvbuf_init_allocator config_num_pvbufs_default;

#define SET_PVBUFS_ALLOCATOR(SIZE)					\
	config_num_pvbufs_default.obj[SIZE - 1].num_pvbufs =		\
		PVBUF_TEST_NUM_PVBUFS_##SIZE(&config);

static void init_global_pvbuf(struct pvbuf_config *config,
			      bool random_pvbuf_size, bool alloc_one_ref)
{
	struct xdp2_short_addr_config short_addr_config;
	struct xdp2_long_addr_config long_addr_config;
	int i;

	for (i = 0; i < 3; i++) {
		short_addr_config.mem_region[i].ops = short_addr_ops;
		short_addr_config.mem_region[i].base = global_addr_base +
				i * XDP2_PADDR_SHORT_ADDR_MAX_OFFSET;
	}

	for (i = 0; i < 64; i++) {
		long_addr_config.mem_region[i].ops = long_addr_ops;
		long_addr_config.mem_region[i].base = global_addr_base;
	}

	xdp2_pvbuf_init(&config_num_pbufs, &config_num_pvbufs,
			random_pvbuf_size, alloc_one_ref,
			&short_addr_config, &long_addr_config);
}

#define ARGS "c:v:I:C:RX:Pi:M:N:rOj:"

static void usage(char *prog)
{
	fprintf(stderr, "Usage: %s [ -c <test-count> ] [ -v <verbose> ]\n",
		prog);
	fprintf(stderr, "\t[ -I <report-interval> ][ -C <cli_port_num> ]\n");
	fprintf(stderr, "\t[-R] [ -X <config-string> ]\n");
	fprintf(stderr, "\t[-P] [ -i <check-interval> ]\n");
	fprintf(stderr, "\t[ -M <maxlen>] [ -N <num_test_buffers> [-r] [-O]\n");

	exit(1);
}

#define PBUF_CONFIG_STRING					\
	"pvbuf_size_128=100000 pvbuf_size_2048=10000 "		\
	"pvbuf_size_4096=10000 pvbuf_size_65536=1000 "		\
	"pvbuf_size_131072=1000 pvbuf_size_262144=1000 "	\
	"pvbuf_size_524288=1000 pvbuf_size_1048576=1000"

#define PVBUF_CONFIG_STRING					\
	"num_pvbufs_1=1000 num_pvbufs_2=1000 "			\
	"num_pvbufs_3=1000 num_pvbufs_4=1000 "			\
	"num_pvbufs_5=1000 num_pvbufs_6=1000 "			\
	"num_pvbufs_7=1000 num_pvbufs_8=1000 "			\
	"num_pvbufs_9=1000 num_pvbufs_10=1000 "			\
	"num_pvbufs_11=1000 num_pvbufs_12=1000 "		\
	"num_pvbufs_13=1000 num_pvbufs_14=1000 "		\
	"num_pvbufs_15=1000 num_pvbufs_16=1000"

int main(int argc, char *argv[])
{
	bool random_pvbuf_size = false, alloc_one_ref = false;
	static struct xdp2_cli_thread_info cli_thread_info;
	size_t max_pbuf_size = 0, maxlen = MAX_MAX_LEN;
	bool table_set = false, compress = false;
	unsigned int cli_port_num = 0;
	unsigned int num_buffers = 1;
	unsigned int report_itvl = 0;
	char *config_string = NULL;
	unsigned long count = 1000;
	bool random_seed = false;
	ssize_t dsize;
	int c, i;
	char *p;

	global_addr_base = sbrk(0);

	PVBUF_TEST_set_default_config(&config);

	while ((c = getopt(argc, argv, ARGS)) != -1) {
		switch (c) {
		case 'c':
			count = strtoull(optarg, NULL, 0);
			break;
		case 'v':
			verbose = strtoul(optarg, NULL, 0);
			break;
		case 'I':
			report_itvl = strtoul(optarg, NULL, 0);
			break;
		case 'C':
			cli_port_num = strtoul(optarg, NULL, 0);
			break;
		case 'R':
			random_seed = true;
			break;
		case 'P':
			compress = true;
			break;
		case 'i':
			check_interval = strtoul(optarg, NULL, 0);
			break;
		case 'j':
			check_at = strtoul(optarg, NULL, 0);
			break;
		case 'M':
			maxlen = strtoul(optarg, NULL, 0);
			if (maxlen > MAX_MAX_LEN) {
				fprintf(stderr, "Maximum length is too big\n");
				exit(1);
			}
			break;
		case 'N':
			num_buffers = strtoul(optarg, NULL, 0);

			if (!num_buffers) {
				fprintf(stderr, "number of buffers must be "
						"at least one\n");
				exit(1);
			}
			break;
		case 'X':
			config_string = optarg;
			break;
		case 'r':
			random_pvbuf_size = true;
			break;
		case 'O':
			alloc_one_ref = true;
			break;
		default:
			usage(argv[0]);
			exit(1);
		}
	}

	if (random_seed)
		srand(time(NULL));

	p = strdup(PBUF_CONFIG_STRING);
	xdp2_config_parse_options_with_defaults(&pvbuf_config_table,
						 &config, p);
	p = strdup(PVBUF_CONFIG_STRING);
	xdp2_config_parse_options(&pvbuf_config_table, &config, p);
	xdp2_config_parse_options(&pvbuf_config_table, &config,
				  config_string);

	XDP2_PVBUF_INIT_FROM_CONFIG(PVBUF_TEST_, &config);

	XDP2_PMACRO_APPLY_ALL(SET_PBUFS_ALLOCATOR, 64, 128, 256, 512,
			       1024, 2048, 4096, 8192, 16384, 32768,
			       65536, 131072, 262144, 524288, 1048576)

	XDP2_PMACRO_APPLY_ALL(SET_PVBUFS_ALLOCATOR, 1, 2, 3, 4, 5, 6, 7,
			       8, 9, 10, 11, 12, 13, 14, 15, 16)

	if (!table_set) {
		memcpy(&config_num_pbufs, &config_num_pbufs_default,
		       sizeof(config_num_pbufs));
		memcpy(&config_num_pvbufs, &config_num_pvbufs_default,
		       sizeof(config_num_pvbufs));
	}

	for (i = 0; i < ARRAY_SIZE(config_num_pbufs.obj); i++)
		if (config_num_pbufs.obj[i].num_objs)
			max_pbuf_size = 1 << (i + 6);

	init_global_pvbuf(NULL, random_pvbuf_size, alloc_one_ref);

	init_short_addr_regions();
	init_long_addr_regions();

	test_short_addr_regions();
	test_long_addr_regions();

	for (i = 0; i < sizeof(data_check); i++)
		data_check[i] = rand();

#if 0
	/* Allocate pvbuf populated with pbufs */
	data_check_paddr = xdp2_pvbuf_alloc(sizeof(data_check),
					    &data_check_pvbuf);
#else
	/* Allocate zero length pvbuf and append buffers to fill out
	 * the length
	 */
	data_check_paddr = xdp2_pvbuf_alloc(0, &data_check_pvbuf);

	XDP2_ASSERT(data_check_paddr, "PVbuf alloca failed");
	for (dsize = sizeof(data_check); dsize > 0;
			dsize -= XDP2_PADDR_SHORT_ADDR_MAX_DATA_LENGTH - 1) {
		size_t alloc_size = xdp2_min(dsize,
				XDP2_PADDR_SHORT_ADDR_MAX_DATA_LENGTH);
		xdp2_paddr_t paddr_1, paddr_2;

		alloc_buffer(alloc_size, &paddr_1, &paddr_2);
		xdp2_pvbuf_append_paddr(data_check_paddr, paddr_1,
					paddr_2, alloc_size, false);
	}
#endif
	if (!data_check_paddr) {
		fprintf(stderr, "Alloc data_check pvbuf failed\n");
		exit(1);
	}
	set_pvbuf_data(data_check_paddr, 0);

	if (cli_port_num) {
		cli_thread_info.port_num = cli_port_num;
		cli_thread_info.classes = -1U;
		cli_thread_info.label = "pvbuf_test";
		cli_thread_info.major_num = 0xff;
		cli_thread_info.minor_num = 0xff;

		xdp2_cli_start(&cli_thread_info);
	}

	test_pvbuf(maxlen, count, num_buffers, report_itvl,
		   max_pbuf_size, compress);
}
