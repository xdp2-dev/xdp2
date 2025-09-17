from scapy.all import *

import sys

import uet_pds
import uet_ses

# Python script to create a pcap file containing samples of UET
# Protocol Delivery Layer packet protocol formats

# Optional first argument is UDP port number
if len(sys.argv) > 1:
    uet_port_num = sys.argv[1]
else:
    uet_port_num = 4793

def MakePacket(req1, req2):
	header1 = Raw(load=req1)
	header2 = Raw(load=req2)
	packet = ether_frame/ip_packet/udp_packet/header1/header2
	packets.append(packet)

packets = []

source_mac = "00:11:22:33:44:55"
destination_mac = "AA:BB:CC:DD:EE:FF"
ether_type = 0x0800

ether_frame = Ether(src=source_mac, dst=destination_mac, type=ether_type)

str = "192.168.1.1"

ip_packet = IP(src="192.168.1.2", dst="192.168.1.2")

udp_packet = UDP(dport = int(uet_port_num), sport = 35433)

# Produce samples of various packet types

ses = uet_ses.MakeRequestSomHdrStd(
    opcode =
    uet_ses.UetSesRequestMsgType.UET_SES_REQUEST_MSG_READ.value,
    delivery_complete = 1, initiator_error = 0,
    relative_addressing = 1, hdr_data_present = 0,
    end_of_msg = 1, message_id = 0x1234,
    ri_generation = 0x77, job_id =0xabcdef, pid_on_fep =0x678,
    resource_index = 0x9ab,
    buffer_offset =0xfedcba9876543210,
    initiator = 0xfedcba98,
    memory_key_match_bits = 0x1122334455667788,
    header_data = 0xaabbddddeeff0011,
    request_length = 0x99887766)

# Plain PDS RUD request

pds = uet_pds.MakeRudRequest(
	next_hdr = uet_pds.UetNextHeaderType.UET_HDR_REQUEST_STD.value,
	retrans = 1, ackreq = 0, clear_psn_offset = 0x1234, psn = 0x98765432,
	spdcid = 0x3456, dpdcid = 0x9abc)

MakePacket(pds, ses)

# PDS RUD request with SYN

pds = uet_pds.MakeRudRequestSyn(
	next_hdr = uet_pds.UetNextHeaderType.UET_HDR_REQUEST_STD.value,
	retrans = 1, ackreq = 0, clear_psn_offset = 0x1234, psn = 0x98765432,
	spdcid = 0x3456, use_rsv_pdc = 1, psn_offset = 0x876)

MakePacket(pds, ses)

# PDS RUD request with CC

pds = uet_pds.MakeRudRequestCc(
	next_hdr = uet_pds.UetNextHeaderType.UET_HDR_REQUEST_STD.value,
	retrans = 1, ackreq = 0, clear_psn_offset = 0x1234, psn = 0x98765432,
	spdcid = 0x3456, dpdcid = 0x9abc, ccc_id = 0x77,
    credit_target = 0x887766)

MakePacket(pds, ses)

# PDS RUD request with SYN and CC

pds = uet_pds.MakeRudRequestSynCc(
	next_hdr = uet_pds.UetNextHeaderType.UET_HDR_REQUEST_STD.value,
	retrans = 1, ackreq = 0, clear_psn_offset = 0x1234, psn = 0x98765432,
	spdcid = 0x3456, use_rsv_pdc = 1, psn_offset = 0x876, ccc_id = 0x77,
    credit_target = 0x887766)

MakePacket(pds, ses)

# Plain PDS ROD request

pds = uet_pds.MakeRodRequest(
	next_hdr = uet_pds.UetNextHeaderType.UET_HDR_REQUEST_STD.value,
	retrans = 1, ackreq = 0, clear_psn_offset = 0x1234, psn = 0x98765432,
	spdcid = 0x3456, dpdcid = 0x9abc)

MakePacket(pds, ses)

# PDS ROD request with SYN

pds = uet_pds.MakeRodRequestSyn(
	next_hdr = uet_pds.UetNextHeaderType.UET_HDR_REQUEST_STD.value,
	retrans = 1, ackreq = 0, clear_psn_offset = 0x1234, psn = 0x98765432,
	spdcid = 0x3456, use_rsv_pdc = 1, psn_offset = 0x876)

