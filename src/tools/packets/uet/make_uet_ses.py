from scapy.all import *

import sys

import uet_pds
import uet_ses

# Python script to create a pcap file containing samples of UET
# Semantic Sublayer (SES) packet formats

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

def MakePacket3(req1, req2, req3):
	header1 = Raw(load=req1)
	header2 = Raw(load=req2)
	header3 = Raw(load=req3)
	packet = ether_frame/ip_packet/udp_packet/header1/header2/header3
	packets.append(packet)

packets = []

source_mac = "00:11:22:33:44:55"
destination_mac = "AA:BB:CC:DD:EE:FF"
ether_type = 0x0800

ether_frame = Ether(src=source_mac, dst=destination_mac, type=ether_type)

ip_packet = IP(src="192.168.1.1", dst="192.168.1.2")

udp_packet = UDP(dport = int(uet_port_num), sport = 8675)

# Produce samples of various packet types

# Standard headers
pds = uet_pds.MakeRudRequest(
	next_hdr = uet_pds.UetNextHeaderType.UET_HDR_REQUEST_STD.value,
	retrans = 1, ackreq = 0, clear_psn_offset = 0x1234, psn = 0x98765432,
	spdcid = 0x3456, dpdcid = 0x9abc)

# Standard request start-of-message
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

MakePacket(pds, ses)

# Standard request no start-of-message
ses = uet_ses.MakeRequestNoSomHdrStd(
    opcode =
    uet_ses.UetSesRequestMsgType.UET_SES_REQUEST_MSG_WRITE.value,
    delivery_complete = 1, initiator_error = 0,
    relative_addressing = 1, hdr_data_present = 0,
    end_of_msg = 1, message_id = 0x1234,
    ri_generation = 0x77, job_id =0xabcdef, pid_on_fep =0x678,
    resource_index = 0x9ab,
    buffer_offset =0xfedcba9876543210,
    initiator = 0xfedcba98,
    memory_key_match_bits = 0x1122334455667788,
    payload_length = 0x345, message_offset = 0x77665544,
    request_length = 0x99887766)

MakePacket(pds, ses)

# Standard deferred send
ses = uet_ses.MakeDeferSendHdrStd(
    opcode =
    uet_ses.UetSesRequestMsgType.UET_SES_REQUEST_MSG_DEFERRABLE_SEND.value,
    delivery_complete = 1, initiator_error = 0,
    relative_addressing = 1, hdr_data_present = 0,
    end_of_msg = 1, start_of_msg = 0, message_id = 0x1234,
    ri_generation = 0x77, job_id =0xabcdef, pid_on_fep =0x678,
    resource_index = 0x9ab,
    initiator_restart_token = 0x87654321,
    target_restart_token = 0x12345678,
    initiator = 0xfedcba98,
    match_bits = 0x1122334455667788,
    header_data = 0xfedcba9876543210,
    request_length = 0x99887766)

MakePacket(pds, ses)

# Ready to restart
ses = uet_ses.MakeReadyToRestartHdrStd(
    opcode =
    uet_ses.UetSesRequestMsgType.UET_SES_REQUEST_MSG_READY_TO_RESTART.value,
    delivery_complete = 1, initiator_error = 0,
    relative_addressing = 1, hdr_data_present = 0,
    end_of_msg = 1, start_of_msg = 0, message_id = 0x1234,
    ri_generation = 0x77, job_id =0xabcdef, pid_on_fep =0x678,
    resource_index = 0x9ab,
    buffer_offset = 0x8899aabbccddeeff,
    initiator = 0xfedcba98,
    initiator_restart_token = 0x87654321,
    target_restart_token = 0x12345678,
    header_data = 0xfedcba9876543210,
    request_length = 0x99887766)

MakePacket(pds, ses)

