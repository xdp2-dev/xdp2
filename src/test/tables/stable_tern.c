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

XDP2_STABLE_TERN_TABLE(
	eth_sxtables_tern,
	struct ethhdr *,
	(
		h_dest,
		h_source,
		h_proto
	), __u32, -1U
)

XDP2_STABLE_ADD_TERN_MATCH(eth_sxtables_tern,
		( ETH_KEY0 ), ( ETH_KEY0 ), 1)

XDP2_STABLE_ADD_TERN_MATCH(eth_sxtables_tern,
		( ETH_KEY1 ), ( ETH_KEY1 ), 2)

XDP2_STABLE_TERN_TABLE_ENTS(
	eth_sxtables_tern_ents,
	struct ethhdr *,
	(
		h_dest,
		h_source,
		h_proto
	), __u32, -1U,
	(
		((ETH_KEY0), (ETH_KEY0), 1),
		((ETH_KEY1), (ETH_KEY1), 2)
	)
)

XDP2_STABLE_TERN_TABLE_NAME(
	eth_sxtables_tern_name,
	struct ethhdr *,
	(
		(h_dest, dst),
		(h_source, src),
		(h_proto, proto)
	), __u32, -1U
)

XDP2_STABLE_ADD_TERN_MATCH(eth_sxtables_tern_name,
		( .dst = ETH_DST_KEY0,
		  .src = ETH_SRC_KEY0,
		  .proto = ETH_PROTO_KEY0),
		( .dst = ETH_DST_KEY0,
		  .src = ETH_SRC_KEY0,
		  .proto = ETH_PROTO_KEY0), 1)

XDP2_STABLE_ADD_TERN_MATCH(eth_sxtables_tern_name,
		(ETH_KEY1), (ETH_KEY1), 2)

XDP2_STABLE_TERN_TABLE_NAME_ENTS(
	eth_sxtables_tern_name_ents,
	struct ethhdr *,
	(
		(h_dest, dst),
		(h_source, src),
		(h_proto, proto)
	), __u32, -1,
	(
		(( .dst = ETH_DST_KEY0,
		   .src = ETH_SRC_KEY0,
		   .proto = ETH_PROTO_KEY0),
		 ( .dst = ETH_DST_KEY0,
		   .src = ETH_SRC_KEY0,
		   .proto = ETH_PROTO_KEY0), 1),
		(( .dst = ETH_DST_KEY1,
		   .src = ETH_SRC_KEY1,
		   .proto = ETH_PROTO_KEY1),
		 ( .dst = ETH_DST_KEY1,
		   .src = ETH_SRC_KEY1,
		   .proto = ETH_PROTO_KEY1), 2)
	)
)

XDP2_STABLE_TERN_TABLE_CAST(
	ipv4_sxtables_tern_cast,
	struct iphdr *,
	(
		(ihl, (__u8)),
		(tos,),
		(saddr,),
		(daddr,),
		(ttl,),
		(protocol,)
	), __u32, -1U
)

XDP2_STABLE_ADD_TERN_MATCH(ipv4_sxtables_tern_cast,
	( IPV4_KEY0 ), ( IPV4_KEY0 ), 1)

XDP2_STABLE_ADD_TERN_MATCH(ipv4_sxtables_tern_cast,
	( IPV4_KEY1 ), ( IPV4_KEY1 ), HIT_DROP)

XDP2_STABLE_ADD_TERN_MATCH(ipv4_sxtables_tern_cast,
	( IPV4_KEY2 ), ( IPV4_KEY2 ), HIT_NOACTION)

XDP2_STABLE_ADD_TERN_MATCH(ipv4_sxtables_tern_cast,
	( IPV4_KEY3 ), ( IPV4_KEY3 ), 1)

XDP2_STABLE_ADD_TERN_MATCH(ipv4_sxtables_tern_cast,
	( IPV4_KEY4 ), ( IPV4_KEY4 ), HIT_DROP)

XDP2_STABLE_ADD_TERN_MATCH(ipv4_sxtables_tern_cast,
	( IPV4_KEY5 ), ( IPV4_KEY5 ), HIT_NOACTION)

