/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __FLOWDIS_DISSECTOR_H__
#define __FLOWDIS_DISSECTOR_H__

#include <arpa/inet.h>

/* Adapted from kernel include/net/flow_dissector.h and include/linux/skbuff.h
 * with some customizations.
 */

#include "flowdis/local_defs.h"
#include "siphash/siphash.h"

/**
 * struct flow_dissector_key_control:
 * @thoff: Transport header offset
 */
struct flow_dissector_key_control {
	u16	thoff;
	u16	addr_type;
	u32	flags;
};

#define FLOW_DIS_IS_FRAGMENT	BIT(0)
#define FLOW_DIS_FIRST_FRAG	BIT(1)
#define FLOW_DIS_ENCAPSULATION	BIT(2)

enum flow_dissect_ret {
	FLOW_DISSECT_RET_OUT_GOOD,
	FLOW_DISSECT_RET_OUT_BAD,
	FLOW_DISSECT_RET_PROTO_AGAIN,
	FLOW_DISSECT_RET_IPPROTO_AGAIN,
	FLOW_DISSECT_RET_CONTINUE,
};

/**
 * struct flow_dissector_key_basic:
 * @n_proto: Network header protocol (eg. IPv4/IPv6)
 * @ip_proto: Transport header protocol (eg. TCP/UDP)
 */
struct flow_dissector_key_basic {
	__be16	n_proto;
	u8	ip_proto;
	u8	padding;
};

struct flow_dissector_key_tags {
	u32	flow_label;
};

struct flow_dissector_key_vlan {
	union {
		struct {
			u16	vlan_id:12,
				vlan_dei:1,
				vlan_priority:3;
		};
		__be16	vlan_tci;
	};
	__be16	vlan_tpid;
};

struct flow_dissector_key_mpls {
	u32	mpls_ttl:8,
		mpls_bos:1,
		mpls_tc:3,
		mpls_label:20;
};

#define FLOW_DIS_TUN_OPTS_MAX 255
/**
 * struct flow_dissector_key_enc_opts:
 * @data: tunnel option data
 * @len: length of tunnel option data
 * @dst_opt_type: tunnel option type
 */
struct flow_dissector_key_enc_opts {
	u8 data[FLOW_DIS_TUN_OPTS_MAX];	/* Using IP_TUNNEL_OPTS_MAX is desired
					 * here but seems difficult to #include
					 */
	u8 len;
	__be16 dst_opt_type;
};

struct flow_dissector_key_keyid {
	__be32	keyid;
};

/**
 * struct flow_dissector_key_ipv4_addrs:
 * @src: source ip address
 * @dst: destination ip address
 */
struct flow_dissector_key_ipv4_addrs {
	/* (src,dst) must be grouped, in the same way than in IP header */
	__be32 src;
	__be32 dst;
};

/**
 * struct flow_dissector_key_ipv6_addrs:
 * @src: source ip address
 * @dst: destination ip address
 */
struct flow_dissector_key_ipv6_addrs {
	/* (src,dst) must be grouped, in the same way than in IP header */
	struct in6_addr src;
	struct in6_addr dst;
};

/**
 * struct flow_dissector_key_tipc:
 * @key: source node address combined with selector
 */
struct flow_dissector_key_tipc {
	__be32 key;
};

/**
 * struct flow_dissector_key_addrs:
 * @v4addrs: IPv4 addresses
 * @v6addrs: IPv6 addresses
 */
struct flow_dissector_key_addrs {
	union {
		struct flow_dissector_key_ipv4_addrs v4addrs;
		struct flow_dissector_key_ipv6_addrs v6addrs;
		struct flow_dissector_key_tipc tipckey;
	};
};

/**
 * flow_dissector_key_arp:
 *	@ports: Operation, source and target addresses for an ARP header
 *              for Ethernet hardware addresses and IPv4 protocol addresses
 *		sip: Sender IP address
 *		tip: Target IP address
 *		op:  Operation
 *		sha: Sender hardware address
 *		tpa: Target hardware address
 */
struct flow_dissector_key_arp {
	__u32 sip;
	__u32 tip;
	__u8 op;
	unsigned char sha[ETH_ALEN];
	unsigned char tha[ETH_ALEN];
};

/**
 * flow_dissector_key_tp_ports:
 *	@ports: port numbers of Transport header
 *		src: source port number
 *		dst: destination port number
 */
