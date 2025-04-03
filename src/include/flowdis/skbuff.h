#ifndef __FLOWDIS_SK_BUFF_H__
#define __FLOWDIS_SK_BUFF_H__

/* Tiny skbuff implementation, just enough for flow dissector APIs. */

#include "flow_dissector.h"

struct sk_buff {
	void *data;
	size_t headlen;
	size_t len;
	__be16 protocol;
	__u16 network_offset;

	__be16 vlan_proto;
	__u16 vlan_tci;
	__u8 vlan_present:1;
};

static inline void *
__skb_header_pointer(const struct sk_buff *skb, int offset,
                     int len, void *data, int hlen, void *buffer)
{
	if (hlen - offset >= len)
		return data + offset;

	return data + offset;
}

static inline int skb_network_offset(const struct sk_buff *skb)
{
	return skb->network_offset;
}

static inline unsigned int skb_headlen(const struct sk_buff *skb)
{
	return skb->headlen;
}

#ifndef NET_IP_ALIGN
#define NET_IP_ALIGN	2
#endif

#endif /* __FLOWDIS_SK_BUFF_H__ */
