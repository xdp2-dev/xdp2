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

XDP2_DFTABLE_LPM_TABLE(
	eth_dtable_lpm,
	struct ethhdr *,
	(
		h_dest,
		h_source,
		h_proto
	), Miss_dtable, ()
)

XDP2_DFTABLE_LPM_TABLE_NAME(
	eth_dtable_lpm_name,
	struct ethhdr *,
	(
		(h_dest, dst),
		(h_source, src),
		(h_proto, proto)
	), Miss_dtable, ()
)

XDP2_DFTABLE_LPM_TABLE_CAST(
	ipv4_dtable_lpm_cast,
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

XDP2_DFTABLE_LPM_TABLE_NAME_CAST(
	ipv4_dtable_lpm_name_cast,
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

XDP2_DFTABLE_LPM_TABLE_SKEY(
	eth_dtable_lpm_skey, struct ethhdr *, Miss_dtable, ()
)

static void init_tables(void)
{
	struct __xdp2_dtable_entry_func_target ftarg;
	int ident;

	{
		struct eth_dtable_lpm_key_struct key = { ETH_KEY0 };

		eth_dtable_lpm_add(&key, 17, forward_dtable, (void *)1);
	}

	XDP2_DFTABLE_ADD_LPM(eth_dtable_lpm,
			 ( ETH_KEY1 ), 22, forward_dtable, (void *)2);

	{
		struct eth_dtable_lpm_name_key_struct key = { ETH_KEY0 };

		eth_dtable_lpm_name_add(&key, 17, forward_dtable, (void *)1);
	}

	XDP2_DFTABLE_ADD_LPM(eth_dtable_lpm_name,
			 ( .dst = ETH_DST_KEY1,
			   .src = ETH_SRC_KEY1,
			   .proto = ETH_PROTO_KEY1
			 ), 15, forward_dtable, (void *)2);

	{
		struct ipv4_dtable_lpm_cast_key_struct key = { IPV4_KEY0 };

		ipv4_dtable_lpm_cast_add(&key, 22, forward_dtable, (void *)2);
	}

	/* Plain cast */
	{
		struct ipv4_dtable_lpm_cast_key_struct key =
			{ IPV4_KEY0 };

		ipv4_dtable_lpm_cast_add(&key, 23, forward_dtable, (void *)1);
	}

	XDP2_DFTABLE_ADD_LPM(ipv4_dtable_lpm_cast,
		( IPV4_KEY1 ), 11, drop_dtable, NULL);

	XDP2_DFTABLE_ADD_LPM(ipv4_dtable_lpm_cast,
		( IPV4_KEY2 ), 23, NoAction_dtable, NULL);

	{
		struct ipv4_dtable_lpm_cast_key_struct key =
			{ IPV4_KEY3 };

		ipv4_dtable_lpm_cast_add(&key, 5, forward_dtable, NULL);
	}

	XDP2_DFTABLE_ADD_LPM(ipv4_dtable_lpm_cast,
		( IPV4_KEY4_ALT ), 23, drop_dtable, NULL);

	XDP2_DFTABLE_ADD_LPM(ipv4_dtable_lpm_cast,
		( IPV4_KEY5_ALT ), 29, NoAction_dtable, NULL);

	ftarg.arg = NULL;

	ident = 0;
	ftarg.func = Miss_dtable;
	eth_lpm = xdp2_dtable_create_lpm("User eth lpm",
			sizeof(struct ethhdr), &ftarg, sizeof(ftarg), &ident);

	ftarg.func = forward_dtable;
	ftarg.arg = (void *)1;
	xdp2_dtable_add_lpm(eth_lpm, 0,
		&eth_keys_match[0], 17, &ftarg);
	ftarg.func = forward_dtable;
	ftarg.arg = (void *)2;
	xdp2_dtable_add_lpm(eth_lpm, 0,
		&eth_keys_match[1], 22, &ftarg);

	/* LPM cast name */

	XDP2_DFTABLE_ADD_LPM(ipv4_dtable_lpm_name_cast,
		( IPV4_KEY0 ), 23, forward_dtable, (void *)1);

	{
		struct ipv4_dtable_lpm_name_cast_key_struct key =
			{ .mihl = IPV4_IHL_KEY1,
			  .mtos = IPV4_TOS_KEY1, .src = IPV4_SADDR_KEY1,
			  .dst = IPV4_DADDR_KEY1, .mttl = IPV4_TTL_KEY1,
			  .proto = IPV4_PROTOCOL_KEY1
			};

		ipv4_dtable_lpm_name_cast_add( &key, 11, drop_dtable, NULL);
	}

	XDP2_DFTABLE_ADD_LPM(ipv4_dtable_lpm_name_cast,
		( IPV4_KEY2 ), 23, NoAction_dtable, (void *)1);

	XDP2_DFTABLE_ADD_LPM(ipv4_dtable_lpm_name_cast,
		( IPV4_KEY3 ), 5, forward_dtable, NULL);

	{
		struct ipv4_dtable_lpm_name_cast_key_struct key =
			{
			  .mihl = IPV4_IHL_KEY4,
			  .mtos = IPV4_TOS_KEY4, .src = IPV4_SADDR_KEY4,
			  .dst = IPV4_DADDR_KEY4, .mttl = IPV4_TTL_KEY4,
			  .proto = IPV4_PROTOCOL_KEY4
			};

		ipv4_dtable_lpm_name_cast_add( &key, 23, drop_dtable, NULL);
	}

	XDP2_DFTABLE_ADD_LPM(ipv4_dtable_lpm_name_cast,
		( IPV4_KEY5_ALT ), 29, NoAction_dtable, (void *)1);

	{
		struct ethhdr key = { ETH_KEY0 };

		eth_dtable_lpm_skey_add(&key, 29, forward_dtable, (void *)1);
	}

	XDP2_DFTABLE_ADD_LPM(eth_dtable_lpm_skey,
		 (.h_dest = ETH_DST_KEY1,
		  .h_source = ETH_SRC_KEY1,
		  .h_proto = ETH_PROTO_KEY1
		  ), 12, forward_dtable, (void *)2);

	ident = 0;
	ftarg.func = Miss_dtable;
	ipv4_lpm = xdp2_dtable_create_lpm("User ipv4 lpm",
			sizeof(struct ipv4_common_keys), &ftarg,
			sizeof(ftarg), &ident);

	ftarg.func = forward_dtable;
	ftarg.arg = (void *)1;
	xdp2_dtable_add_lpm(ipv4_lpm, 0,
		&ipv4_keys_match_alt[0], 23, &ftarg);
	ftarg.func = drop_dtable;
	xdp2_dtable_add_lpm(ipv4_lpm, 0,
		&ipv4_keys_match_alt[1], 11, &ftarg);
	ftarg.func = NoAction_dtable;
	xdp2_dtable_add_lpm(ipv4_lpm, 0,
		&ipv4_keys_match_alt[2], 23, &ftarg);
	ftarg.func = forward_dtable;
	ftarg.arg = (void *)2;
	xdp2_dtable_add_lpm(ipv4_lpm, 0,
		&ipv4_keys_match_alt[3], 5, &ftarg);
	ftarg.func = drop_dtable;
	xdp2_dtable_add_lpm(ipv4_lpm, 0,
		&ipv4_keys_match_alt[4], 23, &ftarg);
	ftarg.func = NoAction_dtable;
	xdp2_dtable_add_lpm(ipv4_lpm, 0,
		&ipv4_keys_match_alt[5], 29, &ftarg);
}

void run_dtable_lpm(void)
{
	int i, status_check;
	struct my_ctx ctx;
	char name[100];

	init_tables();

	ctx.name = name;

	for (i = 0; i < ARRAY_SIZE(eth_keys); i++) {
		status_check = get_base_eth_lpm_status(i);

		RUN_ONE_DETH(lpm);
		RUN_ONE_DETH(lpm_name);
		RUN_ONE_DETH_SKEY(lpm_skey);
		RUN_ONE_TETH(lpm, lpm, eth_lpm);
	}

	for (i = 0; i < ARRAY_SIZE(iphdr_keys); i++) {
		status_check = get_base_ipv4_lpm_status(i);

		RUN_ONE_DIPV4(lpm_cast);
		RUN_ONE_DIPV4(lpm_name_cast);
		RUN_ONE_TIPV4(lpm, lpm, ipv4_lpm);
	}
}
