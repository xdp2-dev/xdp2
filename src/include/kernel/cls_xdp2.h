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

#ifndef __CLS_XDP2_H__
#define __CLS_XDP2_H__

#ifdef __KERNEL__

#include <linux/list.h>
#include <linux/skbuff.h>

#define TCA_XDP2_MAX (__TCA_XDP2_MAX - 1)

struct tc_cls_xdp2_ops {
	const char *name;
	struct list_head list;
	int (*parse)(struct sk_buff *pkt);
	struct module *owner;
};

int register_xdp2_ops(struct tc_cls_xdp2_ops *ops);
int unregister_xdp2_ops(struct tc_cls_xdp2_ops *ops);

#endif /* __KERNEL__ */

/* UAPI */
enum {
	TCA_XDP2_UNSPEC,
	TCA_XDP2_CLASSID,
	TCA_XDP2_PARSER,
	__TCA_XDP2_MAX,
};

#endif /* __CLS_XDP2_H__ */