# Standard atomic operation
ses = uet_ses.MakeRequestNoSomHdrStd(
    opcode =
    uet_ses.UetSesRequestMsgType.UET_SES_REQUEST_MSG_ATOMIC.value,
    delivery_complete = 1, initiator_error = 0,
    relative_addressing = 1, hdr_data_present = 0,
    end_of_msg = 1, message_id = 0x1234,
    ri_generation = 0x77, job_id =0xabcdef, pid_on_fep =0x678,
    resource_index = 0x9ab,
    buffer_offset =0xfedcba9876543210,
    initiator = 0xfedcba98,
    memory_key_match_bits = 0x1122334455667788,
    payload_length = 0x345, message_offset = 0x77665544,
    request_length = 0x99887766)

atomic = uet_ses.MakeAtomicOpExtHdr(
    atomic_code = uet_ses.UetSesAtomicOpcode.UET_SES_AMO_BXOR.value,
    atomic_datatype =
        uet_ses.UetSesAtomicDatatype.UET_SES_AMO_DATATYPE_FLOAT_COMPLEX.value,
    ctrl_cacheable = 1, cpu_coherent = 1, vendor_defined = 7)

MakePacket3(pds, ses, atomic)

# Standard atomic compare and swap operation
ses = uet_ses.MakeRequestNoSomHdrStd(
    opcode =
    uet_ses.UetSesRequestMsgType.UET_SES_REQUEST_MSG_ATOMIC.value,
    delivery_complete = 1, initiator_error = 0,
    relative_addressing = 1, hdr_data_present = 0,
    end_of_msg = 1, message_id = 0x1234,
    ri_generation = 0x77, job_id =0xabcdef, pid_on_fep =0x678,
    resource_index = 0x9ab,
    buffer_offset =0xfedcba9876543210,
    initiator = 0xfedcba98,
    memory_key_match_bits = 0x1122334455667788,
    payload_length = 0x345, message_offset = 0x77665544,
    request_length = 0x99887766)

atomic = uet_ses.MakeAtomicCmpAndSwapHdr(
    atomic_code = uet_ses.UetSesAtomicOpcode.UET_SES_AMO_CSWAP_GE.value,
    atomic_datatype =
        uet_ses.UetSesAtomicDatatype.UET_SES_AMO_DATATYPE_FLOAT_COMPLEX.value,
    ctrl_cacheable = 1, cpu_coherent = 1, vendor_defined = 7,
    compare_value1 = 0x9876543298765432,
    compare_value2 = 0x2345678923456789,
    swap_value1 = 0x123456789abcdef,
    swap_value2 = 0xfedcba0987654321)

MakePacket3(pds, ses, atomic)

pds = uet_pds.MakeRudRequest(
	next_hdr = uet_pds.UetNextHeaderType.UET_HDR_REQUEST_MEDIUM.value,
	retrans = 1, ackreq = 0, clear_psn_offset = 0x1234, psn = 0x98765432,
	spdcid = 0x3456, dpdcid = 0x9abc)

# Medium request
ses = uet_ses.MakeRequestHdrMedium(
    opcode =
    uet_ses.UetSesRequestMsgType.UET_SES_REQUEST_MSG_WRITE.value,
    delivery_complete = 1, initiator_error = 0,
    relative_addressing = 1, hdr_data_present = 0, start_of_msg = 1,
    end_of_msg = 1, message_id = 0x1234,
    ri_generation = 0x77, job_id =0xabcdef, pid_on_fep =0x678,
    resource_index = 0x9ab, header_data = 0x9876543298765432,
    initiator = 0x87654321, memory_key = 0xccbbddeeff009988)

MakePacket(pds, ses)

# Medium atomic
ses = uet_ses.MakeRequestHdrMedium(
    opcode =
    uet_ses.UetSesRequestMsgType.UET_SES_REQUEST_MSG_ATOMIC.value,
    delivery_complete = 1, initiator_error = 0,
    relative_addressing = 1, hdr_data_present = 0, start_of_msg = 1,
    end_of_msg = 1, message_id = 0x1234,
    ri_generation = 0x77, job_id =0xabcdef, pid_on_fep =0x678,
    resource_index = 0x9ab, header_data = 0x9876543298765432,
    initiator = 0x87654321, memory_key = 0xccbbddeeff009988)

