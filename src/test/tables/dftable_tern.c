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

XDP2_DFTABLE_TERN_TABLE(
	eth_dtable_tern,
	struct ethhdr *,
	(
		h_dest,
		h_source,
		h_proto
	), Miss_dtable, ()
)

XDP2_DFTABLE_TERN_TABLE_NAME(
	eth_dtable_tern_name,
	struct ethhdr *,
	(
		(h_dest, dst),
		(h_source, src),
		(h_proto, proto)
	), Miss_dtable, ()
)

XDP2_DFTABLE_TERN_TABLE_CAST(
	ipv4_dtable_tern_cast,
	struct iphdr *,
	(
		(ihl, (__u8)),
		(tos,),
		(saddr,),
		(daddr,),
		(ttl,),
		(protocol,)
	), Miss_dtable, ()
)

XDP2_DFTABLE_TERN_TABLE_NAME_CAST(
	ipv4_dtable_tern_name_cast,
	struct iphdr *,
	(
		(ihl, mihl, (__u8)),
		(tos, mtos,),
		(saddr, src,),
		(daddr, dst,),
		(ttl, mttl,),
		(protocol, proto,)
	), Miss_dtable, ()
)

XDP2_DFTABLE_TERN_TABLE_SKEY(
	eth_dtable_tern_skey, struct ethhdr *, Miss_dtable, ()
)

