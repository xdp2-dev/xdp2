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

#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/types.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "xdp2/parser.h"
#include "xdp2/parser_metadata.h"
#include "xdp2/pcap.h"
#include "xdp2/utility.h"

/* Define protocol nodes that are used below */
#include "xdp2/proto_nodes_def.h"

/* Metadata structure with addresses and port numbers */
struct metadata {
	struct {
		struct {
			__u32 value;
			__u32 echo;
		} timestamp;
	} tcp_options;

#define HASH_START_FIELD addr_type

	__u8 addr_type __aligned(8);
	__u8 ip_proto;

	__u16 rsvd; /* For packing */

	struct {
		__be16 sport;
		__be16 dport;
	} port_pair;

	union {
		union {
			__be32 v4_addrs[2];
			struct {
				__be32 saddr;
				__be32 daddr;
			} v4;
		};
		union {
			struct in6_addr v6_addrs[2];
			struct {
				struct in6_addr saddr;
				struct in6_addr daddr;
			} v6;
		};
	} addrs;
};

/* Extract IP protocol number and address from IPv4 header */
static void extract_ipv4(const void *viph, void *_meta,
			 const struct xdp2_ctrl_data ctrl)
{
	struct metadata *metadata = _meta;
	const struct iphdr *iph = viph;

	metadata->ip_proto = iph->protocol;
	metadata->addr_type = XDP2_ADDR_TYPE_IPV4;
	metadata->addrs.v4.saddr = iph->saddr;
	metadata->addrs.v4.daddr = iph->daddr;
}

/* Extract IP next header and address from IPv4 header */
static void extract_ipv6(const void *viph, void *_meta,
			 const struct xdp2_ctrl_data ctrl)
{
	struct metadata *metadata = _meta;
	const struct ipv6hdr *iph = viph;

	metadata->ip_proto = iph->nexthdr;
	metadata->addr_type = XDP2_ADDR_TYPE_IPV6;
	metadata->addrs.v6.saddr = iph->saddr;
	metadata->addrs.v6.daddr = iph->daddr;
}

/* Extract port number information from a transport header. Typically,
 * the sixteen bit source and destination port numbers are in the
 * first four bytes of a transport header that has ports (e.g. TCP, UDP,
 * etc.
 */
static void extract_ports(const void *hdr, void *_meta,
			  const struct xdp2_ctrl_data ctrl)
{
	struct metadata *metadata = _meta;
	const __be16 *ports = hdr;

	metadata->port_pair.sport = ports[0];
	metadata->port_pair.dport = ports[1];
}

/* Extract TCP timestamp option */
static void extract_tcp_timestamp(const void *vopt, void *_meta,
				  const struct xdp2_ctrl_data ctrl)
{
	const struct tcp_opt_union *opt = vopt;
	struct metadata *metadata = _meta;

	if (opt->opt.len == sizeof(struct tcp_opt) +
			sizeof(struct tcp_timestamp_option_data)) {
		metadata->tcp_options.timestamp.value =
					ntohl(opt->timestamp.value);
		metadata->tcp_options.timestamp.echo =
					ntohl(opt->timestamp.echo);
	}
}

/* Parse nodes */
XDP2_MAKE_PARSE_NODE(ether_node, xdp2_parse_ether, NULL, NULL, ether_table);
XDP2_MAKE_PARSE_NODE(ipv4_node, xdp2_parse_ipv4, extract_ipv4, NULL,
		      ip_table);
XDP2_MAKE_PARSE_NODE(ipv6_node, xdp2_parse_ipv6, extract_ipv6, NULL,
		      ip_table);
XDP2_MAKE_LEAF_PARSE_NODE(ports_node, xdp2_parse_ports, extract_ports, NULL);
XDP2_MAKE_LEAF_TLVS_PARSE_NODE(tcp_node, xdp2_parse_tcp_tlvs, extract_ports,
				NULL, tcp_tlv_table);

/* TCP TLV nodes */
XDP2_MAKE_TLV_PARSE_NODE(tcp_opt_timestamp_node,
			  xdp2_parse_tcp_option_timestamp,
			  extract_tcp_timestamp, NULL);

/* Protocol tables */

XDP2_MAKE_PROTO_TABLE(ether_table,
	{ __cpu_to_be16(ETH_P_IP), &ipv4_node },
	{ __cpu_to_be16(ETH_P_IPV6), &ipv6_node },
);

XDP2_MAKE_PROTO_TABLE(ip_table,
	{ IPPROTO_TCP, &tcp_node.parse_node },
	{ IPPROTO_UDP, &ports_node },
);

XDP2_MAKE_TLV_TABLE(tcp_tlv_table,
	{ TCPOPT_TIMESTAMP, &tcp_opt_timestamp_node },
);

/* Make the parser */
XDP2_PARSER(parser, "Simple parser without md templates", &ether_node);

/* Include common code that actually runs the parser test */
#include "run_parser.h"

void *usage(char *prog)
{
	fprintf(stderr, "%s [-O] [-P <output pcap dump file location>]\n",
		prog);
	exit(-1);
}

/* The optimized parser is built via the xdp2-compiler tool (see Makefile)
 * so that there are two parsers named parser and parser_opt. Provide a forward
 * reference to parser_opt since xdp2-compiler will include this source file
 * in the source file it generates and parser_opt is defined after this file
 * is included
 */
XDP2_PARSER_DECL(parser_opt);

#define ARGS "OHhP:"

extern int app_main(const void *parser_ctx, int argc, char *argv[]);

void send_pkt_buf_to_xdp2_parser(const void *parser_ctx,
		const void *pktbuf, size_t pktbuflen)
{
	run_parser(parser_ctx, pktbuf, pktbuflen);
}

#define DEF_FILE_LOC "/tmp/dpdk_snoop_app_pdump.pcap"
#define OP_FILE_LEN 256
__u8 op_file[OP_FILE_LEN];

int main(int argc, char *argv[])
{
	bool opt_parser = false;
	int index;
	int c;

	sprintf(op_file, "%s", DEF_FILE_LOC);

	while ((c = getopt(argc, argv, ARGS)) != -1) {
		switch (c) {
		case 'O':
			opt_parser = true;
			printf("opt_parser is set\n");
			break;
		case 'H':
		case 'h':
			usage(argv[0]);
			break;
		case 'P':
			snprintf(op_file, sizeof(op_file), "%s", optarg);
			op_file[sizeof(op_file) - 1] = '\0';
			break;
		case '?':
			printf("optopt %c\n", optopt);
			break;
		default:
			usage(argv[0]);
		}
	}

	// blocking call to dpdk app
	app_main(opt_parser ? parser_opt : parser, argc - optind, argv);

	return 0;
}
