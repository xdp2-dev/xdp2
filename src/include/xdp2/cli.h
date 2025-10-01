/* SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
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

#ifndef __XDP2_CLI_H__
#define __XDP2_CLI_H__

#include <linux/types.h>
#include <stdio.h>
#include <stdbool.h>

#include "xdp2/compiler_helpers.h"

struct xdp2_cli_thread_info {
	const char *label;
	int port_num;
	unsigned int classes;
	unsigned char major_num;
	unsigned char minor_num;
	size_t extra_size;
	void (*start_cb)(struct xdp2_cli_thread_info *info);
	void (*stop_cb)(struct xdp2_cli_thread_info *info);
	int (*run_thread_cb)(struct xdp2_cli_thread_info *info);
	int s;
	const char *prompt_color;
};

#define XDP2_CLI_SET_THREAD_INFO(INFO, PORT_NUM, EXTRA) do {		\
	XDP2_PUSH_NO_WEXTRA();						\
	struct xdp2_cli_thread_info _linfo = {				\
		.port_num = PORT_NUM,					\
		.classes = -1U,						\
		.major_num = 0xff,					\
		.minor_num = 0xff,					\
		XDP2_DEPAIR(EXTRA)					\
	};								\
	XDP2_POP_NO_WEXTRA();						\
	INFO = _linfo;							\
} while (0)

/* Section definition for XDP2 CLI show functions */
struct xdp2_cli_show_cmd_def {
	char *name;
	void (*show_func)(void *cli, struct xdp2_cli_thread_info *info,
			  const char *arg);
	unsigned int class;
	bool argok;
} XDP2_ALIGN_SECTION;

#define __XDP2_CLI_ADD_SHOW_CONFIG(NAME, FUNC, CLASS, ARGOK)		\
static const struct xdp2_cli_show_cmd_def XDP2_SECTION_ATTR(		\
					xdp2_cli_show_cmd)		\
			XDP2_UNIQUE_NAME(_xdp2_cli_show_,) = {		\
	.name = NAME,							\
	.show_func = FUNC,						\
	.class = CLASS,							\
	.argok = ARGOK,							\
}

#define XDP2_CLI_ADD_SHOW_CONFIG(NAME, FUNC, CLASS)			\
	__XDP2_CLI_ADD_SHOW_CONFIG(NAME, FUNC, CLASS, false)

#define XDP2_CLI_ADD_SHOW_CONFIG_ARGOK(NAME, FUNC, CLASS)		\
	__XDP2_CLI_ADD_SHOW_CONFIG(NAME, FUNC, CLASS, true)

XDP2_DEFINE_SECTION(xdp2_cli_show_cmd, struct xdp2_cli_show_cmd_def)

/* Section definition for CLI set functions */
struct xdp2_cli_set_cmd_def {
	char *name;
	void (*set_func)(void *cli, struct xdp2_cli_thread_info *info,
			 char *args);
	unsigned int class;
} XDP2_ALIGN_SECTION;

#define XDP2_CLI_ADD_SET_CONFIG(NAME, FUNC, CLASS)			\
static const struct xdp2_cli_set_cmd_def XDP2_SECTION_ATTR(		\
					xdp2_cli_set_cmd)		\
			XDP2_UNIQUE_NAME(__xdp2_cli_set_,) = {	\
	.name = NAME,							\
	.set_func = FUNC,						\
	.class = CLASS							\
}

XDP2_DEFINE_SECTION(xdp2_cli_set_cmd, struct xdp2_cli_set_cmd_def)

#define XDP2_CLI_MAKE_NUMBER_SET(NAME, CMD)			\
static void set_##NAME##_from_cli(void *cli,			\
		struct xdp2_cli_thread_info *info,		\
		char *args)					\
{								\
	char *token;						\
								\
	token = strtok(args, " \t");				\
	if (!token)						\
		return;						\
								\
	NAME = strtoul(token, NULL, 0);				\
}								\
XDP2_CLI_ADD_SET_CONFIG(#CMD, set_##NAME##_from_cli, 0xffff)

#define XDP2_CLI_MAKE_BOOL_SET(NAME, CMD)			\
static void set_##NAME##_from_cli(void *cli,			\
		struct xdp2_cli_thread_info *info,		\
		char *args)					\
{								\
	char *token;						\
								\
	token = strtok(args, " \t");				\
	if (!token)						\
		return;						\
								\
	NAME = !!strtoul(token, NULL, 0);			\
}								\
XDP2_CLI_ADD_SET_CONFIG(#CMD, set_##NAME##_from_cli, 0xffff)

void xdp2_cli_start(struct xdp2_cli_thread_info *info);
void xdp2_cli_register_start(void (*func)(void *));
void xdp2_cli_register_done(void (*func)(void *));

#endif /* __XDP2_CLI_H__ */
