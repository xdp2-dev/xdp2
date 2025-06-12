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

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "cli/cli.h"

#include "xdp2/cli.h"
#include "xdp2/utility.h"

#define CLITEST_PORT 8000
#define MODE_CONFIG_INT 10

static unsigned int regular_count;
static unsigned int debug_regular;

static int cmd_config_int_exit(struct cli_def *cli, const char *command,
			       char *argv[], int argc)
{
	cli_set_configmode(cli, MODE_CONFIG, NULL);
	return CLI_OK;
}

static int regular_callback(struct cli_def *cli)
{
	regular_count++;
	if (debug_regular) {
		cli_print(cli, "Regular callback - %u times so far",
			  regular_count);
		cli_reprompt(cli);
	}
	return CLI_OK;
}

static void xdp2_cli_command(void *cli, struct xdp2_cli_thread_info *info,
			      const char *input)
{
	const struct xdp2_cli_show_cmd_def *show_def_base =
				xdp2_section_base_xdp2_cli_show_cmd();
	unsigned int show_count =
				xdp2_section_array_size_xdp2_cli_show_cmd();
	const struct xdp2_cli_set_cmd_def *set_def_base =
				xdp2_section_base_xdp2_cli_set_cmd();
	unsigned int set_count =
				xdp2_section_array_size_xdp2_cli_set_cmd();
	const struct xdp2_cli_show_cmd_def *show_def;
	const struct xdp2_cli_set_cmd_def *set_def;
	char *token, *line;
	int i;

	line = malloc(strlen(input) + 1);
	if (!line) {
		XDP2_CLI_PRINT(cli, "Malloc failed\n");
		return;
	}

	strcpy(line, input);

	token = strtok(line, " \t\n");
	if (!token) {
		;
	} else if (!strcmp(token, "show")) {
		token = strtok(NULL, " \t\n");
		if (!token) {
			fprintf(stderr, "Need show help\n");
			XDP2_CLI_PRINT(cli, "Need show help\n");
			return;
		}

		for (i = 0; i < show_count; i++) {
			show_def = &show_def_base[i];

			if (!show_def->name || strcmp(token, show_def->name))
				continue;

			token = strtok(NULL, " \t\n");
			if (!token) {
				if (show_def->class & info->classes)
					show_def->show_func(cli, info, NULL);
				else
					XDP2_CLI_PRINT(cli, "Error: Command "
							"is not supported "
							"for CLI class\n");
			} else if (show_def->argok) {
				if (show_def->class & info->classes)
					show_def->show_func(cli, info,
							    token);
				else
					XDP2_CLI_PRINT(cli, "Error: Command "
							"is not supported "
							"for CLI class\n");
			}
			break;
		}
		if (i >= show_count)
			XDP2_CLI_PRINT(cli, "Show command '%s' is "
					     "unrecognized\n", input);
	} else if (!strcmp(token, "set")) {
		token = strtok(NULL, " \t\n");
		if (!token) {
			fprintf(stderr, "Need set help\n");
			XDP2_CLI_PRINT(cli, "Need set help\n");
			return;
		}

		for (i = 0; i < set_count; i++) {
			set_def = &set_def_base[i];

			if (!set_def->name || strcmp(token, set_def->name))
				continue;

			if (set_def->class & info->classes)
				set_def->set_func(cli, info,
					token + strlen(token) + 1);
			else
				XDP2_CLI_PRINT(cli, "Error: Command "
						"is not supported "
						"for CLI class\n");
			break;
		}
		if (i >= set_count)
			XDP2_CLI_PRINT(cli, "Set command '%s' is "
					     "unrecognized\n", input);
	}

	free(line);
}

