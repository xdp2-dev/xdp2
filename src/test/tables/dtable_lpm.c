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

static struct xdp2_dtable_lpm_table  *eth_lpm, *ipv4_lpm;

XDP2_DTABLE_LPM_TABLE(
	eth_dxtable_lpm,
	struct ethhdr *,
	(
		h_dest,
		h_source,
		h_proto
	), __u32, MISS, (.size = 10000)
)

XDP2_DTABLE_LPM_TABLE_NAME(
	eth_dxtable_lpm_name,
	struct ethhdr *,
	(
		(h_dest, dst),
		(h_source, src),
		(h_proto, proto)
	), __u32, MISS, ()
)

XDP2_DTABLE_LPM_TABLE_CAST(
	ipv4_dxtable_lpm_cast,
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

XDP2_DTABLE_LPM_TABLE_NAME_CAST(
	ipv4_dxtable_lpm_name_cast,
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

XDP2_DTABLE_LPM_TABLE_SKEY(
	eth_dxtable_lpm_skey, struct ethhdr *, __u32, MISS, ()
)


static void init_tables(void)
{
	int ident;
	__u32 v;

	{
		struct eth_dxtable_lpm_key_struct key = { ETH_KEY0 };

		eth_dxtable_lpm_add(&key, 17, 1);
	}

	XDP2_DTABLE_ADD_LPM(eth_dxtable_lpm, ( ETH_KEY1 ), 22, 2);

	{
		struct eth_dxtable_lpm_name_key_struct key = { ETH_KEY0 };

		eth_dxtable_lpm_name_add(&key, 17, 1);
	}

	XDP2_DTABLE_ADD_LPM(eth_dxtable_lpm_name,
			 ( .dst = ETH_DST_KEY1,
			   .src = ETH_SRC_KEY1,
			   .proto = ETH_PROTO_KEY1
			 ), 15, 2);

	{
		struct ipv4_dxtable_lpm_cast_key_struct key = { IPV4_KEY0 };

		ipv4_dxtable_lpm_cast_add(&key, 22, 2);
	}

	/* Plain cast */
	{
		struct ipv4_dxtable_lpm_cast_key_struct key = { IPV4_KEY0 };

		ipv4_dxtable_lpm_cast_add(&key, 23, 1);
	}

	XDP2_DTABLE_ADD_LPM(ipv4_dxtable_lpm_cast,
		( IPV4_KEY1 ), 11, HIT_DROP);

	XDP2_DTABLE_ADD_LPM(ipv4_dxtable_lpm_cast,
		( IPV4_KEY2 ), 23, HIT_NOACTION);

	{
		struct ipv4_dxtable_lpm_cast_key_struct key = { IPV4_KEY3 };

		ipv4_dxtable_lpm_cast_add(&key, 5, 1);
	}

	XDP2_DTABLE_ADD_LPM(ipv4_dxtable_lpm_cast,
		( IPV4_KEY4_ALT ), 23, HIT_DROP);

	XDP2_DTABLE_ADD_LPM(ipv4_dxtable_lpm_cast,
		( IPV4_KEY5_ALT ), 29, HIT_NOACTION);

	ident = 0;
	v = MISS;
	eth_lpm = xdp2_dtable_create_lpm("User eth lpm",
			sizeof(struct ethhdr), &v, sizeof(v), &ident);

	v = 1;
	xdp2_dtable_add_lpm(eth_lpm, 0,
		&eth_keys_match[0], 17, &v);
	v = 2;
	xdp2_dtable_add_lpm(eth_lpm, 0,
		&eth_keys_match[1], 22, &v);

	/* LPM cast name */

	XDP2_DTABLE_ADD_LPM(ipv4_dxtable_lpm_name_cast,
		( IPV4_KEY0 ), 23, 1);

	{
		struct ipv4_dxtable_lpm_name_cast_key_struct key = {
			.mihl = IPV4_IHL_KEY1,
			.mtos = IPV4_TOS_KEY1, .src = IPV4_SADDR_KEY1,
			.dst = IPV4_DADDR_KEY1, .mttl = IPV4_TTL_KEY1,
			.proto = IPV4_PROTOCOL_KEY1
		};

		ipv4_dxtable_lpm_name_cast_add(&key, 11, HIT_DROP);
	}

	XDP2_DTABLE_ADD_LPM(ipv4_dxtable_lpm_name_cast,
		( IPV4_KEY2 ), 23, HIT_NOACTION);

	XDP2_DTABLE_ADD_LPM(ipv4_dxtable_lpm_name_cast,
		( IPV4_KEY3 ), 5, 2);

	{
		struct ipv4_dxtable_lpm_name_cast_key_struct key = {
			.mihl = IPV4_IHL_KEY4,
			.mtos = IPV4_TOS_KEY4, .src = IPV4_SADDR_KEY4,
			.dst = IPV4_DADDR_KEY4, .mttl = IPV4_TTL_KEY4,
			.proto = IPV4_PROTOCOL_KEY4
		};

		ipv4_dxtable_lpm_name_cast_add(&key, 23, HIT_DROP);
	}

	XDP2_DTABLE_ADD_LPM(ipv4_dxtable_lpm_name_cast,
		( IPV4_KEY5_ALT ), 29, HIT_NOACTION);

	{
		struct ethhdr key = { ETH_KEY0 };

		eth_dxtable_lpm_skey_add(&key, 29, 1);
	}

	XDP2_DTABLE_ADD_LPM(eth_dxtable_lpm_skey,
		 (.h_dest = ETH_DST_KEY1,
		  .h_source = ETH_SRC_KEY1,
		  .h_proto = ETH_PROTO_KEY1
		  ), 12, 2);

	ident = 0;
	v = MISS;
	ipv4_lpm = xdp2_dtable_create_lpm("User ipv4 lpm",
			sizeof(struct ipv4_common_keys), &v,
			sizeof(v), &ident);

	v = 1;
	xdp2_dtable_add_lpm(ipv4_lpm, 0,
		&ipv4_keys_match_alt[0], 23, &v);
	v = HIT_DROP;
	xdp2_dtable_add_lpm(ipv4_lpm, 0,
		&ipv4_keys_match_alt[1], 11, &v);
	v = HIT_NOACTION;
	xdp2_dtable_add_lpm(ipv4_lpm, 0,
		&ipv4_keys_match_alt[2], 23, &v);

	v = 2;
	xdp2_dtable_add_lpm(ipv4_lpm, 0,
		&ipv4_keys_match_alt[3], 5, &v);

	v = HIT_DROP;
	xdp2_dtable_add_lpm(ipv4_lpm, 0,
		&ipv4_keys_match_alt[4], 23, &v);

	v = HIT_NOACTION;
	xdp2_dtable_add_lpm(ipv4_lpm, 0,
		&ipv4_keys_match_alt[5], 29, &v);
}

void run_dxtable_lpm(void)
{
	int i, status_check;

	init_tables();


	for (i = 0; i < ARRAY_SIZE(eth_keys); i++) {
		status_check = get_base_eth_lpm_status(i);

		RUN_ONE_DX_DETH(lpm);
		RUN_ONE_DX_DETH(lpm_name);
		RUN_ONE_DX_DETH_SKEY(lpm_skey);
		RUN_ONE_DX_TETH(lpm, lpm, eth_lpm);
	}

	for (i = 0; i < ARRAY_SIZE(iphdr_keys); i++) {
		status_check = get_base_ipv4_lpm_status(i);

		RUN_ONE_DX_DIPV4(lpm_cast);
		RUN_ONE_DX_DIPV4(lpm_name_cast);
		RUN_ONE_DX_TIPV4(lpm, lpm, ipv4_lpm);
	}
}
