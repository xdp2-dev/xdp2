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
 * Input method designed for fuzzing.
 *
 * This is a lot like the raw input method, except that it's stripped
 * even further down; it is designed for fuzzing.  It reads exactly
 * one packet from stdin, reading stdin to EOF and making that the
 * packet.  It always delivers exactly one packet to the main line.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>
#include <unistd.h>

extern const char *__progname;

#include "imethod.h"

struct fuzz_priv {
	int close;
	FILE *f;
};

static void in_fuzz_help(void)
{
	fprintf(stderr,
		"For `fuzz' input, arguments, if given, are ignored.\n\n"
		"Input data is raw binary; stdin is read to EOF and the "
		"result is\n"
		"exactly one packet.\n");
}

static void *in_fuzz_init(const char *args)
{
	int *p;

	p = calloc(1, sizeof(int));
	if (!p) {
		fprintf(stderr, "Fuzz init failed\n");
		exit(-1);
	}

	return (p);
}

static enum test_parser_rprv in_fuzz_readpkt(void *pv, unsigned char *data,
					     size_t maxlen, size_t *lenp)
{
	struct fuzz_priv *p = pv;
	struct iovec iov[2];
	int o = 0, n, niov;
	char ovf;

	if (p->close)
		return (PARSER_TEST_RP_EOF);

	p->close = 1;

	while (1) {
		if (o > maxlen)
			return (PARSER_TEST_RP_OVF);
		niov = 0;
		if (o < maxlen) {
			iov[niov].iov_base = data;
			iov[niov].iov_len = maxlen - o;
			niov++;
		}
		iov[niov].iov_base = &ovf;
		iov[niov].iov_len = 1;
		niov++;
		n = readv(0, &iov[0], niov);
		if (n < 0) {
			fprintf(stderr, "%s: input read: %s\n", __progname,
				strerror(errno));
			return PARSER_TEST_RP_ERR;
		}
		if (n == 0) {
			*lenp = o;
			return PARSER_TEST_RP_GOOD;
		}
		o += n;
	}
}

static void in_fuzz_done(void *pv)
{
	free(pv);
}

IMETHOD_DECL(fuzz)