static int xdp2_prep_cmd(struct cli_def *cli,
			  const char *command, char *argv[], int argc)
{
	struct xdp2_cli_thread_info *info =
				(struct xdp2_cli_thread_info *)cli->info;
	size_t size = 0;
	char *line;
	int i;

	for (i = 0; i < argc; i++)
		size += strlen(argv[i]);

	line = malloc(strlen(command) + size + argc + 1);
	if (!line)
		return CLI_OK;

	strcpy(line, command);

	size = strlen(line);

	for (i = 0; i < argc; i++) {
		sprintf(&line[size], " %s", argv[i]);
		size += 1 + strlen(argv[i]);
	}

	if (info->run_thread_cb)
		info->run_thread_cb(info);

	xdp2_cli_command(cli, cli->info, line);

	free(line);

	return CLI_OK;
}

static int idle_timeout(struct cli_def *cli)
{
	cli_print(cli, "Custom idle timeout");
	return CLI_QUIT;
}

typedef void (*cli_cb_func_t)(void *cli);

struct cli_callback {
	unsigned int num;
	unsigned int num_alloced;
	cli_cb_func_t funcs[];
};

#define MIN_CB_ALLOC 4

#define MALLOC_SIZE(NUM) (sizeof(struct cli_callback) +			\
			 (NUM * sizeof(cli_cb_func_t)))

struct cli_callback *start_cbs, *done_cbs;

static void register_cb(struct cli_callback **mcbs, cli_cb_func_t func)
{
	struct cli_callback *cbs = *mcbs;

	if (!cbs || cbs->num >= cbs->num_alloced) {
		unsigned int num_alloc = !cbs ? MIN_CB_ALLOC :
						2 * cbs->num_alloced;
		unsigned int num = !cbs ? 0 : cbs->num;

		cbs = realloc(cbs, MALLOC_SIZE(num_alloc));
		XDP2_ASSERT(cbs, "Malloc failed CLI register callback\n");

		cbs->num_alloced = num_alloc;
		cbs->num = num;
		*mcbs = cbs;
	}

	cbs->funcs[cbs->num++] = func;
}

static void run_cbs(struct cli_callback *cbs, void *cli)
{
	int i;

	if (!cbs)
		return;

	for (i = 0; i < cbs->num; i++)
		cbs->funcs[i](cli);
}

void xdp2_cli_register_start(void (*func)(void *))
{
	register_cb(&start_cbs, func);
}

void xdp2_cli_register_done(void (*func)(void *))
{
	register_cb(&done_cbs, func);
}

XDP2_CLI_ADD_SHOW_CONFIG(NULL, NULL, 0);
XDP2_CLI_ADD_SET_CONFIG(NULL, NULL, 0);

