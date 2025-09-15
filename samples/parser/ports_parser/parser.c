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

/* Example parser for TCP/IP with parsing timestamp option. This variant
 * does not use metadata templates. Both an optimized and a non-optimized
 * parser are built
 *
 * Run by:
 *	./parser_notmpl [-O] <pcap-file>
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
#include "xdp2/proto_defs_define.h"

struct my_metadata {
	__be32 src_addr;
	__be32 dst_addr;

	__be16 src_port;
	__be16 dst_port;
};

static void ipv4_metadata(const void *v, void *_meta,
			  const struct xdp2_ctrl_data ctrl)
{
	struct my_metadata *metadata = _meta;
	const struct iphdr *iph = v;

	metadata->src_addr = iph->saddr;
	metadata->dst_addr = iph->daddr;
}

static void ports_metadata(const void *v, void *_meta,
			   const struct xdp2_ctrl_data ctrl)
{
	struct my_metadata *metadata = _meta;
	const __be16 *ports = v;

	metadata->src_port = ports[0];
	metadata->dst_port = ports[1];
}

XDP2_MAKE_PARSE_NODE(ether_node, xdp2_parse_ether, ether_table, ());

XDP2_MAKE_PARSE_NODE(ipv4_node, xdp2_parse_ipv4, ip_table,
		     (.ops.extract_metadata = ipv4_metadata));

XDP2_MAKE_LEAF_PARSE_NODE(tcp_node, xdp2_parse_tcp_notlvs,
			  (.ops.extract_metadata = ports_metadata));
XDP2_MAKE_LEAF_PARSE_NODE(udp_node, xdp2_parse_udp,
			  (.ops.extract_metadata = ports_metadata));

XDP2_MAKE_PROTO_TABLE(ether_table,
		      ( __cpu_to_be16(ETH_P_IP), ipv4_node )
);
XDP2_MAKE_PROTO_TABLE(ip_table,
		      ( IPPROTO_TCP, tcp_node ),
		      ( IPPROTO_UDP, udp_node )
);

XDP2_PARSER(my_parser, "Example parser", ether_node,
	    (.max_frames = 1,
	     .metameta_size = 0,
	     .frame_size = sizeof(struct my_metadata)
	    )
);

void *usage(char *prog)
{
	fprintf(stderr, "%s [-O] <pcap_file>\n", prog);
	exit(-1);
}

void run_parser(const struct xdp2_parser *parser, struct xdp2_pcap_file *pf)
{
	struct in_addr ipsaddr, ipdaddr;
	struct xdp2_packet_data pdata;
	struct my_metadata metadata;
	__u8 packet[1500];
	ssize_t len;
	size_t plen;
	int i = 0;

	while ((len = xdp2_pcap_readpkt(pf, packet, sizeof(packet),
					 &plen)) >= 0) {
		memset(&metadata, 0, sizeof(metadata));

		XDP2_SET_BASIC_PDATA_LEN_SEQNO(pdata, packet, plen,
					       packet, len, i);

		xdp2_parse(my_parser, &pdata, &metadata, 0);

		ipsaddr.s_addr = metadata.src_addr;
		ipdaddr.s_addr = metadata.dst_addr;

		printf("Packet %u: %s:%u -> %s:%u\n", i,
		       inet_ntoa(ipsaddr), ntohs(metadata.src_port),
		       inet_ntoa(ipdaddr), ntohs(metadata.dst_port));

		i++;
	}
}

/* The optimized parser is built via the xdp2-compiler tool (see Makefile)
 * so that there are two parsers named my_parser and my_parser_opt. Provide
 * a forward reference to my_parser_opt since xdp2-compiler will include this
 * source file in the source file it generates and parser_opt is defined after
 * this file is included
 */
XDP2_PARSER_EXTERN(my_parser_opt);

#define ARGS "O"

int main(int argc, char *argv[])
{
	struct xdp2_pcap_file *pf;
	bool opt_parser = false;
	int c;

	while ((c = getopt(argc, argv, ARGS)) != -1) {
		switch (c) {
		case 'O':
			opt_parser = true;
			break;
		default:
			usage(argv[0]);
		}
	}
	if (optind != argc - 1)
		usage(argv[0]);

	pf = xdp2_pcap_init(argv[optind]);
	if (!pf) {
		fprintf(stderr, "XDP2 pcap init failed\n");

		exit(-1);
	}

	run_parser(opt_parser ? my_parser_opt : my_parser, pf);
}
