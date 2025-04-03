/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef __FLOWDIS_IPV6_H__
#define __FLOWDIS_IPV6_H__

/* Took definitions from kernel include/net/ipv6.h */

#include "flowdis/jhash.h"
#include "flowdis/in.h"

#define IPV6_MAXPLEN		65535

/*
 *	NextHeader field of IPv6 header
 */

#define NEXTHDR_HOP		0	/* Hop-by-hop option header. */
#define NEXTHDR_TCP		6	/* TCP segment. */
#define NEXTHDR_UDP		17	/* UDP message. */
#define NEXTHDR_IPV6		41	/* IPv6 in IPv6 */
#define NEXTHDR_ROUTING		43	/* Routing header. */
#define NEXTHDR_FRAGMENT	44	/* Fragmentation/reassembly header. */
#define NEXTHDR_GRE		47	/* GRE header. */
#define NEXTHDR_ESP		50	/* Encapsulating security payload. */
#define NEXTHDR_AUTH		51	/* Authentication header. */
#define NEXTHDR_ICMP		58	/* ICMP for IPv6. */
#define NEXTHDR_NONE		59	/* No next header */
#define NEXTHDR_DEST		60	/* Destination options header. */
#define NEXTHDR_SCTP		132	/* SCTP message. */
#define NEXTHDR_MOBILITY	135	/* Mobility header. */

#define IPV6_ADDR_ANY		0x0000U

#define IPV6_ADDR_UNICAST	0x0001U
#define IPV6_ADDR_MULTICAST	0x0002U

#define IPV6_ADDR_LOOPBACK	0x0010U
#define IPV6_ADDR_LINKLOCAL	0x0020U
#define IPV6_ADDR_SITELOCAL	0x0040U

#define IPV6_ADDR_COMPATv4	0x0080U

#define IPV6_ADDR_SCOPE_MASK	0x00f0U

#define IPV6_ADDR_MAPPED	0x1000U

/*
 *	Addr scopes
 */
#define IPV6_ADDR_MC_SCOPE(a)	\
	((a)->s6_addr[1] & 0x0f)	/* nonstandard */
#define __IPV6_ADDR_SCOPE_INVALID	-1
#define IPV6_ADDR_SCOPE_NODELOCAL	0x01
#define IPV6_ADDR_SCOPE_LINKLOCAL	0x02
#define IPV6_ADDR_SCOPE_SITELOCAL	0x05
#define IPV6_ADDR_SCOPE_ORGLOCAL	0x08
#define IPV6_ADDR_SCOPE_GLOBAL		0x0e

/*
 *	Addr flags
 */
#define IPV6_ADDR_MC_FLAG_TRANSIENT(a)	\
	((a)->s6_addr[1] & 0x10)
#define IPV6_ADDR_MC_FLAG_PREFIX(a)	\
	((a)->s6_addr[1] & 0x20)
#define IPV6_ADDR_MC_FLAG_RENDEZVOUS(a)	\
	((a)->s6_addr[1] & 0x40)

/*
 *	fragmentation header
 */

struct frag_hdr {
	__u8	nexthdr;
	__u8	reserved;
	__be16	frag_off;
	__be32	identification;
};

#define	IP6_MF		0x0001
#define	IP6_OFFSET	0xFFF8

#define IPV6_FLOWINFO_MASK		cpu_to_be32(0x0FFFFFFF)
#define IPV6_FLOWLABEL_MASK		cpu_to_be32(0x000FFFFF)
#define IPV6_FLOWLABEL_STATELESS_FLAG	cpu_to_be32(0x00080000)

#define IPV6_TCLASS_MASK (IPV6_FLOWINFO_MASK & ~IPV6_FLOWLABEL_MASK)
#define IPV6_TCLASS_SHIFT	20

int __ipv6_addr_type(const struct in6_addr *addr);
static inline int ipv6_addr_type(const struct in6_addr *addr)
{
	return __ipv6_addr_type(addr) & 0xffff;
}

static inline int ipv6_addr_scope(const struct in6_addr *addr)
{
	return __ipv6_addr_type(addr) & IPV6_ADDR_SCOPE_MASK;
}

static inline int __ipv6_addr_src_scope(int type)
{
	return (type == IPV6_ADDR_ANY) ? __IPV6_ADDR_SCOPE_INVALID : (type >> 16);
}