MakePacket(pds, ses)

# PDS ROD request with CC

pds = uet_pds.MakeRodRequestCc(
	next_hdr = uet_pds.UetNextHeaderType.UET_HDR_REQUEST_STD.value,
	retrans = 1, ackreq = 0, clear_psn_offset = 0x1234, psn = 0x98765432,
	spdcid = 0x3456, dpdcid = 0x9abc, ccc_id = 0x77,
    credit_target = 0x887766)

MakePacket(pds, ses)

# PDS RUD request with SYN and CC

pds = uet_pds.MakeRodRequestSynCc(
	next_hdr = uet_pds.UetNextHeaderType.UET_HDR_REQUEST_STD.value,
	retrans = 1, ackreq = 0, clear_psn_offset = 0x1234, psn = 0x98765432,
	spdcid = 0x3456, use_rsv_pdc = 1, psn_offset = 0x876, ccc_id = 0x77,
    credit_target = 0x887766)

MakePacket(pds, ses)

# PDS ACK

ses = uet_ses.MakeHdrResponse(
	list = 3,
	opcode =
	uet_ses.UetSesResponseMsgType.UET_SES_RESPONSE.value,
	return_code =
	uet_ses.UetSesReturnCode.UET_SES_RETURN_CODE_AT_PERM.value,
	message_id = 0x1234, ri_generation = 0x99, job_id = 0x654321,
	modified_length = 0x9abcdef)

pds = uet_pds.MakeAck(
	next_hdr = uet_pds.UetNextHeaderType.UET_HDR_RESPONSE.value,
    ecn_marked = 1, retrans = 1, probe = 0, request = 1,
    cack_psn = 0x2468ace0, ack_psn_offset_probe_opaque = 0x8642,
	spdcid = 0x3456, dpdcid = 0x789a)

MakePacket(pds, ses)

# PDS ACK with CC NSCC

pds = uet_pds.MakeAckCcNscc(
	next_hdr = uet_pds.UetNextHeaderType.UET_HDR_RESPONSE.value,
    ecn_marked = 1, retrans = 1, probe = 0, request = 1,
    cack_psn = 0x2468ace0, ack_psn_offset_probe_opaque = 0x2121,
	spdcid = 0x3456, dpdcid = 0x789a, cc_flags = 0xf, mpr = 0x87,
    sack_psn_offset = 0x6789, sack_bitmap = 0x123456789abcdef0,
    service_time = 0x99aa, restore_cwnd = 1, rcv_cwnd_pend = 0x7f,
    rcvd_bytes = 0x887766, ooo_count = 0x8765)

MakePacket(pds, ses)

# PDS ACK with CC credit

pds = uet_pds.MakeAckCcCredit(
	next_hdr = uet_pds.UetNextHeaderType.UET_HDR_RESPONSE.value,
    ecn_marked = 1, retrans = 1, probe = 0, request = 1,
    cack_psn = 0x2468ace0, ack_psn_offset_probe_opaque = 0x9876,
	spdcid = 0x3456, dpdcid = 0x789a, cc_flags = 0xf, mpr = 0x87,
    sack_psn_offset = 0x9988, sack_bitmap = 0x123456789abcdef0,
    credit = 0x123456, ooo_count = 0x8765)

MakePacket(pds, ses)

# PDS ACK with CCX

pds = uet_pds.MakeAckCcx(
	next_hdr = uet_pds.UetNextHeaderType.UET_HDR_RESPONSE.value,
    ecn_marked = 1, retrans = 1, probe = 0, request = 1,
    cack_psn = 0x2468ace0, ack_psn_offset_probe_opaque = 0x9876,
	spdcid = 0x3456, dpdcid = 0x789a, mpr = 0x87,
    sack_psn_offset = 0x9988, sack_bitmap = 0x123456789abcdef0,
    ccx_type =0xe, ack_cc_state = 0x1122334455667788)

MakePacket(pds, ses)

# PDS NACK

