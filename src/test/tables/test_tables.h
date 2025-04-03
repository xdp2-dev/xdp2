void run_stables_plain(void);
void run_stables_tern(void);
void run_stables_lpm(void);

void run_dtables_plain(void);
void run_dtables_tern(void);
void run_dtables_lpm(void);

struct my_ctx {
	char *name;
	int status;
};

extern int verbose;

enum {
	NO_STATUS,
	HIT_FORWARD,
	HIT_DROP,
	HIT_NOACTION,
	MISS,
};

extern int get_base_eth_status(int w);
extern int get_base_ipv4_status(int w);

extern int get_base_eth_tern_status(int w);
extern int get_base_ipv4_tern_status(int w);

extern int get_base_eth_lpm_status(int w);
extern int get_base_ipv4_lpm_status(int w);

static inline void forward(struct my_ctx *ctx, int v)
{
	if (verbose >= 10)
		printf("Forward %s code: %u\n", ctx->name, v);
	ctx->status = HIT_FORWARD;
}

static inline void drop(struct my_ctx *ctx)
{
	if (verbose >= 10)
		printf("Drop %s\n", ctx->name);
	ctx->status = HIT_DROP;
}

static inline void NoAction(struct my_ctx *ctx)
{
	if (verbose >= 10)
		printf("NoAction %s\n", ctx->name);
	ctx->status = HIT_NOACTION;
}

static inline void Miss(struct my_ctx *ctx)
{
	if (verbose >= 10)
		printf("Miss %s\n", ctx->name);
	ctx->status = MISS;
}

static inline void forward_dtable(void *_ctx, void *entry_arg)
{
	struct my_ctx *ctx = _ctx;

	if (verbose >= 10)
		printf("Forward %s\n", ctx->name);
	ctx->status = HIT_FORWARD;
}

static inline void drop_dtable(void *_ctx, void *entry_arg)
{
	struct my_ctx *ctx = _ctx;

	if (verbose >= 10)
		printf("Drop %s\n", ctx->name);
	ctx->status = HIT_DROP;
}

static inline void NoAction_dtable(void *_ctx, void *entry_arg)
{
	struct my_ctx *ctx = _ctx;

	if (verbose >= 10)
		printf("NoAction %s\n", ctx->name);
	ctx->status = HIT_NOACTION;
}

static inline void Miss_dtable(void *_ctx, void *entry_arg)
{
	struct my_ctx *ctx = _ctx;

	if (verbose >= 10)
		printf("Miss %s\n", ctx->name);
	ctx->status = MISS;
}

static const struct ethhdr eth_keys[] = {
	{{ 0x0, 0x22, 0x22, 0x22, 0x22, 0x22 },
	 { 0x0, 0x22, 0x22, 0x22, 0x22, 0x33 },
	 ETH_P_IP
	},
	{{ 0x0, 0x33, 0x33, 0x33, 0x33, 0x33 },
	 { 0x0, 0x33, 0x33, 0x33, 0x33, 0x44 },
	 ETH_P_IP
	},
	{{ 0x0, 0x11, 0x11, 0x11, 0x11, 0x11 },
	 { 0x0, 0x12, 0x11, 0x11, 0x11, 0x22 },
	 ETH_P_IPV6
	},
	{{ 0x0, 0x22, 0x22, 0x22, 0x22, 0x22 },
	 { 0x0, 0x22, 0x22, 0x22, 0x22, 0x33 },
	 ETH_P_IPV6
	},
	{{ 0x0, 0x33, 0x33, 0x33, 0x33, 0x33 },
	 { 0x0, 0x33, 0x33, 0x33, 0x33, 0x44 },
	 ETH_P_IPV6
	},
	{{ 0x0, 0x13, 0x11, 0x11, 0x11, 0x11 },
	 { 0xa, 0x11, 0x11, 0x11, 0x11, 0x22 },
	 ETH_P_IP
	},
	{{ 0x0, 0x11, 0x11, 0x11, 0x11, 0x11 },
	 { 0x0, 0x11, 0x11, 0x11, 0x11, 0x22 },
	 ETH_P_IP
	},
};