struct flow_dissector_key_ports {
	union {
		__be32 ports;
		struct {
			__be16 src;
			__be16 dst;
		};
	};
};

/**
 * flow_dissector_key_icmp:
 *		type: ICMP type
 *		code: ICMP code
 *		id:   session identifier
 */
struct flow_dissector_key_icmp {
	struct {
		u8 type;
		u8 code;
	};
	u16 id;
};

/**
 * struct flow_dissector_key_eth_addrs:
 * @src: source Ethernet address
 * @dst: destination Ethernet address
 */
struct flow_dissector_key_eth_addrs {
	/* (dst,src) must be grouped, in the same way than in ETH header */
	unsigned char dst[ETH_ALEN];
	unsigned char src[ETH_ALEN];
};

/**
 * struct flow_dissector_key_tcp:
 * @flags: flags
 */
struct flow_dissector_key_tcp {
	__be16 flags;
};

/**
 * struct flow_dissector_key_ip:
 * @tos: tos
 * @ttl: ttl
 */
struct flow_dissector_key_ip {
	__u8	tos;
	__u8	ttl;
};

/**
 * struct flow_dissector_key_meta:
 * @ingress_ifindex: ingress ifindex
 * @ingress_iftype: ingress interface type
 */
struct flow_dissector_key_meta {
	int ingress_ifindex;
	u16 ingress_iftype;
};

/**
 * struct flow_dissector_key_ct:
 * @ct_state: conntrack state after converting with map
 * @ct_mark: conttrack mark
 * @ct_zone: conntrack zone
 * @ct_labels: conntrack labels
 */
struct flow_dissector_key_ct {
	u16	ct_state;
	u16	ct_zone;
	u32	ct_mark;
	u32	ct_labels[4];
};

enum flow_dissector_key_id {
	FLOW_DISSECTOR_KEY_INVALID = 0, /* Invalid key ID */
	FLOW_DISSECTOR_KEY_CONTROL, /* struct flow_dissector_key_control */
	FLOW_DISSECTOR_KEY_BASIC, /* struct flow_dissector_key_basic */
	FLOW_DISSECTOR_KEY_IPV4_ADDRS, /* struct flow_dissector_key_ipv4_addrs */
	FLOW_DISSECTOR_KEY_IPV6_ADDRS, /* struct flow_dissector_key_ipv6_addrs */
	FLOW_DISSECTOR_KEY_PORTS, /* struct flow_dissector_key_ports */
	FLOW_DISSECTOR_KEY_PORTS_RANGE, /* struct flow_dissector_key_ports */
	FLOW_DISSECTOR_KEY_ICMP, /* struct flow_dissector_key_icmp */
	FLOW_DISSECTOR_KEY_ETH_ADDRS, /* struct flow_dissector_key_eth_addrs */
	FLOW_DISSECTOR_KEY_TIPC, /* struct flow_dissector_key_tipc */
	FLOW_DISSECTOR_KEY_ARP, /* struct flow_dissector_key_arp */
	FLOW_DISSECTOR_KEY_VLAN, /* struct flow_dissector_key_vlan */
	FLOW_DISSECTOR_KEY_FLOW_LABEL, /* struct flow_dissector_key_tags */
	FLOW_DISSECTOR_KEY_GRE_KEYID, /* struct flow_dissector_key_keyid */
	FLOW_DISSECTOR_KEY_MPLS_ENTROPY, /* struct flow_dissector_key_keyid */
	FLOW_DISSECTOR_KEY_ENC_KEYID, /* struct flow_dissector_key_keyid */
	FLOW_DISSECTOR_KEY_ENC_IPV4_ADDRS, /* struct flow_dissector_key_ipv4_addrs */
	FLOW_DISSECTOR_KEY_ENC_IPV6_ADDRS, /* struct flow_dissector_key_ipv6_addrs */
	FLOW_DISSECTOR_KEY_ENC_CONTROL, /* struct flow_dissector_key_control */
	FLOW_DISSECTOR_KEY_ENC_PORTS, /* struct flow_dissector_key_ports */
	FLOW_DISSECTOR_KEY_MPLS, /* struct flow_dissector_key_mpls */
	FLOW_DISSECTOR_KEY_TCP, /* struct flow_dissector_key_tcp */
	FLOW_DISSECTOR_KEY_IP, /* struct flow_dissector_key_ip */
	FLOW_DISSECTOR_KEY_CVLAN, /* struct flow_dissector_key_vlan */
	FLOW_DISSECTOR_KEY_ENC_IP, /* struct flow_dissector_key_ip */
	FLOW_DISSECTOR_KEY_ENC_OPTS, /* struct flow_dissector_key_enc_opts */
	FLOW_DISSECTOR_KEY_META, /* struct flow_dissector_key_meta */
	FLOW_DISSECTOR_KEY_CT, /* struct flow_dissector_key_ct */