XDP2_STABLE_TERN_TABLE_CAST_ENTS(
	ipv4_sxtables_tern_cast_ents,
	struct iphdr *,
	(
		(ihl, (__u8)),
		(tos,),
		(saddr,),
		(daddr,),
		(ttl,),
		(protocol,)
	), __u32, -1U,
	(
	     (( IPV4_KEY0), ( IPV4_KEY0), 1),
	     (( IPV4_KEY1), ( IPV4_KEY1), HIT_DROP),
	     (( IPV4_KEY2), ( IPV4_KEY2), HIT_NOACTION),
	     (( IPV4_KEY3), ( IPV4_KEY3), 1),
	     (( IPV4_KEY4), ( IPV4_KEY4), HIT_DROP),
	     (( IPV4_KEY5), ( IPV4_KEY5), HIT_NOACTION)
	)
)

XDP2_STABLE_TERN_TABLE_NAME_CAST(
	ipv4_sxtables_tern_name_cast,
	struct iphdr *,
	(
		(ihl, mihl, (__u8)),
		(tos, mtos,),
		(saddr, src,),
		(daddr, dst,),
		(ttl, mttl,),
		(protocol, proto,)
	), __u32, -1U
)

XDP2_STABLE_ADD_TERN_MATCH(ipv4_sxtables_tern_name_cast,
	(.mihl = IPV4_IHL_KEY0, .mtos = IPV4_TOS_KEY0,
	 .src = IPV4_SADDR_KEY0, .dst = IPV4_DADDR_KEY0,
	 .mttl = IPV4_TTL_KEY0, .proto = IPV4_PROTOCOL_KEY0),
	(.mihl = IPV4_IHL_KEY0, .mtos = IPV4_TOS_KEY0,
	 .src = IPV4_SADDR_KEY0, .dst = IPV4_DADDR_KEY0,
	 .mttl = IPV4_TTL_KEY0, .proto = IPV4_PROTOCOL_KEY0), 1)

XDP2_STABLE_ADD_TERN_MATCH(ipv4_sxtables_tern_name_cast,
	( IPV4_KEY1 ), ( IPV4_KEY1 ), HIT_DROP)

XDP2_STABLE_ADD_TERN_MATCH(ipv4_sxtables_tern_name_cast,
	( IPV4_KEY2 ), ( IPV4_KEY2 ), HIT_NOACTION)

XDP2_STABLE_ADD_TERN_MATCH(ipv4_sxtables_tern_name_cast,
	( IPV4_KEY3 ), ( IPV4_KEY3 ), 1)

XDP2_STABLE_ADD_TERN_MATCH(ipv4_sxtables_tern_name_cast,
	( IPV4_KEY4 ), ( IPV4_KEY4 ), HIT_DROP)

XDP2_STABLE_ADD_TERN_MATCH(ipv4_sxtables_tern_name_cast,
	( IPV4_KEY5 ), ( IPV4_KEY5 ), HIT_NOACTION)

XDP2_STABLE_TERN_TABLE_NAME_CAST_ENTS(
	ipv4_sxtables_tern_name_cast_ents,
	struct iphdr *,
	(
		(ihl, mihl, (__u8)),
		(tos, mtos,),
		(saddr, src,),
		(daddr, dst,),
		(ttl, mttl,),
		(protocol, proto,)
	), __u32, -1U,
	(
		(( IPV4_KEY0 ), ( IPV4_KEY0 ), 1),
		(( IPV4_KEY1 ), ( IPV4_KEY1 ), HIT_DROP),
		(( IPV4_KEY2 ), ( IPV4_KEY2 ), HIT_NOACTION),
		(( IPV4_KEY3 ), ( IPV4_KEY3 ), 1),
		(( IPV4_KEY4 ), ( IPV4_KEY4 ), HIT_DROP),
		(( .mihl = IPV4_IHL_KEY5, .mtos = IPV4_TOS_KEY5,
		   .src = IPV4_SADDR_KEY5, .dst = IPV4_DADDR_KEY5,
		   .mttl = IPV4_TTL_KEY5, .proto = IPV4_PROTOCOL_KEY5),
		 ( .mihl = IPV4_IHL_KEY5, .mtos = IPV4_TOS_KEY5,
		   .src = IPV4_SADDR_KEY5, .dst = IPV4_DADDR_KEY5,
		   .mttl = IPV4_TTL_KEY5, .proto = IPV4_PROTOCOL_KEY5),
		   HIT_NOACTION)

	)
)

