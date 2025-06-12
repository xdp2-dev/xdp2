/* SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
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

#ifndef _TEST_PARSER_OUT_H_0881a558_
#define _TEST_PARSER_OUT_H_0881a558_

#include <linux/types.h>

/*
 * Output structure. This is the glue between the core and the output
 * method. An OUT is where the core puts the things it picks out of
 * the packet and where the output method gets the data it prints.
 */

struct test_parser_out_control {
	unsigned short int thoff;
	unsigned char addr_type;
#define ADDR_TYPE_OTHER 1
#define ADDR_TYPE_IPv4  2
#define ADDR_TYPE_IPv6  3
#define ADDR_TYPE_TIPC  4
	unsigned int flags;
};

struct test_parser_out_basic {
	unsigned short int n_proto;
	unsigned char ip_proto;
};

struct test_parser_out_ipv4_addrs {
	unsigned int src;
	unsigned int dst;
};

struct test_parser_out_ipv6_addrs {
	unsigned char src[16];
	unsigned char dst[16];
};

struct test_parser_out_ports {
	unsigned short int src;
	unsigned short int dst;
};

struct test_parser_out_icmp {
	unsigned char type;
	unsigned char code;
	unsigned short int id;
};

struct test_parser_out_eth_addrs {
	unsigned char src[6];
	unsigned char dst[6];
};

struct test_parser_out_tipc {
	unsigned int key;
};

struct test_parser_out_arp {
	unsigned int s_ip;
	unsigned int t_ip;
	unsigned char op;
	unsigned char s_hw[6];
	unsigned char t_hw[6];
};

struct test_parser_out_vlan {
	unsigned short int vlan_id;
	unsigned char vlan_dei;
	unsigned char vlan_priority;
	unsigned short int vlan_tpid;
};

struct test_parser_out_tags {
	unsigned int flow_label;
};

struct test_parser_out_keyid {
	unsigned int keyid;
};

struct test_parser_out_mpls {
	unsigned char mpls_ttl;
	unsigned char mpls_bos;
	unsigned char mpls_tc;
	unsigned int mpls_label;
};

struct test_parser_out_tcp {
	unsigned short int flags;
};

struct test_parser_out_ip {
	unsigned char tos;
	unsigned char ttl;
};

struct test_parser_out_enc_opts {
	unsigned char data[255];
	unsigned char len;
	unsigned short int dst_opt_type;
};

struct test_parser_out_meta {
	int ingress_ifindex;
	unsigned short int ingress_iftype;
};

struct test_parser_out_ct {
	unsigned short int ct_state;
	unsigned short int ct_zone;
	unsigned int ct_mark;
	unsigned int ct_labels[4];
};

struct test_parser_out_tcp_opt {
	unsigned short int mss;
	unsigned char ws;
	unsigned int ts_val;
	unsigned int ts_echo;
	// First element with l==r==0 is end of sack list
	struct {
		unsigned int l;
		unsigned int r;
	} sack[4];
};

struct test_parser_out_hash {
	unsigned long long hash;
};

struct test_parser_out_gre {
	__u32 flags;
	__be16 csum;
	__be32 keyid;
	__be32 seq;
	__be32 routing;
};

struct test_parser_out_gre_pptp {
	__u32 flags;
	__be16 length;
	__be16 callid;
	__be32 seq;
	__be32 ack;
};

struct test_parser_out {
	struct test_parser_out_control k_control;
	struct test_parser_out_basic k_basic;
	struct test_parser_out_ipv4_addrs k_ipv4_addrs;
	struct test_parser_out_ipv6_addrs k_ipv6_addrs;
	struct test_parser_out_ports k_ports;
	struct test_parser_out_ports k_ports_range;
	struct test_parser_out_icmp k_icmp;
	struct test_parser_out_eth_addrs k_eth_addrs;
	struct test_parser_out_tipc k_tipc;
	struct test_parser_out_arp k_arp;
	struct test_parser_out_vlan k_vlan;
	struct test_parser_out_tags k_flow_label;
	struct test_parser_out_keyid k_gre_keyid;
	struct test_parser_out_keyid k_mpls_entropy;
	struct test_parser_out_keyid k_enc_keyid;
	struct test_parser_out_ipv4_addrs k_enc_ipv4_addrs;
	struct test_parser_out_ipv6_addrs k_enc_ipv6_addrs;
	struct test_parser_out_control k_enc_control;
	struct test_parser_out_ports k_enc_ports;
	struct test_parser_out_mpls k_mpls;
	struct test_parser_out_tcp k_tcp;
	struct test_parser_out_ip k_ip;
	struct test_parser_out_vlan k_cvlan;
	struct test_parser_out_ip k_enc_ip;
	struct test_parser_out_enc_opts k_enc_opts;
	struct test_parser_out_meta k_meta;
	struct test_parser_out_ct k_ct;
	struct test_parser_out_tcp_opt k_tcp_opt;
	struct test_parser_out_hash k_hash;
	struct test_parser_out_gre k_gre;
	struct test_parser_out_gre_pptp k_gre_pptp;
};

#endif
