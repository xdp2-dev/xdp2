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

#include <linux/if_ether.h>
#include <linux/ipv6.h>
#include <linux/types.h>
#include <netinet/ip.h>
#include <stdio.h>

#include "xdp2/dtable.h"

#include "test_table.h"

static struct xdp2_dtable_tern_table  *eth_tern, *ipv4_tern;

XDP2_DTABLE_TERN_TABLE(
	eth_dxtable_tern,
	struct ethhdr *,
	(
		h_dest,
		h_source,
		h_proto
	), __u32, MISS, ()
)

XDP2_DTABLE_TERN_TABLE_NAME(
	eth_dxtable_tern_name,
	struct ethhdr *,
	(
		(h_dest, dst),
		(h_source, src),
		(h_proto, proto)
	), __u32, MISS, ()
)

XDP2_DTABLE_TERN_TABLE_CAST(
	ipv4_dxtable_tern_cast,
	struct iphdr *,
	(
		(ihl, (__u8)),
		(tos,),
		(saddr,),
		(daddr,),
		(ttl,),
		(protocol,)
	), __u32, MISS, ()
)

XDP2_DTABLE_TERN_TABLE_NAME_CAST(
	ipv4_dxtable_tern_name_cast,
	struct iphdr *,
	(
		(ihl, mihl, (__u8)),
		(tos, mtos,),
		(saddr, src,),
		(daddr, dst,),
		(ttl, mttl,),
		(protocol, proto,)
	), __u32, MISS, ()
)

XDP2_DTABLE_TERN_TABLE_SKEY(
	eth_dxtable_tern_skey, struct ethhdr *, __u32, MISS, ()
)

