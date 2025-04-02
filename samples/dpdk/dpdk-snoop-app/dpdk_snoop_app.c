/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2010-2015 Intel Corporation
 */

#include <inttypes.h>
#include <linux/types.h>
#include <rte_cycles.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>
#include <stdint.h>

#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024

#define NUM_MBUFS 8191
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE 32

#define RTE_ETHER_ADDR_BYTES_1(MAC_ADDRS) ((MAC_ADDRS)->addr_bytes[0]), \
	((MAC_ADDRS)->addr_bytes[1]), \
	((MAC_ADDRS)->addr_bytes[2]), \
	((MAC_ADDRS)->addr_bytes[3]), \
	((MAC_ADDRS)->addr_bytes[4]), \
	((MAC_ADDRS)->addr_bytes[5])


extern void send_pkt_buf_to_xdp2_parser(const void *parser_ctx,
	void *pktbuf, size_t pktbuflen);

static const struct rte_eth_conf port_conf_default = {
	.rxmode = {
		.max_rx_pkt_len = RTE_ETHER_MAX_LEN,
	},
};

/* Main functional part of port initialization. */
static inline int port_init(__u16 port, struct rte_mempool *mbuf_pool)
{
	struct rte_eth_conf port_conf = port_conf_default;
	const __u16 rx_rings = 1, tx_rings = 1;
	struct rte_eth_dev_info dev_info;
	struct rte_eth_txconf txconf;
	__u16 nb_rxd = RX_RING_SIZE;
	__u16 nb_txd = TX_RING_SIZE;
	struct rte_ether_addr addr;
	__s32 retval;
	__u16 q;

	if (!rte_eth_dev_is_valid_port(port))
		return -1;

	retval = rte_eth_dev_info_get(port, &dev_info);
	if (retval != 0) {
		printf("Error during getting device (port %u) info: %s\n",
		       port, strerror(-retval));
		return retval;
	}

	if (dev_info.tx_offload_capa & DEV_TX_OFFLOAD_MBUF_FAST_FREE)
		port_conf.txmode.offloads |= DEV_TX_OFFLOAD_MBUF_FAST_FREE;

	/* Configure the Ethernet device. */
	retval = rte_eth_dev_configure(port, rx_rings, tx_rings, &port_conf);
	if (retval != 0)
		return retval;

	retval = rte_eth_dev_adjust_nb_rx_tx_desc(port, &nb_rxd, &nb_txd);
	if (retval != 0)
		return retval;

	/* Allocate and set up 1 RX queue per Ethernet port. */
	for (q = 0; q < rx_rings; q++) {
		retval = rte_eth_rx_queue_setup(port, q, nb_rxd,
						rte_eth_dev_socket_id(port),
						NULL, mbuf_pool);
		if (retval < 0)
			return retval;
	}

	txconf = dev_info.default_txconf;
	txconf.offloads = port_conf.txmode.offloads;
	/* Allocate and set up 1 TX queue per Ethernet port. */
	for (q = 0; q < tx_rings; q++) {
		retval = rte_eth_tx_queue_setup(port, q, nb_txd,
						rte_eth_dev_socket_id(port),
						&txconf);
		if (retval < 0)
			return retval;
	}

	/* Starting Ethernet port. */
	retval = rte_eth_dev_start(port);
	/* End of starting of ethernet port. */

	if (retval < 0)
		return retval;

	/* Display the port MAC address. */
	retval = rte_eth_macaddr_get(port, &addr);
	if (retval != 0)
		return retval;

	printf("Port %u MAC: %02" PRIx8 " %02" PRIx8 " %02" PRIx8
	       " %02" PRIx8 " %02" PRIx8 " %02" PRIx8 "\n",
	       port, RTE_ETHER_ADDR_BYTES_1(&addr));

	/* Enable RX in promiscuous mode for the Ethernet device. */
	retval = rte_eth_promiscuous_enable(port);
	/* End of setting RX port in promiscuous mode. */
	if (retval != 0)
		return retval;

	return 0;
}
/* End of main functional part of port initialization. */

extern __u8 op_file[];

#define DEBUG_DPRINT(BUF, LEN)				\
do {							\
	FILE *FP = fopen(op_file, "a");			\
	unsigned char *T = BUF;				\
	int RET = 0;					\
	assert(FP != NULL);				\
	while (T <= BUF) {				\
		RET = fwrite(T, LEN, 1, FP);		\
		assert(RET != 0);			\
		T += RET;				\
	}						\
	assert(fclose(FP) == 0);			\
} while (0)

struct pcaprec_hdr {
	__u32 ts_sec;         /* timestamp seconds */
	__u32 ts_usec;        /* timestamp microseconds */
	__u32 incl_len;       /* number of octets of packet saved in file */
	__u32 orig_len;       /* actual length of packet */
};

