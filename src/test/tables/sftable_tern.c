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

#include "xdp2/stable.h"

#include "test_table.h"

XDP2_SFTABLE_TERN_TABLE(
	eth_stable_tern,
	struct ethhdr *,
	(
		h_dest,
		h_source,
		h_proto
	),
	(struct my_ctx *ctx), (ctx),
	(drop, forward, NoAction, Miss),
	Miss(ctx)
)

XDP2_SFTABLE_ADD_TERN_MATCH(eth_stable_tern,
		( ETH_KEY0 ), ( ETH_KEY0 ),
		forward(ctx, 1), (struct my_ctx *ctx))

XDP2_SFTABLE_ADD_TERN_MATCH(eth_stable_tern,
		( ETH_KEY1 ), ( ETH_KEY1 ),
		forward(ctx, 2), (struct my_ctx *ctx))

XDP2_SFTABLE_TERN_TABLE_ENTS(
	eth_stable_tern_ents,
	struct ethhdr *,
	(
		h_dest,
		h_source,
		h_proto
	),
	(struct my_ctx *ctx), (ctx),
	(drop, forward, NoAction, Miss),
	Miss(ctx),
	(
		((ETH_KEY0), (ETH_KEY0), forward(ctx, 1)),
		((ETH_KEY1), (ETH_KEY1), forward(ctx, 2))
	)
)

XDP2_SFTABLE_TERN_TABLE_NAME(
	eth_stable_tern_name,
	struct ethhdr *,
	(
		(h_dest, dst),
		(h_source, src),
		(h_proto, proto)
	),
	(struct my_ctx *ctx), (ctx),
	(drop, forward, NoAction, Miss),
	Miss(ctx)
)

XDP2_SFTABLE_ADD_TERN_MATCH(eth_stable_tern_name,
		( .dst = ETH_DST_KEY0,
		  .src = ETH_SRC_KEY0,
		  .proto = ETH_PROTO_KEY0),
		( .dst = ETH_DST_KEY0,
		  .src = ETH_SRC_KEY0,
		  .proto = ETH_PROTO_KEY0),
		forward(ctx, 1), (struct my_ctx *ctx))

XDP2_SFTABLE_ADD_TERN_MATCH(eth_stable_tern_name,
		(ETH_KEY1), (ETH_KEY1),
		forward(ctx, 2), (struct my_ctx *ctx))

XDP2_SFTABLE_TERN_TABLE_NAME_ENTS(
	eth_stable_tern_name_ents,
	struct ethhdr *,
	(
		(h_dest, dst),
		(h_source, src),
		(h_proto, proto)
	),
	(struct my_ctx *ctx), (ctx),
	(drop, forward, NoAction, Miss),
	Miss(ctx),
	(
		(( .dst = ETH_DST_KEY0,
		   .src = ETH_SRC_KEY0,
		   .proto = ETH_PROTO_KEY0),
		 ( .dst = ETH_DST_KEY0,
		   .src = ETH_SRC_KEY0,
		   .proto = ETH_PROTO_KEY0), forward(ctx, 1)),
		(( .dst = ETH_DST_KEY1,
		   .src = ETH_SRC_KEY1,
		   .proto = ETH_PROTO_KEY1),
		 ( .dst = ETH_DST_KEY1,
		   .src = ETH_SRC_KEY1,
		   .proto = ETH_PROTO_KEY1), forward(ctx, 2))
	)
)

XDP2_SFTABLE_TERN_TABLE_CAST(
	ipv4_stable_tern_cast,
	struct iphdr *,
	(
		(ihl, (__u8)),
		(tos,),
		(saddr,),
		(daddr,),
		(ttl,),
		(protocol,)
	),
	(struct my_ctx *ctx), (ctx),
	(drop, forward, NoAction, Miss),
	Miss(ctx)
)

XDP2_SFTABLE_ADD_TERN_MATCH(ipv4_stable_tern_cast,
	( IPV4_KEY0 ), ( IPV4_KEY0 ),
	forward(ctx, 1), (struct my_ctx *ctx))

XDP2_SFTABLE_ADD_TERN_MATCH(ipv4_stable_tern_cast,
	( IPV4_KEY1 ), ( IPV4_KEY1 ),
	drop(ctx), (struct my_ctx *ctx))

XDP2_SFTABLE_ADD_TERN_MATCH(ipv4_stable_tern_cast,
	( IPV4_KEY2 ), ( IPV4_KEY2 ),
	NoAction(ctx), (struct my_ctx *ctx))

XDP2_SFTABLE_ADD_TERN_MATCH(ipv4_stable_tern_cast,
	( IPV4_KEY3 ), ( IPV4_KEY3 ),
	forward(ctx, 1), (struct my_ctx *ctx))

XDP2_SFTABLE_ADD_TERN_MATCH(ipv4_stable_tern_cast,
	( IPV4_KEY4 ), ( IPV4_KEY4 ),
	drop(ctx), (struct my_ctx *ctx))

