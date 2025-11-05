import ctypes
from enum import Enum
from scapy.all import *

# Python helpers for SUPERp protocol format definitions

class SUNH_Header(ctypes.BigEndianStructure):
	_pack_ = 1
	_fields_ = [
		("traffic_class", ctypes.c_uint8),
		("next_hdr", ctypes.c_uint8),
		("hop_limit", ctypes.c_uint16, 4),
		("flow_label", ctypes.c_uint16, 12),
		("src_addr", ctypes.c_uint16),
		("dest_addr", ctypes.c_uint16),
	]

def MakeSUNH_Header(traffic_class, next_hdr, hop_limit, flow_label,
                    src_addr, dest_addr):
    hdr = SUNH_Header()

    hdr.traffic_class = traffic_class
    hdr.next_hdr = next_hdr
    hdr.hop_limit = hop_limit
    hdr.flow_label = flow_label
    hdr.src_addr = src_addr
    hdr.dest_addr = dest_addr

    return hdr
