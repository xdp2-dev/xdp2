// SPDX-License-Identifier: BSD-2-Clause-FreeBSD
/* * Copyright (c) 2025 Tom Herbert
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

/* Parse dump program */

#include <alloca.h>
#include <netinet/ether.h>


#include "xdp2/parser_metadata.h"
#include "xdp2/dtable.h"
#include "xdp2/stable.h"

#include "parse_dump.h"

static void miss(struct metadata *frame)
{
}

static void hit(struct metadata *frame, int seqno, char *text)
{
	PRINTFC(seqno, "\tGot table hit: %s\n", text);
}

XDP2_SFTABLE_PLAIN_TABLE_ENTS(
	ipv4_table_ents,
	struct metadata *,
	(
		addrs.v4.saddr,
		addrs.v4.daddr,
		port_pair.sport,
		port_pair.dport
	),
	(struct metadata *frame, int seqno), (frame, seqno),
	(hit, miss), miss(frame),
	(
		((__cpu_to_be32(0x0a00020f), __cpu_to_be32(0xc0002f3b),
		  __cpu_to_be16(44188), __cpu_to_be16(43)),
		  hit(frame, seqno, "IPv4 plain ents #1")),
		((__cpu_to_be32(0x0a00020f), __cpu_to_be32(0xc0002f3b),
		  __cpu_to_be16(44188), __cpu_to_be16(43)),
		  hit(frame, seqno, "IPv4 plain ents #2")),
		((__cpu_to_be32(0xc0002f3b), __cpu_to_be32(0x0a00020f),
		  __cpu_to_be16(43), __cpu_to_be16(44188)),
		  hit(frame, seqno, "IPv6 plain ents #3"))
	)
)

XDP2_SFTABLE_PLAIN_TABLE(
	ipv4_table_noents,
	struct metadata *,
	(
		addrs.v4.saddr,
		addrs.v4.daddr,
		port_pair.sport,
		port_pair.dport
	),
	(struct metadata *frame, int seqno), (frame, seqno),
	(hit, miss), miss(frame)
)

XDP2_SFTABLE_ADD_PLAIN_MATCH(ipv4_table_noents,
		( __cpu_to_be32(0x0a00020f), __cpu_to_be32(0xc0002f3b),
                  __cpu_to_be16(44188), __cpu_to_be16(43) ),
		  hit(frame, seqno, "IPv4 plain noents #1"),
		  (struct metadata *frame, int seqno));

XDP2_SFTABLE_ADD_PLAIN_MATCH(ipv4_table_noents,
		( __cpu_to_be32(0xc0002f3b), __cpu_to_be32(0x0a00020f),
		  __cpu_to_be16(43), __cpu_to_be16(44188) ),
		  hit(frame, seqno, "IPv4 plain noents #2"),
		  (struct metadata *frame, int seqno));

XDP2_SFTABLE_TERN_TABLE_ENTS(
	ipv4_tern_table_ents,
	struct metadata *,
	(
		addrs.v4.saddr,
		addrs.v4.daddr,
		port_pair.sport,
		port_pair.dport
	),
	(struct metadata *frame, int seqno), (frame, seqno),
	(hit, miss), miss(frame),
	(
		((__cpu_to_be32(0x0a00020f), __cpu_to_be32(0xc0002f3b),
		  __cpu_to_be16(44188), __cpu_to_be16(43)),
		 (__cpu_to_be32(0x0a00020f), __cpu_to_be32(0xc0002f3b),
		  __cpu_to_be16(44188), __cpu_to_be16(43)),
		  hit(frame, seqno, "IPv4 tern ents #1")),
		((__cpu_to_be32(0x0a00020f), __cpu_to_be32(0xc0002f3b),
		  __cpu_to_be16(44188), __cpu_to_be16(43)),
		 (__cpu_to_be32(0x0a00020f), __cpu_to_be32(0xc0002f3b),
		  __cpu_to_be16(44188), __cpu_to_be16(43)),
		  hit(frame, seqno, "IPv4 tern ents #2"))
	)
)

XDP2_SFTABLE_TERN_TABLE(
	ipv4_tern_table_noents,
	struct metadata *,
	(
		addrs.v4.saddr,
		addrs.v4.daddr,
		port_pair.sport,
		port_pair.dport
	),
	(struct metadata *frame, int seqno), (frame, seqno),
	(hit, miss), miss(frame)
)

