from scapy.all import *

module_dir = os.path.abspath(os.path.join(os.path.dirname(__file__),
                                          '..', 'sunh'))
sys.path.insert(0, module_dir)

import sunh
import superp

# Python script to create a pcap file containing samples of Falcon
# packet formats (pull request, pull data, push data, resync, ACK,
# extended ACK, and NACK)


# Optional first argument is SUNH EtherTYpe, second argument is SUPERp
# IPPROTO number, and thrird argument is SUPERp

if len(sys.argv) > 1:
    ether_type_sunh = int(sys.argv[1], 0)
else:
    ether_type_sunh = 0x885b

if len(sys.argv) > 2:
    superp_ipproto = int(sys.argv[2], 0)
else:
    superp_ipproto =  253

if len(sys.argv) > 3:
    superp_port_num = int(sys.argv[3], 0)
else:
    superp_port_num = 4444

def MakePacket(req):
    req_packet = Raw(load=req)
    packet = ether_frame/ip_packet1/udp_packet1/req_packet
    packets.append(packet)

def MakePacket2(req1, req2):
    req_packet1 = Raw(load=req1)
    req_packet2 = Raw(load=req2)
    packet = ether_frame/ip_packet1/udp_packet1/req_packet1/req_packet2
    packets.append(packet)

def MakePacket3(req1, req2, req3):
    req_packet1 = Raw(load=req1)
    req_packet2 = Raw(load=req2)
    req_packet3 = Raw(load=req3)
    packet = ether_frame/ip_packet1/udp_packet1/req_packet1/req_packet2/req_packet3
    packets.append(packet)

def MakePacket4(req1, req2, req3, req4):
    req_packet1 = Raw(load=req1)
    req_packet2 = Raw(load=req2)
    req_packet3 = Raw(load=req3)
    req_packet4 = Raw(load=req4)
    packet = ether_frame/ip_packet1/udp_packet1/req_packet1/req_packet2/req_packet3/req_packet4
    packets.append(packet)

def MakePacket5(req1, req2, req3, req4, req5):
    req_packet1 = Raw(load=req1)
    req_packet2 = Raw(load=req2)
    req_packet3 = Raw(load=req3)
    req_packet4 = Raw(load=req4)
    req_packet5 = Raw(load=req5)
    packet = ether_frame/ip_packet1/udp_packet1/req_packet1/req_packet2/req_packet3/req_packet4/req_packet5
    packets.append(packet)

def MakePacket6(req1, req2, req3, req4, req5, req6):
    req_packet1 = Raw(load=req1)
    req_packet2 = Raw(load=req2)
    req_packet3 = Raw(load=req3)
    req_packet4 = Raw(load=req4)
    req_packet5 = Raw(load=req5)
    req_packet6 = Raw(load=req6)
    packet = ether_frame/ip_packet1/udp_packet1/req_packet1/req_packet2/req_packet3/req_packet4/req_packet5/req_packet6
    packets.append(packet)

def MakePacket7(req1, req2, req3, req4, req5, req6, req7):
    req_packet1 = Raw(load=req1)
    req_packet2 = Raw(load=req2)
    req_packet3 = Raw(load=req3)
    req_packet4 = Raw(load=req4)
    req_packet5 = Raw(load=req5)
    req_packet6 = Raw(load=req6)
    req_packet7 = Raw(load=req7)
    packet = ether_frame/ip_packet1/udp_packet1/req_packet1/req_packet2/req_packet3/req_packet4/req_packet5/req_packet6/req_packet7
    packets.append(packet)

def MakePacket8(req1, req2, req3, req4, req5, req6, req7, req8):
    req_packet1 = Raw(load=req1)
    req_packet2 = Raw(load=req2)
    req_packet3 = Raw(load=req3)
    req_packet4 = Raw(load=req4)
    req_packet5 = Raw(load=req5)
    req_packet6 = Raw(load=req6)
    req_packet7 = Raw(load=req7)
    req_packet8 = Raw(load=req8)
    packet = ether_frame/ip_packet1/udp_packet1/req_packet1/req_packet2/req_packet3/req_packet4/req_packet5/req_packet6/req_packet7/req_packet8
    packets.append(packet)

def MakeSUNHPacket(sunh, req):
    sunh_x = Raw(load=sunh)
    req_packet = Raw(load=req)
    packet = ether_frame_sunh/sunh_x/req_packet
    packets.append(packet)

