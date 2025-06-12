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

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include "parselite/parser.h"
#include "test-parser-core.h"

struct parselite_priv {
	struct parselite_metadata md;
};

static void core_parselite_help(void)
{
	fprintf(stderr,
		"For the `parselite' core, arguments must be either not given "
		"or zero length.\n\n"
		"This core uses the parselite library which impelements a "
		"lightweight parser.\n");
}

static void *core_parselite_init(const char *args)
{
	struct parselite_priv *p;

	if (args && *args) {
		fprintf(stderr, "The parselite core takes no arguments.\n");
		exit(-1);
	}

	p = malloc(sizeof(struct parselite_priv));
	if (!p) {
		fprintf(stderr, "Parselite init failed\n");
		exit(-1);
	}

	return (p);
}

static const char *core_parselite_process(void *pv, void *data,
					  size_t len,
					  struct test_parser_out *out,
					  unsigned int flags, long long *time)
{
	struct parselite_priv *p = pv;
	int suc = false;

	memset(&p->md, 0, sizeof(p->md));
	memset(out, 0, sizeof(*out));


	if (!(flags & CORE_F_NOCORE)) {
		struct timespec begin_tp, now_tp;

		clock_gettime(CLOCK_MONOTONIC_RAW, &begin_tp);
		suc = parselite_parse(data, len, &p->md,
				  PARSELITE_F_STOP_FLOWLABEL,
				  PARSELITE_ENCAP_DEPTH,
				  PARSELITE_START_ETHER);
		if (!suc)
			return "parselite_parse failed";

		clock_gettime(CLOCK_MONOTONIC_RAW, &now_tp);
		*time += (now_tp.tv_sec - begin_tp.tv_sec) * 1000000000 +
			 (now_tp.tv_nsec - begin_tp.tv_nsec);
	}

	if (flags & CORE_F_HASH)
		out->k_hash.hash = parselite_hash_metadata(&p->md);

	out->k_ports.src = p->md.port16[0];
	out->k_ports.dst = p->md.port16[1];

	switch (p->md.addr_type) {
	case PARSELITE_ATYPE_IPV4:
		out->k_ipv4_addrs.src = p->md.addrs.v4_addrs[0];
		out->k_ipv4_addrs.dst = p->md.addrs.v4_addrs[1];
		break;
	case PARSELITE_ATYPE_IPV6:
		memcpy(out->k_ipv6_addrs.src, p->md.addrs.v6_addrs, 16);
		memcpy(out->k_ipv6_addrs.dst, &p->md.addrs.v6_addrs[1], 16);
		break;
	}
	return (0);
}

static void core_parselite_done(void *pv)
{
	free(pv);
}

CORE_DECL(parselite)
