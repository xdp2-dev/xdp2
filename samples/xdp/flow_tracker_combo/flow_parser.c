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

/* Regular program to parser TCP and UDP packet over IPv4 and IPv6 and
 * then print tuple insformation.
 *
 * Run as
 *	./parser [-O] <pcap-file>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xdp2/pcap.h"

#define ARGS "O"

#include "xdp2/parser.h"
#include "xdp2/parser_metadata.h"

/* Common function to run the parser. */

void run_parser(const struct xdp2_parser *parser, struct xdp2_pcap_file *pf)
{
	struct {
		struct xdp2_metadata_all metadata;
	} pmetadata;
	struct xdp2_metadata_all *metadata = &pmetadata.metadata;
	struct xdp2_packet_data pdata;
	unsigned int seqno = 0;
	__u8 packet[1500];
	ssize_t len;
	size_t plen;


	while ((len = xdp2_pcap_readpkt(pf, packet, sizeof(packet),
					 &plen)) >= 0) {
		memset(&pmetadata, 0, sizeof(pmetadata));

		XDP2_SET_BASIC_PDATA_LEN_SEQNO(pdata, packet, len, packet,
					       len, seqno++);
		xdp2_parse(parser, &pdata, metadata, 0);

		/* Print IP addresses and ports in the metadata */
		switch (metadata->addr_type) {
		case XDP2_ADDR_TYPE_IPV4: {
			char sbuf[INET_ADDRSTRLEN];
			char dbuf[INET_ADDRSTRLEN];

			inet_ntop(AF_INET, &metadata->addrs.v4.saddr,
				  sbuf, sizeof(sbuf));
			inet_ntop(AF_INET, &metadata->addrs.v4.daddr,
				  dbuf, sizeof(dbuf));

			printf("IPv4: %s:%u->%s:%u\n", sbuf,
			       ntohs(metadata->port_pair.sport), dbuf,
			       ntohs(metadata->port_pair.dport));

			break;
		}
		case XDP2_ADDR_TYPE_IPV6: {
			char sbuf[INET6_ADDRSTRLEN];
			char dbuf[INET6_ADDRSTRLEN];

			inet_ntop(AF_INET6, &metadata->addrs.v6.saddr,
				  sbuf, sizeof(sbuf));
			inet_ntop(AF_INET6, &metadata->addrs.v6.daddr,
				  dbuf, sizeof(dbuf));

			printf("IPv6: %s:%u->%s:%u\n", sbuf,
			       ntohs(metadata->port_pair.sport), dbuf,
			       ntohs(metadata->port_pair.dport));

			break;
		}
		default:
			fprintf(stderr, "Unknown addr type %u\n",
				metadata->addr_type);
		}

		/* If data is present for TCP timestamp option then print */
		if (metadata->tcp_options.timestamp.value ||
		    metadata->tcp_options.timestamp.echo) {
			printf("\tTCP timestamps value: %u, echo %u\n",
			       metadata->tcp_options.timestamp.value,
			       metadata->tcp_options.timestamp.echo);
		}
	}
}

void *usage(char *prog)
{
	fprintf(stderr, "%s [-O] <pcap_file>\n", prog);
	exit(-1);
}

XDP2_PARSER_EXTERN(xdp2_parser_simple_tuple);
XDP2_PARSER_EXTERN(xdp2_parser_simple_tuple_opt);

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

	run_parser(opt_parser ? xdp2_parser_simple_tuple_opt :
		   xdp2_parser_simple_tuple, pf);
}
