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

XDP2_STABLE_LPM_TABLE(
	eth_sxtables_lpm,
	struct ethhdr *,
	(
		h_dest,
		h_source,
		h_proto
	), __u32, -1U
)

XDP2_STABLE_ADD_LPM_MATCH(eth_sxtables_lpm,
		( ETH_KEY0 ), 17, 1)

XDP2_STABLE_ADD_LPM_MATCH(eth_sxtables_lpm,
		( ETH_KEY1 ), 22, 2)

XDP2_STABLE_LPM_TABLE_ENTS(
	eth_sxtables_lpm_ents,
	struct ethhdr *,
	(
		h_dest,
		h_source,
		h_proto
	), __u32, -1U,
	(
		(( ETH_KEY0 ), 17, 1),
		(( ETH_KEY1 ), 22, 2)
	)
)

XDP2_STABLE_LPM_TABLE_NAME(
	eth_sxtables_lpm_name,
	struct ethhdr *,
	(
		(h_dest, dst),
		(h_source, src),
		(h_proto, proto)
	), __u32, -1U
)

XDP2_STABLE_ADD_LPM_MATCH(eth_sxtables_lpm_name,
		( ETH_KEY0 ), 17, 1)

XDP2_STABLE_ADD_LPM_MATCH(eth_sxtables_lpm_name,
		( ETH_KEY1 ), 22, 2)

XDP2_STABLE_LPM_TABLE_NAME_ENTS(
	eth_sxtables_lpm_name_ents,
	struct ethhdr *,
	(
		(h_dest, dst),
		(h_source, src),
		(h_proto, proto)
	), __u32, -1,
	(
		(( ETH_KEY0 ), 17, 1),
		(( ETH_KEY1 ), 22, 2)
        )
)

XDP2_STABLE_LPM_TABLE_CAST(
	ipv4_sxtables_lpm_cast,
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

XDP2_STABLE_ADD_LPM_MATCH(ipv4_sxtables_lpm_cast,
	( IPV4_KEY0 ), 23, 1)

XDP2_STABLE_ADD_LPM_MATCH(ipv4_sxtables_lpm_cast,
	( IPV4_KEY1 ), 11, HIT_DROP)

XDP2_STABLE_ADD_LPM_MATCH(ipv4_sxtables_lpm_cast,
	( IPV4_KEY2 ), 23, HIT_NOACTION)

XDP2_STABLE_ADD_LPM_MATCH(ipv4_sxtables_lpm_cast,
	( IPV4_KEY3 ), 5, 34)

XDP2_STABLE_ADD_LPM_MATCH(ipv4_sxtables_lpm_cast,
	( IPV4_KEY4_ALT ), 23, 35)

XDP2_STABLE_ADD_LPM_MATCH(ipv4_sxtables_lpm_cast,
	( IPV4_KEY5_ALT ), 29, HIT_NOACTION)

XDP2_STABLE_LPM_TABLE_CAST_ENTS(
	ipv4_sxtables_lpm_cast_ents,
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
		(( IPV4_KEY0 ), 23, 1),
		(( IPV4_KEY1 ), 11, HIT_DROP),
		(( IPV4_KEY2 ), 23, HIT_NOACTION),
		(( IPV4_KEY3_ALT ), 5, 44),
		(( IPV4_KEY4_ALT ), 23, 45),
		(( IPV4_KEY5_ALT ), 29, HIT_NOACTION)
	)
)

