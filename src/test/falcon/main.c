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

#include <linux/types.h>
#include <stdbool.h>
#include <stdlib.h>

#include "falcon/config.h"

#include "xdp2/cli.h"
#include "xdp2/utility.h"

#include "test.h"

/* Test for Falcon
 *
 * Run: ./test_falcon [ -v <verbose> ] [ -C <cli_port_num> ] [-d]
 *		      [  -s <sleep-time> ] [ -P <prompt-color> ] [-U]
 *		      [ -X <config-string> ]
 */

int verbose;
bool use_colors;
bool debug_colors_instances;
__u32 debug_mask;
bool parser_debug;

struct falcon_config falcon_config;

enum tests {
	TEST_TRANSACTIONS,
	TEST_PACKETS,
};

#define ARGS "v:C:d:s:P:UX:T:D"

static void *usage(char *prog)
{
	fprintf(stderr, "Usage: %s [ -v <verbose> ] [ -C <cli_port_num> ]\n",
		prog);
	fprintf(stderr, "\t[-d] [ -s <sleep-time> ]\n");
	fprintf(stderr, "\t[ -P <prompt-color> ] [-U] [ -T <test> ]\n");

	exit(-1);
}

int main(int argc, char *argv[])
{
	static struct xdp2_cli_thread_info cli_thread_info;
	enum tests test = TEST_PACKETS;
	const char *prompt_color = "";
	unsigned int cli_port_num = 0;
	char *config_string = NULL;
	int c, sleep_time = 0;

	while ((c = getopt(argc, argv, ARGS)) != -1) {
		switch (c) {
		case 'v':
			verbose = strtol(optarg, NULL, 10);
			break;
		case 'C':
			cli_port_num = strtoul(optarg, NULL, 10);
			break;
		case 'd':
			debug_mask = strtoul(optarg, NULL, 0);
			break;
		case 'D':
			parser_debug = true;
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
		case 'X':
			config_string = optarg;
			break;
		case 'T':
			if (!strcmp(optarg, "transactions")) {
				test = TEST_TRANSACTIONS;
			} else if (!strcmp(optarg, "packets")) {
				test = TEST_PACKETS;
			} else {
				fprintf(stderr, "Unknown test %s\n",
					optarg);
				usage(argv[0]);
			}
			break;
		default:
			usage(argv[0]);
		}
	}

	xdp2_config_parse_options_with_defaults(&falcon_config_table,
						&falcon_config, config_string);

	if (cli_port_num) {
		XDP2_CLI_SET_THREAD_INFO(cli_thread_info, cli_port_num,
			(.label = "xdp2-falcon",
			 .prompt_color = prompt_color,
			)
		);

		xdp2_cli_start(&cli_thread_info);
	}

	switch (test) {
	case TEST_TRANSACTIONS:
		fprintf(stderr, "Transactions no supported yet");
		exit(-1);
		break;
	case TEST_PACKETS:
		test_basic_packets();
		break;
	default:
		break;
	}

	sleep(sleep_time);
}