def MakeSUNHPacket2(sunh, req1, req2):
    sunh_x = Raw(load=sunh)
    req_packet1 = Raw(load=req1)
    req_packet2 = Raw(load=req2)
    packet = ether_frame_sunh/sunh_x/req_packet1/req_packet2
    packets.append(packet)

def MakeSUNHPacket3(sunh, req1, req2, req3):
    sunh_x = Raw(load=sunh)
    req_packet1 = Raw(load=req1)
    req_packet2 = Raw(load=req2)
    req_packet3 = Raw(load=req3)
    packet = ether_frame_sunh/sunh_x/req_packet1/req_packet2/req_packet3
    packets.append(packet)

def MakeSUNHPacket4(sunh, req1, req2, req3, req4):
    sunh_x = Raw(load=sunh)
    req_packet1 = Raw(load=req1)
    req_packet2 = Raw(load=req2)
    req_packet3 = Raw(load=req3)
    req_packet4 = Raw(load=req4)
    packet = ether_frame_sunh/sunh_x/req_packet1/req_packet2/req_packet3/req_packet4
    packets.append(packet)

def MakeSUNHPacket5(sunh, req1, req2, req3, req4, req5):
    sunh_x = Raw(load=sunh)
    req_packet1 = Raw(load=req1)
    req_packet2 = Raw(load=req2)
    req_packet3 = Raw(load=req3)
    req_packet4 = Raw(load=req4)
    req_packet5 = Raw(load=req5)
    packet = ether_frame_sunh/sunh_x/req_packet1/req_packet2/req_packet3/req_packet4/req_packet5
    packets.append(packet)

def MakeSUNHPacket6(sunh, req1, req2, req3, req4, req5, req6):
    sunh_x = Raw(load=sunh)
    req_packet1 = Raw(load=req1)
    req_packet2 = Raw(load=req2)
    req_packet3 = Raw(load=req3)
    req_packet4 = Raw(load=req4)
    req_packet5 = Raw(load=req5)
    req_packet6 = Raw(load=req6)
    packet = ether_frame_sunh/sunh_x/req_packet1/req_packet2/req_packet3/req_packet4/req_packet5/req_packet6
    packets.append(packet)

def MakeSUNHPacket7(sunh, req1, req2, req3, req4, req5, req6, req7):
    sunh_x = Raw(load=sunh)
    req_packet1 = Raw(load=req1)
    req_packet2 = Raw(load=req2)
    req_packet3 = Raw(load=req3)
    req_packet4 = Raw(load=req4)
    req_packet5 = Raw(load=req5)
    req_packet6 = Raw(load=req6)
    req_packet7 = Raw(load=req7)
    packet = ether_frame_sunh/sunh_x/req_packet1/req_packet2/req_packet3/req_packet4/req_packet5/req_packet6/req_packet7
    packets.append(packet)

def MakeSUNHPacket8(sunh, req1, req2, req3, req4, req5, req6, req7, req8):
    sunh_x = Raw(load=sunh)
    req_packet1 = Raw(load=req1)
    req_packet2 = Raw(load=req2)
    req_packet3 = Raw(load=req3)
    req_packet4 = Raw(load=req4)
    req_packet5 = Raw(load=req5)
    req_packet6 = Raw(load=req6)
    req_packet7 = Raw(load=req7)
    req_packet8 = Raw(load=req8)
    packet = ether_frame_sunh/sunh_x/req_packet1/req_packet2/req_packet3/req_packet4/req_packet5/req_packet6/req_packet7/req_packet8
    packets.append(packet)

packets = []

source_mac = "00:11:22:33:44:55"
destination_mac = "AA:BB:CC:DD:EE:FF"
ether_type = 0x0800

ether_frame = Ether(src=source_mac, dst=destination_mac, type=ether_type)

ether_frame_sunh = Ether(src=source_mac, dst=destination_mac, type=ether_type_sunh)

ip_packet1 = IP(src="192.168.1.1", dst="192.168.1.2")
ip_packet2 = IP(src="192.168.2.1", dst="192.168.2.2")

udp_packet1 = UDP(dport = superp_port_num, sport = 8675)
udp_packet2 = UDP(dport = superp_port_num, sport = 28675)

# Make read with one operation

pdl = superp.MakePacketDeliveryHdr(0x1234, 0x5678, 0x12345678,
                                   0x87654321, 0x89abcdef)

