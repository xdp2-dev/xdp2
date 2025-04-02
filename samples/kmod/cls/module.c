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

#include <linux/string.h>
#include <linux/skbuff.h>

#include "kernel/cls_xdp2.h"

#include "xdp2/parser.h"
#include "xdp2/parser_metadata.h"
#include "xdp2/tc_tmpl.h"

XDP2_PARSER_KMOD_EXTERN(xdp2_parser_big_ether);

/* Meta data structure for just one frame */
struct xdp2_parser_big_metadata_one {
	struct xdp2_metadata xdp2_data;
	struct xdp2_metadata_all frame;
};

static int do_parse(struct sk_buff *skb)
{
	int err;
	struct xdp2_parser_big_metadata_one mdata;
	void *data;
	size_t pktlen;

	memset(&mdata, 0, sizeof(mdata));

	err = skb_linearize(skb);
	if (err < 0)
		return err;

	BUG_ON(skb->data_len);

	data = skb_mac_header(skb);
	pktlen = skb_mac_header_len(skb) + skb->len;

	err = xdp2_parse(XDP2_PARSER_KMOD_NAME(xdp2_parser_big_ether), data,
			  pktlen, &mdata.xdp2_data, 0, 1);
	if (err != XDP2_STOP_OKAY) {
		pr_debug("Failed to parse packet! (%d)", err);
		return -1;
	}

	pr_debug("Parsed packet!");

	return 0;
}

XDP2_TC_MAKE_PARSER_PROGRAM("big", do_parse);