static const struct iphdr iphdr_keys[] = {
	{ .ihl = 5, .tos = 0x3f, .saddr = 0x00aaaaaa,
	  .daddr = 0x00aaaabb, .ttl = 0xff, .protocol = IPPROTO_TCP },
	{ .ihl = 5, .tos = 0x3f, .saddr = 0x00bbbbbb,
	  .daddr = 0x00bbbbcc, .ttl = 0x3f, .protocol = IPPROTO_TCP },
	{ .ihl = 5, .tos = 0x3f, .saddr = 0x00cccccc,
	  .daddr = 0x00ccccdd, .ttl = 0x0f, .protocol = IPPROTO_TCP },
	{ .ihl = 5, .tos = 0x3f, .saddr = 0x00aaaaaa,
	  .daddr = 0x00aaaabb, .ttl = 0xff, .protocol = IPPROTO_UDP },
	{ .ihl = 5, .tos = 0x3f, .saddr = 0x00bbbbbb,
	  .daddr = 0x00bbbbcc, .ttl = 0x3f, .protocol = IPPROTO_UDP },
	{ .ihl = 5, .tos = 0x3f, .saddr = 0x00cccccc,
	  .daddr = 0x00ccccdd, .ttl = 0x0f, .protocol = IPPROTO_UDP },
	{ .ihl = 5, .tos = 0x0c, .saddr = 0x00accccc,
	  .daddr = 0x00ccccdd, .ttl = 0x0f, .protocol = IPPROTO_TCP },
	{ .ihl = 5, .tos = 0x0c, .saddr = 0x00cccccc,
	  .daddr = 0x00ccccdd, .ttl = 0x0f, .protocol = IPPROTO_TCP },
};

#define RUN_ONE_ETH(NAME) do {					\
	sprintf(name, #NAME ":%u", i);				\
	ctx.status = NO_STATUS;					\
	XDP2_JOIN2(xdp2_stables_lookup_eth_stables_, NAME)(	\
		   &eth_keys[i], &ctx);				\
	if (ctx.status != status_check)				\
		printf("Status mismatch @%u: " #NAME		\
		       ": %u !=%u\n", i, ctx.status,		\
				      status_check);		\
	ctx.status = NO_STATUS;					\
	XDP2_JOIN2(xdp2_stables_lookup_by_key_eth_stables_,	\
		   NAME)(					\
		(struct XDP2_JOIN2(				\
			xdp2_tables_key_struct_eth_stables_,	\
			NAME) *)&eth_keys[i], &ctx);		\
	if (ctx.status != status_check)				\
		printf("Status mismatch @%u by key: " #NAME	\
		       ": %u !=%u\n", i, ctx.status,		\
				      status_check);		\
} while (0)

#define RUN_ONE_DETH(NAME) do {					\
	sprintf(name, #NAME ":%u", i);				\
	ctx.status = NO_STATUS;					\
	XDP2_JOIN2(xdp2_dtables_lookup_eth_dtables_, NAME)(	\
		   &eth_keys[i], &ctx);				\
	if (ctx.status != status_check)				\
		printf("Status mismatch @%u: " #NAME		\
		       ": %u !=%u\n", i, ctx.status,		\
				      status_check);		\
	ctx.status = NO_STATUS;					\
	XDP2_JOIN2(xdp2_dtables_lookup_by_key_eth_dtables_,	\
		   NAME)(					\
		(struct XDP2_JOIN2(				\
			xdp2_tables_key_struct_eth_dtables_,	\
			NAME) *)&eth_keys[i], &ctx);		\
	if (ctx.status != status_check)				\
		printf("Status mismatch @%u by key: " #NAME	\
		       ": %u !=%u\n", i, ctx.status,		\
				      status_check);		\
} while (0)

#define RUN_ONE_TETH(NAME, TYPE, TABLE) do {			\
	sprintf(name, #NAME ":%u", i);				\
	ctx.status = NO_STATUS;					\
	XDP2_JOIN2(xdp2_dtables_lookup_, TYPE)(TABLE,		\
				  &eth_keys[i], &ctx);		\
	if (ctx.status != status_check)				\
		printf("Status mismatch @%u: " #NAME		\
		       ": %u !=%u\n", i, ctx.status,		\
				      status_check);		\
} while (0)