XDP2_SFTABLE_ADD_TERN_MATCH(ipv4_tern_table_noents,
	(__cpu_to_be32(0x0a00020f), __cpu_to_be32(0xc0002f3b),
	 __cpu_to_be16(44188), __cpu_to_be16(43)),
	(__cpu_to_be32(0x1a00020f), __cpu_to_be32(0xc0002f3b),
	 __cpu_to_be16(44188), __cpu_to_be16(43)),
	hit(frame, seqno, "IPv4 tern noents #1"),
	(struct metadata *frame, int seqno)
);

XDP2_SFTABLE_ADD_TERN_MATCH(ipv4_tern_table_noents,
	(__cpu_to_be32(0x0a00020f), __cpu_to_be32(0xc0002f3b),
	 __cpu_to_be16(44188), __cpu_to_be16(43)),
	(__cpu_to_be32(0x1a00020f), __cpu_to_be32(0xc0002f3b),
	 __cpu_to_be16(44188), __cpu_to_be16(43)),
	hit(frame, seqno, "IPv4 tern noents #2"),
	(struct metadata *frame, int seqno)
);

XDP2_SFTABLE_ADD_TERN_MATCH(ipv4_tern_table_noents,
	(__cpu_to_be32(0x1a00020f), __cpu_to_be32(0xc0002f3b),
	 __cpu_to_be16(44188), __cpu_to_be16(43)),
	(__cpu_to_be32(0x1a00020f), __cpu_to_be32(0xc0002f3b),
	 __cpu_to_be16(44188), __cpu_to_be16(43)),
	hit(frame, seqno, "IPv4 tern noents #3"),
	(struct metadata *frame, int seqno)
);

XDP2_SFTABLE_PLAIN_TABLE_ENTS(
	ipv6_table_ents,
	struct metadata *,
	(
		addrs.v6.saddr,
		addrs.v6.daddr,
		port_pair.sport,
		port_pair.dport
	),
	(struct metadata *frame, int seqno), (frame, seqno),
	(hit, miss), miss(frame),
	(
		(( {{{ 0x20, 0x01, 0x00, 0x00, 0x5e, 0xf5, 0x79, 0xfd,
		       0x38, 0x0c, 0x1d, 0x57, 0xa6, 0x01, 0x24, 0xfa }}},
		   {{{ 0x20, 0x01, 0x06, 0x7c, 0x21, 0x58, 0xa0, 0x19,
		       0x00, 0x00 , 0x00, 0x00 , 0x00, 0x00 , 0x0a, 0xce }}},
		   __cpu_to_be16(13788), __cpu_to_be16(53104)),
		hit(frame, seqno, "IPv6 plain ents #1"))
	)
)

XDP2_SFTABLE_PLAIN_TABLE(
	ipv6_table_noents,
	struct metadata *,
	(
		addrs.v6.saddr,
		addrs.v6.daddr,
		port_pair.sport,
		port_pair.dport
	),
	(struct metadata *frame, int seqno), (frame, seqno),
	(hit, miss), miss(frame)
)

XDP2_SFTABLE_ADD_PLAIN_MATCH(ipv6_table_noents,
	(
		{{{ 0x20, 0x01, 0x00, 0x00, 0x5e, 0xf5, 0x79, 0xfd,
		    0x38, 0x0c, 0x1d, 0x57, 0xa6, 0x01, 0x24, 0xfa }}},
		{{{ 0x20, 0x01, 0x06, 0xdd, 0x21, 0x58, 0xa0, 0x19,
		    0x00, 0x00 , 0x00, 0x00 , 0x00, 0x00 , 0x0a, 0xce }}},
		 __cpu_to_be16(13788), __cpu_to_be16(53104)
	),
	hit(frame, seqno, "IPv6 plain noents #1"),
	(struct metadata *frame, int seqno));

XDP2_SFTABLE_ADD_PLAIN_MATCH(ipv6_table_noents,
	(
		{{{ 0x20, 0x01, 0x00, 0x00, 0x5e, 0xf5, 0x79, 0xfd,
		    0x38, 0x0c, 0x1d, 0x57, 0xa6, 0x01, 0x24, 0xfa }}},
		{{{ 0x20, 0x01, 0x06, 0x7c, 0x21, 0x58, 0xa0, 0x19,
		    0x00, 0x00 , 0x00, 0x00 , 0x00, 0x00 , 0x0a, 0xce }}},
		 __cpu_to_be16(13788), __cpu_to_be16(53104)
	),
	hit(frame, seqno, "IPv6 plain noents #2"),
	(struct metadata *frame, int seqno));