/* The lcore main. This is the main thread that does the work, reading from
 * an input port and writing to an output port.
 */

/* Basic snooping application lcore. */
static __rte_noreturn void lcore_main(const void *parser_ctx)
{
	struct rte_mbuf *bufs[BURST_SIZE];
	// a magic buffer for pcap file header
	__u8 pcap_file_hdr[] = {
		0xd4, 0xc3, 0xb2, 0xa1, 0x02, 0x00, 0x04, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00,
		0x01, 0x00, 0x00, 0x00
	};
	struct pcaprec_hdr hdr;
	__u16 nb_rx;
	__u16 port;
	__u8 *obuf;
	__u32 id;

	DEBUG_DPRINT(pcap_file_hdr, sizeof(pcap_file_hdr));
	memset(&hdr, 0, sizeof(hdr));
	port = -1;

	/* Check that the port is on the same NUMA node as the polling thread
	 * for best performance.
	 */
	RTE_ETH_FOREACH_DEV(port) {
		if (rte_eth_dev_socket_id(port) >= 0 &&
			rte_eth_dev_socket_id(port) != (int)rte_socket_id())
			printf("WARNING, port %u is on remote NUMA node to "
			       "polling thread.\n\tPerformance will "
			       "not be optimal.\n", port);
		break;
	}

	printf("\nCore %u snooping packets. [Ctrl+C to quit]\n",
	       rte_lcore_id());

	/* Main work of application loop. */
	for (;;) {
		/* Get burst of RX packets, from ports one by one. */
		nb_rx = rte_eth_rx_burst(port, 0, bufs, BURST_SIZE);

		if (unlikely(nb_rx == 0))
			continue;

		for (id = 0; id < nb_rx; id++) {
			// printf("\n nb_rx: %u, %u, %u\n",
			//	  nb_rx, bufs[0]->pkt_len, bufs[0]->data_len);
			obuf = rte_pktmbuf_mtod_offset(bufs[id], __u8 *, 0);
			hdr.incl_len = bufs[id]->pkt_len;
			hdr.orig_len = bufs[id]->pkt_len;
			DEBUG_DPRINT((__u8 *) &hdr, sizeof(hdr));
			// DEBUG_DPRINT(obuf, 42);
			DEBUG_DPRINT(obuf, bufs[id]->pkt_len);
			printf("received a pkt, size: %u bytes\n",
			       bufs[id]->pkt_len);
			// send this pkt to xdp2 parser
			send_pkt_buf_to_xdp2_parser(parser_ctx, obuf,
						     bufs[id]->pkt_len);
			rte_pktmbuf_free(bufs[id]);
		}
	}
}
/* End Basic forwarding application lcore. */

/* The main function, which does initialization and calls the per-lcore
 * functions.
 */
int app_main(const void *parser_ctx, int argc, char *argv[])
{
	struct rte_mempool *mbuf_pool;
	__u16 nb_ports;
	__u16 portid;
	__s32 ret;

	/* Initializion the Environment Abstraction Layer (EAL). */
	ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");
	/* End of initializion the Environment Abstraction Layer (EAL). */

	/* Check that there is an even number of ports to send/receive on. */
	nb_ports = rte_eth_dev_count_avail();
	// if (nb_ports < 2 || (nb_ports & 1))
	if (nb_ports < 1)
		rte_exit(EXIT_FAILURE, "Error: atleast one port is needed\n");
	nb_ports = 1;

	/* Creates a new mempool in memory to hold the mbufs. */

	/* Allocates mempool to hold the mbufs. */
	mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUFS * nb_ports,
					    MBUF_CACHE_SIZE, 0,
					    RTE_MBUF_DEFAULT_BUF_SIZE,
					    rte_socket_id());
	/* End of allocating mempool to hold mbuf. */

	if (mbuf_pool == NULL)
		rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");

	/* Initializing all ports. */
	RTE_ETH_FOREACH_DEV(portid) {
		if (port_init(portid, mbuf_pool) != 0)
			rte_exit(EXIT_FAILURE, "Cannot init port %"PRIu16 "\n",
				 portid);
		break;
	}
	/* End of initializing all ports. */

	if (rte_lcore_count() > 1)
		printf("\nWARNING: Too many lcores enabled. Only 1 used.\n");

	printf("snoopped packets will be stored at file: %s\n", op_file);
	(void) remove(op_file);

	/* Call lcore_main on the main core only. Called on single lcore. */
	lcore_main(parser_ctx);
	/* End of called on single lcore. */

	/* clean up the EAL */
	rte_eal_cleanup();

	return 0;
}