XDP2_SFTABLE_ADD_TERN_MATCH(ipv4_stable_tern_cast,
	( IPV4_KEY5 ), ( IPV4_KEY5 ),
	NoAction(ctx), (struct my_ctx *ctx))

XDP2_SFTABLE_TERN_TABLE_CAST_ENTS(
	ipv4_stable_tern_cast_ents,
	struct iphdr *,
	(
		(ihl, (__u8)),
		(tos,),
		(saddr,),
		(daddr,),
		(ttl,),
		(protocol,)
	),
	(struct my_ctx *ctx), (ctx),
	(drop, forward, NoAction, Miss),
	Miss(ctx),
	(
	     (( IPV4_KEY0), ( IPV4_KEY0), forward(ctx, 1)),
	     (( IPV4_KEY1), ( IPV4_KEY1), drop(ctx)),
	     (( IPV4_KEY2), ( IPV4_KEY2), NoAction(ctx)),
	     (( IPV4_KEY3), ( IPV4_KEY3), forward(ctx, 1)),
	     (( IPV4_KEY4), ( IPV4_KEY4), drop(ctx)),
	     (( IPV4_KEY5), ( IPV4_KEY5), NoAction(ctx))
	)
)

XDP2_SFTABLE_TERN_TABLE_NAME_CAST(
	ipv4_stable_tern_name_cast,
	struct iphdr *,
	(
		(ihl, mihl, (__u8)),
		(tos, mtos,),
		(saddr, src,),
		(daddr, dst,),
		(ttl, mttl,),
		(protocol, proto,)
	),
	(struct my_ctx *ctx), (ctx),
	(drop, forward, NoAction, Miss),
	Miss(ctx)
)

XDP2_SFTABLE_ADD_TERN_MATCH(ipv4_stable_tern_name_cast,
	(.mihl = IPV4_IHL_KEY0, .mtos = IPV4_TOS_KEY0,
	 .src = IPV4_SADDR_KEY0, .dst = IPV4_DADDR_KEY0,
	 .mttl = IPV4_TTL_KEY0, .proto = IPV4_PROTOCOL_KEY0),
	(.mihl = IPV4_IHL_KEY0, .mtos = IPV4_TOS_KEY0,
	 .src = IPV4_SADDR_KEY0, .dst = IPV4_DADDR_KEY0,
	 .mttl = IPV4_TTL_KEY0, .proto = IPV4_PROTOCOL_KEY0),
	forward(ctx, 1), (struct my_ctx *ctx))

XDP2_SFTABLE_ADD_TERN_MATCH(ipv4_stable_tern_name_cast,
	( IPV4_KEY1 ), ( IPV4_KEY1 ), drop(ctx), (struct my_ctx *ctx))

XDP2_SFTABLE_ADD_TERN_MATCH(ipv4_stable_tern_name_cast,
	( IPV4_KEY2 ), ( IPV4_KEY2 ), NoAction(ctx), (struct my_ctx *ctx))

XDP2_SFTABLE_ADD_TERN_MATCH(ipv4_stable_tern_name_cast,
	( IPV4_KEY3 ), ( IPV4_KEY3 ), forward(ctx, 1), (struct my_ctx *ctx))

XDP2_SFTABLE_ADD_TERN_MATCH(ipv4_stable_tern_name_cast,
	( IPV4_KEY4 ), ( IPV4_KEY4 ), drop(ctx), (struct my_ctx *ctx))

XDP2_SFTABLE_ADD_TERN_MATCH(ipv4_stable_tern_name_cast,
	( IPV4_KEY5 ), ( IPV4_KEY5 ), NoAction(ctx), (struct my_ctx *ctx))

XDP2_SFTABLE_TERN_TABLE_NAME_CAST_ENTS(
	ipv4_stable_tern_name_cast_ents,
	struct iphdr *,
	(
		(ihl, mihl, (__u8)),
		(tos, mtos,),
		(saddr, src,),
		(daddr, dst,),
		(ttl, mttl,),
		(protocol, proto,)
	),
	(struct my_ctx *ctx), (ctx),
	(drop, forward, NoAction, Miss),
	Miss(ctx),
	(
		(( IPV4_KEY0 ), ( IPV4_KEY0 ), forward(ctx, 1)),
		(( IPV4_KEY1 ), ( IPV4_KEY1 ), drop(ctx)),
		(( IPV4_KEY2 ), ( IPV4_KEY2 ), NoAction(ctx)),
		(( IPV4_KEY3 ), ( IPV4_KEY3 ), forward(ctx, 1)),
		(( IPV4_KEY4 ), ( IPV4_KEY4 ), drop(ctx)),
		(( .mihl = IPV4_IHL_KEY5, .mtos = IPV4_TOS_KEY5,
		   .src = IPV4_SADDR_KEY5, .dst = IPV4_DADDR_KEY5,
		   .mttl = IPV4_TTL_KEY5, .proto = IPV4_PROTOCOL_KEY5),
		 ( .mihl = IPV4_IHL_KEY5, .mtos = IPV4_TOS_KEY5,
		   .src = IPV4_SADDR_KEY5, .dst = IPV4_DADDR_KEY5,
		   .mttl = IPV4_TTL_KEY5, .proto = IPV4_PROTOCOL_KEY5),
	      NoAction(ctx))

	)
)

