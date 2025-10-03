// SPDX-License-Identifier: BSD-2-Clause-FreeBSD
/*
 * Copyright (c) Tom Herbert, 2025
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

#include "xdp2/cli.h"
#include "xdp2/parser.h"
#include "xdp2/parser_metadata.h"
#include "xdp2/pcap.h"
#include "xdp2/utility.h"

#include "siphash/siphash.h"

#include "router.h"
#include "test.h"

/* Test for a simple router
 *
 * Run: ./test_router [ -c <test-count> ] [ -v <verbose> ]
 *                    [ -I <report-interval> ][ -C <cli_port_num> ]
 *                    [-R] [-d] [  -s <sleep-time> ]
 *		      [ -P <prompt-color> ] [-U]
 */

int verbose;
bool use_colors;

static void run_router(const struct xdp2_parser *parser, char **pcap_files,
		       int num_pfs, unsigned long count,
		       unsigned int interval, bool debug)
{
	__u32 flags = (debug ? XDP2_F_DEBUG : 0);
	struct router_metadata metadata;
	struct xdp2_ctrl_data ctrl;
	struct one_packet *packets;
	int total_packets = 0, pn;
	struct xdp2_pcap_file *pf;
	bool randomize = false;
	unsigned int number;
	__u8 dpacket[1500];
	unsigned long i;
	ssize_t len;
	size_t plen;

	for (i = 0; i < num_pfs; i++) {
		if (!(pf = xdp2_pcap_init(pcap_files[i])))
			XDP2_ERR(1, "Open pcap file %s failed\n",
				 pcap_files[i]);

		while (xdp2_pcap_readpkt(pf, dpacket, sizeof(dpacket),
					&plen) >= 0)
			total_packets++;

		xdp2_pcap_close(pf);
	}

	packets = calloc(total_packets, sizeof(*packets));
	if (!packets)
		XDP2_ERR(1, "Allocate packets structure failed\n");

	for (i = 0, pn = 0; i < num_pfs; i++) {
		if (!(pf = xdp2_pcap_init(pcap_files[i])))
			XDP2_ERR(1, "Open pcap file %s failed\n",
				 pcap_files[i]);

		number = 0;
		while ((len = xdp2_pcap_readpkt(pf, dpacket,
						sizeof(dpacket),
						&plen)) >= 0) {
			if (pn >= total_packets)
				break;

			packets[pn].packet = malloc(len);
			if (!packets[pn].packet)
				XDP2_ERR(1, "Allocate one packet failed\n");

			memcpy(packets[pn].packet, dpacket, len);
			packets[pn].hdr_size = len;
			packets[pn].cap_len = plen;
			packets[pn].pcap_file = pcap_files[i];
			packets[pn].file_number = number++;
			pn++;
		}

		xdp2_pcap_close(pf);
	}

	total_packets = pn;

	if (count)
		randomize = true;
	else
		count = total_packets;

	for (i = 0; i < count; i++) {
		if (interval && !(i % interval))
			printf("I: %lu\n", i);

		pn = randomize ? random() % total_packets : i;

		memset(&metadata, 0, sizeof(metadata));

		XDP2_CTRL_RESET_VAR_DATA(&ctrl);
		XDP2_CTRL_SET_BASIC_PKT_DATA(&ctrl, packets[pn].packet,
					     packets[pn].cap_len, i);

		xdp2_parse(parser, packets[pn].packet, packets[pn].cap_len,
			   &metadata, &ctrl, flags);
	}
}

XDP2_PARSER_EXTERN(simple_router);

static void set_verbose_from_cli(void *cli,
				 struct xdp2_cli_thread_info *info,
				 char *args)
{
	verbose = strtol(args, NULL, 10);
}

XDP2_CLI_ADD_SET_CONFIG("verbose", set_verbose_from_cli, 0xffff);

static void set_use_colors_from_cli(void *cli,
				    struct xdp2_cli_thread_info *info,
				    char *args)
{
	if (!strcmp(args, "yes"))
		use_colors = true;
	else if (!strcmp(args, "no")) {
		use_colors = false;
//		PRINTFC(0, XDP2_NULL_TERM_COLOR);
	}
}

XDP2_CLI_ADD_SET_CONFIG("colors", set_use_colors_from_cli, 0xffff);

#define ARGS "c:v:I:C:Rds:P:U"

static void *usage(char *prog)
{

	fprintf(stderr, "Usage: %s [ -c <test-count> ] [ -v <verbose> ]\n",
		prog);
	fprintf(stderr, "\t[ -I <report-interval> ][ -C <cli_port_num> ]\n");
	fprintf(stderr, "\t[-R] [-d] [ -s <sleep-time> ]\n");
	fprintf(stderr, "\t[ -P <prompt-color> ] [-U] <pcap-file> ...\n");

	exit(-1);
}

int main(int argc, char *argv[])
{
	static struct xdp2_cli_thread_info cli_thread_info;
	unsigned int cli_port_num = 0, interval = 0;
	const char *prompt_color = "";
	bool random_seed = false;
	unsigned long count = 0;
	int c, sleep_time = 0;
	bool debug = false;

	while ((c = getopt(argc, argv, ARGS)) != -1) {
		switch (c) {
		case 'c':
			count = strtoll(optarg, NULL, 10);
			break;
		case 'v':
			verbose = strtol(optarg, NULL, 10);
			break;
		case 'I':
			interval = strtol(optarg, NULL, 10);
			break;
		case 'C':
			cli_port_num = strtoul(optarg, NULL, 10);
			break;
		case 'R':
			random_seed = true;
			break;
		case 'd':
			debug = true;
			break;
		case 's':
			sleep_time = strtol(optarg, NULL, 10);
			break;
		case 'P':
			prompt_color = xdp2_print_color_select_text(optarg);
			break;
		case 'U':
			use_colors = true;
			break;
		default:
			usage(argv[0]);
		}
	}

	if (optind >= argc)
		usage(argv[0]);
	
	if (random_seed)
		srand(time(NULL));

	if (cli_port_num) {
		XDP2_CLI_SET_THREAD_INFO(cli_thread_info, cli_port_num,
			(.label = "xdp2_parse dump",
			 .prompt_color = prompt_color,
			)
		);

		xdp2_cli_start(&cli_thread_info);
	}

	xdp2_dtable_init();

	init_routes();

	run_router(simple_router, &argv[optind], argc - optind,
		   count, interval, debug);

	sleep(sleep_time);
}