	FLOW_DISSECTOR_KEY_MAX,
};

#define FLOW_DISSECTOR_F_PARSE_1ST_FRAG		BIT(0)
#define FLOW_DISSECTOR_F_STOP_AT_FLOW_LABEL	BIT(1)
#define FLOW_DISSECTOR_F_STOP_AT_ENCAP		BIT(2)

struct flow_dissector_key {
	enum flow_dissector_key_id key_id;
	size_t offset; /* offset of struct flow_dissector_key_*
			  in target the struct */
};

struct flow_dissector {
	unsigned int used_keys; /* each bit represents presence of one key id */
	unsigned short int offset[FLOW_DISSECTOR_KEY_MAX];
};

struct flow_keys_basic {
	struct flow_dissector_key_control control;
	struct flow_dissector_key_basic basic;
};

struct flow_keys {
	struct flow_dissector_key_control control;
#define FLOW_KEYS_HASH_START_FIELD basic
	struct flow_dissector_key_basic basic __aligned(SIPHASH_ALIGNMENT);
	struct flow_dissector_key_tags tags;
	struct flow_dissector_key_vlan vlan;
	struct flow_dissector_key_vlan cvlan;
	struct flow_dissector_key_keyid keyid;
	struct flow_dissector_key_ports ports;
	struct flow_dissector_key_icmp icmp;
	/* 'addrs' must be the last member */
	struct flow_dissector_key_addrs addrs;
};

#define FLOW_KEYS_HASH_OFFSET		\
	offsetof(struct flow_keys, FLOW_KEYS_HASH_START_FIELD)

__be32 flow_get_u32_src(const struct flow_keys *flow);
__be32 flow_get_u32_dst(const struct flow_keys *flow);

extern struct flow_dissector flow_keys_dissector;
extern struct flow_dissector flow_keys_basic_dissector;
extern struct flow_dissector flow_keys_dissector_symmetric;

/* struct flow_keys_digest:
 *
 * This structure is used to hold a digest of the full flow keys. This is a
 * larger "hash" of a flow to allow definitively matching specific flows where
 * the 32 bit skb->hash is not large enough. The size is limited to 16 bytes so
 * that it can be used in CB of skb (see sch_choke for an example).
 */
#define FLOW_KEYS_DIGEST_LEN	16
struct flow_keys_digest {
	u8	data[FLOW_KEYS_DIGEST_LEN];
};

/* Make flow keys digest that include protocol, IP protocol, ports
 * and IPv4 source and destination address.
 */
void make_flow_keys_digest(struct flow_keys_digest *digest,
			   const struct flow_keys *flow);

/* Check if flow keys have Layer four information */
static inline bool flow_keys_have_l4(const struct flow_keys *keys)
{
	return (keys->ports.ports || keys->tags.flow_label);
}

/* Return a flow hash from flow_keys input */
u32 flow_hash_from_keys(struct flow_keys *keys);

/* Check if flow dissector uses a key */
static inline bool dissector_uses_key(const struct flow_dissector *flow_dissector,
				      enum flow_dissector_key_id key_id)
{
	return flow_dissector->used_keys & (1 << key_id);
}

/* Extract pointer to some flow dissector key ID */
static inline void *skb_flow_dissector_target(struct flow_dissector *flow_dissector,
					      enum flow_dissector_key_id key_id,
					      void *target_container)
{
	return ((char *)target_container) + flow_dissector->offset[key_id];
}

/* Basic flow dissector structure */
struct bpf_flow_dissector {
	struct bpf_flow_keys	*flow_keys;
	const struct sk_buff	*skb;
	void			*data;
	void			*data_end;
};

/* Initialize keys */
static inline void
flow_dissector_init_keys(struct flow_dissector_key_control *key_control,
			 struct flow_dissector_key_basic *key_basic)
{
	memset(key_control, 0, sizeof(*key_control));
	memset(key_basic, 0, sizeof(*key_basic));
}

/* Initialize a flow dissector */
void skb_flow_dissector_init(struct flow_dissector *flow_dissector,
			     const struct flow_dissector_key *key,
			     unsigned int key_count);