static inline int ipv6_addr_src_scope(const struct in6_addr *addr)
{
	return __ipv6_addr_src_scope(__ipv6_addr_type(addr));
}

static inline bool __ipv6_addr_needs_scope_id(int type)
{
	return type & IPV6_ADDR_LINKLOCAL ||
	       (type & IPV6_ADDR_MULTICAST &&
		(type & (IPV6_ADDR_LOOPBACK|IPV6_ADDR_LINKLOCAL)));
}

static inline int ipv6_addr_cmp(const struct in6_addr *a1, const struct in6_addr *a2)
{
	return memcmp(a1, a2, sizeof(struct in6_addr));
}

static inline bool
ipv6_masked_addr_cmp(const struct in6_addr *a1, const struct in6_addr *m,
		     const struct in6_addr *a2)
{
#if defined(CONFIG_HAVE_EFFICIENT_UNALIGNED_ACCESS) && BITS_PER_LONG == 64
	const unsigned long *ul1 = (const unsigned long *)a1;
	const unsigned long *ulm = (const unsigned long *)m;
	const unsigned long *ul2 = (const unsigned long *)a2;

	return !!(((ul1[0] ^ ul2[0]) & ulm[0]) |
		  ((ul1[1] ^ ul2[1]) & ulm[1]));
#else
	return !!(((a1->s6_addr32[0] ^ a2->s6_addr32[0]) & m->s6_addr32[0]) |
		  ((a1->s6_addr32[1] ^ a2->s6_addr32[1]) & m->s6_addr32[1]) |
		  ((a1->s6_addr32[2] ^ a2->s6_addr32[2]) & m->s6_addr32[2]) |
		  ((a1->s6_addr32[3] ^ a2->s6_addr32[3]) & m->s6_addr32[3]));
#endif
}

static inline void ipv6_addr_prefix(struct in6_addr *pfx,
				    const struct in6_addr *addr,
				    int plen)
{
	/* caller must guarantee 0 <= plen <= 128 */
	int o = plen >> 3,
	    b = plen & 0x7;

	memset(pfx->s6_addr, 0, sizeof(pfx->s6_addr));
	memcpy(pfx->s6_addr, addr, o);
	if (b != 0)
		pfx->s6_addr[o] = addr->s6_addr[o] & (0xff00 >> b);
}

static inline void ipv6_addr_prefix_copy(struct in6_addr *addr,
					 const struct in6_addr *pfx,
					 int plen)
{
	/* caller must guarantee 0 <= plen <= 128 */
	int o = plen >> 3,
	    b = plen & 0x7;

	memcpy(addr->s6_addr, pfx, o);
	if (b != 0) {
		addr->s6_addr[o] &= ~(0xff00 >> b);
		addr->s6_addr[o] |= (pfx->s6_addr[o] & (0xff00 >> b));
	}
}

static inline void __ipv6_addr_set_half(__be32 *addr,
					__be32 wh, __be32 wl)
{
#if defined(CONFIG_HAVE_EFFICIENT_UNALIGNED_ACCESS) && BITS_PER_LONG == 64
#if defined(__BIG_ENDIAN)
	if (__builtin_constant_p(wh) && __builtin_constant_p(wl)) {
		*(__force u64 *)addr = ((__force u64)(wh) << 32 | (__force u64)(wl));
		return;
	}
#elif defined(__LITTLE_ENDIAN)
	if (__builtin_constant_p(wl) && __builtin_constant_p(wh)) {
		*(__force u64 *)addr = ((__force u64)(wl) << 32 | (__force u64)(wh));
		return;
	}
#endif
#endif
	addr[0] = wh;
	addr[1] = wl;
}

static inline void ipv6_addr_set(struct in6_addr *addr,
				     __be32 w1, __be32 w2,
				     __be32 w3, __be32 w4)
{
	__ipv6_addr_set_half(&addr->s6_addr32[0], w1, w2);
	__ipv6_addr_set_half(&addr->s6_addr32[2], w3, w4);
}

static inline bool ipv6_addr_equal(const struct in6_addr *a1,
				   const struct in6_addr *a2)
{
#if defined(CONFIG_HAVE_EFFICIENT_UNALIGNED_ACCESS) && BITS_PER_LONG == 64
	const unsigned long *ul1 = (const unsigned long *)a1;
	const unsigned long *ul2 = (const unsigned long *)a2;

	return ((ul1[0] ^ ul2[0]) | (ul1[1] ^ ul2[1])) == 0UL;
#else
	return ((a1->s6_addr32[0] ^ a2->s6_addr32[0]) |
		(a1->s6_addr32[1] ^ a2->s6_addr32[1]) |
		(a1->s6_addr32[2] ^ a2->s6_addr32[2]) |
		(a1->s6_addr32[3] ^ a2->s6_addr32[3])) == 0;
#endif
}

