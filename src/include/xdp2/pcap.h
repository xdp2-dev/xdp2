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

#ifndef __XDP2_PCAP_H__
#define __XDP2_PCAP_H__

/* XDP2 library utility to read packets from pcap files */

#include <pcap.h>
#include <string.h>

struct xdp2_pcap_file {
	pcap_t *pcapf;
};

static inline ssize_t xdp2_pcap_readpkt(struct xdp2_pcap_file *pf,
					 void *data,
					 size_t maxlen, size_t *lenp)
{
	struct pcap_pkthdr header;
	const void *packet;
	size_t len;

	packet = pcap_next(pf->pcapf, &header);
	if (!packet) {
		/* NULL means either error or EOF. Just return <0 */

		return -1;
	}

	*lenp = header.caplen;

	len = header.caplen > maxlen ? maxlen : header.caplen;

	memcpy(data, packet, len);

	return len;
}

struct xdp2_pcap_file *xdp2_pcap_init(const char *args);

void xdp2_pcap_close(struct xdp2_pcap_file *pf);

#endif /* __XDP2_PCAP_H__ */