XDP2_SFTABLE_TERN_TABLE_SKEY(
	eth_stable_tern_skey,
	struct ethhdr *,
	(struct my_ctx *ctx), (ctx),
	(drop, forward, NoAction, Miss),
	Miss(ctx)
)

XDP2_SFTABLE_ADD_TERN_MATCH(eth_stable_tern_skey,
		(.h_dest = ETH_DST_KEY0, .h_source = ETH_SRC_KEY0,
		 .h_proto = ETH_PROTO_KEY0),
		(.h_dest = ETH_DST_KEY0, .h_source = ETH_SRC_KEY0,
		 .h_proto = ETH_PROTO_KEY0), forward(ctx, 1),
		(struct my_ctx *ctx))

XDP2_SFTABLE_ADD_TERN_MATCH(eth_stable_tern_skey,
		(.h_dest = ETH_DST_KEY1, .h_source = ETH_SRC_KEY1,
		 .h_proto = ETH_PROTO_KEY1),
		(.h_dest = ETH_DST_KEY1, .h_source = ETH_SRC_KEY1,
		 .h_proto = ETH_PROTO_KEY1), forward(ctx, 2),
		(struct my_ctx *ctx))

XDP2_SFTABLE_TERN_TABLE_SKEY_ENTS(
	eth_stable_tern_skey_ents,
	struct ethhdr *,
	(struct my_ctx *ctx), (ctx),
	(drop, forward, NoAction, Miss),
	Miss(ctx),
	(
		((.h_dest = ETH_DST_KEY0, .h_source = ETH_SRC_KEY0,
		  .h_proto = ETH_PROTO_KEY0),
		 (.h_dest = ETH_DST_KEY0, .h_source = ETH_SRC_KEY0,
		  .h_proto = ETH_PROTO_KEY0), forward(ctx, 1)),
		((.h_dest = ETH_DST_KEY1, .h_source = ETH_SRC_KEY1,
		  .h_proto = ETH_PROTO_KEY1),
		 (.h_dest = ETH_DST_KEY1, .h_source = ETH_SRC_KEY1,
		  .h_proto = ETH_PROTO_KEY1), forward(ctx, 2))
	)
)

XDP2_SFTABLE_ADD_TERN_MATCH(eth_stable_tern_skey_ents,
		(.h_dest = ETH_DST_KEY0, .h_source = ETH_SRC_KEY0,
		 .h_proto = ETH_PROTO_KEY0),
		(.h_dest = ETH_DST_KEY0, .h_source = ETH_SRC_KEY0,
		 .h_proto = ETH_PROTO_KEY0), forward(ctx, 1),
		(struct my_ctx *ctx))

XDP2_SFTABLE_ADD_TERN_MATCH(eth_stable_tern_skey_ents,
		(.h_dest = ETH_DST_KEY1, .h_source = ETH_SRC_KEY1,
		 .h_proto = ETH_PROTO_KEY1),
		(.h_dest = ETH_DST_KEY1, .h_source = ETH_SRC_KEY1,
		 .h_proto = ETH_PROTO_KEY1), forward(ctx, 2),
		(struct my_ctx *ctx))

int get_base_eth_tern_status(int w)
{
	static char *name = "Base eth (eth_stable_tern)";
	struct my_ctx ctx = { .status = NO_STATUS, .name = name };

	eth_stable_tern_lookup(&eth_keys[w], &ctx);

	return ctx.status;
}

int get_base_ipv4_tern_status(int w)
{
	static char *name = "Base eth (ipv4_stable_tern)";
	struct my_ctx ctx = { .status = NO_STATUS, .name = name };

	ipv4_stable_tern_cast_lookup(&iphdr_keys[w], &ctx);

	return ctx.status;
}

void run_stable_tern(void)
{
	int i, status_check;
	struct my_ctx ctx;
	char name[100];

	ctx.name = name;

	for (i = 0; i < ARRAY_SIZE(eth_keys); i++) {
		status_check = get_base_eth_tern_status(i);

		RUN_ONE_ETH(tern);
		RUN_ONE_ETH(tern_ents);
		RUN_ONE_ETH(tern_name);
		RUN_ONE_ETH(tern_name_ents);
		RUN_ONE_ETH_SKEY(tern_skey);
		RUN_ONE_ETH_SKEY(tern_skey_ents);
	}

	for (i = 0; i < ARRAY_SIZE(iphdr_keys); i++) {
		status_check = get_base_ipv4_tern_status(i);

		RUN_ONE_IPV4(tern_cast);
		RUN_ONE_IPV4(tern_cast_ents);
		RUN_ONE_IPV4(tern_name_cast);
		RUN_ONE_IPV4(tern_name_cast_ents);
	}
}