#if defined(CONFIG_HAVE_EFFICIENT_UNALIGNED_ACCESS) && BITS_PER_LONG == 64
static inline bool __ipv6_prefix_equal64_half(const __be64 *a1,
					      const __be64 *a2,
					      unsigned int len)
{
	if (len && ((*a1 ^ *a2) & cpu_to_be64((~0UL) << (64 - len))))
		return false;
	return true;
}

static inline bool ipv6_prefix_equal(const struct in6_addr *addr1,
				     const struct in6_addr *addr2,
				     unsigned int prefixlen)
{
	const __be64 *a1 = (const __be64 *)addr1;
	const __be64 *a2 = (const __be64 *)addr2;

	if (prefixlen >= 64) {
		if (a1[0] ^ a2[0])
			return false;
		return __ipv6_prefix_equal64_half(a1 + 1, a2 + 1, prefixlen - 64);
	}
	return __ipv6_prefix_equal64_half(a1, a2, prefixlen);
}
#else
static inline bool ipv6_prefix_equal(const struct in6_addr *addr1,
				     const struct in6_addr *addr2,
				     unsigned int prefixlen)
{
	const __be32 *a1 = addr1->s6_addr32;
	const __be32 *a2 = addr2->s6_addr32;
	unsigned int pdw, pbi;

	/* check complete u32 in prefix */
	pdw = prefixlen >> 5;
	if (pdw && memcmp(a1, a2, pdw << 2))
		return false;

	/* check incomplete u32 in prefix */
	pbi = prefixlen & 0x1f;
	if (pbi && ((a1[pdw] ^ a2[pdw]) & htonl((0xffffffff) << (32 - pbi))))
		return false;

	return true;
}
#endif

static inline bool ipv6_addr_any(const struct in6_addr *a)
{
#if defined(CONFIG_HAVE_EFFICIENT_UNALIGNED_ACCESS) && BITS_PER_LONG == 64
	const unsigned long *ul = (const unsigned long *)a;

	return (ul[0] | ul[1]) == 0UL;
#else
	return (a->s6_addr32[0] | a->s6_addr32[1] |
		a->s6_addr32[2] | a->s6_addr32[3]) == 0;
#endif
}

static inline u32 ipv6_addr_hash(const struct in6_addr *a)
{
#if defined(CONFIG_HAVE_EFFICIENT_UNALIGNED_ACCESS) && BITS_PER_LONG == 64
	const unsigned long *ul = (const unsigned long *)a;
	unsigned long x = ul[0] ^ ul[1];

	return (u32)(x ^ (x >> 32));
#else
	return (__force u32)(a->s6_addr32[0] ^ a->s6_addr32[1] ^
			     a->s6_addr32[2] ^ a->s6_addr32[3]);
#endif
}

/* more secured version of ipv6_addr_hash() */
static inline u32 __ipv6_addr_jhash(const struct in6_addr *a, const u32 initval)
{
	u32 v = (__force u32)a->s6_addr32[0] ^ (__force u32)a->s6_addr32[1];

	return jhash_3words(v,
			    (__force u32)a->s6_addr32[2],
			    (__force u32)a->s6_addr32[3],
			    initval);
}

static inline bool ipv6_addr_loopback(const struct in6_addr *a)
{
#if defined(CONFIG_HAVE_EFFICIENT_UNALIGNED_ACCESS) && BITS_PER_LONG == 64
	const __be64 *be = (const __be64 *)a;

	return (be[0] | (be[1] ^ cpu_to_be64(1))) == 0UL;
#else
	return (a->s6_addr32[0] | a->s6_addr32[1] |
		a->s6_addr32[2] | (a->s6_addr32[3] ^ cpu_to_be32(1))) == 0;
#endif
}

/*
 * Note that we must __force cast these to unsigned long to make sparse happy,
 * since all of the endian-annotated types are fixed size regardless of arch.
 */