#define RUN_ONE_ETH_SKEY(NAME) do {				\
	sprintf(name, #NAME ":%u", i);				\
	ctx.status = NO_STATUS;					\
	XDP2_JOIN2(xdp2_stables_lookup_by_key_eth_stables_,	\
		   NAME)(&eth_keys[i], &ctx);			\
	if (ctx.status != status_check)				\
		printf("Status mismatch @%u: " #NAME		\
		       ": %u !=%u\n", i, ctx.status,		\
				      status_check);		\
} while (0)

#define RUN_ONE_DETH_SKEY(NAME) do {				\
	sprintf(name, #NAME ":%u", i);				\
	ctx.status = NO_STATUS;					\
	XDP2_JOIN2(xdp2_dtables_lookup_by_key_eth_dtables_,	\
		   NAME)(&eth_keys[i], &ctx);			\
	if (ctx.status != status_check)				\
		printf("Status mismatch @%u: " #NAME		\
		       ": %u !=%u\n", i, ctx.status,		\
				      status_check);		\
} while (0)

#define RUN_ONE_IPV4(NAME) do {					\
	struct XDP2_JOIN2(xdp2_tables_key_struct_ipv4_stables_,	\
			  NAME) ipv4_key;			\
	sprintf(name, #NAME ":%u", i);				\
	ctx.status = NO_STATUS;					\
	XDP2_JOIN2(xdp2_stables_lookup_ipv4_stables_, NAME)(	\
		   &iphdr_keys[i], &ctx);			\
	if (ctx.status != status_check)				\
		printf("Status mismatch @%u: " #NAME		\
		       ": %u !=%u\n", i, ctx.status,		\
				      status_check);		\
	XDP2_JOIN2(xdp2_make_key_ipv4_stables_, NAME)(		\
			&iphdr_keys[i], &ipv4_key);		\
	XDP2_JOIN2(xdp2_stables_lookup_by_key_ipv4_stables_,	\
		   NAME)(&ipv4_key, &ctx);			\
	if (ctx.status != status_check)				\
		printf("Status mismatch @%u by key: " #NAME	\
		       ": %u !=%u\n", i, ctx.status,		\
				      status_check);		\
} while (0)

#define RUN_ONE_DIPV4(NAME) do {				\
	struct XDP2_JOIN2(xdp2_tables_key_struct_ipv4_dtables_,	\
			  NAME) ipv4_key;			\
	sprintf(name, #NAME ":%u", i);				\
	ctx.status = NO_STATUS;					\
	XDP2_JOIN2(xdp2_dtables_lookup_ipv4_dtables_, NAME)(	\
		   &iphdr_keys[i], &ctx);			\
	if (ctx.status != status_check)				\
		printf("Status mismatch @%u: " #NAME		\
		       ": %u !=%u\n", i, ctx.status,		\
				      status_check);		\
	XDP2_JOIN2(xdp2_make_key_ipv4_dtables_, NAME)(		\
			&iphdr_keys[i], &ipv4_key);		\
	XDP2_JOIN2(xdp2_dtables_lookup_by_key_ipv4_dtables_,	\
		   NAME)(&ipv4_key, &ctx);			\
	if (ctx.status != status_check)				\
		printf("Status mismatch @%u by key: " #NAME	\
		       ": %u !=%u\n", i, ctx.status,		\
				      status_check);		\
} while (0)

#define RUN_ONE_TIPV4(NAME, TYPE, TABLE) do {			\
	struct ipv4_common_keys com_keys;			\
	sprintf(name, #NAME ":%u", i);				\
	ctx.status = NO_STATUS;					\
	make_ipv4_key(&com_keys, &iphdr_keys[i]);		\
	XDP2_JOIN2(xdp2_dtables_lookup_, TYPE)(TABLE,		\
				  &com_keys, &ctx);		\
	if (ctx.status != status_check)				\
		printf("Status mismatch @%u: " #NAME		\
		       ": %u !=%u\n", i, ctx.status,		\
				      status_check);		\
} while (0)

