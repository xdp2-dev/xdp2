#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/types.h>
#include <netinet/in.h>

#include "xdp2/dtable.h"

struct next_hop {
	__u8 edest[ETH_ALEN];
	unsigned int port;
};

struct router_metadata {
	__u16 ether_offset;
	__u16 ip_offset;
	in_addr_t ip_src_addr;
};

XDP2_DTABLE_LPM_TABLE_NAME_DECL(
	router_lpm_table,
	struct router_metadata *,
	(
		(ip_src_addr, src)
	), struct next_hop *
)

XDP2_DTABLE_LPM_TABLE_SKEY_DECL(router_lpm_skey_table, in_addr_t *,
				struct next_hop *)

