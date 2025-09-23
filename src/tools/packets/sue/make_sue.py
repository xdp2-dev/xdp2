from scapy.all import *

import sue

# Python script to create a pcap file containing samples of SUE
# reliability packet formats (ACK, NACK, INVALID1, INVALID2)

# Optional first argument is UDP port number
if len(sys.argv) > 1:
    sue_port_num = sys.argv[1]
else:
    sue_port_num = 4793

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

udp_packet1 = UDP(dport = int(sue_port_num), sport = 8675)
udp_packet2 = UDP(dport = int(sue_port_num), sport = 28675)

# Make ACK PASN SUE TH header

req = sue.MakeSueRhdr(opcode = sue.SueOpcode.SUE_OPCODE_ACK.value,
                      xpuid = 0x123, psn = 0x6789, vc = 3,
                      partition = 0x345, apsn = 0x4567)

MakePacket(req)

req = sue.MakeSueRhdr(opcode = sue.SueOpcode.SUE_OPCODE_NACK.value,
                      xpuid = 0x123, psn = 0x6789, vc = 3,
                      partition = 0x345, apsn = 0x4567)

MakePacket(req)

req = sue.MakeSueRhdr(opcode = sue.SueOpcode.SUE_OPCODE_INVALID1.value,
                      xpuid = 0x123, psn = 0x6789, vc = 3,
                      partition = 0x345, apsn = 0x4567)

MakePacket(req)

req = sue.MakeSueRhdr(opcode = sue.SueOpcode.SUE_OPCODE_INVALID2.value,
                      xpuid = 0x123, psn = 0x6789, vc = 3,
                      partition = 0x345, apsn = 0x4567)

MakePacket(req)

# Write the packets to the pcap file

wrpcap("sue.pcap", packets)
