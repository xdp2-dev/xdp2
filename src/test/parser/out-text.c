// SPDX-License-Identifier: BSD-2-Clause-FreeBSD
/*
 * Copyright (c) 2020, 2021 by Mojatatu Networks.
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

/* Output method that generates text for humans to read. */

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "omethod.h"
#include "xdp2/proto_defs.h"
#include "xdp2/utility.h"

extern const char *__progname;

struct out_text_priv {
	int showpacket;
	int showall;
	int errorpkt;
	const unsigned char *data;
	size_t len;
};

static void dump_k_control(struct out_text_priv *p, const char *tag,
			   const struct test_parser_out_control *k)
{
	if (!k->thoff && !k->addr_type && !k->flags) {
		if (p->showall)
			printf("%s: not set\n", tag);
	} else {
		printf("%s: thoff=%u addr_type=", tag, k->thoff);
		switch (k->addr_type) {
		case ADDR_TYPE_OTHER:
			printf("OTHER");
			break;
		case ADDR_TYPE_IPv4:
			printf("IPv4");
			break;
		case ADDR_TYPE_IPv6:
			printf("IPv6");
			break;
		case ADDR_TYPE_TIPC:
			printf("TIPC");
			break;
		default:
			printf("%d(?)", (int)k->addr_type);
			break;
		}
		printf(" flags=%#x\n", k->flags);
	}
}

static void dump_k_basic(struct out_text_priv *p, const char *tag,
			 const struct test_parser_out_basic *k)
{
	if (!k->n_proto && !k->ip_proto) {
		if (p->showall)
			printf("%s: not set\n", tag);
	} else {
		printf("%s: n_proto=%04x ip_proto=%d\n", tag,
		       (unsigned int)ntohs(k->n_proto), k->ip_proto);
	}
}

static void dump_k_ipv4_addrs(struct out_text_priv *p, const char *tag,
			      const struct test_parser_out_ipv4_addrs *k)
{
	char str[INET_ADDRSTRLEN];

	if (!k->src && !k->dst) {
		if (p->showall)
			printf("%s: not set\n", tag);
	} else {
		if (!inet_ntop(AF_INET, &k->src, str, INET_ADDRSTRLEN)) {
			perror("inet_ntop");
			exit(-1);
		}
		printf("%s: src=%s ", tag, str);

		if (!inet_ntop(AF_INET, &k->dst, str, INET_ADDRSTRLEN)) {
			perror("inet_ntop");
			exit(-1);
		}
		printf("dst=%s\n", str);
	}
}

static void dump_k_ipv6_addrs(struct out_text_priv *p, const char *tag,
			      const struct test_parser_out_ipv6_addrs *k)
{
	char str[INET6_ADDRSTRLEN];
	int i;

	for (i = 16 - 1; i >= 0; i--) {
		if (k->src[i] || k->dst[i])
			break;
	}
	if (i < 0) {
		if (p->showall)
			printf("%s: not set\n", tag);
		return;
	}

	if (!inet_ntop(AF_INET6, k->src, str, INET6_ADDRSTRLEN)) {
		perror("inet_ntop");
		exit(-1);
	}
	printf("%s: src=%s ", tag, str);

	if (!inet_ntop(AF_INET6, k->dst, str, INET6_ADDRSTRLEN)) {
		perror("inet_ntop");
		exit(-1);
	}
	printf("dst=%s\n", str);
}

static void dump_k_ports(struct out_text_priv *p, const char *tag,
			 const struct test_parser_out_ports *k)
{
	if (!k->src && !k->dst) {
		if (p->showall)
			printf("%s: not set\n", tag);
	} else {
		printf("%s: src=%u dst=%u\n", tag, ntohs(k->src),
		       ntohs(k->dst));
	}
}

static void dump_k_icmp(struct out_text_priv *p, const char *tag,
			const struct test_parser_out_icmp *k)
{
	if (!k->type && !k->code && !k->id) {
		if (p->showall)
			printf("%s: not set\n", tag);
	} else {
		printf("%s: type=%u code=%u id=%04x\n", tag, k->type, k->code,
		       k->id);
	}
}