static void init_tables(void)
{
	struct __xdp2_dtable_entry_func_target ftarg;
	int ident;

	XDP2_DFTABLE_ADD_TERN(eth_dtable_tern,
			 (ETH_DST_KEY1, ETH_SRC_KEY1, ETH_PROTO_KEY1),
			 (ETH_DST_KEY1, ETH_SRC_KEY1, ETH_PROTO_KEY1),
			 200, forward_dtable, (void *)2);

	{
		struct eth_dtable_tern_key_struct key =
			{ ETH_DST_KEY0, ETH_SRC_KEY0, ETH_PROTO_KEY0 };

		struct eth_dtable_tern_key_struct key_mask =
			{ ETH_DST_KEY0, ETH_SRC_KEY0, ETH_PROTO_KEY0 };

		eth_dtable_tern_add(&key, &key_mask, 100, forward_dtable,
				     (void *)1);
	}
	{
		struct eth_dtable_tern_name_key_struct key =
			{ ETH_DST_KEY0, ETH_SRC_KEY0, ETH_PROTO_KEY0 };

		struct eth_dtable_tern_name_key_struct key_mask =
			{ ETH_DST_KEY0, ETH_SRC_KEY0, ETH_PROTO_KEY0 };

		eth_dtable_tern_name_add(&key, &key_mask, 0, forward_dtable,
					  (void *)1);
	}

	XDP2_DFTABLE_ADD_TERN(eth_dtable_tern_name,
			 ( .dst = ETH_DST_KEY1,
			   .src = ETH_SRC_KEY1,
			   .proto = ETH_PROTO_KEY1, ),
			 ( .dst = ETH_DST_KEY1,
			   .src = ETH_SRC_KEY1,
			   .proto = ETH_PROTO_KEY1, ),
			 0, forward_dtable, (void *)2);

	XDP2_DFTABLE_ADD_TERN(ipv4_dtable_tern_cast,
		( IPV4_KEY5 ), ( IPV4_KEY5 ), 66, NoAction_dtable, (void *)1);

	XDP2_DFTABLE_ADD_TERN(ipv4_dtable_tern_cast,
		( IPV4_KEY2 ), ( IPV4_KEY2 ), 33, NoAction_dtable, NULL);

	XDP2_DFTABLE_ADD_TERN(ipv4_dtable_tern_cast,
		( IPV4_KEY1 ), ( IPV4_KEY1 ), 22, drop_dtable, NULL);

	XDP2_DFTABLE_ADD_TERN(ipv4_dtable_tern_cast,
		( IPV4_KEY4 ), ( IPV4_KEY4 ), 55, drop_dtable, NULL);

	{
		struct ipv4_dtable_tern_cast_key_struct key =
			{ IPV4_KEY3 };

		struct ipv4_dtable_tern_cast_key_struct key_mask =
			{ IPV4_KEY3 };

		ipv4_dtable_tern_cast_add(&key, &key_mask, 44,
					   forward_dtable, (void *)1);
	}

	{
		struct ipv4_dtable_tern_cast_key_struct key =
			{ IPV4_KEY0 };

		struct ipv4_dtable_tern_cast_key_struct key_mask =
			{ IPV4_KEY0 };

		ipv4_dtable_tern_cast_add(&key, &key_mask, 11,
					   forward_dtable, (void *)1);
	}

	ftarg.arg = NULL;

	ident = 0;
	ftarg.func = Miss_dtable;
	eth_tern = xdp2_dtable_create_tern("User eth tern",
			sizeof(struct ethhdr), &ftarg, sizeof(ftarg), &ident);

	ftarg.func = forward_dtable;
	ftarg.arg = (void *)2;
	xdp2_dtable_add_tern(eth_tern, 0,
		&eth_keys_match[1], &eth_keys_match[0], 200, &ftarg);
	ftarg.func = forward_dtable;
	ftarg.arg = (void *)1;
	xdp2_dtable_add_tern(eth_tern, 0,
		&eth_keys_match[0], &eth_keys_match[0], 100, &ftarg);

	/* Plain cast name */

	XDP2_DFTABLE_ADD_TERN(ipv4_dtable_tern_name_cast,
		( IPV4_KEY0 ), ( IPV4_KEY0 ), 0, forward_dtable, (void *)1);

	{
		struct ipv4_dtable_tern_name_cast_key_struct
			key = {
			  .mihl = IPV4_IHL_KEY1, .mtos = IPV4_TOS_KEY1,
			  .src = IPV4_SADDR_KEY1, .dst = IPV4_DADDR_KEY1,
			  .mttl = IPV4_TTL_KEY1,
			  .proto = IPV4_PROTOCOL_KEY1
			};

		struct ipv4_dtable_tern_name_cast_key_struct
			key_mask = {
			  .mihl = IPV4_IHL_KEY1, .mtos = IPV4_TOS_KEY1,
			  .src = IPV4_SADDR_KEY1, .dst = IPV4_DADDR_KEY1,
			  .mttl = IPV4_TTL_KEY1,
			  .proto = IPV4_PROTOCOL_KEY1
			};

		ipv4_dtable_tern_name_cast_add(&key, &key_mask, 0,
						drop_dtable, NULL);
	}

	XDP2_DFTABLE_ADD_TERN(ipv4_dtable_tern_name_cast,
		( IPV4_KEY2 ), ( IPV4_KEY2 ), 0, NoAction_dtable, NULL);

	XDP2_DFTABLE_ADD_TERN(ipv4_dtable_tern_name_cast,
		( IPV4_KEY3 ), ( IPV4_KEY3 ), 0, forward_dtable, (void *)1);

	{
		struct ipv4_dtable_tern_name_cast_key_struct
			key = {
			  .mihl = IPV4_IHL_KEY4, .mtos = IPV4_TOS_KEY4,
			  .src = IPV4_SADDR_KEY4, .dst = IPV4_DADDR_KEY4,
			  .mttl = IPV4_TTL_KEY4,
			  .proto = IPV4_PROTOCOL_KEY4
			};

		struct ipv4_dtable_tern_name_cast_key_struct
			key_mask = {
			  .mihl = IPV4_IHL_KEY4, .mtos = IPV4_TOS_KEY4,
			  .src = IPV4_SADDR_KEY4, .dst = IPV4_DADDR_KEY4,
			  .mttl = IPV4_TTL_KEY4,
			  .proto = IPV4_PROTOCOL_KEY4
			};

		ipv4_dtable_tern_name_cast_add(&key, &key_mask, 0,
						drop_dtable, NULL);
	}

	XDP2_DFTABLE_ADD_TERN(ipv4_dtable_tern_name_cast,
		( IPV4_KEY5 ), ( IPV4_KEY5 ), 0, NoAction_dtable, (void *)1);

	XDP2_DFTABLE_ADD_TERN(eth_dtable_tern_skey,
		 ( .h_dest = ETH_DST_KEY0,
		   .h_source = ETH_SRC_KEY0,
		   .h_proto = ETH_PROTO_KEY0,
		 ),
		 ( .h_dest = ETH_DST_KEY0,
		   .h_source = ETH_SRC_KEY0,
		   .h_proto = ETH_PROTO_KEY0,
		 ), 0, forward_dtable, (void *)1);

	XDP2_DFTABLE_ADD_TERN(eth_dtable_tern_skey,
		 ( .h_dest = ETH_DST_KEY1,
		   .h_source = ETH_SRC_KEY1,
		   .h_proto = ETH_PROTO_KEY1,
		 ),
		 ( .h_dest = ETH_DST_KEY1,
		   .h_source = ETH_SRC_KEY1,
		   .h_proto = ETH_PROTO_KEY1,
		 ), 0, forward_dtable, (void *)2);

	ident = 0;
	ftarg.func = Miss_dtable;
	ipv4_tern = xdp2_dtable_create_tern("User ipv4 tern",
			sizeof(struct ipv4_common_keys), &ftarg,
			sizeof(ftarg), &ident);

	ftarg.func = NoAction_dtable;
	ftarg.arg = (void *)1;
	xdp2_dtable_add_tern(ipv4_tern, 0,
			      &ipv4_keys_match[5], &ipv4_keys_match[0], 66,
			      &ftarg);
	ftarg.func = NoAction_dtable;
	xdp2_dtable_add_tern(ipv4_tern, 0,
			      &ipv4_keys_match[2], &ipv4_keys_match[0], 33,
			      &ftarg);
	ftarg.func = drop_dtable;
	xdp2_dtable_add_tern(ipv4_tern, 0, &ipv4_keys_match[1],
			      &ipv4_keys_match[0], 22, &ftarg);
	ftarg.func = drop_dtable;
	xdp2_dtable_add_tern(ipv4_tern, 0, &ipv4_keys_match[4],
			      &ipv4_keys_match[0], 55, &ftarg);
	ftarg.func = forward_dtable;
	ftarg.arg = (void *)1;
	xdp2_dtable_add_tern(ipv4_tern, 0,
			      &ipv4_keys_match[3], &ipv4_keys_match[0], 44,
			      &ftarg);
	ftarg.func = forward_dtable;
	xdp2_dtable_add_tern(ipv4_tern, 0, &ipv4_keys_match[0],
			      &ipv4_keys_match[0], 11, &ftarg);
}

void run_dtable_tern(void)
{
	int i, status_check;
	struct my_ctx ctx;
	char name[100];

	init_tables();

	ctx.name = name;

	for (i = 0; i < ARRAY_SIZE(eth_keys); i++) {
		status_check = get_base_eth_tern_status(i);

		RUN_ONE_DETH(tern);
		RUN_ONE_DETH(tern_name);
		RUN_ONE_DETH_SKEY(tern_skey);
		RUN_ONE_TETH(tern, tern, eth_tern);
	}

	for (i = 0; i < ARRAY_SIZE(iphdr_keys); i++) {
		status_check = get_base_ipv4_tern_status(i);

		RUN_ONE_DIPV4(tern_cast);
		RUN_ONE_DIPV4(tern_name_cast);
		RUN_ONE_TIPV4(tern, tern, ipv4_tern);
	}
}
