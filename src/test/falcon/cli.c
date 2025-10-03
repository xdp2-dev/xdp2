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

/* CLI functions for Falcon */

#include <linux/types.h>
#include <stdbool.h>

#include "falcon/config.h"

#include "xdp2/cli.h"
#include "xdp2/utility.h"

#include "test.h"

static void set_verbose_from_cli(void *cli,
				 struct xdp2_cli_thread_info *info,
				 char *args)
{
	verbose = strtol(args, NULL, 10);
}

XDP2_CLI_ADD_SET_CONFIG("verbose", set_verbose_from_cli, 0xffff);

static void show_falcon_config_plain(void *cli,
		struct xdp2_cli_thread_info *info, const char *arg)
{
	xdp2_config_print_config(&falcon_config_table, cli, "", &falcon_config);
}

XDP2_CLI_ADD_SHOW_CONFIG("config", show_falcon_config_plain, 0xffff);

static void set_use_colors_from_cli(void *cli,
				    struct xdp2_cli_thread_info *info,
				    char *args)
{
	if (!strcmp(args, "yes"))
		use_colors = true;
	else if (!strcmp(args, "no"))
		use_colors = false;
}

XDP2_CLI_ADD_SET_CONFIG("colors", set_use_colors_from_cli, 0xffff);
