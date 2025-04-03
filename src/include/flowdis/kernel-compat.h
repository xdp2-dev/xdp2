/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __FLOWDIS_KERNEL_COMPAT_H__
#define __FLOWDIS_KERNEL_COMPAT_H__

/*
 * Definitions from various kernel headers to Provide enough compatibility
 * with the kernel build environment for flow_dissector.c to compile.
 */

static inline int proto_ports_offset(int proto)
{
	switch (proto) {
	case IPPROTO_TCP:
	case IPPROTO_UDP:
	case IPPROTO_DCCP:
	case IPPROTO_ESP:	/* SPI */
	case IPPROTO_SCTP:
	case IPPROTO_UDPLITE:
		return 0;
	case IPPROTO_AH:	/* SPI */
		return 4;
	default:
		return -EINVAL;
	}
}

static inline unsigned int __tcp_hdrlen(const struct tcphdr *th)
{
	return th->doff * 4;
}

#define IP_CE           0x8000	/* Flag: "Congestion"           */
#define IP_DF           0x4000	/* Flag: "Don't Fragment"       */
#define IP_MF           0x2000	/* Flag: "More Fragments"       */
#define IP_OFFSET       0x1FFF	/* "Fragment Offset" part       */

static inline bool ip_is_fragment(const struct iphdr *iph)
{
	return (iph->frag_off & htons(IP_MF | IP_OFFSET)) != 0;
}

#define IP6_MF          0x0001
#define IP6_OFFSET      0xFFF8

#define IPV6_FLOWINFO_MASK              cpu_to_be32(0x0FFFFFFF)
#define IPV6_FLOWLABEL_MASK             cpu_to_be32(0x000FFFFF)
#define IPV6_FLOWLABEL_STATELESS_FLAG   cpu_to_be32(0x00080000)

#define IPV6_TCLASS_MASK (IPV6_FLOWINFO_MASK & ~IPV6_FLOWLABEL_MASK)
#define IPV6_TCLASS_SHIFT       20

static inline __be32 ip6_flowlabel(const struct ipv6hdr *hdr)
{
	return *(__be32 *) hdr & IPV6_FLOWLABEL_MASK;
}

struct tipc_basic_hdr {
	__be32 w[4];
};

#define KEEPALIVE_MSG_MASK 0x0e080000	/* LINK_PROTOCOL + MSG_IS_KEEPALIVE */

static inline __be32 tipc_hdr_rps_key(struct tipc_basic_hdr *hdr)
{

	u32 w0 = ntohl(hdr->w[0]);
	bool keepalive_msg = (w0 & KEEPALIVE_MSG_MASK) == KEEPALIVE_MSG_MASK;
	__be32 key;

	/* Return source node identity as key */
	if (likely(!keepalive_msg))
		return hdr->w[3];

	/* Spread PROBE/PROBE_REPLY messages across the cores */
	get_random_bytes(&key, sizeof(key));
	return key;
}

#define NEXTHDR_HOP             0	/* Hop-by-hop option header. */
#define NEXTHDR_TCP             6	/* TCP segment. */
#define NEXTHDR_UDP             17	/* UDP message. */
#define NEXTHDR_IPV6            41	/* IPv6 in IPv6 */
#define NEXTHDR_ROUTING         43	/* Routing header. */
#define NEXTHDR_FRAGMENT        44	/* Fragmentation/reassembly header. */
#define NEXTHDR_GRE             47	/* GRE header. */
#define NEXTHDR_ESP             50	/* Encapsulating security payload. */
#define NEXTHDR_AUTH            51	/* Authentication header. */
#define NEXTHDR_ICMP            58	/* ICMP for IPv6. */
#define NEXTHDR_NONE            59	/* No next header */
#define NEXTHDR_DEST            60	/* Destination options header. */
#define NEXTHDR_SCTP            132	/* SCTP message. */
#define NEXTHDR_MOBILITY        135	/* Mobility header. */

struct frag_hdr {
	__u8 nexthdr;
	__u8 reserved;
	__be16 frag_off;
	__be32 identification;
};

static inline u32 ipv6_addr_hash(const struct in6_addr *a)
{
#if defined(CONFIG_HAVE_EFFICIENT_UNALIGNED_ACCESS) && BITS_PER_LONG == 64
	const unsigned long *ul = (const unsigned long *)a;
	unsigned long x = ul[0] ^ ul[1];

	return (u32) (x ^ (x >> 32));
#else
	return (__force u32) (a->s6_addr32[0] ^ a->s6_addr32[1] ^
			      a->s6_addr32[2] ^ a->s6_addr32[3]);
#endif
}

#endif /* __FLOWDIS_KERNEL_COMPAT_H__ */
