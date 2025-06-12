// SPDX-License-Identifier: BSD-2-Clause-FreeBSD
/*
 * Copyright (c) 2020, 2021 by Mojatatu Networks.
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
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include "flowdis/flow_dissector.h"
#include "xdp2/utility.h"
#include "test-parser-core.h"

struct flowdis_priv {
	struct flow_dissector fd;
};

struct alt_keys {
	struct flow_dissector_key_icmp icmp;
	struct flow_dissector_key_eth_addrs eth_addrs;
	struct flow_dissector_key_arp arp;
	struct flow_dissector_key_vlan vlan;
	struct flow_dissector_key_ipv4_addrs enc_ipv4_addrs;
	struct flow_dissector_key_ipv6_addrs enc_ipv6_addrs;
	struct flow_dissector_key_control enc_control;
	struct flow_dissector_key_ports enc_ports;
	struct flow_dissector_key_mpls mpls;
	struct flow_dissector_key_tcp tcp;
	struct flow_dissector_key_ip ip;
	struct flow_dissector_key_vlan cvlan;
	struct flow_dissector_key_ip enc_ip;
	struct flow_dissector_key_enc_opts enc_opts;
	struct flow_dissector_key_meta meta;
	struct flow_dissector_key_ct ct;
	struct flow_dissector_key_keyid gre_keyid;
	struct flow_dissector_key_keyid mpls_entropy;
	struct flow_dissector_key_keyid enc_keyid;
	struct flow_dissector_key_ports ports_range;
};

struct all_keys {
	struct flow_keys f;	/* For computing hash */
	struct alt_keys a;	/* Other keys */
};

#define __FDK(ID, F) { .key_id = (ID), .offset = offsetof(struct all_keys, F) }

static const struct flow_dissector_key fdk[] = {
	/* From flow_keys */
	__FDK(FLOW_DISSECTOR_KEY_CONTROL, f.control),
	__FDK(FLOW_DISSECTOR_KEY_BASIC, f.basic),
	__FDK(FLOW_DISSECTOR_KEY_IPV4_ADDRS, f.addrs.v4addrs),
	__FDK(FLOW_DISSECTOR_KEY_IPV6_ADDRS, f.addrs.v6addrs),
	__FDK(FLOW_DISSECTOR_KEY_TIPC, f.addrs.tipckey),
	__FDK(FLOW_DISSECTOR_KEY_PORTS, f.ports),
	__FDK(FLOW_DISSECTOR_KEY_VLAN, f.vlan),
	__FDK(FLOW_DISSECTOR_KEY_FLOW_LABEL, f.tags),

	/* Additional keys beyond flow_keys */
	__FDK(FLOW_DISSECTOR_KEY_PORTS_RANGE, a.ports_range),
	__FDK(FLOW_DISSECTOR_KEY_ICMP, a.icmp),
	__FDK(FLOW_DISSECTOR_KEY_ETH_ADDRS, a.eth_addrs),
	__FDK(FLOW_DISSECTOR_KEY_ARP, a.arp),
	__FDK(FLOW_DISSECTOR_KEY_MPLS_ENTROPY, a.mpls_entropy),
	__FDK(FLOW_DISSECTOR_KEY_GRE_KEYID, a.gre_keyid),
	__FDK(FLOW_DISSECTOR_KEY_ENC_KEYID, a.enc_keyid),
	__FDK(FLOW_DISSECTOR_KEY_ENC_IPV4_ADDRS, a.enc_ipv4_addrs),
	__FDK(FLOW_DISSECTOR_KEY_ENC_IPV6_ADDRS, a.enc_ipv6_addrs),
	__FDK(FLOW_DISSECTOR_KEY_ENC_CONTROL, a.enc_control),
	__FDK(FLOW_DISSECTOR_KEY_ENC_PORTS, a.enc_ports),
	__FDK(FLOW_DISSECTOR_KEY_ENC_IP, a.enc_ip),
	__FDK(FLOW_DISSECTOR_KEY_ENC_OPTS, a.enc_opts),
	__FDK(FLOW_DISSECTOR_KEY_MPLS, a.mpls),
	__FDK(FLOW_DISSECTOR_KEY_TCP, a.tcp),
	__FDK(FLOW_DISSECTOR_KEY_IP, a.ip),
	__FDK(FLOW_DISSECTOR_KEY_CVLAN, a.cvlan),
	__FDK(FLOW_DISSECTOR_KEY_META, a.meta),
	__FDK(FLOW_DISSECTOR_KEY_CT, a.ct),
};