static void dump_k_eth_addrs(struct out_text_priv *p, const char *tag,
			     const struct test_parser_out_eth_addrs *k)
{
	int i;

	for (i = ARRAY_SIZE(k->dst) - 1; i >= 0; i--)
		if (k->dst[i])
			break;
	if (i < 0)
		for (i = ARRAY_SIZE(k->src) - 1; i >= 0; i--)
			if (k->src[i])
				break;
	if (i < 0) {
		if (p->showall)
			printf("%s: not set\n", tag);
	} else {
		printf("%s: dst=", tag);
		for (i = 0; i < ARRAY_SIZE(k->dst); i++)
			printf("%s%02x", i ? ":" : "", k->dst[i]);
		printf(" src=");
		for (i = 0; i < ARRAY_SIZE(k->src); i++)
			printf("%s%02x", i ? ":" : "", k->src[i]);
		printf("\n");
	}
}

static void dump_k_tipc(struct out_text_priv *p, const char *tag,
			const struct test_parser_out_tipc *k)
{
	if (!k->key) {
		if (p->showall)
			printf("%s: not set\n", tag);
	} else {
		printf("%s: key=%08x\n", tag, (unsigned int)k->key);
	}
}

static void dump_k_arp(struct out_text_priv *p, const char *tag,
		       const struct test_parser_out_arp *k)
{
	int i;

	for (i = ARRAY_SIZE(k->s_hw) - 1; i >= 0; i--)
		if (k->s_hw[i])
			break;
	if (i < 0)
		for (i = ARRAY_SIZE(k->t_hw) - 1; i >= 0; i--)
			if (k->t_hw[i])
				break;
	if ((i < 0) && !k->s_ip && !k->t_ip && !k->op) {
		if (p->showall)
			printf("%s: not set\n", tag);
	} else {
		char str[INET_ADDRSTRLEN];

		/* This format assumes IPv4 and Ethernet! */

		if (!inet_ntop(AF_INET, &k->s_ip, str, INET_ADDRSTRLEN)) {
			perror("inet_ntop");
			exit(-1);
		}
		printf("%s: src=%s op=%u s_hw=", tag, str, k->op);

		if (!inet_ntop(AF_INET, &k->t_ip, str, INET_ADDRSTRLEN)) {
			perror("inet_ntop");
			exit(-1);
		}
		printf("dst=%s\n", str);

		for (i = 0; i < ARRAY_SIZE(k->s_hw); i++)
			printf("%s%02x", i ? ":" : "", k->s_hw[i]);
		printf(" t_hw=");
		for (i = 0; i < ARRAY_SIZE(k->t_hw); i++)
			printf("%s%02x", i ? ":" : "", k->s_hw[i]);
		printf("\n");
	}
}

static void dump_k_gre(struct out_text_priv *p, const char *tag,
		       const struct test_parser_out_gre *k)
{
	bool prev = false;

	if (!(k->flags))
		return;

	printf("%s: ", tag);

	if (k->flags & GRE_KEY) {
		printf("%skeyid: %08x", prev ? ", " : "",
		       ntohl(k->keyid));
		prev = true;
	}
	if (k->flags & GRE_CSUM) {
		printf("%scsum: %04x", prev ? ", " : "", ntohs(k->csum));
		prev = true;
	}
	if (k->flags & GRE_SEQ) {
		printf("%sseq: %u", prev ? ", " : "", ntohl(k->seq));
		prev = true;
	}
	if (k->flags & GRE_ROUTING) {
		printf("%srouting: %08x", prev ? ", " : "", ntohl(k->routing));
		prev = true;
	}

	printf("\n");
}