tal = superp.MakeTransactionHdr(1, 0,
                                superp.SUPERp_Op_Type.SUPERP_OP_TYPE_NOOP.value,
                                0x1234, 0x8765, 0x6789)
MakePacket2(pdl, tal)

pdl = superp.MakePacketDeliveryHdr(0x1234, 0x5678, 0x12345678,
                                   0x87654321, 0x89abcdef)

tal = superp.MakeTransactionHdr(1, 0,
                                superp.SUPERp_Op_Type.SUPERP_OP_TYPE_LAST_NULL.value,
                                0x1234, 0x8765, 0x6789)
MakePacket2(pdl, tal)

tal = superp.MakeTransactionHdr(1, 1,
                                superp.SUPERp_Op_Type.SUPERP_OP_TYPE_TRANSACT_ERR.value,
                                0x1234, 0x8765, 0x6789)

transact_err = superp.MakeTransactErr(0x1234, 0x3, 0x8877, 0x9988)

MakePacket3(pdl, tal, transact_err)

tal = superp.MakeTransactionHdr(1, 5,
                                superp.SUPERp_Op_Type.SUPERP_OP_TYPE_READ_RESP.value,
                                0x1234, 0x8765, 0x6789)

resp1 = superp.MakeReadResp(0x1234, 0x98, 0x87654321)
resp2 = superp.MakeReadResp(0x1235, 0xa9, 0xabcdef01)
resp3 = superp.MakeReadResp(0x1236, 0xba, 0x23456789)
resp4 = superp.MakeReadResp(0x1237, 0xcd, 0xfedcba98)
resp5 = superp.MakeReadResp(0x1238, 0xde, 0x8675309)

bytes = bytearray(5*128)

bytes[0] = 0xf0
bytes[1*128] = 0xf1
bytes[2*128] = 0xf2
bytes[3*128] = 0xf3
bytes[4*128] = 0xf4

MakePacket8(pdl, tal, resp1, resp2, resp3, resp4, resp5, bytes)

tal = superp.MakeTransactionHdr(1, 5,
                                superp.SUPERp_Op_Type.SUPERP_OP_TYPE_WRITE.value,
                                0x1234, 0x8765, 0x6789)

write1 = superp.MakeWriteOp(0x8765432112345678)
write2 = superp.MakeWriteOp(0xabcdef0112345678)
write3 = superp.MakeWriteOp(0x2345678912345678)
write4 = superp.MakeWriteOp(0xfedcba9812345678)
write5 = superp.MakeWriteOp(0x867530912345678)

MakePacket8(pdl, tal, write1, write2, write3, write4, write5, bytes)

tal = superp.MakeTransactionHdr(1, 5,
                                superp.SUPERp_Op_Type.SUPERP_OP_TYPE_SEND.value,
                                0x1234, 0x8765, 0x6789)

send1 = superp.SendOp(0x87654321, 0x12345678)
send2 = superp.SendOp(0xabcdef01, 0x12345678)
send3 = superp.SendOp(0x23456789, 0x12345678)
send4 = superp.SendOp(0xfedcba98, 0x12345678)
send5 = superp.SendOp(0x86753091, 0x2345678)

MakePacket8(pdl, tal, send1, send2, send3, send4, send5, bytes)

tal = superp.MakeTransactionHdr(1, 5,
                                superp.SUPERp_Op_Type.SUPERP_OP_TYPE_SEND_TO_QP.value,
                                0x1234, 0x8765, 0x6789)

send_to_qp1 = superp.SendToQPOp(0x876543)
send_to_qp2 = superp.SendToQPOp(0xabcdef)
send_to_qp3 = superp.SendToQPOp(0x234567)
send_to_qp4 = superp.SendToQPOp(0xfedcba)
send_to_qp5 = superp.SendToQPOp(0x867530)

MakePacket8(pdl, tal, send_to_qp1, send_to_qp2, send_to_qp3, send_to_qp4, send_to_qp5, bytes)

sunh = sunh.MakeSUNH_Header(0x23, superp_ipproto, 0xe, 0xcde, 0x1234, 0x4321)

MakeSUNHPacket8(sunh, pdl, tal, send_to_qp1, send_to_qp2, send_to_qp3, send_to_qp4, send_to_qp5, bytes)

# Write the packets to the pcap file

wrpcap("superp.pcap", packets)
