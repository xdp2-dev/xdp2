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
 * Input method that reads raw binary data from stdin.
 *
 * A packet consists of two bytes of (network-order) length, followed
 * by that many bytes of packet contents.  The next packet, if any,
 * follows immediately.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "imethod.h"

extern const char *__progname;

struct raw_priv {
	int close;
	FILE *f;
};

static const char *iestr(FILE *f)
{
	if (feof(f))
		return ("EOF");
	if (ferror(f))
		return ("error");
	return "incomprehensible status";
}

static void in_raw_help(void)
{
	fprintf(stderr,
		"For `raw' input, if ARGS is not given, or if ARGS is `-', "
		"stdin is\n"
		"read.  Otherwise, ARGS is opened as a file and input is read "
		"from\n"
		"there.\n\n"
		"Input data is raw binary; each packet is simply prefixed "
		"with its size\n"
		"as two bytes, big-endian.\n");
}

static void *in_raw_init(const char *args)
{
	struct raw_priv *p;

	p = calloc(1, sizeof(struct raw_priv));
	if (!p) {
		fprintf(stderr, "Raw init failed\n");
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

static enum test_parser_rprv in_raw_readpkt(void *pv, unsigned char *data,
					    size_t maxlen, size_t *lenp)
{
	struct raw_priv *p = pv;
	int l, i, c, overflow;

	c = getc(p->f);
	if (c == EOF)
		return (PARSER_TEST_RP_EOF);

	l = c * 256;
	c = getc(p->f);
	if (c == EOF) {
		fprintf(stderr, "%s: %s reading packet length\n", __progname,
			iestr(p->f));
		return PARSER_TEST_RP_ERR;
	}

	l += c;
	overflow = (l > maxlen);
	for (i = 0; i < l; i++) {
		c = getc(p->f);
		if (c == EOF) {
			fprintf(stderr, "%s: %s reading packet data\n",
				__progname, iestr(p->f));
			return PARSER_TEST_RP_ERR;
		}
		if (!overflow)
			data[i] = c;
	}
	*lenp = l;

	return overflow ? PARSER_TEST_RP_OVF : PARSER_TEST_RP_GOOD;
}

static void in_raw_done(void *pv)
{
	struct raw_priv *p = pv;

	if (p->close)
		fclose(p->f);
	free(p);
}

IMETHOD_DECL(raw)
