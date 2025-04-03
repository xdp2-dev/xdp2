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

/* Test for XDP2 tables
 *
 * Run: ./test_tables
 */

#include <getopt.h>
#include <linux/if_ether.h>
#include <linux/ipv6.h>
#include <linux/types.h>
#include <netinet/ip.h>
#include <stdio.h>

#include "xdp2/dtables.h"
#include "xdp2/cli.h"
#include "xdp2/stables.h"

#include "test_tables.h"

static int cli_port_num, sleep_time;
static const char *prompt_color;

int verbose;

#define ARGS "C:v:s:P:"

static void usage(char *cmd)
{
	fprintf(stderr, "Usage: %s [ -C <cli-port> ] [ -v <verbose> ] "
			"[ -s <sleep-time> ] [ -P <color> ]\n", cmd);
}

int main(int argc, char *argv[])
{
	static struct xdp2_cli_thread_info cli_thread_info;
	int c;

	while ((c = getopt(argc, argv, ARGS)) != -1) {
		switch (c) {
		case 'C':
			cli_port_num = strtoul(optarg, NULL, 10);
			break;
		case 'v':
			verbose = strtoul(optarg, NULL, 10);
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

	xdp2_dtables_init();

	run_stables_plain();
	run_stables_tern();
	run_stables_lpm();

	run_dtables_plain();
	run_dtables_tern();
	run_dtables_lpm();

	sleep(sleep_time);
}