static void dump_k_gre_pptp(struct out_text_priv *p, const char *tag,
		       const struct test_parser_out_gre_pptp *k)
{
	bool prev = false;

	if (!(k->flags))
		return;

	printf("%s: ", tag);

	if (k->flags & GRE_KEY) {
		unsigned int base_len = gre_v1_len_from_flags(k->flags);

		/* Add the GRE header length for reporting the length since
		 * that is what tcpdump does
		 */
		printf("%scallid: %u, length %u", prev ? ", " : "",
		       ntohs(k->callid), ntohs(k->length) + base_len);
		prev = true;
	}
	if (k->flags & GRE_SEQ) {
		printf("%sseq: %u", prev ? ", " : "", ntohl(k->seq));
		prev = true;
	}
	if (k->flags & GRE_ACK) {
		printf("%sack: %u", prev ? ", " : "", ntohl(k->ack));
		prev = true;
	}

	printf("\n");
}

static void dump_k_vlan(struct out_text_priv *p, const char *tag,
			const struct test_parser_out_vlan *k)
{
	if (!k->vlan_id && !k->vlan_dei && !k->vlan_priority && !k->vlan_tpid) {
		if (p->showall)
			printf("%s: not set\n", tag);
	} else {
		printf("%s: id=%d dei=%d pri=%d tpid=%d\n", tag, k->vlan_id,
		       k->vlan_dei, k->vlan_priority, k->vlan_tpid);
	}
}

static void dump_k_tags(struct out_text_priv *p, const char *tag,
			const struct test_parser_out_tags *k)
{
	if (!k->flow_label) {
		if (p->showall)
			printf("%s: not set\n", tag);
	} else {
		printf("%s: flow_label=%08x\n", tag,
		       (unsigned int)k->flow_label);
	}
}

static void dump_k_keyid(struct out_text_priv *p, const char *tag,
			 const struct test_parser_out_keyid *k)
{
	if (!k->keyid) {
		if (p->showall)
			printf("%s: not set\n", tag);
	} else {
		printf("%s: keyid=%08x\n", tag, k->keyid);
	}
}

static void dump_k_mpls(struct out_text_priv *p, const char *tag,
			const struct test_parser_out_mpls *k)
{
	if (!k->mpls_ttl && !k->mpls_bos && !k->mpls_tc && !k->mpls_label) {
		if (p->showall)
			printf("%s: not set\n", tag);
	} else {
		printf("%s: ttl=%u bos=%u tc=%u label=%05x\n", tag, k->mpls_ttl,
		       k->mpls_bos, k->mpls_tc, k->mpls_label);
	}
}

// XXX byte sex - see file header comment
static void dump_k_tcp(struct out_text_priv *p, const char *tag,
		       const struct test_parser_out_tcp *k)
{
	static struct {
		const char *name;
		unsigned short int bit;
	} flagtext[] = {
		{ "FIN", 0x0001 },
		{ "SYN", 0x0002 },
		{ "RST", 0x0004 },
		{ "PSH", 0x0008 },
		{ "ACK", 0x0010 },
		{ "URG", 0x0020 },
		{ "ECE", 0x0040 },
		{ "CWR", 0x0080 },
		{ 0 }
	};
	unsigned short int f;
	const char *sep;
	int i;

	if (!k->flags) {
		if (p->showall)
			printf("%s: not set\n", tag);
	} else {
		f = ntohs(k->flags);
		printf("%s: flags=%04x <", tag, f);
		sep = "";
		for (i = 0; flagtext[i].name; i++) {
			if (f & flagtext[i].bit) {
				f &= ~flagtext[i].bit;
				printf("%s%s", sep, flagtext[i].name);
				sep = ",";
			}
		}
		printf(">\n");
	}
}

static void dump_k_ip(struct out_text_priv *p, const char *tag,
		      const struct test_parser_out_ip *k)
{
	if (!k->tos && !k->ttl) {
		if (p->showall)
			printf("%s: not set\n", tag);
	} else {
		printf("%s: tos=%02x ttl=%u\n", tag, k->tos, k->ttl);
	}
}