static void init_tables(void)
{
	int ident;
	__u32 v;

	XDP2_DTABLE_ADD_TERN(eth_dxtable_tern,
			 (ETH_DST_KEY1, ETH_SRC_KEY1, ETH_PROTO_KEY1),
			 (ETH_DST_KEY1, ETH_SRC_KEY1, ETH_PROTO_KEY1),
			 200, 2);

	{
		struct eth_dxtable_tern_key_struct key =
			{ ETH_DST_KEY0, ETH_SRC_KEY0, ETH_PROTO_KEY0 };

		struct eth_dxtable_tern_key_struct key_mask =
			{ ETH_DST_KEY0, ETH_SRC_KEY0, ETH_PROTO_KEY0 };

		eth_dxtable_tern_add(&key, &key_mask, 100, 1);
	}
	{
		struct eth_dxtable_tern_name_key_struct key =
			{ ETH_DST_KEY0, ETH_SRC_KEY0, ETH_PROTO_KEY0 };

		struct eth_dxtable_tern_name_key_struct key_mask =
			{ ETH_DST_KEY0, ETH_SRC_KEY0, ETH_PROTO_KEY0 };

		eth_dxtable_tern_name_add(&key, &key_mask, 0, 1);
	}

	XDP2_DTABLE_ADD_TERN(eth_dxtable_tern_name,
			 ( .dst = ETH_DST_KEY1,
			   .src = ETH_SRC_KEY1,
			   .proto = ETH_PROTO_KEY1, ),
			 ( .dst = ETH_DST_KEY1,
			   .src = ETH_SRC_KEY1,
			   .proto = ETH_PROTO_KEY1, ),
			 0, 2);

	XDP2_DTABLE_ADD_TERN(ipv4_dxtable_tern_cast,
		( IPV4_KEY5 ), ( IPV4_KEY5 ), 66, HIT_NOACTION);

	XDP2_DTABLE_ADD_TERN(ipv4_dxtable_tern_cast,
		( IPV4_KEY2 ), ( IPV4_KEY2 ), 33, HIT_NOACTION);

	XDP2_DTABLE_ADD_TERN(ipv4_dxtable_tern_cast,
		( IPV4_KEY1 ), ( IPV4_KEY1 ), 22, HIT_DROP);

	XDP2_DTABLE_ADD_TERN(ipv4_dxtable_tern_cast,
		( IPV4_KEY4 ), ( IPV4_KEY4 ), 55, HIT_DROP);

	{
		struct ipv4_dxtable_tern_cast_key_struct key =
			{ IPV4_KEY3 };

		struct ipv4_dxtable_tern_cast_key_struct key_mask =
			{ IPV4_KEY3 };

		ipv4_dxtable_tern_cast_add(&key, &key_mask, 44, 1);
	}

	{
		struct ipv4_dxtable_tern_cast_key_struct key =
			{ IPV4_KEY0 };

		struct ipv4_dxtable_tern_cast_key_struct key_mask =
			{ IPV4_KEY0 };

		ipv4_dxtable_tern_cast_add(&key, &key_mask, 11, 1);
	}

	ident = 0;
	v = MISS;
	eth_tern = xdp2_dtable_create_tern("User eth tern",
			sizeof(struct ethhdr), &v, sizeof(v), &ident);

	v = 2;
	xdp2_dtable_add_tern(eth_tern, 0,
		&eth_keys_match[1], &eth_keys_match[0], 200, &v);
	v = 1;
	xdp2_dtable_add_tern(eth_tern, 0,
		&eth_keys_match[0], &eth_keys_match[0], 100, &v);

	/* Plain cast name */

	XDP2_DTABLE_ADD_TERN(ipv4_dxtable_tern_name_cast,
		( IPV4_KEY0 ), ( IPV4_KEY0 ), 0, 1);

	{
		struct ipv4_dxtable_tern_name_cast_key_struct
			key = {
			  .mihl = IPV4_IHL_KEY1, .mtos = IPV4_TOS_KEY1,
			  .src = IPV4_SADDR_KEY1, .dst = IPV4_DADDR_KEY1,
			  .mttl = IPV4_TTL_KEY1,
			  .proto = IPV4_PROTOCOL_KEY1
			};

		struct ipv4_dxtable_tern_name_cast_key_struct
			key_mask = {
			  .mihl = IPV4_IHL_KEY1, .mtos = IPV4_TOS_KEY1,
			  .src = IPV4_SADDR_KEY1, .dst = IPV4_DADDR_KEY1,
			  .mttl = IPV4_TTL_KEY1,
			  .proto = IPV4_PROTOCOL_KEY1
			};

		ipv4_dxtable_tern_name_cast_add(&key, &key_mask, 0, HIT_DROP);
	}

	XDP2_DTABLE_ADD_TERN(ipv4_dxtable_tern_name_cast,
		( IPV4_KEY2 ), ( IPV4_KEY2 ), 0, HIT_NOACTION);

	XDP2_DTABLE_ADD_TERN(ipv4_dxtable_tern_name_cast,
		( IPV4_KEY3 ), ( IPV4_KEY3 ), 0, 1);

	{
		struct ipv4_dxtable_tern_name_cast_key_struct
			key = {
			  .mihl = IPV4_IHL_KEY4, .mtos = IPV4_TOS_KEY4,
			  .src = IPV4_SADDR_KEY4, .dst = IPV4_DADDR_KEY4,
			  .mttl = IPV4_TTL_KEY4,
			  .proto = IPV4_PROTOCOL_KEY4
			};

		struct ipv4_dxtable_tern_name_cast_key_struct
			key_mask = {
			  .mihl = IPV4_IHL_KEY4, .mtos = IPV4_TOS_KEY4,
			  .src = IPV4_SADDR_KEY4, .dst = IPV4_DADDR_KEY4,
			  .mttl = IPV4_TTL_KEY4,
			  .proto = IPV4_PROTOCOL_KEY4
			};

		ipv4_dxtable_tern_name_cast_add(&key, &key_mask, 0, HIT_DROP);
	}

	XDP2_DTABLE_ADD_TERN(ipv4_dxtable_tern_name_cast,
		( IPV4_KEY5 ), ( IPV4_KEY5 ), 0, HIT_NOACTION);

	XDP2_DTABLE_ADD_TERN(eth_dxtable_tern_skey,
		 ( .h_dest = ETH_DST_KEY0,
		   .h_source = ETH_SRC_KEY0,
		   .h_proto = ETH_PROTO_KEY0,
		 ),
		 ( .h_dest = ETH_DST_KEY0,
		   .h_source = ETH_SRC_KEY0,
		   .h_proto = ETH_PROTO_KEY0,
		 ), 0, 1);

	XDP2_DTABLE_ADD_TERN(eth_dxtable_tern_skey,
		 ( .h_dest = ETH_DST_KEY1,
		   .h_source = ETH_SRC_KEY1,
		   .h_proto = ETH_PROTO_KEY1,
		 ),
		 ( .h_dest = ETH_DST_KEY1,
		   .h_source = ETH_SRC_KEY1,
		   .h_proto = ETH_PROTO_KEY1,
		 ), 0, 2);

	ident = 0;
	v = MISS;
	ipv4_tern = xdp2_dtable_create_tern("User ipv4 tern",
			sizeof(struct ipv4_common_keys),
			&v, sizeof(v), &ident);

	v = HIT_NOACTION;
	xdp2_dtable_add_tern(ipv4_tern, 0,
			      &ipv4_keys_match[5], &ipv4_keys_match[0], 66,
			      &v);
	v = HIT_NOACTION;
	xdp2_dtable_add_tern(ipv4_tern, 0,
			      &ipv4_keys_match[2], &ipv4_keys_match[0], 33,
			      &v);
	v = HIT_DROP;
	xdp2_dtable_add_tern(ipv4_tern, 0, &ipv4_keys_match[1],
			      &ipv4_keys_match[0], 22, &v);
	v = HIT_DROP;
	xdp2_dtable_add_tern(ipv4_tern, 0, &ipv4_keys_match[4],
			      &ipv4_keys_match[0], 55, &v);

	v = 1;
	xdp2_dtable_add_tern(ipv4_tern, 0,
			      &ipv4_keys_match[3], &ipv4_keys_match[0], 44,
			      &v);

	v = 1;
	xdp2_dtable_add_tern(ipv4_tern, 0, &ipv4_keys_match[0],
			      &ipv4_keys_match[0], 11, &v);
}

void run_dxtable_tern(void)
{
	int i, status_check;

	init_tables();

	for (i = 0; i < ARRAY_SIZE(eth_keys); i++) {
		status_check = get_base_eth_tern_status(i);

		RUN_ONE_DX_DETH(tern);
		RUN_ONE_DX_DETH(tern_name);
		RUN_ONE_DX_DETH_SKEY(tern_skey);
		RUN_ONE_DX_TETH(tern, tern, eth_tern);
	}

	for (i = 0; i < ARRAY_SIZE(iphdr_keys); i++) {
		status_check = get_base_ipv4_tern_status(i);

		RUN_ONE_DX_DIPV4(tern_cast);
		RUN_ONE_DX_DIPV4(tern_name_cast);
		RUN_ONE_DX_TIPV4(tern, tern, ipv4_tern);
	}
}
