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

#include <arpa/inet.h>

#include "router.h"
#include "test.h"

XDP2_DTABLE_LPM_TABLE_DEFINE(router_lpm_table, NULL, ())

XDP2_DTABLE_LPM_TABLE_SKEY_DEFINE(router_lpm_skey_table, NULL, ())

struct {
	in_addr_t prefix;
	__u8 prefix_len;
	struct next_hop nh;
} routes[] = {
	{ .prefix = __cpu_to_be32(0xc0002f00),
	  .prefix_len = 24,
	  .nh.edest = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 },
	  .nh.port = 2,
	},
	{ .prefix = __cpu_to_be32(0x0a00020f),
	  .prefix_len = 32,
	  .nh.edest = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 },
	  .nh.port = 3,
	},
};

void init_routes(void)
{
	struct router_lpm_table_key_struct key;
	int i;

	for (i = 0; i < ARRAY_SIZE(routes); i++) {
		key.src  = routes[i].prefix;
		router_lpm_table_add(&key, 24, &routes[i].nh);

		key.src  = routes[i].prefix;
		router_lpm_table_add(&key, 24, &routes[i].nh);
		router_lpm_skey_table_add(&routes[i].prefix,
					  24, &routes[i].nh);

#if 0
		XDP2_DTABLE_ADD_LPM(router_lpm_table, (routes[i].prefix),
				    24, &routes[i].nh);
#endif
	}
}