static void dump_k_enc_opts(struct out_text_priv *p, const char *tag,
			    const struct test_parser_out_enc_opts *k)
{
	int i;
	const char *pref;

	for (i = ARRAY_SIZE(k->data) - 1; i >= 0; i--)
		if (k->data[i])
			break;
	if ((i < 0) && !k->len && !k->dst_opt_type) {
		if (p->showall)
			printf("%s: not set\n", tag);
	} else {
		printf("%s: data=<", tag);
		pref = "";
		for (i = 0; i < ARRAY_SIZE(k->data); i++) {
			printf("%s%02x", pref, k->data[i]);
			pref = ".";
		}
		printf("> len=%u dst_opt_type=%u\n", k->len, k->dst_opt_type);
	}
}

static void dump_k_meta(struct out_text_priv *p, const char *tag,
			const struct test_parser_out_meta *k)
{
	if (!k->ingress_ifindex && !k->ingress_iftype) {
		if (p->showall)
			printf("%s: not set\n", tag);
	} else {
		printf("%s: ifindex=%u iftype=%u\n", tag, k->ingress_ifindex,
		       k->ingress_iftype);
	}
}

static void dump_k_ct(struct out_text_priv *p, const char *tag,
		      const struct test_parser_out_ct *k)
{
	int i;

	for (i = ARRAY_SIZE(k->ct_labels) - 1; i >= 0; i--)
		if (k->ct_labels[i])
			break;
	if ((i < 0) && !k->ct_state && !k->ct_zone && !k->ct_mark) {
		if (p->showall)
			printf("%s: not set\n", tag);
	} else {
		printf("%s: state=%u zone=%u mark=%u labels=<%u,%u,%u,%u>\n",
		       tag, k->ct_state, k->ct_zone, k->ct_mark,
		       k->ct_labels[0], k->ct_labels[1], k->ct_labels[2],
		       k->ct_labels[3]);
	}
}

static void dump_k_tcp_opt(struct out_text_priv *p, const char *tag,
			   const struct test_parser_out_tcp_opt *k)
{
	int i;

	if (k->mss || k->ws || k->ts_val || k->ts_echo || k->sack[0].l ||
	    k->sack[0].r) {
		printf("%s:", tag);
		if (k->mss)
			printf(" mss=%u", (unsigned int)k->mss);
		if (k->ws)
			printf(" ws=%u", (unsigned int)k->ws);
		if (k->ts_val || k->ts_echo)
			printf(" ts=<%u,%u>", (unsigned int)k->ts_val,
			       (unsigned int)k->ts_echo);
		for (i = 0; i < ARRAY_SIZE(k->sack); i++) {
			if (k->sack[i].l || k->sack[i].r) {
				printf(" sack[%d]=(%u,%u)", i, k->sack[i].l,
				       k->sack[i].r);
			} else {
				break;
			}
		}
		printf("\n");
	}
}

static void dump_k_hash(struct out_text_priv *p, const char *tag,
			const struct test_parser_out_hash *k)
{
	if (k->hash)
		printf("%s: hash=%08llx\n", tag, k->hash);
}

static void out_text_help(void)
{
	fprintf(stderr,
		"For `text' output, ARGS contains comma-separated flags.  These flags\n"
		"can include:\n"
		"p       Print raw packet data in hex before processing\n"
		"a       Print all fields, not just fields that have data\n"
		"e       Print raw packet data if the dissector fails\n"
		"Thus, for example, `-i text,p,a' prints packet data _and_ all fields.\n");
}

static void *out_text_init(const char *args)
{
	struct out_text_priv *p;
	const char *ap, *comma;
	int fl, errs;

	p = calloc(1, sizeof(struct out_text_priv));
	if (!p) {
		fprintf(stderr, "out-text init failed\n");
		exit(-1);
	}

	p->showpacket = 0;
	p->showall = 0;
	p->errorpkt = 0;
	if (args) {
		errs = 0;
		ap = args;
		while (1) {
			comma = index(ap, ',');
			fl = comma ? comma - ap : strlen(ap);
			if ((fl == 1) && (ap[0] == 'p')) {
				p->showpacket = 1;
			} else if ((fl == 1) && (ap[0] == 'a')) {
				p->showall = 1;
			} else if ((fl == 1) && (ap[0] == 'e')) {
				p->errorpkt = 1;
			} else {
				fprintf(stderr,
					"%s: unrecognized text output "
					"flag `%.*s'\n", __progname, fl, ap);
				errs = 1;
			}
			if (!comma)
				break;
			ap = comma + 1;
		}
		if (errs)
			exit(-1);
	}
	return (p);
}