static void *run_cli_thread(void *arg)
{
	const struct xdp2_cli_show_cmd_def *show_def_base =
				xdp2_section_base_xdp2_cli_show_cmd();
	unsigned int show_count =
				xdp2_section_array_size_xdp2_cli_show_cmd();
	const struct xdp2_cli_set_cmd_def *set_def_base =
				xdp2_section_base_xdp2_cli_set_cmd();
	unsigned int set_count =
				xdp2_section_array_size_xdp2_cli_set_cmd();
	struct xdp2_cli_thread_info *info =
					(struct xdp2_cli_thread_info *)arg;
	char hostname[20+strlen("xdp2-XXXXXXXXXXXXXXXXX") + 1];
	struct cli_command *c;
	struct cli_def *cli;
	size_t n = 0;
	int i;

	if (info->label) {
		n += snprintf(hostname + n, sizeof(hostname) - n, "%s", info->label);
		if (info->major_num != 0xff)
			n += snprintf(hostname + n, sizeof(hostname) - n,
				      "-%u", info->major_num);
		if (info->minor_num != 0xff)
			n += snprintf(hostname + n, sizeof(hostname) - n,
				      "-%u", info->minor_num);
	} else {
		n += snprintf(hostname + n, sizeof(hostname) - n, "xdp2-%u", info->port_num);
	}

	cli = cli_init();
	cli_set_hostname(cli, hostname);
	if (info->prompt_color) {
		cli_set_prompt_start_color(cli, info->prompt_color);
		cli_set_prompt_stop_color(cli, XDP2_NULL_TERM_COLOR);
	}

	cli_telnet_protocol(cli, 1);
	cli_regular(cli, regular_callback);

	// change regular update to 5 seconds rather than default of 1 second
	cli_regular_interval(cli, 5);
	cli->info = arg;

	// set 300 second idle timeout
	cli_set_idle_timeout_callback(cli, 300, idle_timeout);
	c = cli_register_command(cli, NULL, "show", NULL,
				 PRIVILEGE_UNPRIVILEGED, MODE_EXEC, NULL);

	/* Register the CLI "show" commands */
	for (i = 0; i < show_count; i++) {
		if (!show_def_base[i].name)
			continue;
		cli_register_command(cli, c, show_def_base[i].name,
				     xdp2_prep_cmd,
			     PRIVILEGE_UNPRIVILEGED, MODE_EXEC, NULL);
	}

	c = cli_register_command(cli, NULL, "set", NULL,
				 PRIVILEGE_UNPRIVILEGED, MODE_EXEC, NULL);
	/* Register the CLI "set" commands */
	for (i = 0; i < set_count; i++) {
		if (!set_def_base[i].name)
			continue;
		cli_register_command(cli, c, set_def_base[i].name,
				     xdp2_prep_cmd, PRIVILEGE_UNPRIVILEGED,
				     MODE_EXEC, NULL);
	}

	cli_register_command(cli, NULL, "exit", cmd_config_int_exit,
			     PRIVILEGE_PRIVILEGED,
			     MODE_CONFIG_INT,
			     "Exit from interface configuration");

	if (info->start_cb)
		(info->start_cb)(info);

	run_cbs(start_cbs, cli);
	cli_loop(cli, info->s);
	run_cbs(done_cbs, cli);
	cli_done(cli);

	if (info->stop_cb)
		(info->stop_cb)(info);

	free(info);
	return NULL;
}

static int xdp2_cli_start_listener(struct xdp2_cli_thread_info *info)
{
	struct xdp2_cli_thread_info *tinfo;
	static pthread_t os_thread;
	struct sockaddr_in addr;
	int on = 1, s, x;

	signal(SIGCHLD, SIG_IGN);

	if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		return 1;
	}

	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)))
		perror("setsockopt");

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(info->port_num);
	if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("bind");
		return 1;
	}

	if (listen(s, 50) < 0) {
		perror("listen");
		return 1;
	}

	while (1) {
		x = accept(s, NULL, 0);
		if (x < 0) {
			perror("accept");
			break;
		}
		tinfo = malloc(sizeof(*tinfo) + info->extra_size);
		if (!tinfo) {
			XDP2_WARN("Malloc failed in libcli_start\n");
			continue;
		}
		memcpy(tinfo, info, sizeof(*tinfo) + info->extra_size);
		tinfo->s = x;

		pthread_create(&os_thread, NULL, run_cli_thread, tinfo);
	}
	return 0;
}

/* CLI thread function */
static void *xdp2_debug_cli(void *arg)
{
	struct xdp2_cli_thread_info *info = arg;
	char *line;

	if (info->port_num >= 0) {
		xdp2_cli_start_listener(info);
		return NULL;
	}

	if (info->run_thread_cb)
		info->run_thread_cb(info);

	fprintf(stderr, "sixdp2> ");
	while ((line = xdp2_getline())) {
		xdp2_cli_command(NULL, info, line);

		free(line);
		fprintf(stderr, "sixdp2> ");
	}

	return NULL;
}

/* Main function to start CLI */
void xdp2_cli_start(struct xdp2_cli_thread_info *info)
{
	struct xdp2_cli_thread_info *tinfo;
	static pthread_t os_thread;	/* OS thread for CLI */


	tinfo = malloc(sizeof(*tinfo) + info->extra_size);
	if (!tinfo) {
		XDP2_WARN("Malloc failed in xdp2_cli_start\n");
		return;
	}

	memcpy(tinfo, info, sizeof(*tinfo) + info->extra_size);

	pthread_create(&os_thread, NULL, xdp2_debug_cli, tinfo);
}
