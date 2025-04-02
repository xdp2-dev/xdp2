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
	size_t network_offset;
	size_t transport_offset;
};

/* Extract network layer offset */
static void extract_network(const void *v, void *_meta,
			 const struct xdp2_ctrl_data ctrl)
{
	struct metadata *metadata = _meta;

	metadata->network_offset = ctrl.hdr_offset;
}

/* Extract transport layer offset */
static void extract_transport(const void *v, void *_meta,
			 const struct xdp2_ctrl_data ctrl)
{
	struct metadata *metadata = _meta;

	metadata->transport_offset = ctrl.hdr_offset;
}

/* Parse nodes */
XDP2_MAKE_PARSE_NODE(ether_node, xdp2_parse_ether, NULL, NULL, ether_table);
XDP2_MAKE_PARSE_NODE(ipv4_node, xdp2_parse_ipv4, extract_network, NULL,
		      ip_table);
XDP2_MAKE_PARSE_NODE(ipv6_node, xdp2_parse_ipv6, extract_network, NULL,
		      ip_table);
XDP2_MAKE_LEAF_PARSE_NODE(ports_node, xdp2_parse_ports, extract_transport,
			   NULL);

/* Protocol tables */

XDP2_MAKE_PROTO_TABLE(ether_table,
	{ __cpu_to_be16(ETH_P_IP), &ipv4_node },
	{ __cpu_to_be16(ETH_P_IPV6), &ipv6_node },
);

XDP2_MAKE_PROTO_TABLE(ip_table,
	{ IPPROTO_TCP, &ports_node },
	{ IPPROTO_UDP, &ports_node },
);

/* Make the parser */
XDP2_PARSER(parser, "Simple parser without md templates", &ether_node);

void *usage(char *prog)
{
	fprintf(stderr, "%s [-O] <pcap_file>\n", prog);
	exit(-1);
}

void run_parser(const struct xdp2_parser *parser, struct xdp2_pcap_file *pf)
{
	struct {
		struct xdp2_metadata xdp2_metadata; /* Must be first */
		struct metadata metadata;
	} pmetadata;
	struct metadata *metadata = &pmetadata.metadata;
	__u8 packet[1500];
	ssize_t len;
	size_t plen;

	while ((len = xdp2_pcap_readpkt(pf, packet, sizeof(packet),
					 &plen)) >= 0) {
		memset(&pmetadata, 0, sizeof(pmetadata));

		xdp2_parse(parser, packet, len,
			    &pmetadata.xdp2_metadata, 0, 0);

		printf("Network offset: %lu\n", metadata->network_offset);
		printf("Transport offset: %lu\n", metadata->transport_offset);
	}
}

/* The optimized parser is built via the xdp2-compilet tool (see Makefile)
 * so that there are two parsers named parser and parser_opt. Provide a forward
 * reference to parser_opt since xdp2-compiler will include this source file
 * in the source file it generates and parser_opt is defined after this file
 * is included
 */
XDP2_PARSER_DECL(parser_opt);

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

	run_parser(opt_parser ? parser_opt : parser, pf);
}