static void dump_packet_data(const unsigned char *data, size_t len)
{
	int i;

	for (i = 0; i < len; i++) {
		switch (i & 0xf) {
		case 0:
			printf(" %04x  ", i);
			break;
		case 8:
			printf(" ");
			break;
		}
		printf(" %02x", data[i]);
		switch (i & 0xf) {
		case 15:
			printf("\n");
			break;
		}
	}
	if (len & 15)
		printf("\n");
}

static void out_text_pre(void *pv, const unsigned char *data, size_t len,
		     unsigned int ser)
{
	struct out_text_priv *p = pv;

	printf("-------- Packet #%u: length %lu\n", ser, len);
	if (p->showpacket)
		dump_packet_data(data, len);
	p->data = data;
	p->len = len;
}

static void out_text_post(void *pv, const char *status,
			  const struct test_parser_out *out)
{
	struct out_text_priv *p = pv;

	if (status) {
		printf("dissector core failed [%s]\n", status);
		if (p->errorpkt)
			dump_packet_data(p->data, p->len);
		return;
	}
	dump_k_control(p, "control", &out->k_control);
	dump_k_basic(p, "basic", &out->k_basic);
	dump_k_ipv4_addrs(p, "ipv4_addrs", &out->k_ipv4_addrs);
	dump_k_ipv6_addrs(p, "ipv6_addrs", &out->k_ipv6_addrs);
	dump_k_gre(p, "gre", &out->k_gre);
	dump_k_gre_pptp(p, "gre_pptp", &out->k_gre_pptp);
	dump_k_ports(p, "ports", &out->k_ports);
	dump_k_ports(p, "ports_range", &out->k_ports_range);
	dump_k_icmp(p, "icmp", &out->k_icmp);
	dump_k_eth_addrs(p, "eth_addrs", &out->k_eth_addrs);
	dump_k_tipc(p, "tipc", &out->k_tipc);
	dump_k_arp(p, "arp", &out->k_arp);
	dump_k_tcp_opt(p, "tcp_opt", &out->k_tcp_opt);
	dump_k_vlan(p, "vlan", &out->k_vlan);
	dump_k_tags(p, "flow_label", &out->k_flow_label);
	dump_k_keyid(p, "gre_keyid", &out->k_gre_keyid);
	dump_k_keyid(p, "mpls_entropy", &out->k_mpls_entropy);
	dump_k_keyid(p, "enc_keyid", &out->k_enc_keyid);
	dump_k_ipv4_addrs(p, "enc_ipv4_addrs", &out->k_enc_ipv4_addrs);
	dump_k_ipv6_addrs(p, "enc_ipv6_addrs", &out->k_enc_ipv6_addrs);
	dump_k_control(p, "enc_control", &out->k_enc_control);
	dump_k_ports(p, "enc_ports", &out->k_enc_ports);
	dump_k_mpls(p, "mpls", &out->k_mpls);
	dump_k_tcp(p, "tcp", &out->k_tcp);
	dump_k_ip(p, "ip", &out->k_ip);
	dump_k_vlan(p, "cvlan", &out->k_cvlan);
	dump_k_ip(p, "enc_ip", &out->k_enc_ip);
	dump_k_enc_opts(p, "enc_opts", &out->k_enc_opts);
	dump_k_meta(p, "meta", &out->k_meta);
	dump_k_ct(p, "ct", &out->k_ct);
	dump_k_hash(p, "hash", &out->k_hash);
}

static void out_text_done(void *pv)
{
	free(pv);
}

OMETHOD_DECL(text)