atomic = uet_ses.MakeAtomicOpExtHdr(
    atomic_code = uet_ses.UetSesAtomicOpcode.UET_SES_AMO_BXOR.value,
    atomic_datatype =
        uet_ses.UetSesAtomicDatatype.UET_SES_AMO_DATATYPE_FLOAT_COMPLEX.value,
    ctrl_cacheable = 1, cpu_coherent = 1, vendor_defined = 7)

MakePacket3(pds, ses, atomic)

# Medium atomic compare and swap operation
atomic = uet_ses.MakeAtomicCmpAndSwapHdr(
    atomic_code = uet_ses.UetSesAtomicOpcode.UET_SES_AMO_CSWAP_GE.value,
    atomic_datatype =
        uet_ses.UetSesAtomicDatatype.UET_SES_AMO_DATATYPE_FLOAT_COMPLEX.value,
    ctrl_cacheable = 1, cpu_coherent = 1, vendor_defined = 7,
    compare_value1 = 0x9876543298765432,
    compare_value2 = 0x2345678923456789,
    swap_value1 = 0x123456789abcdef,
    swap_value2 = 0xfedcba0987654321)

MakePacket3(pds, ses, atomic)

pds = uet_pds.MakeRudRequest(
	next_hdr = uet_pds.UetNextHeaderType.UET_HDR_REQUEST_SMALL.value,
	retrans = 1, ackreq = 0, clear_psn_offset = 0x1234, psn = 0x98765432,
	spdcid = 0x3456, dpdcid = 0x9abc)

# Small request
ses = uet_ses.MakeRequestHdrSmall(
    opcode =
    uet_ses.UetSesRequestMsgType.UET_SES_REQUEST_MSG_READ.value,
    delivery_complete = 1, initiator_error = 0,
    relative_addressing = 1, hdr_data_present = 0, start_of_msg = 1,
    end_of_msg = 1, message_id = 0x1234,
    ri_generation = 0x77, job_id =0xabcdef, pid_on_fep =0x678,
    resource_index = 0x9ab, buffer_offset = 0x9876543298765432)

MakePacket(pds, ses)

# Small atomic
ses = uet_ses.MakeRequestHdrSmall(
    opcode =
    uet_ses.UetSesRequestMsgType.UET_SES_REQUEST_MSG_ATOMIC.value,
    delivery_complete = 1, initiator_error = 0,
    relative_addressing = 1, hdr_data_present = 0, start_of_msg = 1,
    end_of_msg = 1, message_id = 0x1234,
    ri_generation = 0x77, job_id =0xabcdef, pid_on_fep =0x678,
    resource_index = 0x9ab, buffer_offset = 0x9876543298765432)

atomic = uet_ses.MakeAtomicOpExtHdr(
    atomic_code = uet_ses.UetSesAtomicOpcode.UET_SES_AMO_BAND.value,
    atomic_datatype =
        uet_ses.UetSesAtomicDatatype.UET_SES_AMO_DATATYPE_FLOAT.value,
    ctrl_cacheable = 1, cpu_coherent = 1, vendor_defined = 5)

MakePacket3(pds, ses, atomic)

# Small atomic compare and swap operation
atomic = uet_ses.MakeAtomicCmpAndSwapHdr(
    atomic_code = uet_ses.UetSesAtomicOpcode.UET_SES_AMO_CSWAP_GE.value,
    atomic_datatype =
        uet_ses.UetSesAtomicDatatype.UET_SES_AMO_DATATYPE_FLOAT_COMPLEX.value,
    ctrl_cacheable = 1, cpu_coherent = 1, vendor_defined = 7,
    compare_value1 = 0x9876543298765432,
    compare_value2 = 0x2345678923456789,
    swap_value1 = 0x123456789abcdef,
    swap_value2 = 0xfedcba0987654321)