/* Extract the upper layer ports and return nbo port pair */
__be32 __skb_flow_get_ports(const struct sk_buff *skb, int thoff, u8 ip_proto,
			    void *data, int hlen);

/* Dissect a flow and return metadata */
void skb_flow_dissect_meta(const struct sk_buff *skb,
			   struct flow_dissector *flow_dissector,
			   void *target_container);

bool __skb_flow_dissect_err(const struct sk_buff *skb,
			    struct flow_dissector *flow_dissector,
			    void *target_container, void *data,
			    __be16 proto, int nhoff, int hlen,
			    unsigned int flags, const char **errmsg);

static inline bool __skb_flow_dissect(const struct sk_buff *skb,
			struct flow_dissector *fd,
			void *tgt, void *data,
			__be16 proto, int nhoff, int hlen,
			unsigned int flags)
{
	const char *err;

	return(__skb_flow_dissect_err(skb, fd, tgt, data, proto, nhoff, hlen,
				      flags, &err));
}

static inline bool skb_flow_dissect(const struct sk_buff *skb,
				    struct flow_dissector *flow_dissector,
				    void *target_container, unsigned int flags)
{
        return __skb_flow_dissect(skb, flow_dissector, target_container,
				  NULL, 0, 0, 0, flags);
}

static inline bool skb_flow_dissect_flow_keys(const struct sk_buff *skb,
					      struct flow_keys *flow,
					      unsigned int flags)
{
	memset(flow, 0, sizeof(*flow));
	return __skb_flow_dissect(skb, &flow_keys_dissector,
				  flow, NULL, 0, 0, 0, flags);
}

static inline bool
skb_flow_dissect_flow_keys_basic(const struct sk_buff *skb,
				 struct flow_keys_basic *flow, void *data,
				 __be16 proto, int nhoff, int hlen,
				 unsigned int flags)
{
	memset(flow, 0, sizeof(*flow));
	return __skb_flow_dissect(skb, &flow_keys_basic_dissector, flow,
				  data, proto, nhoff, hlen, flags);
}
/* Get symmetric flow hash (source packet and destination packet for the
 * same flow produce the same hash).
 */
u32 __skb_get_hash_symmetric(const struct sk_buff *skb);

/* Get a header hash with perturbing key for different uses */
u32 skb_get_hash_perturb(const struct sk_buff *skb,
			 const siphash_key_t *perturb);

/* Get the offset to encapsulated protocol from already dissected packet */
u32 __skb_get_poff(const struct sk_buff *skb, void *data,
		   const struct flow_keys_basic *keys, int hlen);

/* Get the offset to encapsulated protocol from a packet */
u32 skb_get_poff(const struct sk_buff *skb);

/* Initialize default flow dissectors, essentially the init function for
 * flow_dissector.
 */
int init_default_flow_dissectors(void);

static inline __be32 ip_to_eth_proto(void *data, size_t hlen)
{
	struct iphdr *iph = data;
	__be16 proto;

	if (hlen < 1)
		return 0;

	switch (iph->version) {
	case 4:
		proto = ntohs(ETH_P_IP);
		break;
	case 6:
		proto = ntohs(ETH_P_IPV6);
		break;
	default:
		return 0;
	}

	return proto;
}

/* flowdis interface */

static inline bool __flowdis_dissect(void *data,
				     struct flow_dissector *flow_dissector,
				     void *target_container, __be16 proto,
				     unsigned int hlen, unsigned int nhoff,
				     unsigned int flags)
{
	return __skb_flow_dissect(NULL, flow_dissector, target_container,
				  data, proto, nhoff, hlen, flags);
}

static inline bool __flowdis_dissect_ether(void *data,
					  struct flow_dissector *flow_dissector,
					  void *target_container,
					  unsigned int hlen, unsigned int flags)
{
	struct ethhdr *eh = data;

	return __skb_flow_dissect(NULL, flow_dissector, target_container,
				  data, eh->h_proto, sizeof(*eh), hlen, flags);
}

static inline bool __flowdis_dissect_l3(void *data,
					struct flow_dissector *flow_dissector,
					void *target_container, __be16 proto,
					unsigned int hlen, unsigned int flags)
{
	return __skb_flow_dissect(NULL, flow_dissector, target_container,
				  data, proto, 0, hlen, flags);
}

