/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __FLOWDIS_TCP_H__
#define __FLOWDIS_TCP_H__

/* Took definition from kernel include/net/tcp.h */

static inline unsigned int __tcp_hdrlen(const struct tcphdr *th)
{
        return th->doff * 4;
}

#endif /* __FLOWDIS_TCP_H__ */