MakePacket3(pds, ses, atomic)

pds = uet_pds.MakeRudiResponse(
	next_hdr = uet_pds.UetNextHeaderType.UET_HDR_RESPONSE.value,
    ecn_marked = 1, retrans = 0, pkt_id = 0x99887766)

# Response no data
ses = uet_ses.MakeHdrResponse(
	list = 3,
	opcode =
	uet_ses.UetSesResponseMsgType.UET_SES_RESPONSE_WITH_DATA.value,
	return_code =
	uet_ses.UetSesReturnCode.UET_SES_RETURN_CODE_AT_PERM.value,
	message_id = 0x1234, ri_generation = 0x99, job_id = 0x654321,
	modified_length = 0x9abcdef)

MakePacket(pds, ses)

pds = uet_pds.MakeRudiResponse(
	next_hdr = uet_pds.UetNextHeaderType.UET_HDR_RESPONSE_DATA.value,
    ecn_marked = 1, retrans = 0, pkt_id = 0x99887766)

# Response with data
ses = uet_ses.MakeWithDataHdrResponse(
	list = 3,
	opcode =
	uet_ses.UetSesResponseMsgType.UET_SES_RESPONSE_WITH_DATA.value,
	return_code =
	uet_ses.UetSesReturnCode.UET_SES_RETURN_CODE_AT_PERM.value,
	message_id = 0x1234, job_id = 0x654321, read_request_message_id = 0x1234,
    payload_length = 0x321, modified_length = 0x87654321,
	message_offset = 0x9abcdef)

MakePacket(pds, ses)

pds = uet_pds.MakeRudiResponse(
	next_hdr = uet_pds.UetNextHeaderType.UET_HDR_RESPONSE_DATA_SMALL.value,
    ecn_marked = 1, retrans = 0, pkt_id = 0x99887766)

# Response with small data
ses = uet_ses.MakeWithDataHdrSmallResponse(
	list = 3,
	opcode =
	uet_ses.UetSesResponseMsgType.UET_SES_RESPONSE_WITH_DATA.value,
	return_code =
	uet_ses.UetSesReturnCode.UET_SES_RETURN_CODE_AT_PERM.value,
    job_id = 223344, payload_length = 0x3456, original_request_psn = 0x1234)

MakePacket(pds, ses)

pds = uet_pds.MakeRudRequest(
	next_hdr = uet_pds.UetNextHeaderType.UET_HDR_REQUEST_STD.value,
	retrans = 1, ackreq = 0, clear_psn_offset = 0x1234, psn = 0x98765432,
	spdcid = 0x3456, dpdcid = 0x9abc)

# Standar rendezvous
ses = uet_ses.MakeRequestSomHdrStd(
    opcode =
    uet_ses.UetSesRequestMsgType.UET_SES_REQUEST_MSG_RENDEZVOUS_TSEND.value,
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

MakePacket(pds, ses)

# Medium no-op
pds = uet_pds.MakeRudRequest(
	next_hdr = uet_pds.UetNextHeaderType.UET_HDR_REQUEST_MEDIUM.value,
	retrans = 1, ackreq = 0, clear_psn_offset = 0x1234, psn = 0x98765432,
	spdcid = 0x3456, dpdcid = 0x9abc)

ses = uet_ses.MakeRequestHdrMedium(
    opcode =
    uet_ses.UetSesRequestMsgType.UET_SES_REQUEST_MSG_NO_OP.value,
    delivery_complete = 1, initiator_error = 0,
    relative_addressing = 1, hdr_data_present = 0, start_of_msg = 1,
    end_of_msg = 1, message_id = 0x1234,
    ri_generation = 0x77, job_id =0xabcdef, pid_on_fep =0x678,
    resource_index = 0x9ab, header_data = 0x9876543298765432,
    initiator = 0x87654321, memory_key = 0xccbbddeeff009988)


MakePacket(pds, ses)

wrpcap("uet_ses.pcap", packets)
