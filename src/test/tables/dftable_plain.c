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

static struct xdp2_dtable_plain_table  *eth_plain, *ipv4_plain;

XDP2_DFTABLE_PLAIN_TABLE(
	eth_dtable_plain,
	struct ethhdr *,
	(
		h_dest,
		h_source,
		h_proto
	), Miss_dtable, ()
)

XDP2_DFTABLE_PLAIN_TABLE_NAME(
	eth_dtable_plain_name,
	struct ethhdr *,
	(
		(h_dest, dst),
		(h_source, src),
		(h_proto, proto)
	), Miss_dtable, ()
)

XDP2_DFTABLE_PLAIN_TABLE_CAST(
	ipv4_dtable_plain_cast,
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

XDP2_DFTABLE_PLAIN_TABLE_NAME_CAST(
	ipv4_dtable_plain_name_cast,
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

XDP2_DFTABLE_PLAIN_TABLE_SKEY(
	eth_dtable_plain_skey, struct ethhdr *, Miss_dtable, ()
)

static void init_tables(void)
{
	struct __xdp2_dtable_entry_func_target ftarg;
	int i, ident;

	XDP2_DFTABLE_ADD_PLAIN(eth_dtable_plain, (ETH_KEY0),
			       forward_dtable, (void *)1);

	XDP2_DFTABLE_ADD_PLAIN(eth_dtable_plain, (ETH_KEY1),
			       forward_dtable, (void *)2);

	eth_dtable_plain_name_add(
		(struct eth_dtable_plain_name_key_struct *)
							&eth_keys_match[0],
		forward_dtable, (void *)1);

	XDP2_DFTABLE_ADD_PLAIN(eth_dtable_plain_name,
			       (.dst = ETH_DST_KEY1,
				.src = ETH_SRC_KEY1,
				.proto = ETH_PROTO_KEY1,),
			       forward_dtable, (void *)2);

	/* Plain cast */
	ident = 0;

	ftarg.arg = NULL;

	ftarg.func = Miss_dtable;
	ipv4_plain = xdp2_dtable_create_plain("User ipv4 plain",
			sizeof(struct ipv4_common_keys), &ftarg,
			sizeof(ftarg), &ident);

	ftarg.func = forward_dtable;
	ftarg.arg = (void *)1;
	xdp2_dtable_add_plain(ipv4_plain, 0,
		&ipv4_keys_match[0], &ftarg);
	ftarg.func = drop_dtable;
	ident = xdp2_dtable_add_plain(ipv4_plain, 0,
		&ipv4_keys_match[1], &ftarg);
	ftarg.func = NoAction_dtable;
	xdp2_dtable_add_plain(ipv4_plain, 0,
		&ipv4_keys_match[2], &ftarg);
	ftarg.func = forward_dtable;
	ftarg.arg = (void *)12;
	xdp2_dtable_add_plain(ipv4_plain, 0,
		&ipv4_keys_match[3], &ftarg);
	ftarg.func = drop_dtable;
	xdp2_dtable_add_plain(ipv4_plain, 0,
		&ipv4_keys_match[4], &ftarg);
	ftarg.func = NoAction_dtable;
	xdp2_dtable_add_plain(ipv4_plain, 0,
		&ipv4_keys_match[5], &ftarg);

	xdp2_dtable_del_plain(ipv4_plain, &ipv4_keys_match[0]);
	ftarg.func = forward_dtable;
	ftarg.arg = (void *)9;
	xdp2_dtable_add_plain(ipv4_plain, 0,
		&ipv4_keys_match[0], &ftarg);

	xdp2_dtable_del_plain_by_id(ipv4_plain, ident);
	ftarg.func = drop_dtable;
	ident = xdp2_dtable_add_plain(ipv4_plain, 0,
		&ipv4_keys_match[1], &ftarg);

	ftarg.func = forward_dtable;
	ftarg.arg = (void *)(__u64)12;
	xdp2_dtable_change_plain_by_id(ipv4_plain, ident,
					&ftarg);
	ftarg.func = drop_dtable;
	xdp2_dtable_change_plain(ipv4_plain, &ipv4_keys_match[1],
				  &ftarg);

	ftarg.func = forward_dtable;
	ipv4_dtable_plain_cast_add(
		(struct ipv4_dtable_plain_cast_key_struct *)
							&ipv4_keys_match[0],
		forward_dtable, (void *)9);

	ftarg.func = drop_dtable;
	XDP2_DFTABLE_ADD_PLAIN(ipv4_dtable_plain_cast, (IPV4_KEY1),
			       drop_dtable, (void *)2);

	XDP2_DFTABLE_ADD_PLAIN(ipv4_dtable_plain_cast, (IPV4_KEY2),
			       NoAction_dtable, NULL);

	ipv4_dtable_plain_cast_add(
		(struct ipv4_dtable_plain_cast_key_struct *)
							&ipv4_keys_match[3],
		forward_dtable, (void *)12);

	XDP2_DFTABLE_ADD_PLAIN(ipv4_dtable_plain_cast, (IPV4_KEY4),
			       drop_dtable, NULL);

	XDP2_DFTABLE_ADD_PLAIN(ipv4_dtable_plain_cast, (IPV4_KEY5),
			       NoAction_dtable, NULL);

	ident = 0;
	ftarg.func = Miss_dtable;
	eth_plain = xdp2_dtable_create_plain("User eth plain",
			sizeof(struct ethhdr), &ftarg, sizeof(ftarg), &ident);

	for (i = 0; i < ARRAY_SIZE(eth_keys_match); i++) {
		ftarg.func = forward_dtable;
		ftarg.arg = (void *)(__u64)(i + 1);
		xdp2_dtable_add_plain(eth_plain, 0,
			&eth_keys_match[i], &ftarg);
	}

	/* Plain cast name */

	XDP2_DFTABLE_ADD_PLAIN(ipv4_dtable_plain_name_cast,
			       (IPV4_KEY0), forward_dtable, (void *)9);

	{
		struct ipv4_dtable_plain_name_cast_key_struct
			key = {
			  .mihl = IPV4_IHL_KEY1,
			  .mtos = IPV4_TOS_KEY1, .src = IPV4_SADDR_KEY1,
			  .dst = IPV4_DADDR_KEY1, .mttl = IPV4_TTL_KEY1,
			  .proto = IPV4_PROTOCOL_KEY1
			};

		ipv4_dtable_plain_name_cast_add( &key, drop_dtable, NULL);
	}

	XDP2_DFTABLE_ADD_PLAIN(ipv4_dtable_plain_name_cast,
			       (IPV4_KEY2), NoAction_dtable, NULL);

	XDP2_DFTABLE_ADD_PLAIN(ipv4_dtable_plain_name_cast,
			       (IPV4_KEY3), forward_dtable, (void *)12);

	{
		struct ipv4_dtable_plain_name_cast_key_struct
			key = {
			  .mihl = IPV4_IHL_KEY4, .mtos = IPV4_TOS_KEY4,
			  .src = IPV4_SADDR_KEY4, .dst = IPV4_DADDR_KEY4,
			  .mttl = IPV4_TTL_KEY4, .proto = IPV4_PROTOCOL_KEY4
			};

		ipv4_dtable_plain_name_cast_add( &key, drop_dtable, NULL);
	}

	XDP2_DFTABLE_ADD_PLAIN(ipv4_dtable_plain_name_cast,
			       (IPV4_KEY5), NoAction_dtable, NULL);

	{
		struct ethhdr key = {
			  .h_dest = ETH_DST_KEY1,
			  .h_source = ETH_SRC_KEY1,
			  .h_proto = ETH_PROTO_KEY1,
			};

		eth_dtable_plain_skey_add(&key, forward_dtable, (void *)2);
	}
	XDP2_DFTABLE_ADD_PLAIN(eth_dtable_plain_skey,
			       (.h_dest = ETH_DST_KEY0,
				.h_source = ETH_SRC_KEY0,
				.h_proto = ETH_PROTO_KEY0,
			       ), forward_dtable, (void *)1);
}

void run_dtable_plain(void)
{
	int i, status_check;
	struct my_ctx ctx;
	char name[100];

	init_tables();

	ctx.name = name;

	for (i = 0; i < ARRAY_SIZE(eth_keys); i++) {
		status_check = get_base_eth_status(i);

		RUN_ONE_DETH(plain);
		RUN_ONE_DETH(plain_name);
		RUN_ONE_DETH_SKEY(plain_skey);
		RUN_ONE_TETH(plain, plain, eth_plain);
	}

	for (i = 0; i < ARRAY_SIZE(iphdr_keys); i++) {
		status_check = get_base_ipv4_status(i);

		RUN_ONE_DIPV4(plain_cast);
		RUN_ONE_DIPV4(plain_name_cast);
		RUN_ONE_TIPV4(plain, plain, ipv4_plain);
	}
}
