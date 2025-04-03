/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __FLOWDIS_IP_H__
#define __FLOWDIS_IP_H__

/* Took a few definitions from kernel include/net/ip.h */

/* IP flags. */
#define IP_CE		0x8000		/* Flag: "Congestion"           */
#define IP_DF		0x4000		/* Flag: "Don't Fragment"       */
#define IP_MF		0x2000		/* Flag: "More Fragments"       */
#define IP_OFFSET	0x1FFF		/* "Fragment Offset" part       */

static inline bool ip_is_fragment(const struct iphdr *iph)
{
	return (iph->frag_off & htons(IP_MF | IP_OFFSET)) != 0;
}

#endif /* __FLOWDIS_IP_H__ */
