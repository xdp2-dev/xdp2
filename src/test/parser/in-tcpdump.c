// SPDX-License-Identifier: BSD-2-Clause-FreeBSD
/*
 * Copyright (c) 2020, 2021 by Mojatatu Networks.
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

/*
 * Input method that reads tcpdump -xx output from stdin.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "imethod.h"

extern const char *__progname;

struct tcpdump_priv {
	int close;
	FILE *f;
};

struct tcpdump_line {
	char *b;
	int a;
	int l;
};

static void line_init(struct tcpdump_line *line)
{
	memset(line, 0, sizeof(*line));
}

static void line_savec(struct tcpdump_line *line, int c)
{
	if (line->l >= line->a) {
		line->a = line->l + 16;
		line->b = realloc(line->b, line->a);

		if (!line->b) {
			fprintf(stderr, "tcpdump realloc failed\n");
			exit(-1);
		}
	}

	line->b[line->l++] = c;
}

static int line_getline(struct tcpdump_line *line, FILE *from)
{
	int c;

	line->l = 0;

	while (1) {
		c = getc(from);
		switch (c) {
		case EOF:
			if (line->l == 0)
				return (0);
			fprintf(stderr,
				"%s: missing trailing newline (supplied)\n",
				__progname);
			// fall through
		case '\n':
			line_savec(line, '\0');
			return 1;
		default:
			line_savec(line, c);
			break;
		}
	}
}

static void in_tcpdump_help(void)
{
	fprintf(stderr,
		"For `tcpdump' input, if ARGS is not given, or if ARGS is "
		"`-', stdin\n"
		"is read.  Otherwise, ARGS is opened as a file and input is "
		"read from\n"
		"there.\n\n"
		"Input format is tcpdump -xx output.\n");
}

static void *in_tcpdump_init(const char *args)
{
	struct tcpdump_priv *p;

	p = malloc(sizeof(struct tcpdump_priv));
	if (!p) {
		fprintf(stderr, "tcpdump init failed\n");
		exit(-1);
	}

	if (!args || !strcmp(args, "-")) {
		p->close = 0;
		p->f = stdin;
	} else {
		p->f = fopen(args, "r");
		if (!p->f) {
			fprintf(stderr, "%s: can't open %s: %s\n", __progname,
				args, strerror(errno));
			exit(-1);
		}
		p->close = 1;
	}

	return (p);
}

static enum test_parser_rprv in_tcpdump_readpkt(void *pv, unsigned char *data,
						size_t maxlen, size_t *lenp)
{
	int o, n, po, overflow = 0, len = 0;
	static struct tcpdump_line line;
	struct tcpdump_priv *p = pv;
	static int didinit;
	unsigned int xv;

	if (!didinit) {
		didinit = 1;
		line_init(&line);
	}

	while (line_getline(&line, p->f)) {
		/* Do we have a hex offset? */
		n = -1;
		if (sscanf(line.b, " 0x%x:%n", &xv, &n) == EOF) {
			if (errno != 0) {
				fprintf(stderr, "%s: Failure: %s\n", __progname,
					strerror(errno));
				exit(-1);
			}
		}

		// If not, this is a packet boundary
		if (n < 0) {
			if (len > 0) {
				*lenp = len;
				return (overflow ? PARSER_TEST_RP_OVF :
					PARSER_TEST_RP_GOOD);
			}
			continue;
		}

		po = xv;
		// Make sure we don't mistake text for bytes
		if (line.l >= 49)
			line.b[49] = '\0';

		o = n;
		while (1) {
			n = -1;
			if (sscanf(line.b + o, " %2x%n", &xv, &n) == EOF) {
				if (errno != 0) {
					fprintf(stderr, "%s: Failure: %s\n",
						__progname, strerror(errno));
					exit(-1);
				}
			}
			if (n < 0)
				break;
			o += n;
			if (po < maxlen)
				data[po] = xv;
			else
				overflow = 1;
			po++;
		}
		if (po > len)
			len = po;
	}
	return (PARSER_TEST_RP_EOF);
}

static void in_tcpdump_done(void *pv)
{
	struct tcpdump_priv *p = pv;

	if (p->close)
		fclose(p->f);
	free(p);
}

IMETHOD_DECL(tcpdump)
