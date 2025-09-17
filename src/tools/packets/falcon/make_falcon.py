from scapy.all import *

import falcon

# Python script to create a pcap file containing samples of Falcon
# packet formats (pull request, pull data, push data, resync, ACK,
# extended ACK, and NACK)


# Optional first argument is UDP port number
if len(sys.argv) > 1:
    falcon_port_num = sys.argv[1]
else:
    falcon_port_num = 4793

def MakePacket(req):
    req_packet = Raw(load=req)
    packet = ether_frame/ip_packet1/udp_packet1/req_packet
    packets.append(packet)

packets = []

source_mac = "00:11:22:33:44:55"
destination_mac = "AA:BB:CC:DD:EE:FF"
ether_type = 0x0800

ether_frame = Ether(src=source_mac, dst=destination_mac, type=ether_type)

ip_packet1 = IP(src="192.168.1.1", dst="192.168.1.2")
ip_packet2 = IP(src="192.168.2.1", dst="192.168.2.2")

udp_packet1 = UDP(dport = int(falcon_port_num), sport = 8675)
udp_packet2 = UDP(dport = int(falcon_port_num), sport = 28675)

# Make pull request sample packet

req = falcon.MakePullReq(dest_cid = 0x123456, dest_function = 0x89abcd,
		  protocol_type =
			falcon.FalconProtocolType.FALCON_PROTO_TYPE_RDMA.value,
		  ack_req = 1, recv_data_psn = 0x8765309a,
		  recv_req_psn = 0xfedcba98, psn = 0x31425364,
		  req_seqno = 0xffeeddcc, req_length = 5566)

MakePacket(req)

# Make pull data sample packet

req = falcon.MakePullData(dest_cid = 0x123456, dest_function = 0x89abcd,
		  protocol_type =
			falcon.FalconProtocolType.FALCON_PROTO_TYPE_NVME.value,
		  ack_req = 0, recv_data_psn = 0x8765309a,
		  recv_req_psn = 0xfedcba98, psn = 0x31425364,
		  req_seqno = 0xffeeddcc)

MakePacket(req)

# Make push data sample packet

req = falcon.MakePushData(dest_cid = 0x123456, dest_function = 0x89abcd,
		  protocol_type =
			falcon.FalconProtocolType.FALCON_PROTO_TYPE_RDMA.value,
		  ack_req = 1, recv_data_psn = 0x8765309a,
		  recv_req_psn = 0xfedcba98, psn = 0x31425364,
		  req_seqno = 0xffeeddcc, req_length = 8192)

MakePacket(req)

# Make resync sample packet

req = falcon.MakeResync(dest_cid = 0x123456, dest_function = 0x89abcd,
		 protocol_type =
			falcon.FalconProtocolType.FALCON_PROTO_TYPE_NVME.value,
		 ack_req = 1, recv_data_psn = 0x8765309c,
		 recv_req_psn = 0xfedcba98, psn = 0x31425364,
		 req_seqno = 0xff11dd22,
		 resync_code =
			falcon.FalconResyncCode.FALCON_RESYNC_CODE_REMOTE_XLR_FLOW.
									value,
		 resync_pkt_type =
			falcon.FalconPktType.FALCON_PKT_TYPE_NACK.value,
		 vendor_defined = 0x77778888)

MakePacket(req)

# Make acknowlegdement sample packet

req = falcon.MakeAck(cnx_id = 0x654321, recv_data_wind_seqno = 0xccddeeff,
        recv_req_wind_seqno = 0x12345678,
	    timestamp_t1 = 0x99887766, timestamp_t2 = 0x55667788,
	    hop_count = 7, rx_buffer_occ = 25, ecn_rx_pkt_cnt = 0x2342,
	    rue_info = 0x76655, oo_wind_notify = 3)

MakePacket(req)

# Make extended acknowlegdement sample packet

req = falcon.MakeEack(cnx_id = 0x654323, recv_data_wind_seqno = 0xccddeeff,
            recv_req_wind_seqno = 0x12345678,
	    timestamp_t1 = 0x99887766, timestamp_t2 = 0x55667788,
	    hop_count = 7, rx_buffer_occ = 25, ecn_rx_pkt_cnt = 0x2342,
	    rue_info = 2, oo_wind_notify = 3,
	    rcv_data_seqno_ack_bitmap_1 = 0x1a2a3a4a12345678,
	    rcv_data_seqno_ack_bitmap_2 = 0x1b2b3b4b87654321,
	    rcv_req_seqno_ack_bitmap_1 = 0xabacada789abcdef,
	    rcv_req_seqno_ack_bitmap_2 = 0xebecedecba987654,
	    rcv_req_seqno_bitmap = 0x707172738675309d)

MakePacket(req)

# Make negative acknowlegdement sample packet

req = falcon.MakeNack(cnx_id = 0xbec001, recv_data_wind_seqno = 0xccdd33ff,
            recv_req_wind_seqno = 0x12446678,
	    timestamp_t1 = 0x99887766, timestamp_t2 = 0x55667788,
	    hop_count = 2, rx_buffer_occ = 23, ecn_rx_pkt_cnt = 0x1342,
	    rue_info = 2, nack_psn = 0x55443322,
	    nack_code =
		    falcon.FalconNackCode.FALCON_NACK_CODE_ULP_NONRECOV_ERROR.value,
        rnr_nack_timeout = 29, window = 1, ulp_nack_code =55)

MakePacket(req)

# Write the packets to the pcap file

wrpcap("falcon.pcap", packets)