XDP2_STABLE_LPM_TABLE_NAME_CAST(
	ipv4_sxtables_lpm_name_cast,
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

XDP2_STABLE_ADD_LPM_MATCH(ipv4_sxtables_lpm_name_cast,
	(.mihl = IPV4_IHL_KEY0, .mtos = IPV4_TOS_KEY0,
	 .src = IPV4_SADDR_KEY0, .dst = IPV4_DADDR_KEY0,
	 .mttl = IPV4_TTL_KEY0, .proto = IPV4_PROTOCOL_KEY0),
	23, 1)

XDP2_STABLE_ADD_LPM_MATCH(ipv4_sxtables_lpm_name_cast,
	( IPV4_KEY1 ), 11, HIT_DROP)

XDP2_STABLE_ADD_LPM_MATCH(ipv4_sxtables_lpm_name_cast,
	( IPV4_KEY2 ), 23, HIT_NOACTION)

XDP2_STABLE_ADD_LPM_MATCH(ipv4_sxtables_lpm_name_cast,
	( IPV4_KEY3_ALT ), 5, 54)

XDP2_STABLE_ADD_LPM_MATCH(ipv4_sxtables_lpm_name_cast,
	( IPV4_KEY4_ALT ), 23, 55)

XDP2_STABLE_ADD_LPM_MATCH(ipv4_sxtables_lpm_name_cast,
	( IPV4_KEY5_ALT ), 29, HIT_NOACTION)

XDP2_STABLE_LPM_TABLE_NAME_CAST_ENTS(
	ipv4_sxtables_lpm_name_cast_ents,
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
	     (( IPV4_KEY0 ), 23, 1),
	     (( IPV4_KEY1 ), 11, HIT_DROP),
	     (( IPV4_KEY2 ), 23, HIT_NOACTION),
	     (( IPV4_KEY3_ALT ), 5, 64),
	     (( IPV4_KEY4_ALT ), 23, 65),
	     (( .mihl = IPV4_IHL_KEY5_ALT, .mtos = IPV4_TOS_KEY5_ALT,
		.src = IPV4_SADDR_KEY5_ALT, .dst = IPV4_DADDR_KEY5_ALT,
		.mttl = IPV4_TTL_KEY5_ALT, .proto = IPV4_PROTOCOL_KEY5_ALT),
	      29, HIT_NOACTION)
	)
)

XDP2_STABLE_LPM_TABLE_SKEY(
	eth_sxtables_lpm_skey,
	struct ethhdr *, __u32, -1U
)

XDP2_STABLE_ADD_LPM_MATCH(eth_sxtables_lpm_skey,
		(.h_dest = ETH_DST_KEY0,
		 .h_source = ETH_SRC_KEY0,
		 .h_proto = ETH_PROTO_KEY0),
		17, 1)

XDP2_STABLE_ADD_LPM_MATCH(eth_sxtables_lpm_skey,
		( ETH_KEY1 ), 22, 2)

XDP2_STABLE_LPM_TABLE_SKEY_ENTS(
	eth_sxtables_lpm_skey_ents,
	struct ethhdr *, __u32, -1U,
	(
		((.h_dest = ETH_DST_KEY0, .h_source = ETH_SRC_KEY0,
		  .h_proto = ETH_PROTO_KEY0),
		 17, 81),
		((.h_dest = ETH_DST_KEY1, .h_source = ETH_SRC_KEY1,
		  .h_proto = ETH_PROTO_KEY1),
		 22, 82)
	)
)

XDP2_STABLE_ADD_LPM_MATCH(eth_sxtables_lpm_skey_ents,
		(.h_dest = ETH_DST_KEY0,
		 .h_source = ETH_SRC_KEY0,
		 .h_proto = ETH_PROTO_KEY0),
		17, 1)

XDP2_STABLE_ADD_LPM_MATCH(eth_sxtables_lpm_skey_ents,
		(.h_dest = ETH_DST_KEY1,
		 .h_source = ETH_SRC_KEY1,
		 .h_proto = ETH_PROTO_KEY1),
		22, 2)

void run_sxtables_lpm(void)
{
	int i, status_check;

	for (i = 0; i < ARRAY_SIZE(eth_keys); i++) {
		status_check = get_base_eth_lpm_status(i);

		RUN_ONE_SX_ETH(lpm);
		RUN_ONE_SX_ETH(lpm_ents);
		RUN_ONE_SX_ETH(lpm_name);
		RUN_ONE_SX_ETH(lpm_name_ents);
		RUN_ONE_SX_ETH_SKEY(lpm_skey);
		RUN_ONE_SX_ETH_SKEY(lpm_skey_ents);
	}

	for (i = 0; i < ARRAY_SIZE(iphdr_keys); i++) {
		status_check = get_base_ipv4_lpm_status(i);

		RUN_ONE_SX_IPV4(lpm_cast);
		RUN_ONE_SX_IPV4(lpm_cast_ents);
		RUN_ONE_SX_IPV4(lpm_name_cast);
		RUN_ONE_SX_IPV4(lpm_name_cast_ents);
	}
}