static void core_flowdis_help(void)
{
	fprintf(stderr, "For the `flowdis' core, arguments must be either not "
		"given or zero length.\n\n"
		"This core uses the flowdis library which is a port ot the "
		"Linux kernel flow-dissector code.\n");
}

static void *core_flowdis_init(const char *args)
{
	struct flowdis_priv *p;

	if (args && *args) {
		fprintf(stderr, "The flowdis core takes no arguments.\n");
		exit(-1);
	}
	p = calloc(1, sizeof(struct flowdis_priv));
	if (!p) {
		fprintf(stderr, "Flow dissector init failed\n");
		exit(-1);
	}

	init_default_flow_dissectors();
	skb_flow_dissector_init(&p->fd, fdk, ARRAY_SIZE(fdk));

	return p;
}

#define FIELD_COPY(dstfield, srcfield)					\
	memcpy(&out->dstfield, srcfield, sizeof(out->dstfield))

static const char *core_flowdis_process(void *pv, void *data, size_t len,
					struct test_parser_out *out,
					unsigned int flags, long long *time)
{
	struct flowdis_priv *p = pv;
	struct ethhdr *ehdr = data;
	const char *msg = NULL;
	int suc = 0;
	struct all_keys keys;

	memset(&keys, 0, sizeof(keys));
	memset(out, 0, sizeof(*out));

	if (!(flags & CORE_F_NOCORE)) {
		struct timespec begin_tp, now_tp;

		clock_gettime(CLOCK_MONOTONIC_RAW, &begin_tp);
		suc = __skb_flow_dissect_err(0, &p->fd, &keys, data,
					     ehdr->h_proto, ETH_HLEN, len, 0,
					     &msg);
		if (!suc) {
			if (!msg)
				msg = "__skb_flow_dissect_err failed but "
				      "provided no message";
			return msg;
		}

		clock_gettime(CLOCK_MONOTONIC_RAW, &now_tp);
		*time += (now_tp.tv_sec - begin_tp.tv_sec) * 1000000000 +
			 (now_tp.tv_nsec - begin_tp.tv_nsec);
	}

	out->k_control.thoff = keys.f.control.thoff;

	switch (keys.f.control.addr_type) {
	case 0:
		out->k_control.addr_type = 0;
		break;
	case FLOW_DISSECTOR_KEY_IPV4_ADDRS:
		out->k_control.addr_type = ADDR_TYPE_IPv4;
		out->k_ipv4_addrs.src = keys.f.addrs.v4addrs.src;
		out->k_ipv4_addrs.dst = keys.f.addrs.v4addrs.dst;
		break;
	case FLOW_DISSECTOR_KEY_IPV6_ADDRS:
		out->k_control.addr_type = ADDR_TYPE_IPv6;
		FIELD_COPY(k_ipv6_addrs.src, &keys.f.addrs.v6addrs.src);
		FIELD_COPY(k_ipv6_addrs.dst, &keys.f.addrs.v6addrs.dst);
		break;
	case FLOW_DISSECTOR_KEY_TIPC:
		out->k_control.addr_type = ADDR_TYPE_TIPC;
		out->k_tipc.key = keys.f.addrs.tipckey.key;
		break;
	default:
		out->k_control.addr_type = ADDR_TYPE_OTHER;
		break;
	}

	out->k_control.flags = keys.f.control.flags;
	out->k_basic.n_proto = keys.f.basic.n_proto;
	out->k_basic.ip_proto = keys.f.basic.ip_proto;
	out->k_ports.src = keys.f.ports.src;
	out->k_ports.dst = keys.f.ports.dst;

	out->k_icmp.type = keys.a.icmp.type;
	out->k_icmp.code = keys.a.icmp.code;
	out->k_icmp.id = keys.a.icmp.id;

	FIELD_COPY(k_eth_addrs.src, &keys.a.eth_addrs.src);
	FIELD_COPY(k_eth_addrs.dst, &keys.a.eth_addrs.dst);

	out->k_ports_range.src = keys.a.ports_range.src;
	out->k_ports_range.dst = keys.a.ports_range.dst;
	out->k_arp.s_ip = keys.a.arp.sip;
	out->k_arp.t_ip = keys.a.arp.tip;
	out->k_arp.op = keys.a.arp.op;
	FIELD_COPY(k_arp.s_hw, &keys.a.arp.sha);
	FIELD_COPY(k_arp.t_hw, &keys.a.arp.tha);

	out->k_vlan.vlan_id = keys.a.vlan.vlan_id;
	out->k_vlan.vlan_dei = keys.a.vlan.vlan_dei;
	out->k_vlan.vlan_priority = keys.a.vlan.vlan_priority;
	out->k_vlan.vlan_tpid = keys.a.vlan.vlan_tpid;
	out->k_flow_label.flow_label = keys.f.tags.flow_label;

	out->k_gre_keyid.keyid = keys.a.gre_keyid.keyid;
	out->k_mpls_entropy.keyid = keys.a.mpls_entropy.keyid;
	out->k_enc_keyid.keyid = keys.a.enc_keyid.keyid;

	out->k_enc_ipv4_addrs.src = keys.a.enc_ipv4_addrs.src;
	out->k_enc_ipv4_addrs.dst = keys.a.enc_ipv4_addrs.dst;
	FIELD_COPY(k_enc_ipv6_addrs.src, &keys.a.enc_ipv6_addrs.src);
	FIELD_COPY(k_enc_ipv6_addrs.dst, &keys.a.enc_ipv6_addrs.dst);
	out->k_enc_control.thoff = keys.a.enc_control.thoff;
	out->k_enc_control.addr_type = keys.a.enc_control.addr_type;
	out->k_enc_control.flags = keys.a.enc_control.flags;
	out->k_enc_ports.src = keys.a.enc_ports.src;
	out->k_enc_ports.dst = keys.a.enc_ports.dst;

	out->k_mpls.mpls_ttl = keys.a.mpls.mpls_ttl;
	out->k_mpls.mpls_bos = keys.a.mpls.mpls_bos;
	out->k_mpls.mpls_tc = keys.a.mpls.mpls_tc;
	out->k_mpls.mpls_label = keys.a.mpls.mpls_label;

	out->k_tcp.flags = keys.a.tcp.flags;
	out->k_ip.tos = keys.a.ip.tos;
	out->k_ip.ttl = keys.a.ip.ttl;
	out->k_cvlan.vlan_id = keys.a.cvlan.vlan_id;
	out->k_cvlan.vlan_dei = keys.a.cvlan.vlan_dei;
	out->k_cvlan.vlan_priority = keys.a.cvlan.vlan_priority;
	out->k_cvlan.vlan_tpid = keys.a.cvlan.vlan_tpid;
	out->k_enc_ip.tos = keys.a.enc_ip.tos;
	out->k_enc_ip.ttl = keys.a.enc_ip.ttl;

	FIELD_COPY(k_enc_opts.data, &keys.a.enc_opts.data);
	out->k_enc_opts.len = keys.a.enc_opts.len;
	out->k_enc_opts.dst_opt_type = keys.a.enc_opts.dst_opt_type;
	out->k_meta.ingress_ifindex = keys.a.meta.ingress_ifindex;
	out->k_meta.ingress_iftype = keys.a.meta.ingress_iftype;
	out->k_ct.ct_state = keys.a.ct.ct_state;
	out->k_ct.ct_zone = keys.a.ct.ct_zone;
	out->k_ct.ct_mark = keys.a.ct.ct_mark;

	FIELD_COPY(k_ct.ct_labels, &keys.a.ct.ct_labels);

	if (flags & CORE_F_HASH)
		out->k_hash.hash = flow_hash_from_keys(&keys.f);

	return NULL;
}

#undef FIELD_COPY

static void core_flowdis_done(void *pv)
{
	free(pv);
}

CORE_DECL(flowdis)
