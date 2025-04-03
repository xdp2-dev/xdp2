/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __FLOWDIS_IF_TUNNEL_H__
#define __FLOWDIS_IF_TUNNEL_H__

/* Took some constant definitions from uapi/linux/if_tunnel.h */

#define GRE_CSUM	__cpu_to_be16(0x8000)
#define GRE_ROUTING	__cpu_to_be16(0x4000)
#define GRE_KEY		__cpu_to_be16(0x2000)
#define GRE_SEQ		__cpu_to_be16(0x1000)
#define GRE_STRICT	__cpu_to_be16(0x0800)
#define GRE_REC		__cpu_to_be16(0x0700)
#define GRE_ACK		__cpu_to_be16(0x0080)
#define GRE_FLAGS	__cpu_to_be16(0x0078)
#define GRE_VERSION	__cpu_to_be16(0x0007)

#define TUNNEL_CSUM		__cpu_to_be16(0x01)
#define TUNNEL_ROUTING		__cpu_to_be16(0x02)
#define TUNNEL_KEY		__cpu_to_be16(0x04)
#define TUNNEL_SEQ		__cpu_to_be16(0x08)
#define TUNNEL_STRICT		__cpu_to_be16(0x10)
#define TUNNEL_REC		__cpu_to_be16(0x20)
#define TUNNEL_VERSION		__cpu_to_be16(0x40)
#define TUNNEL_NO_KEY		__cpu_to_be16(0x80)
#define TUNNEL_DONT_FRAGMENT    __cpu_to_be16(0x0100)
#define TUNNEL_OAM		__cpu_to_be16(0x0200)
#define TUNNEL_CRIT_OPT		__cpu_to_be16(0x0400)
#define TUNNEL_GENEVE_OPT	__cpu_to_be16(0x0800)
#define TUNNEL_VXLAN_OPT	__cpu_to_be16(0x1000)
#define TUNNEL_NOCACHE		__cpu_to_be16(0x2000)
#define TUNNEL_ERSPAN_OPT	__cpu_to_be16(0x4000)

#define TUNNEL_OPTIONS_PRESENT \
		(TUNNEL_GENEVE_OPT | TUNNEL_VXLAN_OPT | TUNNEL_ERSPAN_OPT)

#define GRE_VERSION_0		__cpu_to_be16(0x0000)
#define GRE_VERSION_1		__cpu_to_be16(0x0001)
#define GRE_PROTO_PPP		__cpu_to_be16(0x880b)
#define GRE_PPTP_KEY_MASK	__cpu_to_be32(0xffff)

#endif /* __FLOWDIS_IF_TUNNEL_H__ */