static inline bool ipv6_addr_v4mapped(const struct in6_addr *a)
{
	return (
#if defined(CONFIG_HAVE_EFFICIENT_UNALIGNED_ACCESS) && BITS_PER_LONG == 64
		*(unsigned long *)a |
#else
		(__force unsigned long)(a->s6_addr32[0] | a->s6_addr32[1]) |
#endif
		(__force unsigned long)(a->s6_addr32[2] ^
					cpu_to_be32(0x0000ffff))) == 0UL;
}

static inline bool ipv6_addr_v4mapped_loopback(const struct in6_addr *a)
{
	return ipv6_addr_v4mapped(a) && ipv4_is_loopback(a->s6_addr32[3]);
}

static inline u32 ipv6_portaddr_hash(unsigned int mix,
				     const struct in6_addr *addr6,
				     unsigned int port)
{
	unsigned int hash;

	if (ipv6_addr_any(addr6))
		hash = jhash_1word(0, mix);
	else if (ipv6_addr_v4mapped(addr6))
		hash = jhash_1word((__force u32)addr6->s6_addr32[3], mix);
	else
		hash = jhash2((__force u32 *)addr6->s6_addr32, 4, mix);

	return hash ^ port;
}

/*
 * Check for a RFC 4843 ORCHID address
 * (Overlay Routable Cryptographic Hash Identifiers)
 */
static inline bool ipv6_addr_orchid(const struct in6_addr *a)
{
	return (a->s6_addr32[0] & htonl(0xfffffff0)) == htonl(0x20010010);
}

static inline bool ipv6_addr_is_multicast(const struct in6_addr *addr)
{
	return (addr->s6_addr32[0] & htonl(0xFF000000)) == htonl(0xFF000000);
}

static inline void ipv6_addr_set_v4mapped(const __be32 addr,
					  struct in6_addr *v4mapped)
{
	ipv6_addr_set(v4mapped,
			0, 0,
			htonl(0x0000FFFF),
			addr);
}

#if defined(CONFIG_HAVE_EFFICIENT_UNALIGNED_ACCESS) && BITS_PER_LONG == 64
static inline int __ipv6_addr_diff64(const void *token1, const void *token2, int addrlen)
{
	const __be64 *a1 = token1, *a2 = token2;
	int i;

	addrlen >>= 3;

	for (i = 0; i < addrlen; i++) {
		__be64 xb = a1[i] ^ a2[i];
		if (xb)
			return i * 64 + 63 - __fls(be64_to_cpu(xb));
	}

	return addrlen << 6;
}
#endif

/* copy IPv6 saddr & daddr to flow_keys, possibly using 64bit load/store
 * Equivalent to :	flow->v6addrs.src = iph->saddr;
 *			flow->v6addrs.dst = iph->daddr;
 */
static inline void iph_to_flow_copy_v6addrs(struct flow_keys *flow,
					    const struct ipv6hdr *iph)
{
	BUILD_BUG_ON(offsetof(typeof(flow->addrs), v6addrs.dst) !=
		     offsetof(typeof(flow->addrs), v6addrs.src) +
		     sizeof(flow->addrs.v6addrs.src));
	memcpy(&flow->addrs.v6addrs, &iph->saddr, sizeof(flow->addrs.v6addrs));
	flow->control.addr_type = FLOW_DISSECTOR_KEY_IPV6_ADDRS;
}

/*
 *	Header manipulation
 */
static inline void ip6_flow_hdr(struct ipv6hdr *hdr, unsigned int tclass,
				__be32 flowlabel)
{
	*(__be32 *)hdr = htonl(0x60000000 | (tclass << 20)) | flowlabel;
}

static inline __be32 ip6_flowinfo(const struct ipv6hdr *hdr)
{
	return *(__be32 *)hdr & IPV6_FLOWINFO_MASK;
}

static inline __be32 ip6_flowlabel(const struct ipv6hdr *hdr)
{
	return *(__be32 *)hdr & IPV6_FLOWLABEL_MASK;
}

static inline u8 ip6_tclass(__be32 flowinfo)
{
	return ntohl(flowinfo & IPV6_TCLASS_MASK) >> IPV6_TCLASS_SHIFT;
}

static inline __be32 ip6_make_flowinfo(unsigned int tclass, __be32 flowlabel)
{
	return htonl(tclass << IPV6_TCLASS_SHIFT) | flowlabel;
}

#endif /* __FLOWDIS_IPV6_H__ */
