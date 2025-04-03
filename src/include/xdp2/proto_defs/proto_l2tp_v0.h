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

#ifndef __XDP2_PROTO_L2TP_V0_H__
#define __XDP2_PROTO_L2TP_V0_H__

/* L2Tp protocol definitions */

#ifndef IPPROTO_L2TP
#define IPPROTO_L2TP 115
#endif

#ifndef L2TP_F_TYPE
#define L2TP_F_TYPE	__cpu_to_be16(0x8000)
#endif

#ifndef L2TP_F_LENGTH
#define L2TP_F_LENGTH	__cpu_to_be16(0x4000)
#endif

#ifndef L2TP_F_NSNR
#define L2TP_F_NSNR	__cpu_to_be16(0x0800)
#endif

#ifndef L2TP_F_OFFSZ
#define L2TP_F_OFFSZ	__cpu_to_be16(0x0200)
#endif

#ifndef L2TP_F_PRIORITY
#define L2TP_F_PRIORITY	__cpu_to_be16(0x0100)
#endif

/* L2TP flag-field definitions */
static const struct xdp2_flag_fields l2tp_v0_base_flag_fields = {
	.fields = {
#define L2TP_FLAGS_LENGTH_IDX	0
		{
			.flag = L2TP_F_LENGTH,
			.size = sizeof(__be16),
		},
#define L2TP_FLAGS_NSNR_IDX	1
		{
			.flag = L2TP_F_NSNR,
			.size = sizeof(__be32),
		},
#define L2TP_FLAGS_NUM_IDX	2
	},
	.num_idx = L2TP_FLAGS_NUM_IDX
};

static inline size_t l2tp_v0_base_len_from_flags(unsigned int flags)
{
	return 2 + 4 +
		xdp2_flag_fields_length(flags, &l2tp_v0_base_flag_fields);
}

static inline ssize_t l2tp_v0_base_len_check(const void *vl2tp)
{
	return l2tp_v0_base_len_from_flags(*(__u16 *)vl2tp);
}

static inline int l2tp_v0_base_proto_version(const void *vl2tp)
{
	return !!(*(__u16 *)vl2tp & L2TP_F_OFFSZ);
}

static inline ssize_t l2tp_v0_offsz_len(const void *vl2tp)
{
	return 2 + *(__u16 *)vl2tp;
}

#endif /* __XDP2_PROTO_L2TP_V0_H__ */

#ifdef XDP2_DEFINE_PARSE_NODE

/* xdp2_parse_l2tp_base protocol definition
 *
 * Parse base L2TP header as an overlay to determine L2TP version
 *
 * Next protocol operation returns L2TP version number (i.e. 0, 1, or 3).
 */


static const struct xdp2_proto_def
					xdp_parse_l2tp_v0_base __unused() = {
	.name = "L2TP v0",
	.min_len = 2,
	.ops.next_proto = l2tp_v0_base_proto_version,
	.ops.len = l2tp_v0_base_len_check,
};

static const struct xdp2_proto_def xdp2_parse_l2tp_v0_offsz __unused() = {
        .name = "L2TP offset-size field",
        .min_len = 2,
        .ops.len = l2tp_v0_offsz_len,
};

#endif /* XDP2_DEFINE_PARSE_NODE */