static inline bool __flowdis_dissect_ip(void *data,
					struct flow_dissector *flow_dissector,
					void *target_container,
					unsigned int hlen,
					unsigned int flags)
{
	__be16 proto = ip_to_eth_proto(data, hlen);

	if (proto)
		return __skb_flow_dissect(NULL, flow_dissector,
					  target_container, data, proto,
					  0, hlen, flags);
	return false;
}

static inline u32 __flowdis_get_hash(void *data,
				     struct flow_dissector *flow_dissector,
				     __be16 proto, unsigned int hlen,
				     unsigned int nhoff, unsigned int flags)
{
	struct flow_keys keys;

	memset(&keys, 0, sizeof(keys));

	if (__flowdis_dissect(data, flow_dissector, &keys, proto, hlen,
			      nhoff, flags))
		return flow_hash_from_keys(&keys);

	return 0;
}

static inline u32 __flowdis_get_hash_ether(void *data,
					   struct flow_dissector
							*flow_dissector,
					   unsigned int hlen,
					   unsigned int flags)
{
	struct ethhdr *eh = data;

	return __flowdis_get_hash(data, flow_dissector, eh->h_proto, hlen,
				  sizeof(*eh), flags);
}

static inline u32 __flowdis_get_hash_l3(void *data,
					struct flow_dissector *flow_dissector,
					__be16 proto, unsigned int hlen,
					unsigned int flags)
{
	return __flowdis_get_hash(data, flow_dissector, proto, hlen, 0, flags);
}

static inline u32 __flowdis_get_hash_ip(void *data,
					struct flow_dissector *flow_dissector,
					unsigned int hlen, unsigned int flags)
{
	struct flow_keys keys;

	memset(&keys, 0, sizeof(keys));

	if (__flowdis_dissect_ip(data, flow_dissector, &keys, hlen, flags))
		return flow_hash_from_keys(&keys);

	return 0;
}

static inline bool flowdis_dissect(void *data,
				   struct flow_keys *keys, __be16 proto,
				   unsigned int hlen, unsigned int nhoff)
{
	return __flowdis_dissect(data, &flow_keys_dissector, keys, proto,
				 hlen, nhoff, 0);
}

static inline bool flowdis_dissect_ether(void *data, struct flow_keys *keys,
					 unsigned int hlen)
{
	return __flowdis_dissect_ether(data, &flow_keys_dissector, keys,
				       hlen, 0);
}

static inline bool flowdis_dissect_l3(void *data, struct flow_keys *keys,
				      __be16 proto, unsigned int hlen)
{
	return __flowdis_dissect_l3(data, &flow_keys_dissector, keys,
				    proto, hlen, 0);
}

static inline bool flowdis_dissect_ip(void *data, struct flow_keys *keys,
				      unsigned int hlen)
{
	return __flowdis_dissect_ip(data, &flow_keys_dissector, keys, hlen, 0);
}

static inline u32 flowdis_get_hash(void *data, __be16 proto, unsigned int hlen,
				   unsigned int nhoff)
{
	return __flowdis_get_hash(data, &flow_keys_dissector_symmetric,
				  proto, hlen, nhoff,
				  FLOW_DISSECTOR_F_STOP_AT_FLOW_LABEL);
}

static inline u32 flowdis_get_hash_ether(void *data, unsigned int hlen)
{
	return __flowdis_get_hash_ether(data, &flow_keys_dissector_symmetric,
					hlen,
					FLOW_DISSECTOR_F_STOP_AT_FLOW_LABEL);
}

static inline u32 flowdis_get_hash_l3(void *data, __be16 proto,
				       unsigned int hlen)
{
	return __flowdis_get_hash_l3(data, &flow_keys_dissector_symmetric,
				     proto, hlen,
				     FLOW_DISSECTOR_F_STOP_AT_FLOW_LABEL);
}

static inline u32 flowdis_get_hash_ip(void *data, unsigned int hlen)
{
	return __flowdis_get_hash_ip(data, &flow_keys_dissector_symmetric,
				     hlen, FLOW_DISSECTOR_F_STOP_AT_FLOW_LABEL);
}

static inline int flowdis_init(void)
{
	return init_default_flow_dissectors();
}

void flowdis_hash_secret_init(siphash_key_t *init_key);

void flowdis_print_metadata(const struct flow_keys *flow);
void flowdis_print_hash_input(const struct flow_keys *flow);

#endif /* __FLOWDIS_DISSECTOR_H__ */