#define ETH_DST_KEY0 { 0x0, 0x11, 0x11, 0x11, 0x11, 0x11 }
#define ETH_SRC_KEY0 { 0x0, 0x11, 0x11, 0x11, 0x11, 0x22 }
#define ETH_PROTO_KEY0 ETH_P_IP

#define ETH_KEY0 ETH_DST_KEY0, ETH_SRC_KEY0, ETH_PROTO_KEY0,

#define ETH_DST_KEY1 { 0x0, 0x22, 0x22, 0x22, 0x22, 0x22 }
#define ETH_SRC_KEY1 { 0x0, 0x22, 0x22, 0x22, 0x22, 0x33 }
#define ETH_PROTO_KEY1 ETH_P_IP

#define ETH_KEY1 ETH_DST_KEY1, ETH_SRC_KEY1, ETH_PROTO_KEY1,

static const struct ethhdr eth_keys_match[] = { { ETH_KEY0 }, { ETH_KEY1 }, };

#define IPV4_IHL_KEY0 5
#define IPV4_TOS_KEY0 0x3f
#define IPV4_SADDR_KEY0 0x00aaaaaa
#define IPV4_DADDR_KEY0 0x00aaaabb
#define IPV4_TTL_KEY0 0xff
#define IPV4_PROTOCOL_KEY0 IPPROTO_TCP

#define IPV4_KEY0 IPV4_IHL_KEY0, IPV4_TOS_KEY0, IPV4_SADDR_KEY0,	\
	IPV4_DADDR_KEY0, IPV4_TTL_KEY0, IPV4_PROTOCOL_KEY0,

#define IPV4_IHL_KEY1 5
#define IPV4_TOS_KEY1 0x3f
#define IPV4_SADDR_KEY1 0x00bbbbbb
#define IPV4_DADDR_KEY1 0x00bbbbcc
#define IPV4_TTL_KEY1 0x3f
#define IPV4_PROTOCOL_KEY1 IPPROTO_TCP

#define IPV4_KEY1 IPV4_IHL_KEY1, IPV4_TOS_KEY1, IPV4_SADDR_KEY1,	\
	IPV4_DADDR_KEY1, IPV4_TTL_KEY1, IPV4_PROTOCOL_KEY1,

#define IPV4_IHL_KEY2 5
#define IPV4_TOS_KEY2 0x3f
#define IPV4_SADDR_KEY2 0x00cccccc
#define IPV4_DADDR_KEY2 0x00ccccdd
#define IPV4_TTL_KEY2 0x0f
#define IPV4_PROTOCOL_KEY2 IPPROTO_TCP

#define IPV4_KEY2 IPV4_IHL_KEY2, IPV4_TOS_KEY2, IPV4_SADDR_KEY2,	\
	IPV4_DADDR_KEY2, IPV4_TTL_KEY2, IPV4_PROTOCOL_KEY2,

#define IPV4_IHL_KEY3 5
#define IPV4_TOS_KEY3 0x3f
#define IPV4_SADDR_KEY3 0x00aaaaaa
#define IPV4_DADDR_KEY3 0x00aaaabb
#define IPV4_TTL_KEY3 0xff
#define IPV4_PROTOCOL_KEY3 IPPROTO_UDP

#define IPV4_KEY3 IPV4_IHL_KEY3, IPV4_TOS_KEY3, IPV4_SADDR_KEY3,	\
	IPV4_DADDR_KEY3, IPV4_TTL_KEY3, IPV4_PROTOCOL_KEY3,

#define IPV4_IHL_KEY4 5
#define IPV4_TOS_KEY4 0x3f
#define IPV4_SADDR_KEY4 0x00bbbbbb
#define IPV4_DADDR_KEY4 0x00bbbbcc
#define IPV4_TTL_KEY4 0x3f
#define IPV4_PROTOCOL_KEY4 IPPROTO_UDP

#define IPV4_KEY4 IPV4_IHL_KEY4, IPV4_TOS_KEY4, IPV4_SADDR_KEY4,	\
	IPV4_DADDR_KEY4, IPV4_TTL_KEY4, IPV4_PROTOCOL_KEY4,

