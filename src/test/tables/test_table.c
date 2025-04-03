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

#include <getopt.h>
#include <linux/if_ether.h>
#include <linux/ipv6.h>
#include <linux/types.h>
#include <netinet/ip.h>
#include <stdio.h>

#include "xdp2/dtable.h"
#include "xdp2/cli.h"
#include "xdp2/stable.h"

#include "test_table.h"

/* Test for tables
 *
 * Run: ./test_tables [ -C <cli-port> ] [ -v <verbose> ]
 *		      [ -s <sleep-time> ] [ -P <color> ]
 */

static int cli_port_num, sleep_time;
static const char *prompt_color;

int verbose;

#define ARGS "v:C:s:P:"

static void usage(char *prog)
{
	fprintf(stderr, "Usage: %s [ -v <verbose> ] [ -C <cli-port> ]\n"
			"\t[ -s <sleep-time> ] [ -P <color> ]\n", prog);
}

int main(int argc, char *argv[])
{
	static struct xdp2_cli_thread_info cli_thread_info;
	int c;

	while ((c = getopt(argc, argv, ARGS)) != -1) {
		switch (c) {
		case 'v':
			verbose = strtoul(optarg, NULL, 10);
			break;
		case 'C':
			cli_port_num = strtoul(optarg, NULL, 10);
			break;
		case 's':
			sleep_time = strtoul(optarg, NULL, 10);
			break;
		case 'P':
			prompt_color = xdp2_print_color_select_text(optarg);
			break;
		default:
			usage(argv[0]);
			exit(1);
		}
	}
	if (cli_port_num) {
		XDP2_CLI_SET_THREAD_INFO(cli_thread_info, cli_port_num,
			( .label = "xdp2_tables",
			  .prompt_color = prompt_color,
			));

		xdp2_cli_start(&cli_thread_info);
	}

	xdp2_dtable_init();

	run_stable_plain();
	run_stable_tern();
	run_stable_lpm();

	run_sxtables_plain();
	run_sxtables_tern();
	run_sxtables_lpm();

	run_dtable_plain();
	run_dtable_tern();
	run_dtable_lpm();

	run_dxtable_plain();
	run_dxtable_tern();
	run_dxtable_lpm();

	sleep(sleep_time);
}