XDP2_STABLE_TERN_TABLE_SKEY(
	eth_sxtables_tern_skey,
	struct ethhdr *, __u32, -1U
)

XDP2_STABLE_ADD_TERN_MATCH(eth_sxtables_tern_skey,
		(.h_dest = ETH_DST_KEY0, .h_source = ETH_SRC_KEY0,
		 .h_proto = ETH_PROTO_KEY0),
		(.h_dest = ETH_DST_KEY0, .h_source = ETH_SRC_KEY0,
		 .h_proto = ETH_PROTO_KEY0), 1)

XDP2_STABLE_ADD_TERN_MATCH(eth_sxtables_tern_skey,
		(.h_dest = ETH_DST_KEY1, .h_source = ETH_SRC_KEY1,
		 .h_proto = ETH_PROTO_KEY1),
		(.h_dest = ETH_DST_KEY1, .h_source = ETH_SRC_KEY1,
		 .h_proto = ETH_PROTO_KEY1), 2)

XDP2_STABLE_TERN_TABLE_SKEY_ENTS(
	eth_sxtables_tern_skey_ents,
	struct ethhdr *, __u32, -1U,
	(
		((.h_dest = ETH_DST_KEY0, .h_source = ETH_SRC_KEY0,
		  .h_proto = ETH_PROTO_KEY0),
		 (.h_dest = ETH_DST_KEY0, .h_source = ETH_SRC_KEY0,
		  .h_proto = ETH_PROTO_KEY0), 60),
		((.h_dest = ETH_DST_KEY1, .h_source = ETH_SRC_KEY1,
		  .h_proto = ETH_PROTO_KEY1),
		 (.h_dest = ETH_DST_KEY1, .h_source = ETH_SRC_KEY1,
		  .h_proto = ETH_PROTO_KEY1), 61)
	)
)

XDP2_STABLE_ADD_TERN_MATCH(eth_sxtables_tern_skey_ents,
		(.h_dest = ETH_DST_KEY0, .h_source = ETH_SRC_KEY0,
		 .h_proto = ETH_PROTO_KEY0),
		(.h_dest = ETH_DST_KEY0, .h_source = ETH_SRC_KEY0,
		 .h_proto = ETH_PROTO_KEY0), 1)

XDP2_STABLE_ADD_TERN_MATCH(eth_sxtables_tern_skey_ents,
		(.h_dest = ETH_DST_KEY1, .h_source = ETH_SRC_KEY1,
		 .h_proto = ETH_PROTO_KEY1),
		(.h_dest = ETH_DST_KEY1, .h_source = ETH_SRC_KEY1,
		 .h_proto = ETH_PROTO_KEY1), 2)

void run_sxtables_tern(void)
{
	int i, status_check;

	for (i = 0; i < ARRAY_SIZE(eth_keys); i++) {
		status_check = get_base_eth_tern_status(i);

		RUN_ONE_SX_ETH(tern);
		RUN_ONE_SX_ETH(tern_ents);
		RUN_ONE_SX_ETH(tern_name);
		RUN_ONE_SX_ETH(tern_name_ents);
		RUN_ONE_SX_ETH_SKEY(tern_skey);
		RUN_ONE_SX_ETH_SKEY(tern_skey_ents);
	}

	for (i = 0; i < ARRAY_SIZE(iphdr_keys); i++) {
		status_check = get_base_ipv4_tern_status(i);

		RUN_ONE_SX_IPV4(tern_cast);
		RUN_ONE_SX_IPV4(tern_cast_ents);
		RUN_ONE_SX_IPV4(tern_name_cast);
		RUN_ONE_SX_IPV4(tern_name_cast_ents);
	}
}