XDP2_SFTABLE_LPM_TABLE(
	ipv6_lpm_table_noents,
	struct metadata *,
	(
		addrs.v6.saddr,
		addrs.v6.daddr
	),
	(struct metadata *frame, int seqno), (frame, seqno),
	(hit, miss), miss(frame)
)

XDP2_SFTABLE_LPM_TABLE_ENTS(
	ipv6_lpm_table_ents,
	struct metadata *,
	(
		addrs.v6.saddr,
		addrs.v6.daddr,
		port_pair.sport,
		port_pair.dport
	),
	(struct metadata *frame, int seqno), (frame, seqno),
	(hit, miss), miss(frame),
	(
		(( {{{ 0x20, 0x01, 0x00, 0x00, 0x5e, 0xf5, 0x79, 0xfd,
		       0x38, 0x0c, 0x1d, 0x57, 0xa6, 0x01, 0x24, 0xfa }}},
		   {{{ 0x20, 0x01, 0x06, 0x7c, 0x21, 0x58, 0xa0, 0x19,
		       0x00, 0x00 , 0x00, 0x00 , 0x00, 0x00 , 0x0a, 0xce }}},
		   __cpu_to_be16(13788), __cpu_to_be16(53104)),
		47, hit(frame, seqno, "IPv6 lpm ents #1"))
	)
)

XDP2_SFTABLE_ADD_LPM_MATCH(ipv6_lpm_table_noents,
	(
		{{{ 0x20, 0x01, 0x00, 0x00, 0x5e, 0xf5, 0x79, 0xfd,
		    0x38, 0x0c, 0x1d, 0x57, 0xa6, 0x01, 0x24, 0xfa }}},
		{{{ 0x20, 0x01, 0x06, 0xdd, 0x21, 0x58, 0xa0, 0x19,
		    0x00, 0x00 , 0x00, 0x00 , 0x00, 0x00 , 0x0a, 0xce }}}
	),
	35, hit(frame, seqno, "IPv6 lpm noents #1"),
	(struct metadata *frame, int seqno));

XDP2_SFTABLE_ADD_LPM_MATCH(ipv6_lpm_table_noents,
	(
		{{{ 0x20, 0x01, 0x00, 0x00, 0x5e, 0xf5, 0x79, 0xfd,
		    0x38, 0x0c, 0x1d, 0x57, 0xa6, 0x01, 0x24, 0xfa }}},
		{{{ 0x20, 0x01, 0x06, 0xdd, 0x21, 0x58, 0xa0, 0x19,
		    0x00, 0x00 , 0x00, 0x00 , 0x00, 0x00 , 0x0a, 0xce }}}
	),
	75, hit(frame, seqno, "IPv6 lpm noents #2"),
	(struct metadata *frame, int seqno));

XDP2_SFTABLE_ADD_LPM_MATCH(ipv6_lpm_table_noents,
	(
		{{{ 0x20, 0x01, 0x00, 0x00, 0x5e, 0xf5, 0x79, 0xfd,
		    0x38, 0x0c, 0x1d, 0x57, 0xa6, 0x01, 0x24, 0xfa }}},
		{{{ 0x40, 0x01, 0x06, 0xdd, 0x21, 0x58, 0xa0, 0x19,
		    0x00, 0x00 , 0x00, 0x00 , 0x00, 0x00 , 0x0a, 0xce }}}
	),
	130, hit(frame, seqno, "IPv6 lpm noents #3"),
	(struct metadata *frame, int seqno));

void lookup_tuple(struct metadata *frame, int seqno)
{
	switch (frame->addr_type) {
	case XDP2_ADDR_TYPE_IPV4:
		ipv4_table_ents_lookup(frame, frame, seqno);
		ipv4_table_noents_lookup(frame, frame, seqno);
		ipv4_tern_table_noents_lookup(frame, frame,
							   seqno);
		break;
	case XDP2_ADDR_TYPE_IPV6:
		ipv6_table_ents_lookup(frame, frame, seqno);
		ipv6_table_noents_lookup(frame, frame, seqno);
		ipv6_lpm_table_noents_lookup(frame, frame, seqno);
		break;
	default:
		break;
	}
}

static void default_func(void *arg, void *entry_arg)
{
}

XDP2_DFTABLE_PLAIN_TABLE(
	ipv6_dtable,
	struct metadata *,
	(
		addrs.v6.saddr,
		addrs.v6.daddr,
		port_pair.sport,
		port_pair.dport
	),
	default_func, ()
)

void init_tables(void)
{
}
