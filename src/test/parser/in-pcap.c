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
 * Input method that reads pcap files.
 */

#include <errno.h>
#include <pcap.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern const char *__progname;

#include "imethod.h"

struct pcap_priv {
	pcap_t *pcapf;
};

static void in_pcap_help(void)
{
	fprintf(stderr,
		"For `pcap' input, ARGS is opened as a file and input is "
		"read from\n"
		"there.\n\n"
		"Input format is pcap\n");
}

static void *in_pcap_init(const char *args)
{
	char errbuf[PCAP_ERRBUF_SIZE];
	struct pcap_priv *p;

	p = calloc(1, sizeof(struct pcap_priv));
	if (!p) {
		fprintf(stderr, "%s: can't malloc %s: %s\n", __progname, args,
			strerror(errno));
		exit(-1);
	}

	p->pcapf = pcap_open_offline(args, errbuf);
	if (!p->pcapf) {
		fprintf(stderr, "%s: can't open %s: %s\n", __progname, args,
			strerror(errno));
		free(p);
		exit(-1);
	}

	return p;
}

static enum test_parser_rprv in_pcap_readpkt(void *pv, unsigned char *data,
					     size_t maxlen, size_t *lenp)
{
	const unsigned char *packet;
	struct pcap_pkthdr header;
	struct pcap_priv *p = pv;

	packet = pcap_next(p->pcapf, &header);
	if (!packet)
		return PARSER_TEST_RP_EOF;

	*lenp = header.caplen;

	//XXX: check maxlen >= caplen return RP_OVF

	memcpy(data, packet, header.caplen);

	return PARSER_TEST_RP_GOOD;
}

static void in_pcap_done(void *pv)
{
	struct pcap_priv *p = pv;

	pcap_close(p->pcapf);
	free(p);
}

IMETHOD_DECL(pcap)