#define IPV4_IHL_KEY5 5
#define IPV4_TOS_KEY5 0x3f
#define IPV4_SADDR_KEY5 0x00cccccc
#define IPV4_DADDR_KEY5 0x00ccccdd
#define IPV4_TTL_KEY5 0x0f
#define IPV4_PROTOCOL_KEY5 IPPROTO_UDP

#define IPV4_KEY5 IPV4_IHL_KEY5, IPV4_TOS_KEY5, IPV4_SADDR_KEY5,	\
	IPV4_DADDR_KEY5, IPV4_TTL_KEY5, IPV4_PROTOCOL_KEY5,

#define IPV4_IHL_KEY3_ALT 5
#define IPV4_TOS_KEY3_ALT 0x0c
#define IPV4_SADDR_KEY3_ALT 0x00aaaaaa
#define IPV4_DADDR_KEY3_ALT 0x00aaaabb
#define IPV4_TTL_KEY3_ALT 0xff
#define IPV4_PROTOCOL_KEY3_ALT IPPROTO_UDP

#define IPV4_KEY3_ALT IPV4_IHL_KEY3_ALT, IPV4_TOS_KEY3_ALT,		\
	IPV4_SADDR_KEY3_ALT, IPV4_DADDR_KEY3_ALT, IPV4_TTL_KEY3,	\
	IPV4_PROTOCOL_KEY3,

#define IPV4_IHL_KEY4_ALT 5
#define IPV4_TOS_KEY4_ALT 0x0c
#define IPV4_SADDR_KEY4_ALT 0x00bbbbbb
#define IPV4_DADDR_KEY4_ALT 0x00bbbbcc
#define IPV4_TTL_KEY4_ALT 0x3f
#define IPV4_PROTOCOL_KEY4_ALT IPPROTO_UDP

#define IPV4_KEY4_ALT IPV4_IHL_KEY4_ALT, IPV4_TOS_KEY4_ALT,		\
	IPV4_SADDR_KEY4_ALT, IPV4_DADDR_KEY4_ALT, IPV4_TTL_KEY4_ALT,	\
	IPV4_PROTOCOL_KEY4_ALT,

#define IPV4_IHL_KEY5_ALT 5
#define IPV4_TOS_KEY5_ALT 0x0c
#define IPV4_SADDR_KEY5_ALT 0x00cccccc
#define IPV4_DADDR_KEY5_ALT 0x00ccccdd
#define IPV4_TTL_KEY5_ALT 0x0f
#define IPV4_PROTOCOL_KEY5_ALT IPPROTO_UDP

#define IPV4_KEY5_ALT IPV4_IHL_KEY5_ALT, IPV4_TOS_KEY5_ALT,		\
	IPV4_SADDR_KEY5_ALT, IPV4_DADDR_KEY5_ALT, IPV4_TTL_KEY5_ALT,	\
	IPV4_PROTOCOL_KEY5_ALT,

struct ipv4_common_keys {
	__u8 ihl;
	__u8 tos;
	__u32 saddr;
	__u32 daddr;
	__u8 ttl;
	__u8 protocol;
} __packed;

static const struct ipv4_common_keys ipv4_keys_match[] = {
	{ IPV4_KEY0 }, { IPV4_KEY1 }, { IPV4_KEY2 },
	{ IPV4_KEY3 }, { IPV4_KEY4 }, { IPV4_KEY5 },
};

static const struct ipv4_common_keys ipv4_keys_match_alt[] = {
	{ IPV4_KEY0 }, { IPV4_KEY1 }, { IPV4_KEY2 },
	{ IPV4_KEY3 }, { IPV4_KEY4_ALT }, { IPV4_KEY5_ALT },
};

static inline void make_ipv4_key(struct ipv4_common_keys *com,
				 const struct iphdr *iphdr)
{
	com->ihl = iphdr->ihl;
	com->tos = iphdr->tos;
	com->saddr = iphdr->saddr;
	com->daddr = iphdr->daddr;
	com->ttl = iphdr->ttl;
	com->protocol = iphdr->protocol;
}