pds = uet_pds.MakeNack(
	next_hdr = uet_pds.UetNextHeaderType.UET_HDR_RESPONSE.value,
    ecn_marked = 1, retrans = 1,
    nak_type = uet_pds.UetPdsNackType.UET_PDS_NACK_TYPE_RUDI.value,
    nak_code =
        uet_pds.UetPdsNackCode.UET_PDS_NACK_CODE_PDC_MODE_MISMATCH.value,
    vendor_code = 0x87, nak_psn_pkt_id = 0x99887766,
	spdcid = 0x3456, dpdcid = 0x789a, payload = 0x56789abc)

MakePacket(pds, ses)

# PDS NACK CCX

pds = uet_pds.MakeNackCcx(
	next_hdr = uet_pds.UetNextHeaderType.UET_HDR_RESPONSE.value,
    ecn_marked = 0, retrans = 1,
    nak_type = uet_pds.UetPdsNackType.UET_PDS_NACK_TYPE_RUD_ROD.value,
    nak_code =
        uet_pds.UetPdsNackCode.UET_PDS_NACK_CODE_INVALID_SYN.value,
    vendor_code = 0x87, nak_psn_pkt_id = 0x99887766,
	spdcid = 0x3456, dpdcid = 0x789a, payload = 0x56789abc,
    nccx_type = 3, nccx_ccx_state1 = 0xf, nccx_ccx_state2 = 0xedcba9876543210)

MakePacket(pds, ses)

# PDS control packet

pds = uet_pds.MakeControlPkt(ctl_type =
    uet_pds.UetPdsAckControlPktType.UET_PDS_CTL_TYPE_CREDIT_REQ.value,
    rsvd_isrod = 1, retrans = 1, ack_req = 0, probe_opaque = 0x9876,
    psn = 0xcdef0123, spdcid = 0xcdef, dpdcid = 0xfedc)

MakePacket(pds, ses)

# PDS control packet with SYN

pds = uet_pds.MakeControlPktSyn(ctl_type =
    uet_pds.UetPdsAckControlPktType.UET_PDS_CTL_TYPE_NEGOTIATION.value,
    rsvd_isrod = 0, retrans = 1, probe_opaque = 0x1234,  spdcid = 0xcdef,
    psn = 0xcdef0123, use_rsv_pdc = 1, psn_offset = 0x876)

MakePacket(pds, ses)

ses = uet_ses.MakeRequestSomHdrStd(
    opcode =
    uet_ses.UetSesRequestMsgType.UET_SES_REQUEST_MSG_READ.value,
    delivery_complete = 1, initiator_error = 0,
    relative_addressing = 1, hdr_data_present = 0,
    end_of_msg = 1, message_id = 0x1234,
    ri_generation = 0x77, job_id =0xabcdef, pid_on_fep =0x678,
    resource_index = 0x9ab,
    buffer_offset =0xfedcba9876543210,
    initiator = 0xfedcba98,
    memory_key_match_bits = 0x1122334455667788,
    header_data = 0xaabbddddeeff0011,
    request_length = 0x99887766)

# UUD request

pds = uet_pds.MakeUudReq(uet_pds.UetNextHeaderType.UET_HDR_REQUEST_STD.value)

MakePacket(pds, ses)

# RUDI request

pds = uet_pds.MakeRudiRequest(
	next_hdr = uet_pds.UetNextHeaderType.UET_HDR_REQUEST_STD.value,
    ecn_marked = 1, retrans = 0, pkt_id = 0x99887766)

MakePacket(pds, ses)

# RUDI response

ses = uet_ses.MakeHdrResponse(
	list = 3,
	opcode =
	uet_ses.UetSesResponseMsgType.UET_SES_RESPONSE.value,
	return_code =
	uet_ses.UetSesReturnCode.UET_SES_RETURN_CODE_AT_PERM.value,
	message_id = 0x1234, ri_generation = 0x99, job_id = 0x654321,
	modified_length = 0x9abcdef)

pds = uet_pds.MakeRudiResponse(
	next_hdr = uet_pds.UetNextHeaderType.UET_HDR_RESPONSE.value,
    ecn_marked = 1, retrans = 0, pkt_id = 0x99887766)

MakePacket(pds, ses)

# Write the packets to the pcap file

wrpcap("uet_pds.pcap", packets)
