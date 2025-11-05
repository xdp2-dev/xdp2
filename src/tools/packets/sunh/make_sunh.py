from scapy.all import *

import sunh

# Python script to create a pcap file containing samples of Falcon
# packet formats (pull request, pull data, push data, resync, ACK,
# extended ACK, and NACK)


# Optional first argument is SUNH EtherTYpe, second argument is SUPERp
# IPPROTO number, and thrird argument is SUPERp

if len(sys.argv) > 1:
    ether_type_sunh = int(sys.argv[1], 0)
else:
    ether_type_sunh = 0x885b

def MakeSUNHPacket(sunh, req):
    sunh_x = Raw(load=sunh)
    req_packet = Raw(load=req)
    packet = ether_frame_sunh/sunh_x/udp_packet1/req_packet
    packets.append(packet)

packets = []

source_mac = "00:11:22:33:44:55"
destination_mac = "AA:BB:CC:DD:EE:FF"
ether_type = 0x0800

ether_frame_sunh = Ether(src=source_mac, dst=destination_mac, type=ether_type_sunh)

ip_packet1 = IP(src="192.168.1.1", dst="192.168.1.2")
ip_packet2 = IP(src="192.168.2.1", dst="192.168.2.2")

udp_packet1 = UDP(dport = 80, sport = 8675)
udp_packet2 = UDP(dport = 8000, sport = 28675)

sunh = sunh.MakeSUNH_Header(0x23, 17, 0xe, 0xcde, 0x1234, 0x4321)

bytes = bytearray(5*128)

bytes[0] = 0xf0
bytes[1*128] = 0xf1
bytes[2*128] = 0xf2
bytes[3*128] = 0xf3
bytes[4*128] = 0xf4

MakeSUNHPacket(sunh, bytes)

# Write the packets to the pcap file

wrpcap("sunh.pcap", packets)
